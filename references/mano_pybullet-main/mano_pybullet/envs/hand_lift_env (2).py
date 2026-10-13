"""举物环境。"""

from .hand_object_env import HandObjectEnv

__all__ = ('HandLiftEnv')


class HandLiftEnv(HandObjectEnv):
    """举物环境类。"""

    def __init__(self, target_height=0.25, **kwargs):
        """HandLiftEnv 构造函数。

        关键字参数:
            target_height {float} -- 目标举起高度 (默认: {0.25})
        """
        super().__init__(self, **kwargs)
        self._target_height = target_height

    def reset(self, **kwargs):
        """重置环境到初始状态并返回初始观察。

        返回:
            observation (object): 初始观察。
        """
        # pylint: disable=arguments-differ
        observation = super().reset(**kwargs)

        # 调试目标标记
        self._client.addUserDebugLine(
            (0.1, 0.1, self._target_height),
            (-0.1, -0.1, self._target_height),
            (1.0, 0.0, 0.0), 2)
        self._client.addUserDebugLine(
            (0.1, -0.1, self._target_height),
            (-0.1, 0.1, self._target_height),
            (1.0, 0.0, 0.0), 4)

        return observation

    def _get_reward(self, observation):
        """计算当前环境状态的奖励。

        返回:
            observation (object): 智能体对当前环境的观察
        """
        _hands_state, (object_pos, _object_orn) = observation
        return object_pos[2]

    def _is_done(self, observation):
        """检查当前环境状态是否结束。

        返回:
            observation (object): 智能体对当前环境的观察
        """
        _hands_state, (object_pos, _object_orn) = observation
        return object_pos[2] > self._target_height


if __name__ == '__main__':
    from .wrappers.json_player import JSONPlayer

    env = HandLiftEnv()
    env.show_window(True)
    env = JSONPlayer(env, './data/lift_duck.json')
    env.reset()
    while True:
        _observation, _reward, done, _info = env.step()
        if done:
            break
