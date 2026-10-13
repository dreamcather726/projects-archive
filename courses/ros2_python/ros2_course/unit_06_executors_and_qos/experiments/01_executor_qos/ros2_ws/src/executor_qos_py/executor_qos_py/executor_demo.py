"""用相同节点比较执行器，并通过参数制造 QoS 不兼容。"""

import time

import rclpy
from rclpy.callback_groups import MutuallyExclusiveCallbackGroup
from rclpy.executors import MultiThreadedExecutor, SingleThreadedExecutor
from rclpy.node import Node
from rclpy.qos import (
    DurabilityPolicy,
    HistoryPolicy,
    QoSProfile,
    ReliabilityPolicy,
)
from std_msgs.msg import Int32


class ExecutorDemo(Node):
    def __init__(self) -> None:
        super().__init__('executor_demo')
        self.declare_parameter('request_reliable', False)

        # 不同互斥组：各自内部不重入，但多线程执行器可让两组并发。
        self._sensor_group = MutuallyExclusiveCallbackGroup()
        self._slow_group = MutuallyExclusiveCallbackGroup()

        reliability = (
            ReliabilityPolicy.RELIABLE
            if self.get_parameter('request_reliable').value
            else ReliabilityPolicy.BEST_EFFORT
        )
        qos = QoSProfile(
            history=HistoryPolicy.KEEP_LAST,
            depth=10,
            reliability=reliability,
            durability=DurabilityPolicy.VOLATILE,
        )
        self._received = 0
        self._last_sequence = -1
        self._subscription = self.create_subscription(
            Int32,
            '/fast_sensor',
            self._on_sensor,
            qos,
            callback_group=self._sensor_group,
        )
        self._slow_timer = self.create_timer(
            2.0, self._slow_job, callback_group=self._slow_group
        )
        self._report_timer = self.create_timer(
            1.0, self._report, callback_group=self._sensor_group
        )

    def _on_sensor(self, message: Int32) -> None:
        self._received += 1
        self._last_sequence = message.data

    def _slow_job(self) -> None:
        self.get_logger().warning('慢任务开始：阻塞 1 秒')
        # 故意阻塞，用来突出执行器差异。实际节点应避免在回调中 sleep。
        time.sleep(1.0)
        self.get_logger().warning('慢任务结束')

    def _report(self) -> None:
        self.get_logger().info(
            f'累计接收={self._received}, 最新序号={self._last_sequence}'
        )


def _run(executor_type) -> None:
    rclpy.init()
    node = ExecutorDemo()
    executor = executor_type()
    executor.add_node(node)
    node.get_logger().info(f'正在使用 {executor_type.__name__}')
    try:
        executor.spin()
    except KeyboardInterrupt:
        pass
    finally:
        executor.shutdown()
        node.destroy_node()
        rclpy.try_shutdown()


def main_single() -> None:
    _run(SingleThreadedExecutor)


def main_multi() -> None:
    _run(MultiThreadedExecutor)


if __name__ == '__main__':
    main_single()
