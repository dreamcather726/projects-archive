import argparse
import importlib.util
import json
import random
import time
from pathlib import Path

import numpy as np
import pybullet as p
import pybullet_data

from reward_geometry import calculate_reach_reward
from reach_env import (
    ACTION_PENALTY_WEIGHT,
    ALIGNMENT_PROGRESS_WEIGHT,
    MIN_MOVEMENT_DEG,
    MOVEMENT_REWARD_WEIGHT,
    NO_MOVEMENT_PENALTY,
    PROGRESS_WEIGHT,
    SMALL_MOVEMENT_PENALTY,
    STEP_PENALTY,
)


SCRIPT_DIR = Path(__file__).resolve().parent
STUDY_DIR = SCRIPT_DIR.parent
OUTPUT_DIR = STUDY_DIR / "outputs"
JXB_DIR = STUDY_DIR.parent
PYBULLET_FILE = JXB_DIR / "Pybullet" / "Pybullet.py"


def load_pybullet_tools():
    """按文件路径加载 JXB/Pybullet/Pybullet.py，避免从 scripts 目录运行时导入失败。"""
    if not PYBULLET_FILE.exists():
        raise FileNotFoundError(f"没有找到 PyBullet 工具文件: {PYBULLET_FILE}")

    spec = importlib.util.spec_from_file_location("jxb_pybullet_tools", PYBULLET_FILE)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


pybullet_tools = load_pybullet_tools()
ROBOT_BASE_POSITION = pybullet_tools.ROBOT_BASE_POSITION
SERVO_INIT_DEG = pybullet_tools.SERVO_INIT_DEG
draw_coordinate_system = pybullet_tools.draw_coordinate_system
make_runtime_urdf = pybullet_tools.make_runtime_urdf
servo_deg_to_joint_rad = pybullet_tools.servo_deg_to_joint_rad


DEFAULT_TARGETS_PATH = OUTPUT_DIR / "random_targets.json"
DEFAULT_POINTS_PATH = OUTPUT_DIR / "reachable_points.npy"
JOINT_COUNT = 4
PRINT_INTERVAL = 0.3

# 目标点小球的半径，单位是米。这个值影响视觉效果，但不影响奖励计算。
TARGET_MARKER_RADIUS = 0.012


def get_controllable_joints(robot_id):
    """找出 URDF 里真正可以控制的关节。"""
    joints = []
    for joint_id in range(p.getNumJoints(robot_id)):
        joint_info = p.getJointInfo(robot_id, joint_id)
        joint_type = joint_info[2]
        if joint_type in (p.JOINT_REVOLUTE, p.JOINT_PRISMATIC):
            joints.append(joint_id)
    return joints[:JOINT_COUNT]


def get_tool_tip_link(robot_id):
    """优先使用 tool_tip 作为末端；如果没有，就使用最后一个 link。"""
    for joint_id in range(p.getNumJoints(robot_id)):
        link_name = p.getJointInfo(robot_id, joint_id)[12].decode("utf-8", errors="replace")
        if link_name == "tool_tip":
            return joint_id
    return p.getNumJoints(robot_id) - 1


def get_end_pose(robot_id, tip_link):
    """读取末端位置和末端四元数朝向。"""
    link_state = p.getLinkState(robot_id, tip_link, computeForwardKinematics=True)
    end_position = np.asarray(link_state[4], dtype=np.float32)
    end_orientation = link_state[5]
    return end_position, end_orientation


def create_servo_sliders(joints):
    """创建 4 个舵机滑条，用来手动改变机械臂姿态。"""
    sliders = []
    for servo_index, joint_id in enumerate(joints):
        slider = p.addUserDebugParameter(
            f"servo {servo_index + 1}",
            0,
            180,
            SERVO_INIT_DEG[servo_index],
        )
        sliders.append((servo_index, joint_id, slider))
    return sliders


def apply_slider_angles(robot_id, sliders):
    """读取滑条角度，并应用到 PyBullet 关节。"""
    servo_angles = []
    for servo_index, joint_id, slider in sliders:
        servo_angle = p.readUserDebugParameter(slider)
        servo_angles.append(servo_angle)

        # 真实舵机角度是 0-180 度，PyBullet 关节使用弧度。
        joint_rad = servo_deg_to_joint_rad(servo_index, servo_angle)
        p.setJointMotorControl2(
            robot_id,
            joint_id,
            p.POSITION_CONTROL,
            targetPosition=joint_rad,
            force=5,
            positionGain=0.12,
        )
    return servo_angles


