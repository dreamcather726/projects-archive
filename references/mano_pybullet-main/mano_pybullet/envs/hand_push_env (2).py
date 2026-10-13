"""推物环境。"""

from .hand_object_env import HandObjectEnv

__all__ = ('HandPushEnv')


class HandPushEnv(HandObjectEnv):
    """推物环境类。"""

    def __init__(self, target_distance=0.2, **kwargs):
        """HandPushEnv 构造函数。

        关键字参数:
            target_distance {float} -- 到目标的距离 (默认: {0.2})
        """
        super().__init__(self, **kwargs)
        self._target_distance = target_distance

    def reset(self, **kwargs):
        """重置环境到初始状态并返回初始观察。

        返回:
            observation (object): 初始观察。
        """
        observation = super().reset(**kwargs)

        # 降低地平面摩擦
        self._client.changeDynamics(
            self._table_id, -1, lateralFriction=0.04, spinningFriction=0.04)

        # 调试目标标记
        self._client.addUserDebugLine(
            (self._target_distance, -0.1, 0.0),
            (self._target_distance, 0.1, 0.0),
            (1.0, 0.0, 0.0), 4)

        return observation

    def _get_reward(self, observation):
        """计算当前环境状态的奖励。

        返回:
            observation (object): 智能体对当前环境的观察
        """
        _hands_state, (object_pos, _object_orn) = observation
        return object_pos[0]

    def _is_done(self, observation):
        """检查当前环境状态是否结束。

        返回:
            observation (object): 智能体对当前环境的观察
        """
        _hands_state, (object_pos, _object_orn) = observation
        return object_pos[0] > self._target_distance


if __name__ == '__main__':
    from .wrappers.json_player import JSONPlayer

    env = HandPushEnv()
    env.show_window(True)
    env = JSONPlayer(env, './data/push_teddy.json')
    env.reset()
    while True:
        _observation, _reward, done, _info = env.step()
        if done:
            break
