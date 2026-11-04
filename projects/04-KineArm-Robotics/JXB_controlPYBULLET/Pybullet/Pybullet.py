import argparse
import math
import socket
import tempfile
import time
import xml.etree.ElementTree as ET
from pathlib import Path

import pybullet as p
import pybullet_data


# Folder layout:
# JXB/Pybullet/robot_arm2.SLDASM/robot_arm2.SLDASM/
BASE = Path(__file__).resolve().parent
ROOT = next(path for path in BASE.glob("*.SLDASM/*") if (path / "urdf").exists())
URDF = next((ROOT / "urdf").glob("*.urdf"))
MESH_DIR = ROOT / "meshes"

# Servo angles copied from JXB/JXB.ino:
# int currentAngle[count] = {90, 0, 180, 130, 90};
SERVO_INIT_DEG = [90, 0, 180, 130, 90]

# Change a value to -1 if a simulated joint moves opposite to the real servo.
SERVO_DIRECTION = [1, 1, 1, 1, 1]
UDP_PORT = 8888
SEND_HZ = 20


def servo_deg_to_joint_rad(servo_index, angle_deg):
    """Convert a 0-180 servo angle to a PyBullet joint angle."""
    # The Arduino init angle is the real robot's start pose.
    # Therefore each servo's init angle maps to joint angle 0 in the URDF.
    delta_deg = angle_deg - SERVO_INIT_DEG[servo_index]
    return math.radians(delta_deg * SERVO_DIRECTION[servo_index]*-1)


def apply_servo_angles(robot, sliders, servo_angles):
    """Apply servo angles to the simulated joints."""
    for servo_index, joint_id, _slider in sliders:
        if joint_id is None:
            continue

        target_rad = servo_deg_to_joint_rad(servo_index, servo_angles[servo_index])
        p.resetJointState(robot, joint_id, target_rad)
        p.setJointMotorControl2(
            robot,
            joint_id,
            p.POSITION_CONTROL,
            targetPosition=target_rad,
            force=5,
        )


def create_servo_sliders(robot, revolute_joints):
    """Create five servo sliders and initialize the simulated joints."""
    sliders = []
    for servo_index, init_deg in enumerate(SERVO_INIT_DEG):
        if servo_index < len(revolute_joints):
            joint_id, joint_name = revolute_joints[servo_index]
            p.resetJointState(robot, joint_id, servo_deg_to_joint_rad(servo_index, init_deg))
            slider_name = joint_name
        else:
            joint_id = None
            slider_name = f"Servo {servo_index + 1} gripper"

        slider = p.addUserDebugParameter(slider_name, 0, 180, init_deg)
        sliders.append((servo_index, joint_id, slider))

    return sliders


class WifiServoSender:
    """Send servo angles to the ESP32 WiFi controller."""

    def __init__(self, robot_ip, udp_port=UDP_PORT, send_hz=SEND_HZ):
        self.robot_ip = robot_ip
        self.udp_port = udp_port
        self.send_interval = 1.0 / send_hz
        self.last_send_time = 0
        self.last_angles = None
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) if robot_ip else None

    def send(self, angles):
        if self.sock is None:
            return

        now = time.time()
        changed = angles != self.last_angles
        due = now - self.last_send_time >= self.send_interval
        if not changed and not due:
            return

        command = "movej:" + ",".join(str(int(angle)) for angle in angles)
        self.sock.sendto(command.encode("ascii"), (self.robot_ip, self.udp_port))
        self.last_angles = angles[:]
        self.last_send_time = now