def load_targets_from_json(targets_path):
    """从 random_targets.json 读取目标点。"""
    targets_path = Path(targets_path)
    if not targets_path.exists():
        return []

    with targets_path.open("r", encoding="utf-8") as file:
        data = json.load(file)

    targets = data.get("targets", [])
    return [np.asarray(target, dtype=np.float32) for target in targets]


def load_random_target_from_points(points_path, rng):
    """如果没有 random_targets.json，就直接从 reachable_points.npy 随机抽一个目标。"""
    points_path = Path(points_path)
    if not points_path.exists():
        raise FileNotFoundError(
            f"没有找到 {points_path}，请先运行 reachable_workspace.py 生成可达点云。"
        )

    points = np.load(points_path)
    point_index = rng.randrange(len(points))
    return np.asarray(points[point_index], dtype=np.float32)


def choose_initial_target(args, rng):
    """选择初始目标点：优先使用命令行，其次使用 json，最后使用可达点云。"""
    if args.target is not None:
        return np.asarray(args.target, dtype=np.float32), []

    targets = load_targets_from_json(args.targets)
    if targets:
        return targets[0], targets

    return load_random_target_from_points(args.points, rng), []


def create_target_marker(target):
    """在 PyBullet 里创建红色目标点小球。"""
    visual = p.createVisualShape(
        p.GEOM_SPHERE,
        radius=TARGET_MARKER_RADIUS,
        rgbaColor=[1.0, 0.1, 0.1, 1.0],
    )
    return p.createMultiBody(
        baseMass=0,
        baseVisualShapeIndex=visual,
        basePosition=target,
    )


def draw_target_line(end_position, target_position, lifetime=0.1):
    """画一条从末端指向目标点的线，方便观察目标方向。"""
    p.addUserDebugLine(
        end_position,
        target_position,
        lineColorRGB=[1.0, 0.2, 0.2],
        lineWidth=2,
        lifeTime=lifetime,
    )


def format_vec(values):
    """把 xyz 坐标格式化成小数点后两位。"""
    return ", ".join(f"{float(value):.2f}" for value in values)


def parse_args():
    parser = argparse.ArgumentParser(description="实时显示 JXB 奖励函数调试信息。")
    parser.add_argument("--targets", default=str(DEFAULT_TARGETS_PATH), help="random_targets.json 路径。")
    parser.add_argument("--points", default=str(DEFAULT_POINTS_PATH), help="reachable_points.npy 路径。")
    parser.add_argument("--target", nargs=3, type=float, default=None, help="手动指定目标点 x y z。")
    parser.add_argument("--seed", type=int, default=42, help="随机种子。")
    parser.add_argument(
        "--forward-axis",
        nargs=3,
        type=float,
        default=[1.0, 0.0, 0.0],
        help="末端哪个局部轴算正前方，默认 X 轴。",
    )
    return parser.parse_args()


