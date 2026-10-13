"""A ROS 2 node that periodically logs simulated robot status."""

import rclpy
from rclpy.node import Node


class RobotStatusNode(Node):
    def __init__(self) -> None:
        # Node 父类初始化完成后，才能使用定时器和日志等节点 API。
        super().__init__("robot_status_node")
        # 实例属性在多次回调之间保存当前机器人状态。
        self.robot_name = "robot_01"
        self.battery_percent = 85
        self.speed_mps = 0.30
        # 保存定时器对象，并注册回调方法本身（这里不加括号）。
        self.timer = self.create_timer(1.0, self.timer_callback)

    def timer_callback(self) -> None:
        # ROS 2 日志会附带时间、级别和节点名称，便于排查问题。
        self.get_logger().info(
            f"{self.robot_name}：电量 {self.battery_percent}%，"
            f"速度 {self.speed_mps:.2f} m/s"
        )
        # max() 保证模拟电量不会降到 0 以下。
        self.battery_percent = max(0, self.battery_percent - 1)


def main(args=None) -> None:
    # 初始化通信上下文后，才能创建 ROS 2 节点。
    rclpy.init(args=args)
    node = RobotStatusNode()

    try:
        # spin 让执行器持续等待并调度定时器回调。
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        # 退出时显式释放节点资源并关闭通信上下文。
        node.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    main()
