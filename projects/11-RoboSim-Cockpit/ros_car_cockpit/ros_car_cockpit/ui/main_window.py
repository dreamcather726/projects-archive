try:
    from PySide6.QtCore import QTimer
    from PySide6.QtWidgets import QHBoxLayout, QLabel, QMainWindow, QVBoxLayout, QWidget
except ImportError:
    from PyQt5.QtCore import QTimer
    from PyQt5.QtWidgets import QHBoxLayout, QLabel, QMainWindow, QVBoxLayout, QWidget

from ros_car_cockpit.ui.control_panel import ControlPanel
from ros_car_cockpit.ui.node_status_panel import NodeStatusPanel
from ros_car_cockpit.ui.rviz_panel import RvizPanel
from ros_car_cockpit.ui.status_panel import StatusPanel
from ros_car_cockpit.ui.style import DARK_STYLE


class MainWindow(QMainWindow):
    def __init__(self, app_state, ros_node):
        super().__init__()
        self.app_state = app_state
        self.ros_node = ros_node
        self.setWindowTitle("ROS2 控制 cockpit")
        self.resize(1850, 980)
        self.setStyleSheet(DARK_STYLE)

        root_widget = QWidget()
        root_layout = QVBoxLayout(root_widget)
        root_layout.setContentsMargins(10, 8, 10, 10)
        root_layout.setSpacing(8)

        title_label = QLabel("ROS2 控制 cockpit")
        title_label.setObjectName("appTitle")
        root_layout.addWidget(title_label)

        content_layout = QHBoxLayout()
        content_layout.setSpacing(8)
        self.status_panel = StatusPanel(app_state)
        self.rviz_panel = RvizPanel(app_state)
        self.control_panel = ControlPanel(ros_node)
        content_layout.addWidget(self.status_panel, 2)
        content_layout.addWidget(self.rviz_panel, 7)
        content_layout.addWidget(self.control_panel, 3)
        root_layout.addLayout(content_layout, 1)

        self.node_status_panel = NodeStatusPanel(app_state)
        root_layout.addWidget(self.node_status_panel)
        self.setCentralWidget(root_widget)

        self.refresh_timer = QTimer(self)
        self.refresh_timer.timeout.connect(self.refresh_status)
        self.refresh_timer.start(500)

    def refresh_status(self):
        self.status_panel.refresh()
        self.node_status_panel.refresh()

    def show_camera_image(self, message):
        return

    def closeEvent(self, event):
        self.rviz_panel.stop_rviz()
        super().closeEvent(event)