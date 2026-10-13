"""发布带时间和状态语义的模拟传感器数据。"""

import rclpy
from rclpy.node import Node
from rclpy.qos import qos_profile_sensor_data

from sensor_interfaces.msg import SensorStatus


class SensorPublisher(Node):
    """循环发布一组可预测的数据，方便课堂观察每种状态。"""

    def __init__(self) -> None:
        super().__init__('sensor_publisher')

        # 传感器数据通常重视新鲜度。这里显式使用传感器 QoS，
        # 并让订阅端使用同一配置，避免初学阶段出现兼容性干扰。
        self._publisher = self.create_publisher(
            SensorStatus, '/sensor/status', qos_profile_sensor_data
        )
        self._samples = [
            (1.80, 31.0),
            (0.90, 44.0),
            (0.35, 38.0),
            (1.20, 63.0),
        ]
        self._index = 0
        self._timer = self.create_timer(0.5, self._publish_sample)

    def _publish_sample(self) -> None:
        distance, temperature = self._samples[self._index]
        self._index = (self._index + 1) % len(self._samples)

        message = SensorStatus()
        # 使用 ROS 时钟而不是 time.time，切换仿真时间后代码无需改变。
        message.header.stamp = self.get_clock().now().to_msg()
        message.header.frame_id = 'front_laser'
        message.source = 'simulated_front_sensor'
        message.distance_m = distance
        message.temperature_c = temperature

        if distance < 0.50 or temperature > 60.0:
            message.status = SensorStatus.STATUS_DANGER
        elif distance < 1.00 or temperature > 40.0:
            message.status = SensorStatus.STATUS_WARNING
        else:
            message.status = SensorStatus.STATUS_OK

        self._publisher.publish(message)
        self.get_logger().info(
            f'发布：distance={distance:.2f} m, '
            f'temperature={temperature:.1f} C, status={message.status}'
        )


def main(args=None) -> None:
    rclpy.init(args=args)
    node = SensorPublisher()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.try_shutdown()


if __name__ == '__main__':
    main()