def main():
    args = parse_args()
    rng = random.Random(args.seed)
    target, target_list = choose_initial_target(args, rng)
    target_index = 0

    p.connect(p.GUI)
    p.setAdditionalSearchPath(pybullet_data.getDataPath())
    p.resetSimulation()
    p.setGravity(0, 0, -9.8)
    p.loadURDF("plane.urdf")
    p.resetDebugVisualizerCamera(
        cameraDistance=0.18,
        cameraYaw=45,
        cameraPitch=-28,
        cameraTargetPosition=[-0.02, -0.03, 0.12],
    )

    robot_id = p.loadURDF(
        str(make_runtime_urdf()),
        basePosition=ROBOT_BASE_POSITION,
        useFixedBase=True,
        flags=p.URDF_USE_INERTIA_FROM_FILE,
    )
    joints = get_controllable_joints(robot_id)
    tip_link = get_tool_tip_link(robot_id)
    sliders = create_servo_sliders(joints)
    target_marker = create_target_marker(target)

    # 如果 random_targets.json 里有多个目标，按按钮可以切换到下一个。
    next_target_button = p.addUserDebugParameter("next target", 1, 0, 0)
    last_button_value = p.readUserDebugParameter(next_target_button)

    text_id = None
    last_print_time = 0.0
    last_distance = None
    last_servo_angles = None

    while p.isConnected():
        button_value = p.readUserDebugParameter(next_target_button)
        if button_value != last_button_value:
            last_button_value = button_value
            if target_list:
                target_index = (target_index + 1) % len(target_list)
                target = target_list[target_index]
            else:
                target = load_random_target_from_points(args.points, rng)
            p.resetBasePositionAndOrientation(target_marker, target, [0, 0, 0, 1])
            last_distance = None
            last_servo_angles = None

        servo_angles = apply_slider_angles(robot_id, sliders)
        p.stepSimulation()

        # 如果用户关闭了 PyBullet 窗口，后面就不能再画线或文字了。
        # 这里提前退出循环，避免出现 Not connected to physics server 报错。
        if not p.isConnected():
            break

        end_position, end_orientation = get_end_pose(robot_id, tip_link)
        reward_info = calculate_reach_reward(
            end_position,
            end_orientation,
            target,
            local_forward_axis=args.forward_axis,
        )
        distance = reward_info["distance"]
        if last_distance is None:
            progress = 0.0
        else:
            progress = last_distance - distance
        last_distance = distance

        base_progress_reward = PROGRESS_WEIGHT * progress
        alignment_score = (reward_info["alignment_cos"] + 1.0) / 2.0
        if progress > 0:
            progress_reward = base_progress_reward * (
                1.0 + ALIGNMENT_PROGRESS_WEIGHT * alignment_score
            )
        else:
            progress_reward = base_progress_reward

        # GUI 滑条不是策略网络 action，这里用滑条角度变化量估算“运动量”。
        if last_servo_angles is None:
            movement_deg = 0.0
        else:
            movement_deg = float(np.linalg.norm(np.asarray(servo_angles) - np.asarray(last_servo_angles)))
        last_servo_angles = servo_angles

        if movement_deg < 1e-6:
            movement_reward = -NO_MOVEMENT_PENALTY
            movement_status = "no_movement"
        elif movement_deg < MIN_MOVEMENT_DEG:
            movement_reward = -SMALL_MOVEMENT_PENALTY
            movement_status = "too_small"
        else:
            movement_reward = MOVEMENT_REWARD_WEIGHT * (movement_deg - MIN_MOVEMENT_DEG)
            movement_status = "valid"

        # GUI 调试里没有策略网络 action，这里 action_penalty 保持为 0。
        action_penalty = 0.0
        step_reward = progress_reward + movement_reward - STEP_PENALTY - action_penalty

        try:
            draw_coordinate_system(end_position, end_orientation)
            draw_target_line(end_position, target)
        except p.error:
            # 用户关闭窗口时，PyBullet 可能刚好断开连接。
            # 这种情况不是程序逻辑错误，直接结束调试显示即可。
            break

        now = time.time()
        if now - last_print_time >= PRINT_INTERVAL:
            text = (
                f"end: {format_vec(end_position)}\n"
                f"target: {format_vec(target)}\n"
                f"distance: {distance:.2f} m\n"
                f"progress: {progress:+.4f} m\n"
                f"base progress reward: {base_progress_reward:+.3f}\n"
                f"cos: {reward_info['alignment_cos']:.2f}\n"
                f"alignment score: {alignment_score:.2f}\n"
                f"progress reward: {progress_reward:+.3f}\n"
                f"movement: {movement_deg:.2f} deg\n"
                f"movement status: {movement_status}\n"
                f"movement reward: {movement_reward:+.3f}\n"
                f"step penalty: -{STEP_PENALTY:.3f}\n"
                f"reward: {step_reward:+.3f}"
            )
            if text_id is not None:
                p.removeUserDebugItem(text_id)
            text_id = p.addUserDebugText(
                text,
                [0.05, -0.30, 0.30],
                textColorRGB=[0.1, 0.9, 1.0],
                textSize=1.1,
                lifeTime=PRINT_INTERVAL + 0.1,
            )
            print(text.replace("\n", " | "))
            last_print_time = now

        time.sleep(1 / 240)


if __name__ == "__main__":
    main()
