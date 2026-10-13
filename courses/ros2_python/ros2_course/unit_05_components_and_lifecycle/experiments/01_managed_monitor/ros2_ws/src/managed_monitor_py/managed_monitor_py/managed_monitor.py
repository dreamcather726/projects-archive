"""只有激活后才发布业务告警的生命周期节点。"""

import rclpy
from rclpy.lifecycle import LifecycleNode, TransitionCallbackReturn
from std_msgs.msg import String

from managed_monitor_py.anomaly_detector import AnomalyDetector


class ManagedMonitor(LifecycleNode):
    def __init__(self) -> None:
        super().__init__('managed_monitor')
        self.declare_parameter('min_distance', 0.6)
        self._detector = None
        self._publisher = None
        self._timer = None
        self._active = False
        self._samples = [1.2, 0.8, 0.4, -1.0]
        self._index = 0

    def on_configure(self, state) -> TransitionCallbackReturn:
        del state
        try:
            threshold = self.get_parameter('min_distance').value
            # 设备连接、文件读取、模型加载也适合在 configure 阶段完成。
            self._detector = AnomalyDetector(threshold)
            self._publisher = self.create_lifecycle_publisher(
                String, '/monitor/alarm', 10
            )
            self._timer = self.create_timer(1.0, self._check_sample)
        except (TypeError, ValueError) as error:
            self.get_logger().error(f'配置失败：{error}')
            return TransitionCallbackReturn.FAILURE

        self.get_logger().info('配置完成：资源已创建，等待激活')
        return TransitionCallbackReturn.SUCCESS

    def on_activate(self, state) -> TransitionCallbackReturn:
        result = super().on_activate(state)
        if result == TransitionCallbackReturn.SUCCESS:
            self._active = True
            self.get_logger().info('节点已激活，开始检测并发布告警')
        return result

    def on_deactivate(self, state) -> TransitionCallbackReturn:
        self._active = False
        self.get_logger().info('节点已停用，保留资源但停止业务输出')
        return super().on_deactivate(state)

    def on_cleanup(self, state) -> TransitionCallbackReturn:
        del state
        self._active = False
        if self._timer is not None:
            self.destroy_timer(self._timer)
        if self._publisher is not None:
            self.destroy_publisher(self._publisher)
        self._timer = None
        self._publisher = None
        self._detector = None
        self.get_logger().info('清理完成：资源已释放')
        return TransitionCallbackReturn.SUCCESS

    def _check_sample(self) -> None:
        # inactive 状态节点仍在 spin，定时器也可能触发；必须阻止业务输出。
        if not self._active or self._detector is None:
            return

        distance = self._samples[self._index]
        self._index = (self._index + 1) % len(self._samples)
        alarm = self._detector.evaluate(distance)
        if alarm is None:
            self.get_logger().info(f'距离正常：{distance:.2f} m')
            return

        message = String()
        message.data = alarm
        self._publisher.publish(message)
        self.get_logger().warning(alarm)


def main(args=None) -> None:
    rclpy.init(args=args)
    node = ManagedMonitor()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.try_shutdown()


if __name__ == '__main__':
    main()
