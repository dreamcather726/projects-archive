import rclpy
from geometry_msgs.msg import PointStamped
from rclpy.duration import Duration
from rclpy.node import Node
from rclpy.time import Time
from tf2_geometry_msgs import do_transform_point
from tf2_ros import Buffer, TransformException, TransformListener


class TFPointDemo(Node):
    def __init__(self):
        super().__init__('tf_point_demo')
        self.buffer = Buffer(cache_time=Duration(seconds=10.0), node=self)
        self.listener = TransformListener(self.buffer, self)
        self.timer = self.create_timer(1.0, self.convert_point)

    def convert_point(self):
        point = PointStamped()
        point.header.frame_id = 'laser_frame'
        point.header.stamp = self.get_clock().now().to_msg()
        point.point.x = 1.0
        point.point.y = 0.0
        point.point.z = 0.0
        try:
            transform = self.buffer.lookup_transform(
                'base_link', 'laser_frame',
                Time.from_msg(point.header.stamp))
            converted = do_transform_point(point, transform)
            # 本次只改变参考系，保留模拟采样时间。
            converted.header.stamp = point.header.stamp
            self.get_logger().info(
                f'frame={converted.header.frame_id} '
                f'point=({converted.point.x:.2f}, '
                f'{converted.point.y:.2f}, {converted.point.z:.2f})')
        except TransformException as error:
            self.get_logger().warning(
                f'等待或检查 TF: {error}', throttle_duration_sec=2.0)


def main(args=None):
    rclpy.init(args=args)
    node = TFPointDemo()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == '__main__':
    main()
