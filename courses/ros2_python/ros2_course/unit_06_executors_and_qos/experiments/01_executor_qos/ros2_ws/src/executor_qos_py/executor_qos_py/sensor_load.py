"""以 20 Hz 和 best-effort QoS 发布模拟传感器序号。"""

import rclpy
from rclpy.node import Node
from rclpy.qos import qos_profile_sensor_data
from std_msgs.msg import Int32


class SensorLoad(Node):
    def __init__(self) -> None:
        super().__init__('sensor_load')
        # 高频传感器通常更在意最新数据和低延迟，而不是补发每一条旧数据。
        self._publisher = self.create_publisher(
            Int32, '/fast_sensor', qos_profile_sensor_data
        )
        self._sequence = 0
        self._timer = self.create_timer(0.05, self._publish)

    def _publish(self) -> None:
        message = Int32()
        message.data = self._sequence
        self._sequence += 1
        self._publisher.publish(message)


def main(args=None) -> None:
    rclpy.init(args=args)
    node = SensorLoad()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.try_shutdown()


if __name__ == '__main__':
    main()
