import time

import rclpy
from rclpy.action import ActionClient
from rclpy.node import Node
from action_msgs.msg import GoalStatus

from robot_interfaces.action import CountSteps


def main():
    rclpy.init()
    node = Node('count_steps_cancel_client')
    client = ActionClient(node, CountSteps, '/count_steps')

    def feedback_callback(message):
        step = message.feedback.current_step
        node.get_logger().info(f'已完成 {step} 步')

    try:
        if not client.wait_for_server(timeout_sec=3.0):
            node.get_logger().error('动作服务端不可用')
            return

        goal = CountSteps.Goal()
        goal.target_steps = 30
        goal_future = client.send_goal_async(
            goal, feedback_callback=feedback_callback
        )
        rclpy.spin_until_future_complete(node, goal_future, timeout_sec=3.0)
        if not goal_future.done():
            node.get_logger().error('受理响应超时，目标是否受理尚不确定')
            return

        goal_handle = goal_future.result()
        if not goal_handle.accepted:
            node.get_logger().warning('目标被拒绝')
            return

        result_future = goal_handle.get_result_async()
        deadline = time.monotonic() + 1.0
        while rclpy.ok() and not result_future.done():
            if time.monotonic() >= deadline:
                break
            rclpy.spin_once(node, timeout_sec=0.1)

        if not result_future.done():
            cancel_future = goal_handle.cancel_goal_async()
            rclpy.spin_until_future_complete(
                node, cancel_future, timeout_sec=3.0
            )
            if cancel_future.done():
                response = cancel_future.result()
                ids = [info.goal_id for info in response.goals_canceling]
                if goal_handle.goal_id in ids:
                    node.get_logger().info('取消已受理，继续等待最终结果')
                else:
                    node.get_logger().warning('目标未进入取消中，继续查询结果')
            else:
                node.get_logger().warning('取消响应超时，取消是否受理尚不确定')

        rclpy.spin_until_future_complete(
            node, result_future, timeout_sec=3.0
        )
        if not result_future.done():
            node.get_logger().error('最终结果超时，任务结束状态尚未确认')
            return

        response = result_future.result()
        status_names = {
            GoalStatus.STATUS_SUCCEEDED: 'SUCCEEDED',
            GoalStatus.STATUS_CANCELED: 'CANCELED',
            GoalStatus.STATUS_ABORTED: 'ABORTED',
        }
        name = status_names.get(response.status, str(response.status))
        node.get_logger().info(
            f'终态={name}，完成步数={response.result.completed_steps}，'
            f'说明={response.result.message}'
        )
    except Exception as exc:
        node.get_logger().error(f'动作调用异常：{exc}')
    finally:
        client.destroy()
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == '__main__':
    main()
