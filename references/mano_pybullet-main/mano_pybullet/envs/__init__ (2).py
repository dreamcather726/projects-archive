"""手部环境。"""

try:
    import gym
except ModuleNotFoundError as err:
    raise RuntimeError('未找到包: gym。使用 `pip install gym` 安装。') from err


gym.envs.registration.register(
    id='HandLiftEnv-v1',
    entry_point='mano_pybullet.envs:HandLiftEnv',
)

gym.envs.registration.register(
    id='HandPushEnv-v1',
    entry_point='mano_pybullet.envs:HandPushEnv',
)