def make_runtime_urdf():
    """Create a temporary URDF that PyBullet can load directly."""
    tree = ET.parse(URDF)
    robot = tree.getroot()

    # PyBullet does not understand ROS package:// mesh paths.
    # Replace them with absolute paths to the STL files.
    for mesh in robot.findall(".//mesh"):
        mesh_name = Path(mesh.attrib["filename"].replace("\\", "/")).name
        mesh_path = (MESH_DIR / mesh_name).resolve()
        mesh.set("filename", str(mesh_path))

    # SolidWorks exported revolute joints with zero range and zero effort.
    # Fix only the temporary URDF so the sliders can move J1-J4.
    for joint in robot.findall("joint"):
        if joint.attrib.get("type") != "revolute":
            continue

        limit = joint.find("limit")
        if limit is None:
            limit = ET.SubElement(joint, "limit")

        limit.set("lower", f"{-math.pi:.6g}")
        limit.set("upper", f"{math.pi:.6g}")
        limit.set("effort", "10")
        limit.set("velocity", "2")

    output = Path(tempfile.gettempdir()) / "jxb2_runtime.urdf"
    tree.write(output, encoding="utf-8", xml_declaration=True)
    return output


def parse_args():
    parser = argparse.ArgumentParser(description="Simulate and optionally control JXB.")
    parser.add_argument(
        "--robot-ip",
        default=None,
        help="ESP32 IP address. If omitted, only PyBullet simulation runs.",
    )
    parser.add_argument("--udp-port", type=int, default=UDP_PORT)
    return parser.parse_args()


def main():
    args = parse_args()
    wifi_sender = WifiServoSender(args.robot_ip, args.udp_port)
    if args.robot_ip:
        print(f"WiFi control enabled: UDP {args.robot_ip}:{args.udp_port}")
    else:
        print("WiFi control disabled: run with --robot-ip <ESP32_IP> to control the arm.")

    p.connect(p.GUI)
    p.setAdditionalSearchPath(pybullet_data.getDataPath())
    p.setGravity(0, 0, -9.8)

    p.loadURDF("plane.urdf")
    p.resetDebugVisualizerCamera(
        cameraDistance=0.6,
        cameraYaw=45,
        cameraPitch=-35,
        cameraTargetPosition=[-0.08, 0.06, 0.05],
    )

    robot = p.loadURDF(
        str(make_runtime_urdf()),
        basePosition=[0, 0, 0.08],
        useFixedBase=True,
        flags=p.URDF_USE_INERTIA_FROM_FILE,
    )

    revolute_joints = []
    for joint_id in range(p.getNumJoints(robot)):
        info = p.getJointInfo(robot, joint_id)
        if info[2] == p.JOINT_REVOLUTE:
            revolute_joints.append((joint_id, info[1].decode("utf-8", errors="replace")))

    # Sliders show all five servo angles in degrees, exactly like JXB.ino.
    # The first four also drive URDF joints. Servo 5 is the real gripper.
    sliders = create_servo_sliders(robot, revolute_joints)
    servo_angles = SERVO_INIT_DEG[:]
    wifi_sender.send(servo_angles)

    reset_button = p.addUserDebugParameter("Reset", 1, 0, 0)
    last_reset_value = p.readUserDebugParameter(reset_button)

    while p.isConnected():
        reset_value = p.readUserDebugParameter(reset_button)
        if reset_value != last_reset_value:
            last_reset_value = reset_value
            for _servo_index, _joint_id, slider in sliders:
                p.removeUserDebugItem(slider)
            p.removeUserDebugItem(reset_button)

            servo_angles = SERVO_INIT_DEG[:]
            sliders = create_servo_sliders(robot, revolute_joints)
            reset_button = p.addUserDebugParameter("Reset", 1, 0, 0)
            last_reset_value = p.readUserDebugParameter(reset_button)

            apply_servo_angles(robot, sliders, servo_angles)
            wifi_sender.send(servo_angles)

        for servo_index, joint_id, slider in sliders:
            servo_angle_deg = p.readUserDebugParameter(slider)
            servo_angles[servo_index] = int(round(servo_angle_deg))

            if joint_id is None:
                continue

            target_rad = servo_deg_to_joint_rad(servo_index, servo_angle_deg)
            p.setJointMotorControl2(
                robot,
                joint_id,
                p.POSITION_CONTROL,
                targetPosition=target_rad,
                force=5,
            )

        wifi_sender.send(servo_angles)
        p.stepSimulation()
        time.sleep(1 / 240)


if __name__ == "__main__":
    main()
