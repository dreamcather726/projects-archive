"""发送巡检距离目标，显示反馈，并可按参数自动取消。"""

import rclpy
from rclpy.action import ActionClient
from rclpy.node import Node

from patrol_project_interfaces.action import PatrolDistance


class PatrolClient(Node):
    def __init__(self) -> None:
        super().__init__('patrol_client')
        self.declare_parameter('target_distance', 8.0)
        self.declare_parameter('cancel_after', 0.0)
        self.done = False
        self._goal_handle = None
        self._client = ActionClient(self, PatrolDistance, '/patrol_distance')
        self._start_timer = self.create_timer(0.1, self._start)
        self._cancel_timer = None

    def _start(self) -> None:
        self._start_timer.cancel()
        if not self._client.wait_for_server(timeout_sec=5.0):
            self.get_logger().error('巡检动作服务不可用')
            self.done = True
            return

        goal = PatrolDistance.Goal()
        goal.target_distance = self.get_parameter('target_distance').value
        future = self._client.send_goal_async(
            goal, feedback_callback=self._feedback
        )
        future.add_done_callback(self._goal_response)

    def _goal_response(self, future) -> None:
        self._goal_handle = future.result()
        if not self._goal_handle.accepted:
            self.get_logger().warning('巡检目标被拒绝')
            self.done = True
            return

        self.get_logger().info('巡检目标已接受')
        result_future = self._goal_handle.get_result_async()
        result_future.add_done_callback(self._result)

        cancel_after = self.get_parameter('cancel_after').value
        if cancel_after > 0.0:
            self._cancel_timer = self.create_timer(cancel_after, self._cancel)

    def _feedback(self, feedback_message) -> None:
        feedback = feedback_message.feedback
        self.get_logger().info(
            f'{feedback.state}: {feedback.traveled_distance:.2f} m, '
            f'{feedback.progress * 100.0:.1f}%'
        )

    def _cancel(self) -> None:
        self._cancel_timer.cancel()
        self.get_logger().warning('到达 cancel_after，发送取消请求')
        self._goal_handle.cancel_goal_async()

    def _result(self, future) -> None:
        result = future.result().result
        self.get_logger().info(
            f'结果：success={result.success}, state={result.final_state}, '
            f'distance={result.traveled_distance:.2f} m, {result.message}'
        )
        self.done = True


def main(args=None) -> None:
    rclpy.init(args=args)
    node = PatrolClient()
    try:
        while rclpy.ok() and not node.done:
            rclpy.spin_once(node, timeout_sec=0.2)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.try_shutdown()


if __name__ == '__main__':
    main()
