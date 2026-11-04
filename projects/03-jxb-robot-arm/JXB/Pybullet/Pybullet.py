import argparse
import atexit
import math
import os
import shutil
import socket
import sys
import tempfile
import time
import xml.etree.ElementTree as ET
from functools import lru_cache
from pathlib import Path

import pybullet as p
import pybullet_data


BASE = Path(__file__).resolve().parent


@lru_cache(maxsize=1)
def get_runtime_dir():
    """Keep files read by PyBullet's native loader on an ASCII-only path."""
    candidates = [Path(tempfile.gettempdir()), BASE]
    if getattr(sys, "frozen", False):
        candidates.append(Path(sys.executable).resolve().parent)

    for candidate in candidates:
        candidate = candidate.resolve()
        if not str(candidate).isascii():
            continue
        try:
            runtime_dir = Path(tempfile.mkdtemp(prefix="jxb_runtime_", dir=candidate))
        except OSError:
            continue
        atexit.register(shutil.rmtree, runtime_dir, ignore_errors=True)
        return runtime_dir

    raise RuntimeError(
        "PyBullet 需要可写的纯英文资源路径。请将项目或 EXE 放到 D:/JXB "
        "等纯英文目录，或将 TEMP/TMP 设置为可写的纯英文目录后重新运行。"
    )


def prepare_plane_urdf():
    """Stage the bundled ground assets when their installed path is non-ASCII."""
    data_dir = Path(pybullet_data.getDataPath()).resolve()
    plane = data_dir / "plane.urdf"
    if not plane.is_file():
        raise FileNotFoundError(f"PyBullet 地面模型缺失：{plane}")
    if str(data_dir).isascii():
        return plane

    plane_dir = get_runtime_dir() / "plane"
    plane_dir.mkdir(exist_ok=True)
    for name in ("plane.urdf", "plane100.obj", "plane.mtl", "checker_blue.png"):
        shutil.copy2(data_dir / name, plane_dir / name)
    # Some distributions include this ambient texture referenced by plane.mtl.
    if (data_dir / "cube.tga").is_file():
        shutil.copy2(data_dir / "cube.tga", plane_dir / "cube.tga")
    return plane_dir / "plane.urdf"


def find_robot_root():
    """查找包含 urdf 和 meshes 的机器人模型根目录。"""
    # 有些 SolidWorks 导出的目录是：
    # robot_arm2.SLDASM/urdf
    # robot_arm2.SLDASM/meshes
    for path in BASE.glob("*.SLDASM"):
        if (path / "urdf").exists() and (path / "meshes").exists():
            return path

    # 有些旧目录是多套了一层：
    # robot_arm2.SLDASM/robot_arm2.SLDASM/urdf
    # robot_arm2.SLDASM/robot_arm2.SLDASM/meshes
    for path in BASE.glob("*.SLDASM/*"):
        if (path / "urdf").exists() and (path / "meshes").exists():
            return path

    raise FileNotFoundError(
        f"在 {BASE} 下没有找到包含 urdf 和 meshes 的 *.SLDASM 模型目录"
    )


ROOT = find_robot_root()
URDF = next((ROOT / "urdf").glob("*.urdf"))
MESH_DIR = ROOT / "meshes"

# 机械臂整体放到 PyBullet 世界坐标系中的位置。
# 如果模型整体在第三象限，也就是 x/y 都偏负，就把前两个数调大。
ROBOT_BASE_POSITION = [0.2, -0.2, 0.08]

# 机械臂整体的旋转角度，单位是弧度，顺序是 [绕 X, 绕 Y, 绕 Z]。
# math.pi 表示 180 度，所以这里表示整个模型绕世界 Z 轴旋转 180 度。

# Servo angles copied from JXB/JXB.ino:
# int currentAngle[count] = {90, 0, 180, 130, 90};
SERVO_INIT_DEG = [90, 0, 180, 130, 90]

