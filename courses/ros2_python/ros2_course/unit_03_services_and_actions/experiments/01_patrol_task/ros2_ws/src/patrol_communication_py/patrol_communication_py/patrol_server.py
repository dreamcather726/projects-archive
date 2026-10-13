"""同时提供巡检启停服务和长时巡检动作。"""

import asyncio

import rclpy
from rclpy.action import ActionServer, CancelResponse, GoalResponse
from rclpy.callback_groups import ReentrantCallbackGroup
from rclpy.executors import MultiThreadedExecutor
from rclpy.node import Node

from patrol_interfaces.action import RunPatrol
from patrol_interfaces.srv import SetPatrol


class PatrolServer(Node):
    """用不同接口表达“快速设置”和“长时任务”。"""

    def __init__(self) -> None:
        super().__init__('patrol_server')
        self._enabled = False

        # 使用可重入回调组和多线程执行器，使动作执行期间仍能响应取消和服务。
        # 具体调度原理会在第 6 章展开。
        self._callback_group = ReentrantCallbackGroup()
        self._enable_service = self.create_service(
            SetPatrol,
            '/set_patrol_enabled',
            self._set_enabled,
            callback_group=self._callback_group,
        )
        self._action_server = ActionServer(
            self,
            RunPatrol,
            '/run_patrol',
            execute_callback=self._execute_patrol,
            goal_callback=self._on_goal,
            cancel_callback=self._on_cancel,
            callback_group=self._callback_group,
        )

    def _set_enabled(self, request, response):
        self._enabled = request.enabled
        response.success = True
        response.message = '巡检已启用' if self._enabled else '巡检已停用'
        self.get_logger().info(response.message)
        return response

    def _on_goal(self, goal_request) -> GoalResponse:
        if not self._enabled:
            self.get_logger().warning('拒绝目标：巡检功能尚未启用')
            return GoalResponse.REJECT
        if goal_request.waypoint_count <= 0 or goal_request.dwell_seconds <= 0.0:
            self.get_logger().warning('拒绝目标：点数和停留时间必须大于 0')
            return GoalResponse.REJECT
        return GoalResponse.ACCEPT

    def _on_cancel(self, _goal_handle) -> CancelResponse:
        self.get_logger().warning('收到取消请求')
        return CancelResponse.ACCEPT

    async def _execute_patrol(self, goal_handle):
        request = goal_handle.request
        feedback = RunPatrol.Feedback()
        result = RunPatrol.Result()

        for index in range(1, request.waypoint_count + 1):
            # 取消不是强行终止线程；服务端必须在合适位置主动检查。
            if goal_handle.is_cancel_requested:
                goal_handle.canceled()
                result.success = False
                result.visited_count = index - 1
                result.message = '任务已由客户端取消'
                return result

            await asyncio.sleep(request.dwell_seconds)
            feedback.current_waypoint = index
            feedback.progress = index / request.waypoint_count
            goal_handle.publish_feedback(feedback)
            self.get_logger().info(
                f'已访问巡检点 {index}/{request.waypoint_count}'
            )

        goal_handle.succeed()
        result.success = True
        result.visited_count = request.waypoint_count
        result.message = '全部巡检点访问完成'
        return result

    def destroy_node(self):
        self._action_server.destroy()
        super().destroy_node()


def main(args=None) -> None:
    rclpy.init(args=args)
    node = PatrolServer()
    executor = MultiThreadedExecutor(num_threads=2)
    executor.add_node(node)
    try:
        executor.spin()
    except KeyboardInterrupt:
        pass
    finally:
        executor.shutdown()
        node.destroy_node()
        rclpy.try_shutdown()


if __name__ == '__main__':
    main()
