import sys

import rclpy

try:
    from PySide6.QtWidgets import QApplication
except ImportError:
    from PyQt5.QtWidgets import QApplication

from ros_car_cockpit.app_state import AppState
from ros_car_cockpit.ros_client import CockpitRosNode, RosSpinThread
from ros_car_cockpit.ui.main_window import MainWindow


def main(args=None):
    rclpy.init(args=args)
    app = QApplication(sys.argv)
    app_state = AppState()

    window_holder = {}

    def handle_camera_image(message):
        window = window_holder.get("window")
        if window is not None:
            window.show_camera_image(message)

    ros_node = CockpitRosNode(app_state, image_callback=handle_camera_image)
    spin_thread = RosSpinThread(ros_node)
    spin_thread.start()

    window = MainWindow(app_state, ros_node)
    window_holder["window"] = window
    window.show()

    exit_code = app.exec()
    ros_node.destroy_node()
    rclpy.shutdown()
    sys.exit(exit_code)


if __name__ == "__main__":
    main()
