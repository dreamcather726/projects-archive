import argparse
import importlib.util
import json
import random
from pathlib import Path

import numpy as np
import pybullet as p
import pybullet_data


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
make_runtime_urdf = pybullet_tools.make_runtime_urdf
servo_deg_to_joint_rad = pybullet_tools.servo_deg_to_joint_rad


JOINT_COUNT = 4

# 这里先按普通舵机范围采样：0 到 180 度。
# 如果以后某个关节实际不能转这么大，就改对应那一项。
DEFAULT_SERVO_LIMITS = [
    (0.0, 180.0),
    (0.0, 180.0),
    (0.0, 180.0),
    (0.0, 180.0),
]


def round_vector(values, digits=2):
    """把 xyz 数值统一保留指定小数位，方便阅读和保存。"""
    return [round(float(value), digits) for value in values]


def get_controllable_joints(robot_id):
    """找出 URDF 里真正可以转动的关节。"""
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


def random_servo_angles(rng, servo_limits):
    """随机生成一组舵机角度，用来探索机械臂可达空间。"""
    return [rng.uniform(low, high) for low, high in servo_limits]


def apply_servo_angles(robot_id, joints, servo_angles):
    """把舵机角度转换成 PyBullet 关节角度，并直接设置到仿真模型里。"""
    for servo_index, joint_id in enumerate(joints):
        joint_rad = servo_deg_to_joint_rad(servo_index, servo_angles[servo_index])
        p.resetJointState(robot_id, joint_id, joint_rad)


def read_tip_position(robot_id, tip_link):
    """读取当前姿态下末端点在世界坐标系中的 xyz 位置。"""
    link_state = p.getLinkState(robot_id, tip_link, computeForwardKinematics=True)
    return np.asarray(link_state[4], dtype=np.float32)


def sample_workspace(samples, seed, servo_limits):
    """通过大量随机关节角度采样，估计机械臂末端能到达的空间。"""
    rng = random.Random(seed)

    # DIRECT 模式不会打开可视化窗口，适合快速计算。
    client_id = p.connect(p.DIRECT)
    p.setAdditionalSearchPath(pybullet_data.getDataPath(), physicsClientId=client_id)

    try:
        p.resetSimulation(physicsClientId=client_id)
        p.setGravity(0, 0, -9.8, physicsClientId=client_id)
        p.loadURDF("plane.urdf", physicsClientId=client_id)

        robot_id = p.loadURDF(
            str(make_runtime_urdf()),
            basePosition=ROBOT_BASE_POSITION,
            useFixedBase=True,
            flags=p.URDF_USE_INERTIA_FROM_FILE,
            physicsClientId=client_id,
        )

        joints = get_controllable_joints(robot_id)
        tip_link = get_tool_tip_link(robot_id)
        points = []

        for _ in range(samples):
            # 随机给 4 个可控关节一组舵机角度。
            servo_angles = random_servo_angles(rng, servo_limits)

            # 用正运动学计算这组角度下的末端位置。
            apply_servo_angles(robot_id, joints, servo_angles)
            p.stepSimulation(physicsClientId=client_id)
            points.append(read_tip_position(robot_id, tip_link))

        return np.stack(points, axis=0)
    finally:
        p.disconnect(physicsClientId=client_id)


def summarize_workspace(points, margin):
    """把采样点云整理成最小/最大范围和推荐目标范围。"""
    raw_min = points.min(axis=0)
    raw_max = points.max(axis=0)

    # 百分位范围会去掉极少数边缘点，比直接 min/max 更适合做随机目标范围。
    p02 = np.percentile(points, 2, axis=0)
    p98 = np.percentile(points, 98, axis=0)

    # 再额外缩小一点 margin，避免目标点太贴近极限位置。
    safe_min = p02 + margin
    safe_max = p98 - margin

    # JSON 里的范围只用于阅读和后续配置，所以保留两位小数即可。
    return {
        "raw_min": round_vector(raw_min),
        "raw_max": round_vector(raw_max),
        "percentile_2": round_vector(p02),
        "percentile_98": round_vector(p98),
        "safe_min": round_vector(safe_min),
        "safe_max": round_vector(safe_max),
    }


