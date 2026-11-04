import threading

import rclpy
from geometry_msgs.msg import Twist
from nav_msgs.msg import Odometry
from rclpy.node import Node
from sensor_msgs.msg import BatteryState, Image, Imu, LaserScan
from std_msgs.msg import Bool, String


class CockpitRosNode(Node):
    """Keep ROS2 communication separate from the Qt widgets."""

    def __init__(self, app_state, image_callback=None):
        super().__init__("ros_car_cockpit_client")
        self.app_state = app_state
        self.image_callback = image_callback
        self.cmd_vel_pub = self.create_publisher(Twist, "/cmd_vel", 10)
        self.camera_enabled_pub = self.create_publisher(Bool, "/simulator/camera_enabled", 10)
        self.create_subscription(BatteryState, "/battery_state", self.handle_battery, 10)
        self.create_subscription(Odometry, "/odom", self.handle_odom, 10)
        self.create_subscription(Imu, "/imu/data", self.handle_imu, 10)
        self.create_subscription(LaserScan, "/scan", self.handle_scan, 10)
        self.create_subscription(Image, "/camera/image_raw", self.handle_camera_image, 10)
        self.create_subscription(String, "/simulator/status", self.handle_simulator_status, 10)
        self.app_state.set_node_state("cockpit", True)

    def publish_cmd_vel(self, linear_speed_mps, angular_speed_radps):
        message = Twist()
        message.linear.x = float(linear_speed_mps)
        message.angular.z = float(angular_speed_radps)
        self.cmd_vel_pub.publish(message)
        self.app_state.last_cmd_linear_mps = message.linear.x
        self.app_state.last_cmd_angular_radps = message.angular.z
        self.app_state.mark_topic_update("/cmd_vel")

    def publish_camera_enabled(self, is_enabled):
        self.app_state.camera_enabled = bool(is_enabled)
        self.camera_enabled_pub.publish(Bool(data=bool(is_enabled)))
        self.app_state.mark_topic_update("/simulator/camera_enabled")

    def handle_battery(self, message):
        self.app_state.battery_percent = message.percentage * 100.0
        self.app_state.battery_voltage = message.voltage
        self.app_state.mark_topic_update("/battery_state")

    def handle_odom(self, message):
        self.app_state.odom_x_m = message.pose.pose.position.x
        self.app_state.odom_y_m = message.pose.pose.position.y
        self.app_state.linear_speed_mps = message.twist.twist.linear.x
        self.app_state.angular_speed_radps = message.twist.twist.angular.z
        self.app_state.mark_topic_update("/odom", 50.0)

    def handle_imu(self, message):
        self.app_state.mark_topic_update("/imu/data", 100.0)

    def handle_scan(self, message):
        rate_hz = None
        if message.scan_time > 0.0:
            rate_hz = 1.0 / message.scan_time
        self.app_state.mark_topic_update("/scan", rate_hz)

    def handle_camera_image(self, message):
        self.app_state.mark_topic_update("/camera/image_raw", 30.0)
        if self.image_callback is not None:
            self.image_callback(message)

    def handle_simulator_status(self, message):
        self.app_state.mark_topic_update("/simulator/status", 20.0)
        self.app_state.set_node_state("simulator", True)


class RosSpinThread(threading.Thread):
    def __init__(self, node):
        super().__init__(daemon=True)
        self.node = node

    def run(self):
        rclpy.spin(self.node)