"""Subscribe to simulated robot status messages."""

import rclpy
from rclpy.node import Node
from std_msgs.msg import String


class RobotStatusSubscriber(Node):
    def __init__(self) -> None:
        # 初始化节点后，订阅与发布端相同的消息类型和话题。
        super().__init__("robot_status_subscriber")
        self.subscription = self.create_subscription(
            String,
            "robot_status",
            # 收到消息时，执行器会把消息对象传给这个回调。
            self.listener_callback,
            10,
        )

    def listener_callback(self, message: String) -> None:
        # data 是 std_msgs/msg/String 消息定义的字符串字段。
        self.get_logger().info(f"收到：{message.data}")


def main(args=None) -> None:
    # ROS 2 的标准运行顺序：初始化 → 创建节点 → spin → 清理。
    rclpy.init(args=args)
    node = RobotStatusSubscriber()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        # finally 保证中断后也执行资源清理。
        node.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    main()