# Change a value to -1 if a simulated joint moves opposite to the real servo.
SERVO_DIRECTION = [1, 1, 1, 1, 1]
UDP_PORT = 8888
SEND_HZ = 20
COLLISION_PRINT_INTERVAL = 1.0
FK_PRINT_INTERVAL = 0.5
IK_FORCE = 5
MARKER_RADIUS = 0.012
TOOL_TIP_LINK_NAME = "tool_tip"
TOOL_TIP_LOCAL_OFFSET = [0.07365, 0.0, 0.0]


def servo_deg_to_joint_rad(servo_index, angle_deg):
    """Convert a 0-180 servo angle to a PyBullet joint angle."""
    # The Arduino init angle is the real robot's start pose.
    # Therefore each servo's init angle maps to joint angle 0 in the URDF.
    delta_deg = angle_deg - SERVO_INIT_DEG[servo_index]
    return math.radians(delta_deg * SERVO_DIRECTION[servo_index]*-1)


def joint_rad_to_servo_deg(servo_index, joint_rad):
    """Convert a PyBullet joint angle back to a 0-180 servo angle."""
    delta_deg = math.degrees(joint_rad) / (SERVO_DIRECTION[servo_index] * -1)
    return SERVO_INIT_DEG[servo_index] + delta_deg


def clamp_servo_angle(angle_deg):
    """Keep generated servo targets inside the physical servo range."""
    return max(0, min(180, int(round(angle_deg))))


def get_controllable_joints(robot):
    """Return PyBullet joint ids that can be driven by kinematics."""
    joints = []
    for joint_id in range(p.getNumJoints(robot)):
        joint_type = p.getJointInfo(robot, joint_id)[2]
        if joint_type in (p.JOINT_REVOLUTE, p.JOINT_PRISMATIC):
            joints.append(joint_id)
    return joints


def get_end_effector_index(robot):
    """Use the generated tool tip link as the end effector."""
    for joint_id in range(p.getNumJoints(robot)):
        link_name = p.getJointInfo(robot, joint_id)[12].decode("utf-8", errors="replace")
        if link_name == TOOL_TIP_LINK_NAME:
            return joint_id

    # Fallback for older URDFs without the generated tip link.
    return p.getNumJoints(robot) - 1


def get_end_effector_pose(robot, end_effector_index):
    """Forward kinematics: read the current tool position and orientation."""
    link_state = p.getLinkState(robot, end_effector_index, computeForwardKinematics=True)
    return link_state[4], link_state[5]


def calculate_ik(robot, end_effector_index, target_pos, controllable_joints):
    """Inverse kinematics: calculate joint and servo targets for target_pos."""
    joint_poses = p.calculateInverseKinematics(
        robot,
        end_effector_index,
        target_pos,
        maxNumIterations=80,
        residualThreshold=1e-5,
    )

    joint_targets = {}
    servo_angles = SERVO_INIT_DEG[:]
    was_limited = False
    for servo_index, _joint_id in enumerate(controllable_joints[: len(SERVO_INIT_DEG)]):
        if servo_index >= len(joint_poses):
            break
        joint_target = joint_poses[servo_index]
        servo_angle = joint_rad_to_servo_deg(servo_index, joint_target)
        limited_servo_angle = clamp_servo_angle(servo_angle)
        joint_targets[controllable_joints[servo_index]] = joint_target
        servo_angles[servo_index] = limited_servo_angle
        was_limited = was_limited or abs(servo_angle - limited_servo_angle) > 0.5
    return joint_targets, servo_angles, was_limited


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


