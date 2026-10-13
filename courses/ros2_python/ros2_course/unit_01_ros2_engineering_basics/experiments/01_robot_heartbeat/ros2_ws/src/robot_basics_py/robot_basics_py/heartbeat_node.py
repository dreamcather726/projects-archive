"""机器人心跳节点。

实验目的：用最少的 ROS 2 概念跑通“创建节点—注册定时器—进入
执行器—安全退出”的完整流程。教学时请重点观察定时回调只有在
``rclpy.spin`` 运行期间才会被执行。
"""

import time

import rclpy
from rclpy.node import Node


class RobotHeartbeat(Node):
    """周期输出存活信息的教学节点。"""

    def __init__(self) -> None:
        super().__init__('robot_heartbeat')

        # 为什么使用参数：真实机器人在不同硬件上需要不同心跳周期，
        # 不应为了修改一个配置值就改源码。第 4 章会系统学习参数。
        self.declare_parameter('timer_period', 1.0)
        timer_period = self.get_parameter('timer_period').value

        # 初学阶段先做最小防御：非法周期会让定时器行为不可用。
        # 验收任务会让学生把这里改成更完整的参数校验。
        if timer_period <= 0.0:
            self.get_logger().error(
                'timer_period 必须大于 0，已回退到默认值 1.0 秒'
            )
            timer_period = 1.0

        self._started_at = time.monotonic()
        self._heartbeat_count = 0

        # 必须把 Timer 保存为实例属性，让它与节点具有相同生命周期。
        # 回调函数后面不加括号：这里传入函数本身，而不是立即调用它。
        self._timer = self.create_timer(timer_period, self._on_timer)

        self.get_logger().info(
            f'机器人心跳节点已启动，周期为 {timer_period:.2f} 秒'
        )

    def _on_timer(self) -> None:
        """定时器到期时由执行器调用。"""
        self._heartbeat_count += 1
        elapsed = time.monotonic() - self._started_at
        self.get_logger().info(
            f'心跳 #{self._heartbeat_count}：节点已运行 {elapsed:.2f} 秒'
        )


def main(args=None) -> None:
    """初始化 ROS、运行节点并在退出时释放资源。"""
    rclpy.init(args=args)
    node = RobotHeartbeat()

    try:
        # spin 让执行器持续等待并调度 Timer 等回调。
        rclpy.spin(node)
    except KeyboardInterrupt:
        node.get_logger().info('收到 Ctrl+C，准备安全退出')
    finally:
        # 显式清理使退出顺序清晰，也方便今后加入设备或文件资源。
        node.destroy_node()
        rclpy.try_shutdown()


if __name__ == '__main__':
    main()
