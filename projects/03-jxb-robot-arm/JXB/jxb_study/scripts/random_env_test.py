import argparse
import time

import numpy as np

from reach_env import JOINT_COUNT, JXBReachEnv


def progress_iter(total_steps):
    """优先使用 tqdm 进度条；如果没有安装 tqdm，就退回到普通 range。"""
    try:
        from tqdm import trange

        return trange(1, total_steps + 1, desc="random test", unit="step")
    except ImportError:
        return range(1, total_steps + 1)


def format_vec(values):
    """把 xyz 或 action 数组格式化成小数点后三位。"""
    return "[" + ", ".join(f"{float(value):+.3f}" for value in values) + "]"


def format_step_line(step, reward, info):
    """用“进度调”的形式展示每一步奖励来源。"""
    return (
        f"step={step:05d} "
        f"progress={info['progress']:+.4f}m "
        f"base={info['base_progress_reward']:+.3f} "
        f"align_score={info['alignment_score']:.3f} "
        f"progress_reward={info['progress_reward']:+.3f} "
        f"move={info['movement_deg']:.3f}deg "
        f"move_reward={info['movement_reward']:+.3f} "
        f"move_status={info['movement_status']} "
        f"step_penalty=-{info['step_penalty']:.3f} "
        f"action_penalty=-{info['action_penalty']:.3f} "
        f"reward={reward:+.3f} "
        f"distance={info['distance']:.3f}m "
        f"cos={info['alignment_cos']:.3f} "
        f"dist_ok={info['distance_success']} "
        f"align_ok={info['alignment_success']} "
        f"success={info['success']}"
    )


def print_success_detail(episode, episode_step, total_step, reward, info, action, env):
    """成功时打印更完整的信息，方便分析成功案例。"""
    end_position, _end_orientation = env._end_pose()
    print("\n===== 成功案例 =====")
    print(f"episode: {episode}")
    print(f"episode_step: {episode_step}")
    print(f"total_step: {total_step}")
    print(f"target: {format_vec(env.target)}")
    print(f"end: {format_vec(end_position)}")
    print(f"action: {format_vec(action)}")
    print(f"distance: {info['distance']:.4f} m")
    print(f"distance_success: {info['distance_success']}")
    print(f"alignment_success: {info['alignment_success']}")
    print(f"progress: {info['progress']:+.4f} m")
    print(f"base_progress_reward: {info['base_progress_reward']:+.3f}")
    print(f"alignment_score: {info['alignment_score']:.3f}")
    print(f"progress_reward: {info['progress_reward']:+.3f}")
    print(f"movement_deg: {info['movement_deg']:.3f} deg")
    print(f"movement_reward: {info['movement_reward']:+.3f}")
    print(f"movement_status: {info['movement_status']}")
    print(f"step_penalty: -{info['step_penalty']:.3f}")
    print(f"action_penalty: -{info['action_penalty']:.3f}")
    print(f"success_bonus: 已加入最终 reward")
    print(f"final_reward: {reward:+.3f}")
    print("====================\n")


def parse_args():
    parser = argparse.ArgumentParser(description="用随机动作测试 JXBReachEnv。")
    parser.add_argument("--steps", type=int, default=30, help="随机动作测试总步数。")
    parser.add_argument("--seed", type=int, default=42, help="随机种子。")
    parser.add_argument("--render", action="store_true", help="打开 PyBullet GUI。")
    parser.add_argument(
        "--target",
        nargs=3,
        type=float,
        default=None,
        help="固定目标点 x y z；不填则使用随机目标。",
    )
    return parser.parse_args()


def main():
    args = parse_args()
    rng = np.random.default_rng(args.seed)
    env = JXBReachEnv(render=args.render, seed=args.seed, fixed_target=args.target)
    success_count = 0
    episode = 1
    episode_step = 0

    try:
        obs = env.reset()
        print(f"初始观测维度: {obs.shape}")
        print(f"初始目标点: {format_vec(env.target)}")

        progress = progress_iter(args.steps)
        for total_step in progress:
            episode_step += 1

            # 随机动作范围是 [-1, 1]，对应 4 个可控关节。
            action = rng.uniform(-1.0, 1.0, size=JOINT_COUNT)
            obs, reward, done, info = env.step(action)

            if hasattr(progress, "set_postfix"):
                progress.set_postfix(
                    reward=f"{reward:+.3f}",
                    distance=f"{info['distance']:.3f}",
                    progress=f"{info['progress']:+.4f}",
                    success=success_count,
                )

            if info["success"]:
                success_count += 1
                if hasattr(progress, "write"):
                    progress.write(format_step_line(total_step, reward, info))
                else:
                    print(format_step_line(total_step, reward, info))
                print_success_detail(
                    episode=episode,
                    episode_step=episode_step,
                    total_step=total_step,
                    reward=reward,
                    info=info,
                    action=action,
                    env=env,
                )

            if args.render:
                time.sleep(1 / 60)

            if done:
                episode += 1
                episode_step = 0
                obs = env.reset()

        print(f"\n总步数: {args.steps}")
        print(f"成功次数: {success_count}")
    finally:
        env.close()


if __name__ == "__main__":
    main()
