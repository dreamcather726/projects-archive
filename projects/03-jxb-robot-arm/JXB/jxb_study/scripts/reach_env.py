import importlib.util
import random
from pathlib import Path

import numpy as np
import pybullet as p
import pybullet_data

from reward_geometry import calculate_reach_reward, direction_alignment_cos

#这个程序是一个强化学习环境的实现，专门用于训练机械臂到达目标点。它使用 PyBullet 进行物理模拟，并定义了一个名为 JXBReachEnv 的类，包含了环境的初始化、重置、执行动作和计算奖励等功能。环境的观测包括关节位置、速度、末端位置、目标位置、误差和朝向信息。奖励设计考虑了进步奖励、运动奖励和成功奖励等因素，以引导模型学会更有效地控制机械臂。
# ===== 环境常用参数配置 =====
SCRIPT_DIR = Path(__file__).resolve().parent
STUDY_DIR = SCRIPT_DIR.parent
OUTPUT_DIR = STUDY_DIR / "outputs"
JXB_DIR = STUDY_DIR.parent
PYBULLET_FILE = JXB_DIR / "Pybullet" / "Pybullet.py"

# 机械臂前 4 个关节参与到达任务，第 5 个舵机一般是夹爪，这里先不控制。
JOINT_COUNT = 4

# 动作缩放：强化学习输出的动作范围是 [-1, 1]，这里把它变成每一步关节变化量。
# 单位是弧度，0.02 rad 大约等于 1.15 度。
ACTION_SCALE = 0.5

# Low-pass smoothing for policy actions. Higher values follow the policy faster;
# lower values suppress jitter more strongly.
ACTION_SMOOTHING_ALPHA = 0.35

# 每一局最多运行多少步，超过后强制结束。
MAX_STEPS = 500

# 末端距离目标小于这个值时认为成功，单位是米。
SUCCESS_DISTANCE = 0.02

# =====     环境奖励配置 =====
# 进步奖励权重：这一帧比上一帧更接近目标，就给正奖励；远离目标就给负奖励。
PROGRESS_WEIGHT =2.0

# 每走一步都扣一点分，避免模型学会“不动也能一直拿奖励”。
STEP_PENALTY =1.0

# 最小运动角度：每一步总关节变化量小于这个值，就认为动作太小。
# 这里用“4 个关节变化量的合成角度”，单位是度。
MIN_MOVEMENT_DEG = 1.0

# 不动惩罚：动作几乎为 0 时扣分。
NO_MOVEMENT_PENALTY =2.0

# 小动作惩罚：动了，但没有达到最小运动角度时扣分。
SMALL_MOVEMENT_PENALTY = 2.0

# 运动奖励权重：超过最小运动角度后，角度变化越大，奖励越高。
MOVEMENT_REWARD_WEIGHT = 0.005

# 保留这个字段是为了兼容打印信息；现在不再惩罚大动作。
ACTION_PENALTY_WEIGHT = 0.0

# Penalize rapid action changes and high joint speed so the learned motion is smoother.
ACTION_JITTER_PENALTY_WEIGHT = 0.16
JOINT_VELOCITY_PENALTY_WEIGHT = 0.02

# 朝向进度增益：只有这一帧确实更接近目标时，朝向好才会放大进步奖励。
# 这样朝向不会在“不动”时单独产生奖励。
ALIGNMENT_PROGRESS_WEIGHT = 2.0

# 成功奖励：末端进入目标范围时一次性加分。
SUCCESS_BONUS = 10.0

# 关节允许偏离初始姿态的最大范围，单位是弧度。
JOINT_LIMIT_RAD = np.array([0.8, 0.9, 0.9, 0.8], dtype=np.float32)

# 默认从可达点云里随机取目标点。
DEFAULT_POINTS_PATH = OUTPUT_DIR / "reachable_points.npy"


def load_pybullet_tools():
    """按文件路径加载 JXB/Pybullet/Pybullet.py，避免目录变化导致导入失败。"""
    if not PYBULLET_FILE.exists():
        raise FileNotFoundError(f"没有找到 PyBullet 工具文件: {PYBULLET_FILE}")

    spec = importlib.util.spec_from_file_location("jxb_pybullet_tools", PYBULLET_FILE)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


