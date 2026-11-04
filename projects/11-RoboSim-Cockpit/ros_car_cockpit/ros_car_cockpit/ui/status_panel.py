try:
    from PySide6.QtWidgets import QFrame, QHBoxLayout, QLabel, QVBoxLayout
except ImportError:
    from PyQt5.QtWidgets import QFrame, QHBoxLayout, QLabel, QVBoxLayout

from ros_car_cockpit.ui.node_status_panel import StatusLight


class StatusPanel(QFrame):
    def __init__(self, app_state):
        super().__init__()
        self.app_state = app_state
        self.title_lights = {}
        self.battery_label = self.create_value_label()
        self.speed_label = self.create_value_label()
        self.odom_label = self.create_value_label()
        self.imu_label = self.create_value_label()
        self.scan_label = self.create_value_label()
        self.serial_label = self.create_value_label()

        layout = QVBoxLayout(self)
        layout.setContentsMargins(12, 10, 12, 12)
        layout.setSpacing(8)
        self.add_card(layout, "电池", "/battery_state", self.battery_label)
        self.add_card(layout, "速度", "/cmd_vel", self.speed_label)
        self.add_card(layout, "里程计", "/odom", self.odom_label)
        self.add_card(layout, "IMU", "/imu/data", self.imu_label)
        self.add_card(layout, "雷达状态", "/scan", self.scan_label)
        self.add_card(layout, "串口状态", "/simulator/status", self.serial_label)
        layout.addStretch(1)
        self.refresh()

    def create_value_label(self):
        label = QLabel()
        label.setObjectName("cardValue")
        label.setMinimumHeight(48)
        return label

    def add_card(self, layout, title, topic_name, value_label):
        title_row = QFrame()
        title_row.setObjectName("cardTitleRow")
        row_layout = QHBoxLayout(title_row)
        row_layout.setContentsMargins(0, 0, 0, 0)
        row_layout.setSpacing(8)
        light = StatusLight()
        title_label = QLabel(title)
        title_label.setObjectName("cardTitle")
        row_layout.addWidget(light)
        row_layout.addWidget(title_label)
        row_layout.addStretch(1)
        self.title_lights[topic_name] = light
        layout.addWidget(title_row)
        layout.addWidget(value_label)

    def refresh(self):
        self.battery_label.setText(
            f"{self.app_state.battery_percent:.0f}%    {self.app_state.battery_voltage:.1f} V"
        )
        self.speed_label.setText(
            f"线速度 {self.app_state.linear_speed_mps:.2f} m/s\n"
            f"角速度 {self.app_state.angular_speed_radps:.2f} rad/s"
        )
        self.odom_label.setText(
            f"X {self.app_state.odom_x_m:.2f} m\nY {self.app_state.odom_y_m:.2f} m"
        )
        self.imu_label.setText("加速度 Z 9.81 m/s²\n姿态 yaw 0.00 rad")
        scan_rate = self.app_state.topic_rates_hz.get("/scan", 0.0)
        self.scan_label.setText(f"/scan {scan_rate:.1f} Hz\n点数 360")
        self.serial_label.setText("设备 /dev/ttyUSB0\n波特率 115200")
        for topic_name, light in self.title_lights.items():
            light.set_state(self.app_state.is_topic_active(topic_name))