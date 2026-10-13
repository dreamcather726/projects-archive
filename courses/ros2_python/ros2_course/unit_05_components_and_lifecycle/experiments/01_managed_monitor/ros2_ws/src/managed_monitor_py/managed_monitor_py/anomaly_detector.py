"""不依赖 ROS 2 的纯 Python 业务逻辑。"""


class AnomalyDetector:
    """根据安全距离判断传感器读数。

    把算法从 Node 中分离的作用是：无需启动 ROS 2、无需构造消息，
    就能快速测试所有边界值。
    """

    def __init__(self, min_distance: float) -> None:
        if min_distance <= 0.0:
            raise ValueError('min_distance 必须大于 0')
        self._min_distance = min_distance

    def evaluate(self, distance: float) -> str | None:
        if distance < 0.0:
            return '传感器数据非法'
        if distance < self._min_distance:
            return f'障碍物过近：{distance:.2f} m'
        return None
