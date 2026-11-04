import argparse
import time

import torch

from reach_env import JXBReachEnv
from train_fixed_target_ppo import ActorCritic


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--target", nargs=3, type=float, default=[-0.02, -0.10, 0.14])
    parser.add_argument("--model-path", default="arm_ppo_final.pt")
    parser.add_argument("--hidden-dim", type=int, default=128)
    parser.add_argument("--action-dim", type=int, default=4)
    parser.add_argument("--step-delay", type=float, default=0.05)
    args = parser.parse_args()

    env = JXBReachEnv(render=True, fixed_target=args.target)
    obs = env.reset()
    obs_dim = len(obs)

    model = ActorCritic(obs_dim, args.action_dim, args.hidden_dim)
    model.load_state_dict(torch.load(args.model_path, map_location="cpu"))
    model.eval()

    print("=" * 50)
    print("Model loaded. Running visual policy playback.")
    print(f"Target: {args.target}")
    print("=" * 50)

    while True:
        obs_tensor = torch.tensor(obs, dtype=torch.float32)
        with torch.no_grad():
            _, action, _, _ = model.act(obs_tensor)

        obs, reward, done, step_info = env.step(action.numpy())
        success = step_info["success"]
        distance = step_info["distance"]
        print(
            f"\rDistance: {distance:.3f} | Reward: {reward:+.2f} | Success: {success}",
            end="",
        )

        if done:
            print("\nTarget reached or episode ended. Resetting environment.")
            obs = env.reset()

        time.sleep(args.step_delay)


if __name__ == "__main__":
    main()
