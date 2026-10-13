"""先启用巡检，再异步发送动作目标并可定时取消。"""

import rclpy
from rclpy.action import ActionClient
from rclpy.node import Node

from patrol_interfaces.action import RunPatrol
from patrol_interfaces.srv import SetPatrol


class PatrolClient(Node):
    """演示由多个 Future 串联起来的非阻塞客户端流程。"""

    def __init__(self) -> None:
        super().__init__('patrol_client')
        self.declare_parameter('waypoint_count', 5)
        self.declare_parameter('dwell_seconds', 0.8)
        self.declare_parameter('cancel_after', 0.0)

        self.done = False
        self._goal_handle = None
        self._service_client = self.create_client(
            SetPatrol, '/set_patrol_enabled'
        )
        self._action_client = ActionClient(self, RunPatrol, '/run_patrol')
        # 用一次性定时器进入流程，避免在构造函数中阻塞执行器。
        self._start_timer = self.create_timer(0.1, self._start)
        self._cancel_timer = None

    def _start(self) -> None:
        self._start_timer.cancel()
        if not self._service_client.wait_for_service(timeout_sec=2.0):
            self.get_logger().error('启停服务不可用，请先启动 patrol_server')
            self.done = True
            return

        request = SetPatrol.Request()
        request.enabled = True
        future = self._service_client.call_async(request)
        future.add_done_callback(self._on_enabled)

    def _on_enabled(self, future) -> None:
        try:
            response = future.result()
        except Exception as error:  # ROS 调用异常需要明确记录。
            self.get_logger().error(f'启用服务调用失败：{error}')
            self.done = True
            return

        self.get_logger().info(response.message)
        if not response.success or not self._action_client.wait_for_server(2.0):
            self.get_logger().error('动作服务不可用或巡检启用失败')
            self.done = True
            return

        goal = RunPatrol.Goal()
        goal.waypoint_count = self.get_parameter('waypoint_count').value
        goal.dwell_seconds = self.get_parameter('dwell_seconds').value
        send_future = self._action_client.send_goal_async(
            goal, feedback_callback=self._on_feedback
        )
        send_future.add_done_callback(self._on_goal_response)

    def _on_goal_response(self, future) -> None:
        self._goal_handle = future.result()
        if not self._goal_handle.accepted:
            self.get_logger().error('动作目标被拒绝')
            self.done = True
            return

        self.get_logger().info('动作目标已接受；等待反馈和最终结果')
        cancel_after = self.get_parameter('cancel_after').value
        if cancel_after > 0.0:
            self._cancel_timer = self.create_timer(cancel_after, self._cancel_goal)

        result_future = self._goal_handle.get_result_async()
        result_future.add_done_callback(self._on_result)

    def _on_feedback(self, feedback_message) -> None:
        feedback = feedback_message.feedback
        self.get_logger().info(
            f'反馈：当前点={feedback.current_waypoint}，'
            f'进度={feedback.progress:.0%}'
        )

    def _cancel_goal(self) -> None:
        self._cancel_timer.cancel()
        if self._goal_handle is not None:
            self.get_logger().warning('达到 cancel_after，发送取消请求')
            self._goal_handle.cancel_goal_async()

    def _on_result(self, future) -> None:
        result = future.result().result
        self.get_logger().info(
            f'结果：success={result.success}, '
            f'visited={result.visited_count}, message={result.message}'
        )
        self.done = True


def main(args=None) -> None:
    rclpy.init(args=args)
    node = PatrolClient()
    try:
        # 主循环观察 done，使示例在拿到结果后能自行退出。
        while rclpy.ok() and not node.done:
            rclpy.spin_once(node, timeout_sec=0.1)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.try_shutdown()


if __name__ == '__main__':
    main()
