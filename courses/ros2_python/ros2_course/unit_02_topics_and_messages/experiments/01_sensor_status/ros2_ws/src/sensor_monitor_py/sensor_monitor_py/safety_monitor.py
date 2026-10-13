"""订阅传感器状态，同时检查数据值与数据是否超时。"""

import rclpy
from rclpy.node import Node
from rclpy.qos import qos_profile_sensor_data

from sensor_interfaces.msg import SensorStatus


class SafetyMonitor(Node):
    """把通信数据转换成安全判断的订阅节点。"""

    def __init__(self) -> None:
        super().__init__('safety_monitor')
        self._last_message_time = None
        self._timeout_reported = False

        self._subscription = self.create_subscription(
            SensorStatus,
            '/sensor/status',
            self._on_status,
            qos_profile_sensor_data,
        )
        # 为什么另设定时器：没有消息时订阅回调不会运行，
        # 因此“数据超时”不能只在订阅回调里检查。
        self._watchdog_timer = self.create_timer(0.5, self._check_timeout)

    def _on_status(self, message: SensorStatus) -> None:
        self._last_message_time = self.get_clock().now()
        self._timeout_reported = False

        if message.status == SensorStatus.STATUS_DANGER:
            self.get_logger().error(
                f'危险：{message.source} 距离={message.distance_m:.2f} m, '
                f'温度={message.temperature_c:.1f} C'
            )
        elif message.status == SensorStatus.STATUS_WARNING:
            self.get_logger().warning(
                f'警告：距离={message.distance_m:.2f} m, '
                f'温度={message.temperature_c:.1f} C'
            )
        else:
            self.get_logger().info('传感器状态正常')

    def _check_timeout(self) -> None:
        if self._last_message_time is None:
            return

        age_seconds = (
            self.get_clock().now() - self._last_message_time
        ).nanoseconds / 1_000_000_000

        if age_seconds > 1.5 and not self._timeout_reported:
            self.get_logger().error(
                f'数据超时：已经 {age_seconds:.1f} 秒没有收到新消息'
            )
            self._timeout_reported = True


def main(args=None) -> None:
    rclpy.init(args=args)
    node = SafetyMonitor()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.try_shutdown()


if __name__ == '__main__':
    main()
