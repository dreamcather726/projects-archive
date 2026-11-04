"""Single/multi executors and callback group experiments; run inside ROS 2 Humble."""
import argparse
import threading
import time
import rclpy
from rclpy.node import Node
from rclpy.executors import SingleThreadedExecutor, MultiThreadedExecutor
from rclpy.callback_groups import MutuallyExclusiveCallbackGroup, ReentrantCallbackGroup
from std_msgs.msg import String


class Probe(Node):
    def __init__(self, group_mode, slow_seconds):
        super().__init__('executor_probe')
        self.slow_seconds = slow_seconds
        self.last_fast = time.monotonic()
        self.lock = threading.Lock()
        self.publisher = self.create_publisher(String, '/executor/fast', 10)
        self.groups = []
        slow_group = fast_group = self.default_callback_group
        if group_mode == 'split':
            self.groups = [MutuallyExclusiveCallbackGroup(), MutuallyExclusiveCallbackGroup()]
            slow_group, fast_group = self.groups
        elif group_mode == 'reentrant':
            self.groups = [ReentrantCallbackGroup()]
            slow_group = fast_group = self.groups[0]
        self.slow_timer = self.create_timer(1.0, self.slow, callback_group=slow_group)
        self.fast_timer = self.create_timer(0.1, self.fast, callback_group=fast_group)

    def slow(self):
        worker = threading.get_ident()
        self.get_logger().info(f'SLOW BEGIN thread={worker}')
        time.sleep(self.slow_seconds)
        self.get_logger().info(f'SLOW END thread={worker}')

    def fast(self):
        now = time.monotonic()
        with self.lock:
            gap = now - self.last_fast
            self.last_fast = now
        message = String()
        message.data = f'FAST gap={gap:.3f}s thread={threading.get_ident()}'
        self.publisher.publish(message)
        self.get_logger().info(message.data)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--executor', choices=['none', 'single', 'multi'], default='single')
    parser.add_argument('--groups', choices=['default', 'split', 'reentrant'], default='default')
    parser.add_argument('--slow', type=float, default=0.6)
    args, ros_args = parser.parse_known_args()
    if args.slow < 0:
        parser.error('--slow must be nonnegative')
    rclpy.init(args=ros_args)
    node = Probe(args.groups, args.slow)
    executor = None
    try:
        if args.executor == 'none':
            node.get_logger().info('No spin: entities exist, callbacks are not executed for 5s')
            time.sleep(5.0)
        else:
            executor = (SingleThreadedExecutor() if args.executor == 'single'
                        else MultiThreadedExecutor(num_threads=2))
            executor.add_node(node)
            executor.spin()
    except KeyboardInterrupt:
        pass
    finally:
        if executor is not None:
            executor.shutdown()
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == '__main__':
    main()
