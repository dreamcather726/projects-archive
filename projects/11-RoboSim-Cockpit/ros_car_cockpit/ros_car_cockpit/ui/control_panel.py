try:
    from PySide6.QtWidgets import QFrame, QGridLayout, QLabel, QPushButton, QVBoxLayout
except ImportError:
    from PyQt5.QtWidgets import QFrame, QGridLayout, QLabel, QPushButton, QVBoxLayout


class ControlPanel(QFrame):
    def __init__(self, ros_node):
        super().__init__()
        self.ros_node = ros_node
        layout = QVBoxLayout(self)
        layout.setContentsMargins(12, 10, 12, 12)
        layout.setSpacing(10)

        title_label = QLabel("控制面板")
        title_label.setObjectName("sectionTitle")
        layout.addWidget(title_label)

        mode_label = QLabel("模式    手动    建图    导航    机械臂")
        mode_label.setObjectName("cardValue")
        layout.addWidget(mode_label)

        manual_title = QLabel("手动控制")
        manual_title.setObjectName("cardTitle")
        layout.addWidget(manual_title)

        grid = QGridLayout()
        grid.setSpacing(8)
        self.forward_button = QPushButton("前进")
        self.backward_button = QPushButton("后退")
        self.left_button = QPushButton("左转")
        self.right_button = QPushButton("右转")
        self.stop_button = QPushButton("急停")
        self.stop_button.setObjectName("stopButton")
        grid.addWidget(self.forward_button, 0, 1)
        grid.addWidget(self.left_button, 1, 0)
        grid.addWidget(self.stop_button, 1, 1)
        grid.addWidget(self.right_button, 1, 2)
        grid.addWidget(self.backward_button, 2, 1)
        layout.addLayout(grid)

        speed_label = QLabel("速度限制\n线速度 0.80 m/s\n角速度 1.50 rad/s")
        speed_label.setObjectName("cardValue")
        layout.addWidget(speed_label)

        pid_label = QLabel("PID 参数\n线速度  Kp 1.20   Ki 0.00   Kd 0.10\n角速度  Kp 2.00   Ki 0.00   Kd 0.20")
        pid_label.setObjectName("cardValue")
        layout.addWidget(pid_label)

        arm_label = QLabel("机械臂舵机目标\n肩部 45°   肘部 30°\n腕部 -10°  夹爪 80°")
        arm_label.setObjectName("cardValue")
        layout.addWidget(arm_label)

        self.camera_button = QPushButton("摄像头预留：开启采集")
        self.camera_button.setObjectName("cameraButton")
        layout.addWidget(self.camera_button)
        layout.addStretch(1)

        self.forward_button.clicked.connect(lambda: self.ros_node.publish_cmd_vel(0.3, 0.0))
        self.backward_button.clicked.connect(lambda: self.ros_node.publish_cmd_vel(-0.3, 0.0))
        self.left_button.clicked.connect(lambda: self.ros_node.publish_cmd_vel(0.0, 0.8))
        self.right_button.clicked.connect(lambda: self.ros_node.publish_cmd_vel(0.0, -0.8))
        self.stop_button.clicked.connect(lambda: self.ros_node.publish_cmd_vel(0.0, 0.0))
        self.camera_button.clicked.connect(self.toggle_camera)
        self.is_camera_enabled = False

    def toggle_camera(self):
        self.is_camera_enabled = not self.is_camera_enabled
        self.ros_node.publish_camera_enabled(self.is_camera_enabled)
        self.camera_button.setText(
            "摄像头预留：停止采集" if self.is_camera_enabled else "摄像头预留：开启采集"
        )