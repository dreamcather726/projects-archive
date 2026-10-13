"""Publish simulated robot status as std_msgs/String."""

import rclpy
from rclpy.node import Node
from std_msgs.msg import String


class RobotStatusPublisher(Node):
    def __init__(self) -> None:
        # 先初始化 Node 父类，并设置 ROS 2 图中显示的节点名称。
        super().__init__("robot_status_publisher")
        # 创建 String 发布者；10 是 KEEP_LAST 历史深度的简写。
        self.publisher = self.create_publisher(String, "robot_status", 10)
        self.robot_name = "robot_01"
        self.battery_percent = 85
        self.speed_mps = 0.30
        # 每隔约 1 秒由执行器调度一次 timer_callback。
        self.timer = self.create_timer(1.0, self.timer_callback)

    def timer_callback(self) -> None:
        # 消息对象必须先创建并填写字段，调用 publish() 后才会发送。
        message = String()
        message.data = (
            f"{self.robot_name}|{self.battery_percent}|{self.speed_mps:.2f}"
        )
        self.publisher.publish(message)
        self.get_logger().info(f"发布：{message.data}")
        self.battery_percent = max(0, self.battery_percent - 1)


def main(args=None) -> None:
    # 初始化通信上下文后再创建节点。
    rclpy.init(args=args)
    node = RobotStatusPublisher()

    try:
        # spin 持续等待并执行定时器等已经准备好的回调。
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        # 无论正常结束还是按 Ctrl+C，都集中释放节点和上下文资源。
        node.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    main()
