import json
import math
import time

import rclpy
from geometry_msgs.msg import Quaternion, Twist
from nav_msgs.msg import Odometry
from rclpy.node import Node
from sensor_msgs.msg import BatteryState, Image, Imu, LaserScan
from std_msgs.msg import Bool, String


def create_yaw_quaternion(yaw_rad):
    quaternion = Quaternion()
    quaternion.z = math.sin(yaw_rad * 0.5)
    quaternion.w = math.cos(yaw_rad * 0.5)
    return quaternion


class SimulatorNode(Node):
    """Publish fake car data so the cockpit can be developed without hardware."""

    def __init__(self):
        super().__init__("ros_car_simulator")
        self.cmd_vel_sub = self.create_subscription(Twist, "/cmd_vel", self.handle_cmd_vel, 10)
        self.camera_enabled_sub = self.create_subscription(
            Bool, "/simulator/camera_enabled", self.handle_camera_enabled, 10
        )

        self.scan_pub = self.create_publisher(LaserScan, "/scan", 10)
        self.odom_pub = self.create_publisher(Odometry, "/odom", 10)
        self.imu_pub = self.create_publisher(Imu, "/imu/data", 10)
        self.battery_pub = self.create_publisher(BatteryState, "/battery_state", 10)
        self.camera_pub = self.create_publisher(Image, "/camera/image_raw", 10)
        self.status_pub = self.create_publisher(String, "/simulator/status", 10)

        self.linear_speed_mps = 0.0
        self.angular_speed_radps = 0.0
        self.position_x_m = 0.0
        self.position_y_m = 0.0
        self.yaw_rad = 0.0
        self.battery_percent = 87.0
        self.camera_enabled = False
        self.frame_index = 0
        self.last_update_time = time.monotonic()

        self.timer = self.create_timer(0.05, self.publish_simulated_data)
        self.get_logger().info("ROS-CAR simulator started")

    def handle_cmd_vel(self, message):
        self.linear_speed_mps = message.linear.x
        self.angular_speed_radps = message.angular.z

    def handle_camera_enabled(self, message):
        self.camera_enabled = message.data

    def publish_simulated_data(self):
        now = self.get_clock().now().to_msg()
        current_time = time.monotonic()
        delta_seconds = current_time - self.last_update_time
        self.last_update_time = current_time

        self.yaw_rad += self.angular_speed_radps * delta_seconds
        self.position_x_m += math.cos(self.yaw_rad) * self.linear_speed_mps * delta_seconds
        self.position_y_m += math.sin(self.yaw_rad) * self.linear_speed_mps * delta_seconds
        self.battery_percent = max(15.0, self.battery_percent - 0.0005)

        self.publish_scan(now)
        self.publish_odom(now)
        self.publish_imu(now)
        self.publish_battery(now)
        self.publish_status()
        if self.camera_enabled:
            self.publish_camera(now)

    def publish_scan(self, stamp):
        scan = LaserScan()
        scan.header.stamp = stamp
        scan.header.frame_id = "laser_link"
        scan.angle_min = -math.pi
        scan.angle_max = math.pi
        scan.angle_increment = math.radians(1.0)
        scan.time_increment = 0.0
        scan.scan_time = 0.1
        scan.range_min = 0.15
        scan.range_max = 12.0
        point_count = int((scan.angle_max - scan.angle_min) / scan.angle_increment)
        scan.ranges = []
        for index in range(point_count):
            angle = scan.angle_min + index * scan.angle_increment
            wall = 3.5 + 0.8 * math.sin(angle * 3.0 + self.frame_index * 0.03)
            obstacle = 1.2 if abs(angle - 0.6) < 0.08 else wall
            scan.ranges.append(float(max(scan.range_min, min(scan.range_max, obstacle))))
        self.scan_pub.publish(scan)

    def publish_odom(self, stamp):
        odom = Odometry()
        odom.header.stamp = stamp
        odom.header.frame_id = "odom"
        odom.child_frame_id = "base_link"
        odom.pose.pose.position.x = self.position_x_m
        odom.pose.pose.position.y = self.position_y_m
        odom.pose.pose.orientation = create_yaw_quaternion(self.yaw_rad)
        odom.twist.twist.linear.x = self.linear_speed_mps
        odom.twist.twist.angular.z = self.angular_speed_radps
        self.odom_pub.publish(odom)

    def publish_imu(self, stamp):
        imu = Imu()
        imu.header.stamp = stamp
        imu.header.frame_id = "imu_link"
        imu.orientation = create_yaw_quaternion(self.yaw_rad)
        imu.angular_velocity.z = self.angular_speed_radps
        imu.linear_acceleration.z = 9.81
        self.imu_pub.publish(imu)

    def publish_battery(self, stamp):
        battery = BatteryState()
        battery.header.stamp = stamp
        battery.voltage = 24.6 * self.battery_percent / 100.0
        battery.current = 2.3
        battery.percentage = self.battery_percent / 100.0
        self.battery_pub.publish(battery)

    def publish_camera(self, stamp):
        width = 640
        height = 360
        image = Image()
        image.header.stamp = stamp
        image.header.frame_id = "camera_link"
        image.height = height
        image.width = width
        image.encoding = "rgb8"
        image.is_bigendian = False
        image.step = width * 3
        pixels = bytearray(width * height * 3)
        stripe_x = (self.frame_index * 5) % width
        for row in range(height):
            for col in range(width):
                offset = (row * width + col) * 3
                pixels[offset] = (col + self.frame_index) % 255
                pixels[offset + 1] = (row * 2) % 255
                pixels[offset + 2] = 80 if abs(col - stripe_x) > 8 else 255
        image.data = bytes(pixels)
        self.camera_pub.publish(image)
        self.frame_index += 1

    def publish_status(self):
        status = {
            "linear_speed_mps": round(self.linear_speed_mps, 3),
            "angular_speed_radps": round(self.angular_speed_radps, 3),
            "battery_percent": round(self.battery_percent, 1),
            "camera_enabled": self.camera_enabled,
        }
        self.status_pub.publish(String(data=json.dumps(status)))


def main(args=None):
    rclpy.init(args=args)
    node = SimulatorNode()
    try:
        rclpy.spin(node)
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    main()
