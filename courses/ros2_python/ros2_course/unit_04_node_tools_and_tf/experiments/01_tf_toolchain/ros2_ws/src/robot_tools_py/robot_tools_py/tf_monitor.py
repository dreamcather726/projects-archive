"""查询激光雷达相对底盘的变换并处理查询失败。"""

import rclpy
from rclpy.duration import Duration
from rclpy.node import Node
from rclpy.time import Time
from tf2_ros import Buffer, TransformException, TransformListener


class TransformMonitor(Node):
    def __init__(self) -> None:
        super().__init__('tf_monitor')
        self._buffer = Buffer()
        # Listener 必须保存在实例属性中，才能持续接收 /tf 和 /tf_static。
        self._listener = TransformListener(self._buffer, self)
        self._timer = self.create_timer(1.0, self._query_transform)

    def _query_transform(self) -> None:
        try:
            # 目标 base_link、源 front_laser：结果描述源坐标系在目标中的位姿。
            transform = self._buffer.lookup_transform(
                'base_link',
                'front_laser',
                Time(),
                timeout=Duration(seconds=0.2),
            )
        except TransformException as error:
            self.get_logger().warning(f'TF 查询失败：{error}')
            return

        translation = transform.transform.translation
        self.get_logger().info(
            'front_laser 在 base_link 中的位置：'
            f'x={translation.x:.2f}, y={translation.y:.2f}, z={translation.z:.2f}'
        )


def main(args=None) -> None:
    rclpy.init(args=args)
    node = TransformMonitor()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.try_shutdown()


if __name__ == '__main__':
    main()
