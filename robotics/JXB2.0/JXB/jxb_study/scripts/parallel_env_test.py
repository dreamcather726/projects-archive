import argparse

import numpy as np

from reach_env import JOINT_COUNT, VecJXBReachEnv


def progress_iter(total_steps):
    """优先使用 tqdm 进度条；如果没有安装 tqdm，就退回到普通 range。"""
    try:
        from tqdm import trange

        return trange(1, total_steps + 1, desc="parallel random test", unit="step")
    except ImportError:
        return range(1, total_steps + 1)


def format_vec(values):
    """把数组格式化成小数点后三位。"""
    return "[" + ", ".join(f"{float(value):+.3f}" for value in values) + "]"


def print_success_detail(total_step, env_index, reward, info):
    """某个并行环境成功时，打印这个成功样本的信息。"""
    print("\n===== 并行成功案例 =====")
    print(f"total_step: {total_step}")
    print(f"env_index: {env_index}")
    print(f"distance: {info['distance']:.4f} m")
    print(f"distance_success: {info['distance_success']}")
    print(f"alignment_success: {info['alignment_success']}")
    print(f"progress: {info['progress']:+.4f} m")
    print(f"progress_reward: {info['progress_reward']:+.3f}")
    print(f"movement_deg: {info['movement_deg']:.3f} deg")
    print(f"movement_reward: {info['movement_reward']:+.3f}")
    print(f"movement_status: {info['movement_status']}")
    print(f"cos: {info['alignment_cos']:.3f}")
    print(f"reward: {reward:+.3f}")
    print("========================\n")


def parse_args():
    parser = argparse.ArgumentParser(description="测试多个 JXBReachEnv 并行 step。")
    parser.add_argument("--num-envs", type=int, default=8, help="并行机械臂环境数量。")
    parser.add_argument("--steps", type=int, default=1000, help="每个并行环境推进多少步。")
    parser.add_argument("--seed", type=int, default=42, help="随机种子。")
    return parser.parse_args()


def main():
    args = parse_args()
    rng = np.random.default_rng(args.seed)
    env = VecJXBReachEnv(num_envs=args.num_envs, seed=args.seed)
    success_count = 0

    try:
        obs = env.reset()
        print(f"并行环境数量: {args.num_envs}")
        print(f"观测形状: {obs.shape}")

        progress = progress_iter(args.steps)
        for total_step in progress:
            # 每个环境一组 4 关节动作，所以 actions 形状是 [num_envs, 4]。
            actions = rng.uniform(-1.0, 1.0, size=(args.num_envs, JOINT_COUNT))
            obs, rewards, dones, infos = env.step(actions)

            step_success = sum(1 for info in infos if info["success"])
            success_count += step_success

            if hasattr(progress, "set_postfix"):
                progress.set_postfix(
                    reward=f"{float(np.mean(rewards)):+.3f}",
                    success=success_count,
                    done=int(np.sum(dones)),
                )

            for env_index, info in enumerate(infos):
                if info["success"]:
                    if hasattr(progress, "write"):
                        progress.write(
                            f"success step={total_step} env={env_index} "
                            f"distance={info['distance']:.3f} cos={info['alignment_cos']:.3f}"
                        )
                    print_success_detail(total_step, env_index, rewards[env_index], info)

        print(f"\n每个环境步数: {args.steps}")
        print(f"并行环境数量: {args.num_envs}")
        print(f"总环境步数: {args.steps * args.num_envs}")
        print(f"成功次数: {success_count}")
    finally:
        env.close()


if __name__ == "__main__":
    main()
