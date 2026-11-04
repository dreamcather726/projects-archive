import argparse
import json
import random
from pathlib import Path

import numpy as np


# 当前文件位置：JXB/jxb_study/scripts/target_sampler.py
SCRIPT_DIR = Path(__file__).resolve().parent
STUDY_DIR = SCRIPT_DIR.parent
OUTPUT_DIR = STUDY_DIR / "outputs"
DEFAULT_POINTS_PATH = OUTPUT_DIR / "reachable_points.npy"
DEFAULT_OUTPUT_PATH = OUTPUT_DIR / "random_targets.json"


def round_vector(values, digits=2):
    """把 xyz 坐标保留到指定小数位，主要用于打印和保存给人看。"""
    return [round(float(value), digits) for value in values]


def load_reachable_points(points_path=DEFAULT_POINTS_PATH):
    """读取 reachable_workspace.py 生成的真实可达点云。"""
    points_path = Path(points_path)
    if not points_path.exists():
        raise FileNotFoundError(
            f"没有找到 {points_path}，请先运行 reachable_workspace.py 生成可达点云。"
        )

    points = np.load(points_path)
    if points.ndim != 2 or points.shape[1] != 3:
        raise ValueError("reachable_points.npy 的形状应该是 [点数量, 3]")
    return points.astype(np.float32)


def sample_target_from_points(points, rng, noise_radius=0.005):
    """从真实可达点云里随机抽一个目标点。

    这样做的好处是：抽出来的点本身就是某个机械臂姿态真的到达过的位置，
    比在 xyz 外包围盒里随机更可靠。
    """
    point_index = rng.randrange(len(points))
    target = points[point_index].copy()

    # 加一点很小的扰动，让目标点不是永远重复落在采样点上。
    # noise_radius 单位是米，默认 0.005 表示 5 毫米。
    if noise_radius > 0:
        noise = np.asarray(
            [
                rng.uniform(-noise_radius, noise_radius),
                rng.uniform(-noise_radius, noise_radius),
                rng.uniform(-noise_radius, noise_radius),
            ],
            dtype=np.float32,
        )
        target = target + noise

    return target


def sample_targets(count, seed=42, noise_radius=0.005, points_path=DEFAULT_POINTS_PATH):
    """一次生成多个随机目标点。"""
    rng = random.Random(seed)
    points = load_reachable_points(points_path)
    targets = []

    for _ in range(count):
        target = sample_target_from_points(points, rng, noise_radius)
        targets.append(target)

    return np.stack(targets, axis=0)


def save_targets(targets, output_path=DEFAULT_OUTPUT_PATH):
    """保存随机目标点，默认保存成两位小数，方便直接查看。"""
    output_path = Path(output_path)
    output_path.parent.mkdir(parents=True, exist_ok=True)

    data = {
        "targets": [round_vector(target) for target in targets],
    }
    with output_path.open("w", encoding="utf-8") as file:
        json.dump(data, file, ensure_ascii=False, indent=2)

    return output_path


def parse_args():
    parser = argparse.ArgumentParser(description="从可达点云中随机生成目标点。")
    parser.add_argument("--count", type=int, default=10, help="要生成的目标点数量。")
    parser.add_argument("--seed", type=int, default=42, help="随机种子，方便复现实验结果。")
    parser.add_argument(
        "--noise-radius",
        type=float,
        default=0.005,
        help="目标点随机扰动半径，单位米；0 表示不加扰动。",
    )
    parser.add_argument(
        "--points",
        default=str(DEFAULT_POINTS_PATH),
        help="reachable_workspace.py 生成的 reachable_points.npy 路径。",
    )
    parser.add_argument(
        "--output",
        default=str(DEFAULT_OUTPUT_PATH),
        help="随机目标点保存路径。",
    )
    return parser.parse_args()


def main():
    args = parse_args()
    targets = sample_targets(
        count=args.count,
        seed=args.seed,
        noise_radius=args.noise_radius,
        points_path=args.points,
    )
    output_path = save_targets(targets, args.output)

    print(f"目标点数量: {len(targets)}")
    print(f"前 5 个目标点: {[round_vector(target) for target in targets[:5]]}")
    print(f"目标点已保存: {output_path}")


if __name__ == "__main__":
    main()
