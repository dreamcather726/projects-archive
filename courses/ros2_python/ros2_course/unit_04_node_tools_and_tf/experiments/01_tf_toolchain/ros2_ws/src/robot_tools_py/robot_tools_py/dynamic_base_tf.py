"""模拟机器人运动并发布 odom 到 base_link 的动态变换。"""

import math

import rclpy
from geometry_msgs.msg import TransformStamped
from rclpy.node import Node
from tf2_ros import TransformBroadcaster


class DynamicBaseTransform(Node):
    def __init__(self) -> None:
        super().__init__('dynamic_base_tf')
        self._angle = 0.0
        self._broadcaster = TransformBroadcaster(self)
        self._timer = self.create_timer(0.1, self._publish_transform)

    def _publish_transform(self) -> None:
        self._angle += 0.03
        transform = TransformStamped()
        # 动态变换必须使用当前时间，TF 缓存才能按时间查询。
        transform.header.stamp = self.get_clock().now().to_msg()
        transform.header.frame_id = 'odom'
        transform.child_frame_id = 'base_link'
        transform.transform.translation.x = math.cos(self._angle)
        transform.transform.translation.y = math.sin(self._angle)
        transform.transform.rotation.z = math.sin((self._angle + 1.57) / 2.0)
        transform.transform.rotation.w = math.cos((self._angle + 1.57) / 2.0)
        self._broadcaster.sendTransform(transform)


def main(args=None) -> None:
    rclpy.init(args=args)
    node = DynamicBaseTransform()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.try_shutdown()


if __name__ == '__main__':
    main()
