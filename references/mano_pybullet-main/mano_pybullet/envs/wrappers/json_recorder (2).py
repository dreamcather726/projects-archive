"""数据集录制辅助包装器。"""

import json

import gym
import numpy as np

__all__ = ('JSONRecorder')


class JSONRecorder(gym.Wrapper):
    """包装环境以允许将动作和观察录制到文件。"""

    def __init__(self, env, filename_template, use_seed=False):
        """Recorder 构造函数。

        参数:
            env {gym.Env} -- 要包装的环境。
            filename_template {str} -- JSON 文件名模板（如 'record_{:04d}.json'）。

        关键字参数:
            use_seed {bool} -- 使用种子作为文件 ID（在模板中替换）(默认: {False})。
        """
        super().__init__(env)
        self._filename_template = filename_template
        self._use_seed = use_seed
        self._seed = None
        self._kwargs = {}
        self._observations = []
        self._actions = []
        self._counter = 0

    def reset(self, **kwargs):
        """重置环境到初始状态。

        将录制的数据转储到文件，清理记录，保存重置参数。
        """
        if self._observations:
            self._dump()
        self._kwargs = kwargs
        observation = self.env.reset(**kwargs)
        self._observations.append(observation)
        return observation

    def step(self, action):
        """运行环境动力学的一个时间步。

        录制动作和观察。
        """
        self._actions.append(action)
        observation, reward, done, info = self.env.step(action)
        self._observations.append(observation)
        return observation, reward, done, info

    def close(self):
        """关闭环境。

        将录制的数据转储到文件。
        """
        if self._observations:
            self._dump()
        return self.env.close()

    def seed(self, seed=None):
        """设置环境随机数生成器的种子。

        记录种子值。
        """
        self._seed = seed
        return self.env.seed(seed)

    def _dump(self):
        uid = self._seed if self._use_seed else self._counter
        filename = self._filename_template.format(uid)
        data = dict(
            version=1.0,
            kwargs=self._kwargs,
            seed=self._seed,
            observations=self.env.observation_space.to_jsonable(self._observations),
            actions=self.env.action_space.to_jsonable(self._actions),
        )
        with open(filename, 'w') as json_file:
            json.dump(data, json_file, cls=_SafeJSONEncoder)
        self._counter += 1
        self._observations = []
        self._actions = []


class _SafeJSONEncoder(json.JSONEncoder):
    def default(self, obj):
        if isinstance(obj, np.ndarray):
            return obj.tolist()
        return json.JSONEncoder.default(self, obj)
