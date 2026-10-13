"""订阅模拟激光并发布安全的反应式速度命令。"""

import math

import rclpy
from geometry_msgs.msg import Twist
from rclpy.node import Node
from rclpy.qos import qos_profile_sensor_data
from sensor_msgs.msg import LaserScan


class ReactiveController(Node):
    def __init__(self) -> None:
        super().__init__('reactive_controller')
        self.declare_parameter('safe_distance', 0.75)
        self.declare_parameter('forward_speed', 0.30)
        self.declare_parameter('turn_speed', 0.75)

        self._last_scan_time = None
        self._front_distance = math.inf
        self._publisher = self.create_publisher(Twist, '/cmd_vel', 10)
        self._subscription = self.create_subscription(
            LaserScan, '/scan', self._on_scan, qos_profile_sensor_data
        )
        self._timer = self.create_timer(0.1, self._control)

    def _on_scan(self, message: LaserScan) -> None:
        # 取扫描中央约 20 度。过滤 inf/nan，防止 min() 得到无效判断。
        center = len(message.ranges) // 2
        half_window = max(1, int(math.radians(10.0) / message.angle_increment))
        front_ranges = message.ranges[center - half_window:center + half_window]
        valid = [value for value in front_ranges if math.isfinite(value)]
        self._front_distance = min(valid) if valid else math.inf
        self._last_scan_time = self.get_clock().now()

    def _control(self) -> None:
        command = Twist()
        # 没有传感器或数据超时就停车。这是控制程序的失效安全默认值。
        if self._last_scan_time is None:
            self._publisher.publish(command)
            return

        age = (self.get_clock().now() - self._last_scan_time).nanoseconds / 1e9
        if age > 0.5:
            self.get_logger().warning('激光数据超时，发布零速度')
            self._publisher.publish(command)
            return

        if self._front_distance < self.get_parameter('safe_distance').value:
            command.angular.z = self.get_parameter('turn_speed').value
        else:
            command.linear.x = self.get_parameter('forward_speed').value
        self._publisher.publish(command)


def main(args=None) -> None:
    rclpy.init(args=args)
    node = ReactiveController()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        # 控制节点退出前主动发布零速度，避免最后一条非零命令残留。
        node._publisher.publish(Twist())
        node.destroy_node()
        rclpy.try_shutdown()


if __name__ == '__main__':
    main()
