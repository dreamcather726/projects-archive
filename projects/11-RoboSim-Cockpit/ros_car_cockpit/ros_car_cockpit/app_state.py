import time
from dataclasses import dataclass, field


@dataclass
class AppState:
    battery_percent: float = 0.0
    battery_voltage: float = 0.0
    linear_speed_mps: float = 0.0
    angular_speed_radps: float = 0.0
    odom_x_m: float = 0.0
    odom_y_m: float = 0.0
    camera_enabled: bool = False
    last_cmd_linear_mps: float = 0.0
    last_cmd_angular_radps: float = 0.0
    topic_rates_hz: dict = field(default_factory=dict)
    topic_update_times: dict = field(default_factory=dict)
    manual_node_states: dict = field(default_factory=dict)

    def mark_topic_update(self, topic_name, rate_hz=None):
        self.topic_update_times[topic_name] = time.monotonic()
        if rate_hz is not None:
            self.topic_rates_hz[topic_name] = rate_hz

    def set_node_state(self, node_name, is_active):
        self.manual_node_states[node_name] = bool(is_active)

    def is_topic_active(self, topic_name, timeout_seconds=2.5):
        update_time = self.topic_update_times.get(topic_name)
        if update_time is None:
            return False
        return time.monotonic() - update_time <= timeout_seconds

    def is_node_active(self, node_name):
        return self.manual_node_states.get(node_name, False)