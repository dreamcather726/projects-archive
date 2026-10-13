import math
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


def servo_deg_to_joint_rad(servo_index, angle_deg):
    """Convert a 0-180 servo angle to a PyBullet joint angle."""
    # The Arduino init angle is the real robot's start pose.
    # Therefore each servo's init angle maps to joint angle 0 in the URDF.
    delta_deg = angle_deg - SERVO_INIT_DEG[servo_index]
    return math.radians(delta_deg * SERVO_DIRECTION[servo_index]*-1)


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


def main():
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

    # Sliders show servo angles in degrees, exactly like the Arduino code.
    # The PyBullet motor receives radians internally because the API requires it.
    sliders = []
    for joint_id in range(p.getNumJoints(robot)):
        info = p.getJointInfo(robot, joint_id)
        if info[2] != p.JOINT_REVOLUTE:
            continue

        servo_index = len(sliders)
        init_deg = SERVO_INIT_DEG[servo_index]
        joint_name = info[1].decode("utf-8", errors="replace")

        p.resetJointState(robot, joint_id, servo_deg_to_joint_rad(servo_index, init_deg))
        slider = p.addUserDebugParameter(joint_name, 0, 180, init_deg)
        sliders.append((joint_id, servo_index, slider))

    while p.isConnected():
        for joint_id, servo_index, slider in sliders:
            servo_angle_deg = p.readUserDebugParameter(slider)
            target_rad = servo_deg_to_joint_rad(servo_index, servo_angle_deg)
            p.setJointMotorControl2(
                robot,
                joint_id,
                p.POSITION_CONTROL,
                targetPosition=target_rad,
                force=5,
            )

        p.stepSimulation()
        time.sleep(1 / 240)


if __name__ == "__main__":
    main()
