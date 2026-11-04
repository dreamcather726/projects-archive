import threading
import time

import rclpy
from rclpy.action import ActionServer, CancelResponse, GoalResponse
from rclpy.callback_groups import ReentrantCallbackGroup
from rclpy.executors import MultiThreadedExecutor
from rclpy.node import Node

from robot_interfaces.action import CountSteps


class CountStepsServer(Node):
    def __init__(self):
        super().__init__('count_steps_server')
        self.group = ReentrantCallbackGroup()
        self.lock = threading.Lock()
        self.busy = False
        self.action_server = ActionServer(
            self,
            CountSteps,
            '/count_steps',
            execute_callback=self.execute,
            goal_callback=self.accept_goal,
            cancel_callback=self.accept_cancel,
            callback_group=self.group,
        )

    def accept_goal(self, request):
        if not 1 <= request.target_steps <= 100:
            self.get_logger().warning('拒绝目标：步数必须在 [1, 100] 内')
            return GoalResponse.REJECT
        with self.lock:
            if self.busy:
                self.get_logger().warning('拒绝目标：当前已有任务')
                return GoalResponse.REJECT
            self.busy = True
        return GoalResponse.ACCEPT

    def accept_cancel(self, goal_handle):
        self.get_logger().info('接受取消请求，等待执行逻辑完成清理')
        return CancelResponse.ACCEPT

    def execute(self, goal_handle):
        result = CountSteps.Result()
        result.completed_steps = 0
        try:
            for _ in range(goal_handle.request.target_steps):
                if goal_handle.is_cancel_requested:
                    result.message = '任务已取消，返回已完成步数'
                    goal_handle.canceled()
                    return result
                if not rclpy.ok():
                    result.message = '节点关闭，任务中止'
                    goal_handle.abort()
                    return result

                # 模拟一小步工作，不控制真实硬件
                time.sleep(0.2)
                result.completed_steps += 1
                feedback = CountSteps.Feedback()
                feedback.current_step = result.completed_steps
                goal_handle.publish_feedback(feedback)

            # 处理最后一步期间收到的取消请求
            if goal_handle.is_cancel_requested:
                result.message = '任务已取消'
                goal_handle.canceled()
            else:
                result.message = '全部步骤完成'
                goal_handle.succeed()
            return result
        except Exception as exc:
            result.message = f'执行异常：{exc}'
            if goal_handle.is_active:
                goal_handle.abort()
            return result
        finally:
            with self.lock:
                self.busy = False


def main():
    rclpy.init()
    node = CountStepsServer()
    executor = MultiThreadedExecutor(num_threads=3)
    executor.add_node(node)
    try:
        executor.spin()
    except KeyboardInterrupt:
        pass
    finally:
        executor.shutdown()
        node.action_server.destroy()
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == '__main__':
    main()
