try:
    from PySide6.QtWidgets import QFrame, QGridLayout, QHBoxLayout, QLabel, QVBoxLayout
except ImportError:
    from PyQt5.QtWidgets import QFrame, QGridLayout, QHBoxLayout, QLabel, QVBoxLayout


class StatusLight(QLabel):
    def __init__(self):
        super().__init__()
        self.setFixedSize(14, 14)
        self.set_state(False)

    def set_state(self, is_active):
        color = "#4ade80" if is_active else "#334155"
        glow = "#86efac" if is_active else "#475569"
        self.setStyleSheet(
            f"background-color: {color}; border: 2px solid {glow}; border-radius: 7px;"
        )


class NodeStatusPanel(QFrame):
    def __init__(self, app_state):
        super().__init__()
        self.app_state = app_state
        self.status_items = []

        layout = QVBoxLayout(self)
        layout.setContentsMargins(14, 10, 14, 10)
        layout.setSpacing(8)

        title = QLabel("ROS Topic 状态")
        title.setObjectName("sectionTitle")
        layout.addWidget(title)

        grid = QGridLayout()
        grid.setHorizontalSpacing(26)
        grid.setVerticalSpacing(10)
        layout.addLayout(grid)

        items = [
            ("/cmd_vel", "topic"),
            ("/odom", "topic"),
            ("/imu/data", "topic"),
            ("/scan", "topic"),
            ("/battery_state", "topic"),
            ("/camera/image_raw", "topic"),
            ("/simulator/status", "topic"),
            ("RViz2", "rviz"),
            ("cockpit", "node"),
        ]
        for index, (name, kind) in enumerate(items):
            item_widget, light, value_label = self.create_status_item(name)
            self.status_items.append((name, kind, light, value_label))
            grid.addWidget(item_widget, index // 5, index % 5)
        self.refresh()

    def create_status_item(self, name):
        item = QFrame()
        item.setObjectName("topicItem")
        layout = QHBoxLayout(item)
        layout.setContentsMargins(8, 5, 8, 5)
        layout.setSpacing(8)
        light = StatusLight()
        name_label = QLabel(name)
        name_label.setObjectName("topicName")
        value_label = QLabel("--")
        value_label.setObjectName("topicRate")
        layout.addWidget(light)
        layout.addWidget(name_label)
        layout.addWidget(value_label)
        return item, light, value_label

    def refresh(self):
        for name, kind, light, value_label in self.status_items:
            if kind == "topic":
                is_active = self.app_state.is_topic_active(name)
                rate_hz = self.app_state.topic_rates_hz.get(name)
                value_label.setText(f"{rate_hz:.1f} Hz" if rate_hz else "online" if is_active else "--")
            elif kind == "rviz":
                is_active = self.app_state.is_node_active("rviz2")
                value_label.setText("running" if is_active else "--")
            else:
                is_active = self.app_state.is_node_active(name)
                value_label.setText("online" if is_active else "--")
            light.set_state(is_active)