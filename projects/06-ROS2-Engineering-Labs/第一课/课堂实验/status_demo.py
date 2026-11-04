import rclpy
from rclpy.node import Node
from robot_interfaces.msg import RobotStatus


class StatusDemo(Node):
    def __init__(self):
        super().__init__('status_demo')

        self.publisher = self.create_publisher(
            RobotStatus, '/status', 10)

        self.subscription = self.create_subscription(
            RobotStatus, '/status', self.status_callback, 10)

        self.timer = self.create_timer(1.0, self.publish_status)

    def publish_status(self):
        msg = RobotStatus()
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.header.frame_id = 'map'
        msg.status = RobotStatus.STATUS_RUN
        msg.battery_pct = 90.0
        msg.is_charging = False
        msg.position = [1.0, 2.0]
        msg.yaw = 0.5
        self.publisher.publish(msg)

    def status_callback(self, msg):
        self.get_logger().info(
            f'status={msg.status}, battery={msg.battery_pct:.1f}%')


def main():
    rclpy.init()
    node = StatusDemo()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == '__main__':
    main()
