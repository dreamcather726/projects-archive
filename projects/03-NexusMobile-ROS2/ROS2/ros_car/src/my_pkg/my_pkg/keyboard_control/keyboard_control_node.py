"""
keyboard_control_node

作用：
1. 读取键盘 W/A/S/D。
2. 发布 /cmd_vel，控制小车前进、后退、左转、右转。
3. 键盘短时间没有输入时自动发布停止速度。

运行：
ros2 run my_pkg keyboard_control_node

按键：
W: 前进
S: 后退
A: 左转
D: 右转
Space/X: 停止
Ctrl-C: 退出
"""

import sys
import time

import rclpy
from geometry_msgs.msg import Twist
from rclpy.node import Node


DEFAULT_LINEAR_SPEED_M_S = 0.10
DEFAULT_ANGULAR_SPEED_RAD_S = 0.80
DEFAULT_PUBLISH_RATE_HZ = 50.0
DEFAULT_KEY_TIMEOUT_SEC = 0.30
CMD_VEL_TOPIC = "/cmd_vel"


class KeyboardReader:
    """Read keyboard keys without waiting for Enter."""

    def __init__(self):
        self.original_terminal_settings = None
        self.input_stream = sys.stdin
        self.should_close_input_stream = False
        self.is_windows = sys.platform.startswith("win")

    def __enter__(self):
        if self.is_windows:
            return self

        import termios
        import tty

        if not self.input_stream.isatty():
            self.input_stream = open("/dev/tty", "r", buffering=1)
            self.should_close_input_stream = True

        self.original_terminal_settings = termios.tcgetattr(
            self.input_stream,
        )
        tty.setcbreak(self.input_stream.fileno())
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        if self.is_windows:
            return

        try:
            if self.original_terminal_settings is not None:
                import termios

                termios.tcsetattr(
                    self.input_stream,
                    termios.TCSADRAIN,
                    self.original_terminal_settings,        
                )
        finally:
            if self.should_close_input_stream:
                self.input_stream.close()

    def read_keys(self):
        """Return all buffered keys so long key presses do not lag behind."""
        keys = []
        if self.is_windows:
            import msvcrt

            while msvcrt.kbhit():
                keys.append(msvcrt.getwch().lower())
            return keys

        import select

        while True:
            ready_inputs, _, _ = select.select(
                [self.input_stream],
                [],
                [],
                0.0,
            )
            if not ready_inputs:
                break
            keys.append(self.input_stream.read(1).lower())

        return keys

    def read_key(self):
        """Return the latest buffered key for backward compatibility."""
        keys = self.read_keys()
        if not keys:
            return None
        return keys[-1]


class KeyboardControlNode(Node):
    """Publish /cmd_vel from W/A/S/D keyboard input."""

    def __init__(self, keyboard_reader):
        super().__init__("keyboard_control_node")

        self.declare_parameter("linear_speed_m_s", DEFAULT_LINEAR_SPEED_M_S)
        self.declare_parameter(
            "angular_speed_rad_s",
            DEFAULT_ANGULAR_SPEED_RAD_S,
        )
        self.declare_parameter("publish_rate_hz", DEFAULT_PUBLISH_RATE_HZ)
        self.declare_parameter("key_timeout_sec", DEFAULT_KEY_TIMEOUT_SEC)

        publish_rate_hz = self.get_parameter("publish_rate_hz").value
        timer_period_sec = 1.0 / float(publish_rate_hz)

        self.keyboard_reader = keyboard_reader
        self.cmd_vel_pub = self.create_publisher(Twist, CMD_VEL_TOPIC, 10)
        self.current_linear_speed = 0.0
        self.current_angular_speed = 0.0
        self.pressed_keys = set()
        self.last_key_time = 0.0
        self.create_timer(timer_period_sec, self.timer_callback)

        self.get_logger().info(
            "keyboard_control_node initialized: "
            "W/S forward/backward, A/D turn, Space/X stop"
        )

    def timer_callback(self):
        """Read keyboard and publish the latest command velocity."""
        for key in self.keyboard_reader.read_keys():
            self.handle_key(key)

        self.update_speed_from_pressed_keys()
        self.stop_when_key_timeout()
        self.publish_cmd_vel()

    def handle_key(self, key):
        """Track valid keys and let timeout handle key release on terminals."""
        if key not in ("w", "s", "a", "d", " ", "x"):
            return

        self.last_key_time = time.monotonic()
        if key == " " or key == "x":
            self.stop_motion()
            return

        if key == "w":
            self.pressed_keys.discard("s")
        elif key == "s":
            self.pressed_keys.discard("w")
        elif key == "a":
            self.pressed_keys.discard("d")
        else:
            self.pressed_keys.discard("a")

        self.pressed_keys.add(key)

    def update_speed_from_pressed_keys(self):
        """Convert the current key state into linear and angular speed."""
        linear_speed_m_s = self.get_parameter("linear_speed_m_s").value
        angular_speed_rad_s = self.get_parameter(
            "angular_speed_rad_s",
        ).value

        linear_speed = 0.0
        angular_speed = 0.0

        if "w" in self.pressed_keys:
            linear_speed += linear_speed_m_s
        if "s" in self.pressed_keys:
            linear_speed -= linear_speed_m_s
        if "a" in self.pressed_keys:
            angular_speed += angular_speed_rad_s
        if "d" in self.pressed_keys:
            angular_speed -= angular_speed_rad_s

        self.current_linear_speed = linear_speed
        self.current_angular_speed = angular_speed

    def stop_when_key_timeout(self):
        """Stop robot if no valid key has been received recently."""
        key_timeout_sec = self.get_parameter("key_timeout_sec").value
        if time.monotonic() - self.last_key_time <= key_timeout_sec:
            return

        self.stop_motion()

    def stop_motion(self):
        """Clear active key state and publish zero speed on the next tick."""
        self.pressed_keys.clear()
        self.current_linear_speed = 0.0
        self.current_angular_speed = 0.0

    def publish_cmd_vel(self):
        """Publish current command velocity to /cmd_vel."""
        twist_msg = Twist()
        twist_msg.linear.x = float(self.current_linear_speed)
        twist_msg.angular.z = float(self.current_angular_speed)
        self.cmd_vel_pub.publish(twist_msg)


def main(args=None):
    """Run keyboard_control_node."""
    rclpy.init(args=args)

    with KeyboardReader() as keyboard_reader:
        node = KeyboardControlNode(keyboard_reader)
        try:
            rclpy.spin(node)
        finally:
            stop_msg = Twist()
            node.cmd_vel_pub.publish(stop_msg)
            node.destroy_node()
            rclpy.shutdown()


if __name__ == "__main__":
    main()