pybullet_tools = load_pybullet_tools()
ROBOT_BASE_POSITION = pybullet_tools.ROBOT_BASE_POSITION
SERVO_INIT_DEG = pybullet_tools.SERVO_INIT_DEG
make_runtime_urdf = pybullet_tools.make_runtime_urdf
servo_deg_to_joint_rad = pybullet_tools.servo_deg_to_joint_rad


class JXBReachEnv:
    """JXB 机械臂到达目标点的最小强化学习环境。"""

    def __init__(self, render=False, seed=42, points_path=DEFAULT_POINTS_PATH, fixed_target=None):
        self.render = render
        self.rng = random.Random(seed)
        self.points_path = Path(points_path)
        self.fixed_target = (
            None if fixed_target is None else np.asarray(fixed_target, dtype=np.float32)
        )

        self.client_id = p.connect(p.GUI if render else p.DIRECT)
        p.setAdditionalSearchPath(pybullet_data.getDataPath(), physicsClientId=self.client_id)

        self.robot_id = None
        self.joints = []
        self.tip_link = None
        self.target = np.zeros(3, dtype=np.float32)
        self.target_marker = None
        self.step_count = 0
        self.last_distance = 0.0
        self.start_joint_pos = np.zeros(JOINT_COUNT, dtype=np.float32)
        self.previous_action = np.zeros(JOINT_COUNT, dtype=np.float32)
        self.reachable_points = self._load_reachable_points()

    def close(self):
        """关闭 PyBullet 连接。"""
        if p.isConnected(self.client_id):
            p.disconnect(physicsClientId=self.client_id)

    def reset(self, target=None):
        """重置机械臂，并返回初始观测。"""
        self.step_count = 0
        self.previous_action = np.zeros(JOINT_COUNT, dtype=np.float32)
        p.resetSimulation(physicsClientId=self.client_id)
        p.setGravity(0, 0, -9.8, physicsClientId=self.client_id)
        p.loadURDF("plane.urdf", physicsClientId=self.client_id)
        if self.render:
            p.resetDebugVisualizerCamera(
                cameraDistance=0.18,
                cameraYaw=45,
                cameraPitch=-28,
                cameraTargetPosition=[-0.02, -0.03, 0.12],
                physicsClientId=self.client_id,
            )

        self.robot_id = p.loadURDF(
            str(make_runtime_urdf()),
            basePosition=ROBOT_BASE_POSITION,
            useFixedBase=True,
            flags=p.URDF_USE_INERTIA_FROM_FILE,
            physicsClientId=self.client_id,
        )
        self.joints = self._controllable_joints()
        self.tip_link = self._tool_tip_link()
        self._reset_start_pose()

        if target is not None:
            self.target = np.asarray(target, dtype=np.float32)
        elif self.fixed_target is not None:
            # 固定目标测试/训练阶段：每个 episode 都使用同一个目标点。
            self.target = self.fixed_target.copy()
        else:
            self.target = self._sample_target()

        if self.render:
            self._create_target_marker()

        end_position, _end_orientation = self._end_pose()
        self.last_distance = float(np.linalg.norm(self.target - end_position))
        return self._get_obs()

    def step(self, action):
        """执行一步动作，返回 obs、reward、done、info。"""
        self.step_count += 1

        # action 来自策略网络时通常是 [-1, 1]，这里先截断，避免动作过大。
        action = np.clip(np.asarray(action, dtype=np.float32), -1.0, 1.0)
        raw_action = np.clip(np.asarray(action, dtype=np.float32), -1.0, 1.0)
        smoothed_action = (
            ACTION_SMOOTHING_ALPHA * raw_action
            + (1.0 - ACTION_SMOOTHING_ALPHA) * self.previous_action
        )
        action_change = smoothed_action - self.previous_action
        self.previous_action = smoothed_action
        joint_delta = smoothed_action * ACTION_SCALE

        for joint_index, joint_id in enumerate(self.joints):
            current = p.getJointState(self.robot_id, joint_id, physicsClientId=self.client_id)[0]

            # 限制每个关节不要偏离初始姿态太多，避免机械臂跑到极端姿态。
            low = self.start_joint_pos[joint_index] - JOINT_LIMIT_RAD[joint_index]
            high = self.start_joint_pos[joint_index] + JOINT_LIMIT_RAD[joint_index]
            target_joint = float(np.clip(current + joint_delta[joint_index], low, high))

            p.setJointMotorControl2(
                self.robot_id,
                joint_id,
                p.POSITION_CONTROL,
                targetPosition=target_joint,
                force=5,
                positionGain=0.12,
                physicsClientId=self.client_id,
            )

        p.stepSimulation(physicsClientId=self.client_id)

        obs = self._get_obs()
        end_position, end_orientation = self._end_pose()
        reward_info = calculate_reach_reward(end_position, end_orientation, self.target)

        distance = reward_info["distance"]
        progress = self.last_distance - distance
        self.last_distance = distance

        # 真正给强化学习使用的 step reward。
        # 重点不是“当前姿态看起来有多好”，而是“这一小步有没有让末端更接近目标”。
        base_progress_reward = PROGRESS_WEIGHT * progress

        # 把 cos 从 [-1, 1] 转换到 [0, 1]，作为进度奖励的增益。
        # 只有 progress > 0 时才使用朝向增益，避免“不动但朝向好”也拿奖励。
        alignment_score = (reward_info["alignment_cos"] + 1.0) / 2.0
        if progress > 0:
            progress_reward = base_progress_reward * (
                1.0 + ALIGNMENT_PROGRESS_WEIGHT * alignment_score
            )
        else:
            progress_reward = base_progress_reward

        # 运动量奖励/惩罚：
        # 不动扣分；动得太小扣分；超过最小角度后，动得越大分数越高。
        movement_rad = float(np.linalg.norm(joint_delta))
        movement_deg = float(np.degrees(movement_rad))
        if movement_deg < 1e-6:
            movement_reward = -NO_MOVEMENT_PENALTY
            movement_status = "no_movement"
        elif movement_deg < MIN_MOVEMENT_DEG:
            movement_reward = -SMALL_MOVEMENT_PENALTY
            movement_status = "too_small"
        else:
            movement_reward = MOVEMENT_REWARD_WEIGHT * (movement_deg - MIN_MOVEMENT_DEG)
            movement_status = "valid"

        action_change_deg = float(np.degrees(np.linalg.norm(action_change * ACTION_SCALE)))
        jitter_penalty = ACTION_JITTER_PENALTY_WEIGHT * action_change_deg
        _joint_pos, joint_vel = self._joint_state()
        joint_velocity_penalty = JOINT_VELOCITY_PENALTY_WEIGHT * float(np.linalg.norm(joint_vel))
        action_penalty = ACTION_PENALTY_WEIGHT * float(np.linalg.norm(smoothed_action))
        reward = (
            progress_reward
            + movement_reward
            - STEP_PENALTY
            - action_penalty
            - jitter_penalty
            - joint_velocity_penalty
        )

        success = distance < SUCCESS_DISTANCE
        timeout = self.step_count >= MAX_STEPS
        done = success or timeout
        if success:
            reward += SUCCESS_BONUS

        info = {
            "success": success,
            "timeout": timeout,
            "progress": progress,
            "base_progress_reward": base_progress_reward,
            "progress_reward": progress_reward,
            "alignment_score": alignment_score,
            "movement_deg": movement_deg,
            "movement_reward": movement_reward,
            "movement_status": movement_status,
            "step_penalty": STEP_PENALTY,
            "action_penalty": action_penalty,
            "action_change_deg": action_change_deg,
            "jitter_penalty": jitter_penalty,
            "joint_velocity_penalty": joint_velocity_penalty,
            **reward_info,
        }
        return obs, reward, done, info

    def _load_reachable_points(self):
        """读取可达点云，后面 reset 时从里面随机抽目标。"""
        if not self.points_path.exists():
            raise FileNotFoundError(
                f"没有找到 {self.points_path}，请先运行 reachable_workspace.py 生成可达点云。"
            )

        points = np.load(self.points_path)
        if points.ndim != 2 or points.shape[1] != 3:
            raise ValueError("reachable_points.npy 的形状应该是 [点数量, 3]")
        return points.astype(np.float32)

    def _sample_target(self):
        """从真实可达点云里随机选一个目标点。"""
        index = self.rng.randrange(len(self.reachable_points))
        return self.reachable_points[index].copy()

    def _controllable_joints(self):
        """找出 URDF 里真正可以控制的关节。"""
        joints = []
        for joint_id in range(p.getNumJoints(self.robot_id, physicsClientId=self.client_id)):
            joint_info = p.getJointInfo(self.robot_id, joint_id, physicsClientId=self.client_id)
            joint_type = joint_info[2]
            if joint_type in (p.JOINT_REVOLUTE, p.JOINT_PRISMATIC):
                joints.append(joint_id)
        return joints[:JOINT_COUNT]

    def _tool_tip_link(self):
        """优先使用 tool_tip 作为末端；如果没有，就使用最后一个 link。"""
        for joint_id in range(p.getNumJoints(self.robot_id, physicsClientId=self.client_id)):
            link_name = p.getJointInfo(
                self.robot_id,
                joint_id,
                physicsClientId=self.client_id,
            )[12].decode("utf-8", errors="replace")
            if link_name == "tool_tip":
                return joint_id
        return p.getNumJoints(self.robot_id, physicsClientId=self.client_id) - 1

    def _reset_start_pose(self):
        """把机械臂重置到 Arduino 代码里的初始舵机角度。"""
        for servo_index, joint_id in enumerate(self.joints):
            joint_rad = servo_deg_to_joint_rad(servo_index, SERVO_INIT_DEG[servo_index])
            self.start_joint_pos[servo_index] = joint_rad
            p.resetJointState(
                self.robot_id,
                joint_id,
                joint_rad,
                physicsClientId=self.client_id,
            )

    def _end_pose(self):
        """读取末端位置和末端四元数朝向。"""
        link_state = p.getLinkState(
            self.robot_id,
            self.tip_link,
            computeForwardKinematics=True,
            physicsClientId=self.client_id,
        )
        return np.asarray(link_state[4], dtype=np.float32), link_state[5]

    def _joint_state(self):
        """读取 4 个关节的位置和速度。"""
        joint_pos = []
        joint_vel = []
        for joint_id in self.joints:
            state = p.getJointState(self.robot_id, joint_id, physicsClientId=self.client_id)
            joint_pos.append(state[0])
            joint_vel.append(state[1])
        return np.asarray(joint_pos, dtype=np.float32), np.asarray(joint_vel, dtype=np.float32)

    def _get_obs(self):
        """构造强化学习观测值。"""
        joint_pos, joint_vel = self._joint_state()
        end_position, end_orientation = self._end_pose()
        error = self.target - end_position
        alignment_cos = direction_alignment_cos(end_position, end_orientation, self.target)

        # 观测包括：关节位置、关节速度、末端位置、目标位置、误差、朝向 cos。
        return np.concatenate(
            [
                joint_pos,
                joint_vel,
                end_position.astype(np.float32),
                self.target.astype(np.float32),
                error.astype(np.float32),
                np.array([alignment_cos], dtype=np.float32),
            ]
        )

    def _create_target_marker(self):
        """在 GUI 模式下创建红色目标点小球。"""
        visual = p.createVisualShape(
            p.GEOM_SPHERE,
            radius=0.012,
            rgbaColor=[1.0, 0.1, 0.1, 1.0],
            physicsClientId=self.client_id,
        )
        self.target_marker = p.createMultiBody(
            baseMass=0,
            baseVisualShapeIndex=visual,
            basePosition=self.target,
            physicsClientId=self.client_id,
        )
