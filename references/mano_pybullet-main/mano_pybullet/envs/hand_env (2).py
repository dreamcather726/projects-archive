"""基础手部环境。"""

import gym
import numpy as np
import pybullet as pb
import pybullet_data as pd
from numpy.random import RandomState
from pybullet_utils.bullet_client import BulletClient

from ..hand_body import HandBody
from ..hand_model import HandModel20

__all__ = ('HandEnv')


class HandEnv(gym.Env):
    """基础手部环境类。"""

    # 环境可以有双手或单手
    HAND_RIGHT = 1
    HAND_LEFT = 2
    HAND_BOTH = HAND_LEFT + HAND_RIGHT

    # 环境可以在关节空间或 MANO 姿态空间中控制
    MODE_JOINT = 1
    MODE_MANO = 2
    MODE_LIST = (MODE_JOINT, MODE_MANO)

    # 形状 beta 参数幅度
    SHAPE_BETAS_MAGNITUDE = 0.1

    metadata = {
        'render.modes': ['rgb_array'],
        'video.frames_per_second': 60.0,
        'video.output_frames_per_second': 30.0,
        'step_time': 1.0 / 60.0}

    def __init__(self,
                 hand_side=HAND_RIGHT,
                 control_mode=MODE_JOINT,
                 hand_model_cls=HandModel20,
                 randomize_hand_shape=False):
        """HandEnv 构造函数。

        关键字参数:
            hand_side {int} -- 手部侧：左、右或双手 (默认: {HAND_BOTH})
            control_mode {int} -- 控制模式 (默认: {MODE_JOINT})
            hand_model_cls {type} -- 手部运动学模型 (默认: {HandModel20})
            randomize_hand_shape {bool} -- 重置时随机化手部形状 (默认: {True})
        """
        assert hand_side & self.HAND_BOTH, f'错误的手部侧标志: {hand_side}'
        assert control_mode in self.MODE_LIST, f'错误的控制模式标志: {control_mode}'

        self._hand_side = hand_side
        self._control_mode = control_mode
        self._randomize_hand_shape = randomize_hand_shape

        self._show_window = False
        self._random = None
        self._client = None
        self._hand_bodies = None
        self._camera_shape = (320, 240)
        self._camera_view = None
        self._camera_proj = None

        self._hand_models = []
        if self.HAND_LEFT & hand_side:
            self._hand_models.append(hand_model_cls(left_hand=True))
        if self.HAND_RIGHT & hand_side:
            self._hand_models.append(hand_model_cls(left_hand=False))

        # 如果在关节空间控制
        if self.MODE_JOINT == control_mode:
            joint_low, joint_high = map(np.float32, self._hand_models[0].dofs_limits)
            self.action_space = gym.spaces.Tuple([
                gym.spaces.Tuple((
                    gym.spaces.Box(-5.0, 5.0, shape=(3,)),  # 基座位置 x,y,z
                    gym.spaces.Box(-1.0, 1.0, shape=(4,)),  # 基座四元数 x,y,z,w
                    gym.spaces.Box(joint_low, joint_high),  # 关节角度
                )) for _ in self._hand_models])
            self.observation_space = gym.spaces.Tuple([
                gym.spaces.Tuple((
                    gym.spaces.Box(-5.0, 5.0, shape=(3,)),  # 基座位置 x,y,z
                    gym.spaces.Box(-1.0, 1.0, shape=(4,)),  # 基座四元数 x,y,z,w
                    gym.spaces.Box(-10.0, 10.0, shape=(6,)),  # 基座约束力
                    gym.spaces.Box(joint_low, joint_high),  # 关节角度
                    gym.spaces.Box(-3.0, 3.0, shape=(len(joint_low),)),  # 关节速度
                    gym.spaces.Box(-1.0, 1.0, shape=(len(joint_low),)),  # 关节力矩
                )) for _ in self._hand_models])
        # 如果在 MANO 空间控制
        elif self.MODE_MANO == control_mode:
            self.action_space = gym.spaces.Tuple([
                gym.spaces.Tuple((
                    gym.spaces.Box(-5.0, 5.0, shape=(3,)),  # 平移
                    gym.spaces.Box(-np.pi, np.pi, shape=(16, 3)),  # 姿态
                )) for _ in self._hand_models])
            self.observation_space = gym.spaces.Tuple([
                gym.spaces.Tuple((
                    gym.spaces.Box(-5.0, 5.0, shape=(3,)),  # 平移
                    gym.spaces.Box(-np.pi, np.pi, shape=(16, 3)),  # 姿态
                )) for _ in self._hand_models])

    def show_window(self, show=True):
        """显示或隐藏 GUI 窗口。

        关键字参数:
            show {bool} -- 显示标志 (默认: {True})
        """
        self._show_window = show

    def reset(self, initial_hands_state=None, **_kwargs):
        """重置环境到初始状态并返回初始观察。

        关键字参数:
            initial_hands_state {int} -- 覆盖初始手部状态 (默认: {None})

        返回:
            observation (object): 初始观察。
        """
        if self._client is None:
            self._client = BulletClient(pb.GUI if self._show_window else pb.DIRECT)
            self._client.configureDebugVisualizer(pb.COV_ENABLE_GUI, 0)
            self._setup_camera()

        self._client.resetSimulation()
        self._client.setAdditionalSearchPath(pd.getDataPath())
        self._client.setPhysicsEngineParameter(deterministicOverlappingPairs=1)
        self._client.setGravity(0, 0, -10)
        self._client.configureDebugVisualizer(pb.COV_ENABLE_RENDERING, 0)

        # 如果需要，随机化手部形状
        betas = None
        if self._randomize_hand_shape:
            betas = self._random.rand(10) * self.SHAPE_BETAS_MAGNITUDE

        # 生成手部
        self._hand_bodies = []
        for i, model in enumerate(self._hand_models):
            hand_body = HandBody(self._client, model, shape_betas=betas)

            if initial_hands_state is not None:
                pos, orn, angles = initial_hands_state[i]
            else:
                pos_y, orn_z = (0.10, 0.0) if model.is_left_hand else (-0.10, -np.pi)
                pos, orn = (0.0, pos_y, 0.25), pb.getQuaternionFromEuler((np.pi/2, 0.0, orn_z))
                angles = np.zeros(len(hand_body.joint_indices))

            hand_body.reset(pos, orn, angles)
            hand_body.set_target(pos, orn, angles)
            self._hand_bodies.append(hand_body)

        self._client.configureDebugVisualizer(pb.COV_ENABLE_RENDERING, 1)
        return self._get_observation()

    def step(self, action):
        """运行环境动力学的一个时间步。

        参数:
            action (object): 智能体提供的动作

        返回:
            observation (object): 智能体对当前环境的观察
            reward (float): 执行动作后返回的奖励值
            done (bool): 是否结束回合
            info (dict): 包含辅助诊断信息
        """
        self._take_action(action)

        num_steps = int(self.metadata.get('step_time') * 240.0)
        for _ in range(num_steps):
            self._client.stepSimulation()

        observation = self._get_observation()
        reward = self._get_reward(observation)
        done = self._is_done(observation)
        info = {}
        return observation, reward, done, info

    def render(self, mode='human'):
        """渲染环境。

        参数:
            mode (str): 渲染模式
        """
        if mode == 'rgb_array':
            data = pb.getCameraImage(*self._camera_shape, self._camera_view, self._camera_proj)
            rgba = data[2]
            return rgba[:, :, :3]
        return super().render(mode=mode)

    def close(self):
        """执行必要的清理。

        环境在垃圾回收或程序退出时会自动调用 close()。
        """
        if self._client.isConnected():
            self._client.disconnect()

    def seed(self, seed=None):
        """设置环境随机数生成器的种子。

        参数:
            seed (int): 提供的随机数生成器种子
        """
        self._random = RandomState(seed)
        return [seed]

    def _take_action(self, action):
        """执行动作。

        参数:
            action (object): 智能体提供的动作
        """
        if self.MODE_JOINT == self._control_mode:
            for hand, (pos, orn, angles) in zip(self._hand_bodies, action):
                hand.set_target(pos, orn, angles)
        elif self.MODE_MANO == self._control_mode:
            for hand, (trans, pose) in zip(self._hand_bodies, action):
                hand.set_target_from_mano(trans, pose)

    def _get_observation(self):
        """计算当前环境的观察。

        返回:
            observation (object): 智能体对当前环境的观察
        """
        if self.MODE_JOINT == self._control_mode:
            return [hand.get_state() for hand in self._hand_bodies]
        if self.MODE_MANO == self._control_mode:
            return [hand.get_mano_state() for hand in self._hand_bodies]
        return None

    def _get_reward(self, observation):
        """计算当前环境状态的奖励。

        返回:
            observation (object): 智能体对当前环境的观察
        """
        # pylint: disable=no-self-use, unused-argument
        return 0.0

    def _is_done(self, observation):
        """检查当前环境状态是否结束。

        返回:
            observation (object): 智能体对当前环境的观察
        """
        # pylint: disable=no-self-use, unused-argument
        return False

    def _setup_camera(self):
        """设置虚拟相机。"""
        self._client.resetDebugVisualizerCamera(
            cameraDistance=0.5,
            cameraYaw=-45.0,
            cameraPitch=-40.0,
            cameraTargetPosition=[0, 0, 0.1])

        self._camera_view = self._client.computeViewMatrixFromYawPitchRoll(
            cameraTargetPosition=[0.0, 0.0, 0.1],
            distance=0.5,
            yaw=-45.0,
            pitch=-40.0,
            roll=0.0,
            upAxisIndex=2)

        self._camera_proj = self._client.computeProjectionMatrixFOV(
            fov=60.0,
            aspect=self._camera_shape[0] / self._camera_shape[1],
            nearVal=0.1,
            farVal=10.0)


if __name__ == '__main__':
    import time

    env = HandEnv(HandEnv.HAND_BOTH)
    env.show_window()
    env.seed(0)
    env.reset()

    while pb.isConnected():
        time.sleep(0.1)
