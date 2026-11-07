"""Print compact laser distance observations instead of a full 360-element array."""
import math
import rclpy
from rclpy.node import Node
from rclpy.qos import qos_profile_sensor_data
from sensor_msgs.msg import LaserScan


class ScanProbe(Node):
    def __init__(self):
        super().__init__('scan_probe')
        self.subscription = self.create_subscription(LaserScan, '/scan', self.receive,
                                                      qos_profile_sensor_data)

    def receive(self, scan):
        values = [x for x in scan.ranges if math.isfinite(x) and scan.range_min <= x <= scan.range_max]
        front = float('nan')
        if scan.ranges and scan.angle_increment != 0:
            index = round((0.0 - scan.angle_min) / scan.angle_increment)
            if 0 <= index < len(scan.ranges):
                front = scan.ranges[index]
        closest = min(values) if values else float('inf')
        self.get_logger().info(f'frame={scan.header.frame_id} beams={len(scan.ranges)} '
                               f'front={front:.3f}m min={closest:.3f}m valid={len(values)}')


def main():
    rclpy.init()
    node = ScanProbe()
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
