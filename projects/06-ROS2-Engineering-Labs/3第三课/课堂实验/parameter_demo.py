import math

import rclpy
from rclpy.node import Node
from rclpy.parameter import Parameter
from rcl_interfaces.msg import (
    FloatingPointRange, ParameterDescriptor, SetParametersResult)


class ParameterDemo(Node):
    def __init__(self):
        super().__init__('parameter_demo')
        self.add_on_set_parameters_callback(self.validate_parameters)
        self.declare_parameter(
            'controller.max_speed', 0.5,
            ParameterDescriptor(
                description='速度上限，单位 m/s，范围 0.0～2.0',
                floating_point_range=[FloatingPointRange(
                    from_value=0.0, to_value=2.0, step=0.0)]))
        self.declare_parameter('enabled', True)
        self.declare_parameter(
            'robot_id', 'demo_01',
            ParameterDescriptor(description='课堂机器人编号', read_only=True))
        self.last_config = None
        self.timer = self.create_timer(1.0, self.observe_config)

    def validate_parameters(self, parameters):
        for parameter in parameters:
            if parameter.name == 'controller.max_speed':
                value = parameter.value
                if (parameter.type_ != Parameter.Type.DOUBLE or
                        not math.isfinite(value) or not 0.0 <= value <= 2.0):
                    return SetParametersResult(
                        successful=False,
                        reason='max_speed 必须是 0.0～2.0 的有限浮点数')
        return SetParametersResult(successful=True)

    def observe_config(self):
        speed = self.get_parameter('controller.max_speed').value
        enabled = self.get_parameter('enabled').value
        robot_id = self.get_parameter('robot_id').value
        config = (speed, enabled, robot_id)
        if config != self.last_config:
            self.get_logger().info(
                f'config robot_id={robot_id} enabled={enabled} '
                f'max_speed={speed:.2f} m/s')
            self.last_config = config


def main(args=None):
    rclpy.init(args=args)
    node = None
    try:
        node = ParameterDemo()
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        if node is not None:
            node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == '__main__':
    main()
