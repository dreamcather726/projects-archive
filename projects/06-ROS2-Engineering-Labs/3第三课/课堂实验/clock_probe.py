import rclpy
from rclpy.clock import Clock, ClockType
from rclpy.node import Node


class ClockProbe(Node):
    def __init__(self):
        super().__init__('clock_probe')
        self.system_clock = Clock(clock_type=ClockType.SYSTEM_TIME)
        self.steady_clock = Clock(clock_type=ClockType.STEADY_TIME)
        self.start_steady = self.steady_clock.now().nanoseconds
        self.ros_ticks = 0
        self.ros_timer = self.create_timer(1.0, self.on_ros_tick)
        self.observer = self.create_timer(
            1.0, self.observe, clock=self.steady_clock)

    def on_ros_tick(self):
        self.ros_ticks += 1
        self.get_logger().info(f'ROS timer tick={self.ros_ticks}')

    def observe(self):
        ros_ns = self.get_clock().now().nanoseconds
        system_ns = self.system_clock.now().nanoseconds
        elapsed_ns = self.steady_clock.now().nanoseconds - self.start_steady
        self.get_logger().info(
            f'ros={ros_ns / 1e9:.3f}s '
            f'system={system_ns / 1e9:.3f}s '
            f'steady_elapsed={elapsed_ns / 1e9:.3f}s '
            f'ros_ticks={self.ros_ticks}')


def main(args=None):
    rclpy.init(args=args)
    node = ClockProbe()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == '__main__':
    main()
