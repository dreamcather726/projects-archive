"""数据集播放辅助包装器。"""

import glob
import json
import os

import gym

__all__ = ('JSONPlayer')


class JSONPlayer(gym.Wrapper):
    """包装环境以允许从文件播放动作。"""

    def __init__(self, env, path):
        """Player 构造函数。

        参数:
            env {gym.Env} -- 要包装的环境。
            path {str} -- 录制的 JSON 文件路径或包含文件的目录路径。
        """
        super().__init__(env)
        if os.path.isdir(path):
            self._filenames = list(glob.glob(os.path.join(path, '*.json')))
        else:
            self._filenames = [path]
        self._action_iter = None
        self._counter = 0

    @property
    def records(self):
        """数据集中的记录数量。

        返回:
            int -- 数据集中的文件数量
        """
        return len(self._filenames)

    def reset(self, **kwargs_override):
        """重置环境到初始状态。

        从文件加载录制的数据，应用录制的参数和种子。
        """
        seed, kwargs = self._load()
        if seed is not None:
            self.seed(seed)
        kwargs.update(kwargs_override)
        return self.env.reset(**kwargs)

    def step(self, action=None):
        """运行环境动力学的一个时间步。

        从文件加载动作。
        """
        action = next(self._action_iter, None)
        if action is None:
            raise RuntimeError('回放期间环境未完成。')
        return self.env.step(action)

    def _load(self):
        filename = self._filenames[self._counter]
        with open(filename, 'r') as json_file:
            data = json.load(json_file)
        self._action_iter = iter(self.env.action_space.from_jsonable(data.get('actions')))
        self._counter += 1
        return data.get('seed'), data.get('kwargs')
