"""从参数读取安装位姿并发布一次静态变换。"""

import math

import rclpy
from geometry_msgs.msg import TransformStamped
from rclpy.node import Node
from tf2_ros import StaticTransformBroadcaster


class StaticSensorTransform(Node):
    def __init__(self) -> None:
        super().__init__('static_sensor_tf')
        for name, default in (
            ('parent_frame', 'base_link'),
            ('child_frame', 'front_laser'),
            ('x', 0.25), ('y', 0.0), ('z', 0.18), ('yaw', 0.0),
        ):
            self.declare_parameter(name, default)

        transform = TransformStamped()
        transform.header.stamp = self.get_clock().now().to_msg()
        transform.header.frame_id = self.get_parameter('parent_frame').value
        transform.child_frame_id = self.get_parameter('child_frame').value
        transform.transform.translation.x = self.get_parameter('x').value
        transform.transform.translation.y = self.get_parameter('y').value
        transform.transform.translation.z = self.get_parameter('z').value

        # 本实验只绕 Z 轴旋转，可以直接由 yaw 计算四元数。
        yaw = self.get_parameter('yaw').value
        transform.transform.rotation.z = math.sin(yaw / 2.0)
        transform.transform.rotation.w = math.cos(yaw / 2.0)

        # 静态发布器会使用适合固定关系的 QoS，后加入节点也能收到变换。
        self._broadcaster = StaticTransformBroadcaster(self)
        self._broadcaster.sendTransform(transform)
        self.get_logger().info(
            f'已发布静态变换：{transform.header.frame_id} -> '
            f'{transform.child_frame_id}'
        )


def main(args=None) -> None:
    rclpy.init(args=args)
    node = StaticSensorTransform()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.try_shutdown()


if __name__ == '__main__':
    main()
