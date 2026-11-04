import argparse
import csv
import numpy as np
import torch
import torch.nn as nn
from torch.distributions import Normal
from tqdm import tqdm

from reach_env import JXBReachEnv, JOINT_COUNT

# ===== PPO 超参数 =====
GAMMA = 0.99
GAE_LAMBDA = 0.95
CLIP_RATIO = 0.2
VALUE_COEF = 0.5
ENTROPY_COEF = 0.01
MAX_GRAD_NORM = 1.0


def save_training_metrics(metrics, metrics_path):
    with open(metrics_path, "w", newline="", encoding="utf-8") as metrics_file:
        writer = csv.DictWriter(
            metrics_file,
            fieldnames=[
                "iteration",
                "mean_reward",
                "success_count",
                "mean_jitter_penalty",
                "mean_joint_velocity_penalty",
                "policy_loss",
                "value_loss",
            ],
        )
        writer.writeheader()
        writer.writerows(metrics)


def save_training_plot(metrics, plot_path):
    try:
        import matplotlib

        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        print("matplotlib is not installed, skipped training curve image.")
        return

    iterations = [row["iteration"] for row in metrics]
    mean_rewards = [row["mean_reward"] for row in metrics]
    success_counts = [row["success_count"] for row in metrics]

    figure, reward_axis = plt.subplots(figsize=(10, 5))
    reward_axis.plot(iterations, mean_rewards, color="#2563eb", label="Mean reward")
    reward_axis.set_xlabel("Iteration")
    reward_axis.set_ylabel("Mean reward")
    reward_axis.grid(True, alpha=0.3)

    success_axis = reward_axis.twinx()
    success_axis.plot(iterations, success_counts, color="#f97316", label="Success count")
    success_axis.set_ylabel("Success count")

    lines = reward_axis.get_lines() + success_axis.get_lines()
    labels = [line.get_label() for line in lines]
    reward_axis.legend(lines, labels, loc="best")
    figure.tight_layout()
    figure.savefig(plot_path, dpi=160)
    plt.close(figure)

# ===== 神经网络 =====
class ActorCritic(nn.Module):
    def __init__(self, obs_dim, action_dim, hidden_dim):
        super().__init__()
        self.actor = nn.Sequential(
            nn.Linear(obs_dim, hidden_dim),
            nn.ELU(),
            nn.Linear(hidden_dim, hidden_dim),
            nn.ELU(),
            nn.Linear(hidden_dim, action_dim),
        )
        self.critic = nn.Sequential(
            nn.Linear(obs_dim, hidden_dim),
            nn.ELU(),
            nn.Linear(hidden_dim, hidden_dim),
            nn.ELU(),
            nn.Linear(hidden_dim, 1),
        )
        self.log_std = nn.Parameter(torch.zeros(action_dim))

    def distribution(self, obs):
        mean = self.actor(obs)
        std = torch.exp(self.log_std).expand_as(mean)
        return Normal(mean, std)

    def act(self, obs):
        dist = self.distribution(obs)
        raw_action = dist.sample()
        log_prob = dist.log_prob(raw_action).sum(-1)
        value = self.critic(obs).squeeze(-1)
        env_action = torch.tanh(raw_action)
        return raw_action, env_action, log_prob, value

    def evaluate(self, obs, raw_action):
        dist = self.distribution(obs)
        log_prob = dist.log_prob(raw_action).sum(-1)
        entropy = dist.entropy().sum(-1)
        value = self.critic(obs).squeeze(-1)
        return log_prob, entropy, value

# ===== 收集数据（快 + 完整学习）=====
def collect_rollout(env, model, obs, rollout_steps, device):
    obs_buf = []
    act_buf = []
    logp_buf = []
    rew_buf = []
    done_buf = []
    val_buf = []
    jitter_penalty_buf = []
    joint_velocity_penalty_buf = []
    success = 0

    for _ in range(rollout_steps):
        obs_t = torch.tensor(obs, dtype=torch.float32, device=device)
        with torch.no_grad():
            raw_act, env_act, logp, val = model.act(obs_t)

        next_obs, rew, done, info = env.step(env_act.cpu().numpy())
        if info["success"]:
            success += 1

        obs_buf.append(obs)
        act_buf.append(raw_act.cpu().numpy())
        logp_buf.append(logp.cpu().numpy())
        rew_buf.append(rew)
        done_buf.append(done)
        val_buf.append(val.cpu().numpy())
        jitter_penalty_buf.append(info.get("jitter_penalty", 0.0))
        joint_velocity_penalty_buf.append(info.get("joint_velocity_penalty", 0.0))

        obs = next_obs
        if done:
            obs = env.reset()

    with torch.no_grad():
        last_val = model.critic(torch.tensor(obs, dtype=torch.float32, device=device)).item()

    rollout = {
        "obs": np.array(obs_buf),
        "raw_actions": np.array(act_buf),
        "logp": np.array(logp_buf),
        "rewards": np.array(rew_buf),
        "dones": np.array(done_buf),
        "values": np.array(val_buf),
        "last_val": last_val,
        "jitter_penalties": np.array(jitter_penalty_buf),
        "joint_velocity_penalties": np.array(joint_velocity_penalty_buf),
        "success": success
    }
    return obs, rollout

