"""带物体的基础手部环境。"""

import os

import gym
import numpy as np
import pybullet as pb

from .hand_env import HandEnv

__all__ = ('HandObjectEnv')


class HandObjectEnv(HandEnv):
    """带物体的基础手部环境类。"""

    def __init__(self, model_path=None, up_axis='z', make_convex=True, **kwargs):
        """HandObjectEnv 构造函数。

        关键字参数:
            model_path {str|list} -- 物体模型文件路径 (默认: {None})
            up_axis {str} -- 物体向上轴 (默认: {'z'})
            make_convex {bool} -- 生成凸碰撞网格 (默认: {True})
        """
        super().__init__(**kwargs)
        self._model_path = model_path
        self._up_axis = up_axis
        self._make_convex = make_convex
        self._body_id = -1
        self._table_id = -1

        self.observation_space = gym.spaces.Tuple((
            self.observation_space,
            gym.spaces.Tuple((
                gym.spaces.Box(-5.0, 5.0, shape=(3,)),  # 物体的基座位置
                gym.spaces.Box(-1.0, 1.0, shape=(4,)),  # 物体的基座四元数
            ))))

    def reset(self, model_path=None, up_axis=None, **kwargs):
        """重置环境到初始状态并返回初始观察。

        返回:
            observation (object): 初始观察。

        关键字参数:
            model_path {str|list} -- 覆盖物体模型文件路径 (默认: {None})
            up_axis {str} -- 覆盖物体向上轴 (默认: {None})
        """
        up_axis = up_axis or self._up_axis
        model_path = model_path or self._model_path
        self._body_id = -1

        super().reset(**kwargs)
        self._client.configureDebugVisualizer(pb.COV_ENABLE_RENDERING, 0)

        # 生成地平面
        shape_id = self._client.createCollisionShape(pb.GEOM_PLANE)
        self._table_id = self._client.createMultiBody(0.0, shape_id)

        # 生成物体
        if isinstance(model_path, (list, tuple)):
            model_path = self._random.choice(model_path)
        self._body_id = self._load_model(model_path)
        self._client.changeDynamics(
            self._body_id, -1, linearDamping=0.5, angularDamping=0.5)

        # 将物体放置在地平面上
        orig, _ = self._client.getBasePositionAndOrientation(self._body_id)
        bmin, _ = self._client.getAABB(self._body_id, -1)
        if up_axis == 'x':
            pose = (0.0, 0.0, orig[0]-bmin[0]), pb.getQuaternionFromEuler((0.0, np.pi/2, 0.0))
        elif up_axis == 'y':
            pose = (0.0, 0.0, orig[1]-bmin[1]), pb.getQuaternionFromEuler((np.pi/2, 0.0, 0.0))
        elif up_axis == 'z':
            pose = (0.0, 0.0, orig[2]-bmin[2]), (0.0, 0.0, 0.0, 1.0)
        self._client.resetBasePositionAndOrientation(self._body_id, *pose)

        self._client.configureDebugVisualizer(pb.COV_ENABLE_RENDERING, 1)
        return self._get_observation()

    def _load_model(self, path):
        """从文件加载模型。

        参数:
            path {str} -- 物体模型文件路径
        """
        path = os.path.expandvars(path)
        root, ext = os.path.splitext(path)
        if ext.lower() == '.urdf':
            return self._client.loadURDF(path)
        if ext.lower() == '.obj':
            vid = self._client.createVisualShape(pb.GEOM_MESH, fileName=path)
            if self._make_convex:
                # 生成凸分解网格并保存到磁盘
                in_path = path
                path = root + '_vhacd' + ext
                if not os.path.exists(path):
                    self._client.vhacd(in_path, path, '')
            cid = self._client.createCollisionShape(pb.GEOM_MESH, fileName=path)
            return self._client.createMultiBody(0.2, cid, vid)
        raise RuntimeError(f'无法从以下路径加载模型: "{path}"')

    def _get_observation(self):
        """计算当前环境的观察。

        返回:
            observation (object): 智能体对当前环境的观察
        """
        if self._body_id >= 0:
            hands_observation = super()._get_observation()
            base_pos, base_orn = self._client.getBasePositionAndOrientation(self._body_id)
            return hands_observation, (base_pos, base_orn)
        return None


if __name__ == '__main__':
    import time

    env = HandObjectEnv()
    env.show_window()
    env.reset(model_path='duck_vhacd.urdf', up_axis='y')

    while pb.isConnected():
        time.sleep(0.1)