def save_results(points, summary, output_dir=OUTPUT_DIR):
    """保存工作空间统计结果，后面生成随机目标点时可以复用。"""
    output_dir.mkdir(parents=True, exist_ok=True)
    points_path = output_dir / "reachable_points.npy"
    summary_path = output_dir / "reachable_workspace.json"

    # 点云保留原始精度，后面如果从真实可达点里采目标会更准确。
    np.save(points_path, points)
    with summary_path.open("w", encoding="utf-8") as file:
        json.dump(summary, file, ensure_ascii=False, indent=2)

    return points_path, summary_path


def plot_workspace(points, output_dir=OUTPUT_DIR):
    """把可达点云画成 3D 散点图，并保存到文件。"""
    # matplotlib 只在绘图时才导入，这样如果只想计算范围，不会额外影响启动。
    import matplotlib.pyplot as plt

    output_dir.mkdir(parents=True, exist_ok=True)
    plot_path = output_dir / "reachable_workspace.png"

    x_values = points[:, 0]
    y_values = points[:, 1]
    z_values = points[:, 2]

    fig = plt.figure(figsize=(8, 6))
    ax = fig.add_subplot(111, projection="3d")

    # c=z_values 表示用高度给点着色，能更容易看出上下空间分布。
    scatter = ax.scatter(
        x_values,
        y_values,
        z_values,
        c=z_values,
        cmap="viridis",
        s=4,
        alpha=0.55,
    )

    ax.set_title("JXB reachable workspace")
    ax.set_xlabel("X / m")
    ax.set_ylabel("Y / m")
    ax.set_zlabel("Z / m")

    # 尽量让三个坐标轴比例一致，否则点云形状会被视觉拉伸。
    max_range = max(
        float(x_values.max() - x_values.min()),
        float(y_values.max() - y_values.min()),
        float(z_values.max() - z_values.min()),
    )
    center_x = float((x_values.max() + x_values.min()) / 2.0)
    center_y = float((y_values.max() + y_values.min()) / 2.0)
    center_z = float((z_values.max() + z_values.min()) / 2.0)
    half_range = max_range / 2.0

    ax.set_xlim(center_x - half_range, center_x + half_range)
    ax.set_ylim(center_y - half_range, center_y + half_range)
    ax.set_zlim(center_z - half_range, center_z + half_range)

    fig.colorbar(scatter, ax=ax, label="Z / m", shrink=0.75)
    fig.tight_layout()
    fig.savefig(plot_path, dpi=200)
    plt.close(fig)

    return plot_path


def parse_args():
    parser = argparse.ArgumentParser(description="计算 JXB 机械臂末端可达范围。")
    parser.add_argument("--samples", type=int, default=5000, help="随机采样次数，越大越准确。")
    parser.add_argument("--seed", type=int, default=42, help="随机种子，方便复现实验结果。")
    parser.add_argument("--margin", type=float, default=0.01, help="推荐范围向内收缩的安全边距，单位米。")
    parser.add_argument("--no-plot", action="store_true", help="只计算范围，不绘制点云图。")
    return parser.parse_args()


def main():
    args = parse_args()
    points = sample_workspace(
        samples=args.samples,
        seed=args.seed,
        servo_limits=DEFAULT_SERVO_LIMITS,
    )
    summary = summarize_workspace(points, margin=args.margin)
    points_path, summary_path = save_results(points, summary, OUTPUT_DIR)
    plot_path = None
    if not args.no_plot:
        plot_path = plot_workspace(points, OUTPUT_DIR)

    print(f"采样点数量: {len(points)}")
    print(f"原始范围 min: {summary['raw_min']}")
    print(f"原始范围 max: {summary['raw_max']}")
    print(f"推荐目标 min: {summary['safe_min']}")
    print(f"推荐目标 max: {summary['safe_max']}")
    print(f"点云已保存: {points_path}")
    print(f"统计结果已保存: {summary_path}")
    if plot_path is not None:
        print(f"点云图已保存: {plot_path}")


if __name__ == "__main__":
    main()