def apply_joint_targets(robot, joint_targets):
    """Apply raw IK joint targets to the simulation."""
    for joint_id, target_rad in joint_targets.items():
        p.setJointMotorControl2(
            robot,
            joint_id,
            p.POSITION_CONTROL,
            targetPosition=target_rad,
            force=IK_FORCE,
            positionGain=0.12,
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


def create_kinematics_controls(initial_target):
    """Create debug controls for FK/IK in the PyBullet side panel."""
    return {
        "ik_enable": p.addUserDebugParameter("IK enable 0/1", 0, 1, 1),
        "target_x": p.addUserDebugParameter("IK target X", -0.35, 0.15, initial_target[0]),
        "target_y": p.addUserDebugParameter("IK target Y", -0.20, 0.30, initial_target[1]),
        "target_z": p.addUserDebugParameter("IK target Z", 0.02, 0.35, initial_target[2]),
    }


def create_target_marker(initial_target):
    """Create a small red target marker used by inverse kinematics."""
    marker_visual = p.createVisualShape(
        p.GEOM_SPHERE,
        radius=MARKER_RADIUS,
        rgbaColor=[1, 0.15, 0.15, 1],
    )
    return p.createMultiBody(
        baseMass=0,
        baseVisualShapeIndex=marker_visual,
        basePosition=initial_target,
    )


def remove_debug_controls(sliders, kinematics_controls, reset_button):
    """Remove old side-panel controls before rebuilding them."""
    for _servo_index, _joint_id, slider in sliders:
        p.removeUserDebugItem(slider)
    for control_id in kinematics_controls.values():
        p.removeUserDebugItem(control_id)
    p.removeUserDebugItem(reset_button)


def draw_coordinate_system(position, orientation, length=0.04, lifetime=0.1):
    """Draw the tool coordinate frame: X red, Y green, Z blue."""
    rotation_matrix = p.getMatrixFromQuaternion(orientation)
    rotation_matrix = [rotation_matrix[0:3], rotation_matrix[3:6], rotation_matrix[6:9]]
    axes = (
        ([1, 0, 0], 0),
        ([0, 1, 0], 1),
        ([0, 0, 1], 2),
    )
    for color, column in axes:
        end_pos = [
            position[row] + rotation_matrix[row][column] * length
            for row in range(3)
        ]
        p.addUserDebugLine(position, end_pos, color, lineWidth=2, lifeTime=lifetime)


def get_link_names(robot):
    """Return a map from PyBullet link index to readable link name."""
    names = {-1: "base_link"}
    for joint_id in range(p.getNumJoints(robot)):
        info = p.getJointInfo(robot, joint_id)
        names[joint_id] = info[12].decode("utf-8", errors="replace")
    return names


def get_parent_links(robot):
    """Return child link index -> parent link index."""
    parents = {}
    for joint_id in range(p.getNumJoints(robot)):
        info = p.getJointInfo(robot, joint_id)
        parents[joint_id] = info[16]
    return parents


def are_neighbor_links(link_a, link_b, parents):
    """Parent-child links touch at joints, so do not treat them as collisions."""
    return parents.get(link_a) == link_b or parents.get(link_b) == link_a


def get_self_collision_pairs(robot, parents, ignored_pairs=None):
    """Find non-neighbor self-collision link index pairs."""
    ignored_pairs = ignored_pairs or set()
    collisions = set()
    for contact in p.getContactPoints(bodyA=robot, bodyB=robot):
        link_a = contact[3]
        link_b = contact[4]
        if link_a == link_b or are_neighbor_links(link_a, link_b, parents):
            continue

        pair = tuple(sorted((link_a, link_b)))
        if pair not in ignored_pairs:
            collisions.add(pair)

    return collisions


def format_collision_pairs(pairs, link_names, limit=3):
    """Convert collision link index pairs to readable text."""
    readable = []
    for link_a, link_b in sorted(pairs):
        readable.append(
            f"{link_names.get(link_a, str(link_a))} <-> {link_names.get(link_b, str(link_b))}"
        )
    return ", ".join(readable[:limit])


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
    runtime_dir = get_runtime_dir()
    tree = ET.parse(URDF)
    robot = tree.getroot()

    # PyBullet does not understand ROS package:// mesh paths.
    # Replace them with absolute paths to the STL files.
    for mesh_index, mesh in enumerate(robot.findall(".//mesh")):
        mesh_name = Path(mesh.attrib["filename"].replace("\\", "/")).name
        mesh_path = (MESH_DIR / mesh_name).resolve()
        if not str(mesh_path).isascii():
            mesh_dir = runtime_dir / "meshes"
            mesh_dir.mkdir(exist_ok=True)
            staged_mesh = mesh_dir / f"mesh_{mesh_index}{mesh_path.suffix}"
            shutil.copy2(mesh_path, staged_mesh)
            mesh_path = staged_mesh
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

    if robot.find(f"link[@name='{TOOL_TIP_LINK_NAME}']") is None:
        tip_link = ET.SubElement(robot, "link", {"name": TOOL_TIP_LINK_NAME})
        inertial = ET.SubElement(tip_link, "inertial")
        ET.SubElement(inertial, "origin", {"xyz": "0 0 0", "rpy": "0 0 0"})
        ET.SubElement(inertial, "mass", {"value": "0.001"})
        ET.SubElement(
            inertial,
            "inertia",
            {
                "ixx": "1e-8",
                "ixy": "0",
                "ixz": "0",
                "iyy": "1e-8",
                "iyz": "0",
                "izz": "1e-8",
            },
        )

        tip_joint = ET.SubElement(robot, "joint", {"name": "T_tool_tip", "type": "fixed"})
        ET.SubElement(
            tip_joint,
            "origin",
            {
                "xyz": " ".join(f"{value:.6g}" for value in TOOL_TIP_LOCAL_OFFSET),
                "rpy": "0 0 0",
            },
        )
        ET.SubElement(tip_joint, "parent", {"link": "Empty_TOOT"})
        ET.SubElement(tip_joint, "child", {"link": TOOL_TIP_LINK_NAME})

    output = runtime_dir / f"jxb2_runtime_{os.getpid()}.urdf"
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

    plane_urdf = prepare_plane_urdf()
    robot_urdf = make_runtime_urdf()

    p.connect(p.GUI)
    p.setAdditionalSearchPath(str(plane_urdf.parent))
    p.setGravity(0, 0, -9.8)

    p.loadURDF(str(plane_urdf))
    p.resetDebugVisualizerCamera(
        cameraDistance=0.6,
        cameraYaw=45,
        cameraPitch=-35,
        cameraTargetPosition=[-0.08, 0.06, 0.05],
    )

    robot = p.loadURDF(
        str(robot_urdf),
        basePosition=ROBOT_BASE_POSITION,
        useFixedBase=True,
        flags=p.URDF_USE_INERTIA_FROM_FILE | p.URDF_USE_SELF_COLLISION,
    )
    link_names = get_link_names(robot)
    parent_links = get_parent_links(robot)
    controllable_joints = get_controllable_joints(robot)
    end_effector_index = get_end_effector_index(robot)
    last_collision_print = 0
    last_fk_print = 0
    fk_text_id = None
    ik_was_limited = False
    ignored_collision_pairs = set()

    revolute_joints = []
    for joint_id in range(p.getNumJoints(robot)):
        info = p.getJointInfo(robot, joint_id)
        if info[2] == p.JOINT_REVOLUTE:
            revolute_joints.append((joint_id, info[1].decode("utf-8", errors="replace")))

    # Sliders show all five servo angles in degrees, exactly like JXB.ino.
    # The first four also drive URDF joints. Servo 5 is the real gripper.
    sliders = create_servo_sliders(robot, revolute_joints)
    servo_angles = SERVO_INIT_DEG[:]
    apply_servo_angles(robot, sliders, servo_angles)
    p.stepSimulation()
    initial_target, initial_orientation = get_end_effector_pose(robot, end_effector_index)
    kinematics_controls = create_kinematics_controls(initial_target)
    target_marker = create_target_marker(initial_target)
    draw_coordinate_system(initial_target, initial_orientation)

    ignored_collision_pairs = get_self_collision_pairs(robot, parent_links)
    if ignored_collision_pairs:
        pairs = format_collision_pairs(ignored_collision_pairs, link_names, limit=6)
        print(f"Ignoring initial model contacts: {pairs}")

    wifi_sender.send(servo_angles)

    reset_button = p.addUserDebugParameter("areset", 1, 0, 0)
    last_reset_value = p.readUserDebugParameter(reset_button)

    while p.isConnected():
        reset_value = p.readUserDebugParameter(reset_button)
        if reset_value != last_reset_value:
            servo_angles = SERVO_INIT_DEG[:]
            apply_servo_angles(robot, sliders, servo_angles)
            p.stepSimulation()
            reset_target, reset_orientation = get_end_effector_pose(robot, end_effector_index)
            p.resetBasePositionAndOrientation(target_marker, reset_target, [0, 0, 0, 1])
            draw_coordinate_system(reset_target, reset_orientation)

            remove_debug_controls(sliders, kinematics_controls, reset_button)
            sliders = create_servo_sliders(robot, revolute_joints)
            kinematics_controls = create_kinematics_controls(reset_target)
            reset_button = p.addUserDebugParameter("areset", 1, 0, 0)
            last_reset_value = p.readUserDebugParameter(reset_button)
            wifi_sender.send(servo_angles)
            continue

        current_slider_values = [
            int(round(p.readUserDebugParameter(slider)))
            for _servo_index, _joint_id, slider in sliders
        ]
        ik_enabled = p.readUserDebugParameter(kinematics_controls["ik_enable"]) >= 0.5
        ik_target = [
            p.readUserDebugParameter(kinematics_controls["target_x"]),
            p.readUserDebugParameter(kinematics_controls["target_y"]),
            p.readUserDebugParameter(kinematics_controls["target_z"]),
        ]
        p.resetBasePositionAndOrientation(target_marker, ik_target, [0, 0, 0, 1])

        if ik_enabled:
            joint_targets, servo_angles, ik_was_limited = calculate_ik(
                robot,
                end_effector_index,
                ik_target,
                controllable_joints,
            )
            apply_joint_targets(robot, joint_targets)
        else:
            ik_was_limited = False
            for servo_index, joint_id, slider in sliders:
                servo_angle_deg = current_slider_values[servo_index]
                servo_angles[servo_index] = int(round(servo_angle_deg))

                if joint_id is None:
                    continue

                target_rad = servo_deg_to_joint_rad(servo_index, servo_angle_deg)
                p.setJointMotorControl2(
                    robot,
                    joint_id,
                    p.POSITION_CONTROL,
                    targetPosition=target_rad,
                    force=IK_FORCE,
                )

        p.stepSimulation()
        end_pos, end_orn = get_end_effector_pose(robot, end_effector_index)
        draw_coordinate_system(end_pos, end_orn)

        now = time.time()
        if now - last_fk_print >= FK_PRINT_INTERVAL:
            euler_deg = [math.degrees(value) for value in p.getEulerFromQuaternion(end_orn)]
            target_error = math.dist(end_pos, ik_target)
            text = (
                f"FK end: x={end_pos[0]:.3f}, y={end_pos[1]:.3f}, z={end_pos[2]:.3f}\n"
                f"rpy={euler_deg[0]:.1f}, {euler_deg[1]:.1f}, {euler_deg[2]:.1f} deg\n"
                f"IK {'ON' if ik_enabled else 'OFF'} target: "
                f"{ik_target[0]:.3f}, {ik_target[1]:.3f}, {ik_target[2]:.3f}\n"
                f"IK error={target_error:.3f} m"
                f"{' servo limit' if ik_was_limited else ''}"
            )
            if fk_text_id is not None:
                p.removeUserDebugItem(fk_text_id)
            fk_text_id = p.addUserDebugText(
                text,
                [0.05, -0.28, 0.28],
                textColorRGB=[0.1, 0.9, 1.0],
                textSize=1.1,
                lifeTime=FK_PRINT_INTERVAL + 0.1,
            )
            print(text.replace("\n", " | "))
            last_fk_print = now

        collisions = get_self_collision_pairs(robot, parent_links, ignored_collision_pairs)
        if collisions:
            now = time.time()
            if now - last_collision_print >= COLLISION_PRINT_INTERVAL:
                pairs = format_collision_pairs(collisions, link_names)
                print(f"Collision detected, WiFi command not sent: {pairs}")
                last_collision_print = now
        else:
            wifi_sender.send(servo_angles)

        time.sleep(1 / 240)


if __name__ == "__main__":
    main()
