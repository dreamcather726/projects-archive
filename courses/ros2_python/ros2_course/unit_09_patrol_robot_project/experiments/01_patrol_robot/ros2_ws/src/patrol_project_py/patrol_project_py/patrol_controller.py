"""自主巡检控制器：动作管理任务，定时器运行安全状态机。"""

import asyncio
import math

import rclpy
from geometry_msgs.msg import Twist
from nav_msgs.msg import Odometry
from rclpy.action import ActionServer, CancelResponse, GoalResponse
from rclpy.callback_groups import ReentrantCallbackGroup
from rclpy.executors import MultiThreadedExecutor
from rclpy.node import Node
from rclpy.qos import (
    DurabilityPolicy,
    QoSProfile,
    ReliabilityPolicy,
    qos_profile_sensor_data,
)
from sensor_msgs.msg import LaserScan
from std_msgs.msg import String
from std_srvs.srv import SetBool

from patrol_project_interfaces.action import PatrolDistance


class PatrolController(Node):
    """把传感器、动作、服务和速度输出组合为一个可验收系统。"""

    IDLE = 'IDLE'
    FORWARD = 'FORWARD'
    TURNING = 'TURNING'
    COMPLETED = 'COMPLETED'
    CANCELED = 'CANCELED'
    EMERGENCY_STOP = 'EMERGENCY_STOP'
    FAULT = 'FAULT'

    def __init__(self) -> None:
        super().__init__('patrol_controller')
        for name, default in (
            ('safe_distance', 0.75),
            ('clear_distance', 0.95),
            ('forward_speed', 0.30),
            ('turn_speed', 0.75),
            ('sensor_timeout', 1.0),
        ):
            self.declare_parameter(name, default)

        self._callback_group = ReentrantCallbackGroup()
        self._state = self.IDLE
        self._emergency_stop = False
        self._task_active = False
        self._target_distance = 0.0
        self._traveled_distance = 0.0
        self._previous_xy = None
        self._front_distance = math.inf
        self._left_distance = math.inf
        self._right_distance = math.inf
        self._turn_direction = 1.0
        self._last_scan_time = None
        self._last_odom_time = None

        self._cmd_publisher = self.create_publisher(Twist, '/cmd_vel', 10)
        # 状态使用 transient local：稍后启动的观察者也能立即获得最新状态。
        status_qos = QoSProfile(
            depth=1,
            reliability=ReliabilityPolicy.RELIABLE,
            durability=DurabilityPolicy.TRANSIENT_LOCAL,
        )
        self._status_publisher = self.create_publisher(
            String, '/patrol/status', status_qos
        )
        self._scan_subscription = self.create_subscription(
            LaserScan,
            '/scan',
            self._on_scan,
            qos_profile_sensor_data,
            callback_group=self._callback_group,
        )
        self._odom_subscription = self.create_subscription(
            Odometry,
            '/odom',
            self._on_odom,
            qos_profile_sensor_data,
            callback_group=self._callback_group,
        )
        self._emergency_service = self.create_service(
            SetBool,
            '/patrol/emergency_stop',
            self._set_emergency_stop,
            callback_group=self._callback_group,
        )
        self._action_server = ActionServer(
            self,
            PatrolDistance,
            '/patrol_distance',
            execute_callback=self._execute,
            goal_callback=self._on_goal,
            cancel_callback=self._on_cancel,
            callback_group=self._callback_group,
        )
        self._control_timer = self.create_timer(
            0.1, self._control, callback_group=self._callback_group
        )
        self._publish_state(self.IDLE)

    def _sector_min(self, message: LaserScan, start_angle: float, end_angle: float) -> float:
        start = max(0, int((start_angle - message.angle_min) / message.angle_increment))
        end = min(len(message.ranges), int((end_angle - message.angle_min) / message.angle_increment))
        valid = [value for value in message.ranges[start:end] if math.isfinite(value)]
        return min(valid) if valid else math.inf

    def _on_scan(self, message: LaserScan) -> None:
        self._front_distance = self._sector_min(
            message, math.radians(-12), math.radians(12)
        )
        self._left_distance = self._sector_min(
            message, math.radians(20), math.radians(90)
        )
        self._right_distance = self._sector_min(
            message, math.radians(-90), math.radians(-20)
        )
        self._last_scan_time = self.get_clock().now()

    def _on_odom(self, message: Odometry) -> None:
        position = message.pose.pose.position
        current_xy = (position.x, position.y)
        # 只在任务期间累加相邻里程计位置，避免把启动前运动计入本次目标。
        if self._task_active and self._previous_xy is not None:
            self._traveled_distance += math.dist(self._previous_xy, current_xy)
        self._previous_xy = current_xy
        self._last_odom_time = self.get_clock().now()

    def _sensors_ready(self) -> bool:
        return self._last_scan_time is not None and self._last_odom_time is not None

    def _on_goal(self, request) -> GoalResponse:
        if request.target_distance <= 0.0:
            self.get_logger().warning('拒绝目标：巡检距离必须大于 0')
            return GoalResponse.REJECT
        if self._task_active or self._emergency_stop or not self._sensors_ready():
            self.get_logger().warning('拒绝目标：系统忙、急停或传感器未就绪')
            return GoalResponse.REJECT
        return GoalResponse.ACCEPT

    def _on_cancel(self, _goal_handle) -> CancelResponse:
        return CancelResponse.ACCEPT

    async def _execute(self, goal_handle):
        self._task_active = True
        self._target_distance = goal_handle.request.target_distance
        self._traveled_distance = 0.0
        # 重新从下一条 odom 开始累计，避免上一任务终点到当前点的跳变。
        self._previous_xy = None
        self._publish_state(self.FORWARD)
        result = PatrolDistance.Result()

        try:
            while rclpy.ok():
                if goal_handle.is_cancel_requested:
                    goal_handle.canceled()
                    self._publish_state(self.CANCELED)
                    return self._make_result(False, '客户端取消了巡检')

                if self._emergency_stop:
                    goal_handle.abort()
                    return self._make_result(False, '急停触发，任务终止')

                if self._state == self.FAULT:
                    goal_handle.abort()
                    return self._make_result(False, '传感器或里程计超时')

                if self._traveled_distance >= self._target_distance:
                    goal_handle.succeed()
                    self._publish_state(self.COMPLETED)
                    return self._make_result(True, '已达到目标巡检距离')

                feedback = PatrolDistance.Feedback()
                feedback.traveled_distance = self._traveled_distance
                feedback.progress = min(
                    1.0, self._traveled_distance / self._target_distance
                )
                feedback.state = self._state
                goal_handle.publish_feedback(feedback)
                await asyncio.sleep(0.25)
        finally:
            self._task_active = False
            self._publish_stop()
            if self._state not in (
                self.COMPLETED, self.CANCELED, self.EMERGENCY_STOP, self.FAULT
            ):
                self._publish_state(self.IDLE)

    def _make_result(self, success: bool, message: str):
        result = PatrolDistance.Result()
        result.success = success
        result.traveled_distance = self._traveled_distance
        result.final_state = self._state
        result.message = message
        return result

    def _set_emergency_stop(self, request, response):
        self._emergency_stop = request.data
        if self._emergency_stop:
            self._publish_state(self.EMERGENCY_STOP)
            self._publish_stop()
            response.message = '急停已触发'
        else:
            self._publish_state(self.IDLE)
            response.message = '急停已解除，可以重新发送任务'
        response.success = True
        return response

    def _data_timed_out(self) -> bool:
        if not self._sensors_ready():
            return True
        now = self.get_clock().now()
        timeout = self.get_parameter('sensor_timeout').value
        scan_age = (now - self._last_scan_time).nanoseconds / 1e9
        odom_age = (now - self._last_odom_time).nanoseconds / 1e9
        return scan_age > timeout or odom_age > timeout

    def _control(self) -> None:
        if not self._task_active or self._emergency_stop:
            self._publish_stop()
            return
        if self._data_timed_out():
            self._publish_state(self.FAULT)
            self._publish_stop()
            return

        safe = self.get_parameter('safe_distance').value
        clear = self.get_parameter('clear_distance').value

        # 使用不同进入/退出阈值形成滞回，防止临界距离附近高速切换状态。
        if self._state == self.TURNING:
            if self._front_distance > clear:
                self._publish_state(self.FORWARD)
        elif self._front_distance < safe:
            self._turn_direction = 1.0 if self._left_distance >= self._right_distance else -1.0
            self._publish_state(self.TURNING)

        command = Twist()
        if self._state == self.FORWARD:
            command.linear.x = self.get_parameter('forward_speed').value
        elif self._state == self.TURNING:
            command.angular.z = (
                self._turn_direction * self.get_parameter('turn_speed').value
            )
        self._cmd_publisher.publish(command)

    def _publish_stop(self) -> None:
        self._cmd_publisher.publish(Twist())

    def _publish_state(self, new_state: str) -> None:
        if new_state != self._state:
            self.get_logger().info(f'状态变化：{self._state} -> {new_state}')
        self._state = new_state
        message = String()
        message.data = new_state
        self._status_publisher.publish(message)

    def destroy_node(self):
        self._publish_stop()
        self._action_server.destroy()
        super().destroy_node()


def main(args=None) -> None:
    rclpy.init(args=args)
    node = PatrolController()
    executor = MultiThreadedExecutor(num_threads=4)
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