# ===== PPO 更新 =====
def compute_gae(rollout):
    rewards = rollout["rewards"]
    dones = rollout["dones"]
    values = rollout["values"]
    last_val = rollout["last_val"]

    adv = np.zeros_like(rewards)
    last_adv = 0
    for t in reversed(range(len(rewards))):
        next_val = last_val if t == len(rewards)-1 else values[t+1]
        delta = rewards[t] + GAMMA * next_val * (1-dones[t]) - values[t]
        last_adv = delta + GAMMA * GAE_LAMBDA * (1-dones[t]) * last_adv
        adv[t] = last_adv
    ret = adv + values
    return adv, ret

def update(model, opt, rollout, device):
    adv, ret = compute_gae(rollout)
    adv = (adv - adv.mean()) / (adv.std() + 1e-7)

    obs = torch.tensor(rollout["obs"], dtype=torch.float32, device=device)
    acts = torch.tensor(rollout["raw_actions"], dtype=torch.float32, device=device)
    old_logp = torch.tensor(rollout["logp"], dtype=torch.float32, device=device)
    adv_t = torch.tensor(adv, dtype=torch.float32, device=device)
    ret_t = torch.tensor(ret, dtype=torch.float32, device=device)

    for _ in range(4):
        logp, entropy, val = model.evaluate(obs, acts)
        ratio = torch.exp(logp - old_logp)
        clip = torch.clamp(ratio, 1-CLIP_RATIO, 1+CLIP_RATIO)
        policy_loss = -torch.min(ratio * adv_t, clip * adv_t).mean()

        value_loss = (val - ret_t).pow(2).mean()
        loss = policy_loss + VALUE_COEF * value_loss - ENTROPY_COEF * entropy.mean()

        opt.zero_grad()
        loss.backward()
        nn.utils.clip_grad_norm_(model.parameters(), MAX_GRAD_NORM)
        opt.step()
    return policy_loss.item(), value_loss.item()

# ===== 主函数 =====
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--target", nargs=3, type=float, default=[-0.02, -0.10, 0.14])
    parser.add_argument("--iterations", type=int, default=2000)
    parser.add_argument("--rollout-steps", type=int, default=64)
    parser.add_argument("--device", default="cpu")
    parser.add_argument("--metrics-path", default="training_metrics.csv")
    parser.add_argument("--plot-path", default="training_curve.png")
    args = parser.parse_args()

    env = JXBReachEnv(render=False, fixed_target=args.target)
    obs = env.reset()
    obs_dim = len(obs)

    model = ActorCritic(obs_dim, JOINT_COUNT, 128).to(args.device)
    opt = torch.optim.Adam(model.parameters(), lr=3e-4)

    print(f"目标点: {args.target}")
    print(f"观测维度: {obs_dim}")
    print("开始训练...\n")

    pbar = tqdm(range(1, args.iterations + 1), desc="训练进度", unit="iter")
    metrics = []
    for it in pbar:
        obs, rollout = collect_rollout(env, model, obs, args.rollout_steps, args.device)
        policy_loss, value_loss = update(model, opt, rollout, args.device)

        mean_rew = rollout["rewards"].mean()
        succ = rollout["success"]
        metrics.append(
            {
                "iteration": it,
                "mean_reward": float(mean_rew),
                "success_count": int(succ),
                "mean_jitter_penalty": float(rollout["jitter_penalties"].mean()),
                "mean_joint_velocity_penalty": float(
                    rollout["joint_velocity_penalties"].mean()
                ),
                "policy_loss": float(policy_loss),
                "value_loss": float(value_loss),
            }
        )

        pbar.set_postfix(reward=f"{mean_rew:+.2f}", success=succ)

        if it % 200 == 0:
            torch.save(model.state_dict(), f"arm_ppo_{it}.pt")

    torch.save(model.state_dict(), "arm_ppo_final.pt")
    save_training_metrics(metrics, args.metrics_path)
    save_training_plot(metrics, args.plot_path)
    print(f"\nSaved training metrics to {args.metrics_path}")
    print(f"Saved training curve to {args.plot_path}")
    print("\n训练完成！")

if __name__ == "__main__":
    main()
