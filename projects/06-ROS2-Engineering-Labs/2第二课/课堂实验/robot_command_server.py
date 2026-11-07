import rclpy
from rclpy.node import Node
from robot_interfaces.srv import RobotCommand


class CommandServer(Node):
    def __init__(self):
        super().__init__('robot_command_server')
        self.tasks = {}
        self.service = self.create_service(
            RobotCommand, '/robot_command', self.handle_request
        )

    def handle_request(self, request, response):
        if request.task_id < 0:
            response.success = False
            response.message = 'task_id 必须 >= 0'
        elif request.command not in ('start', 'stop', 'pause'):
            response.success = False
            response.message = 'command 只允许 start / stop / pause'
        else:
            # 软件状态模拟：同一任务重复设为同一状态，效果不变
            self.tasks[request.task_id] = request.command
            response.success = True
            response.message = (
                f'任务 {request.task_id} 状态设为 {request.command}'
            )
        self.get_logger().info(response.message)
        return response


def main():
    rclpy.init()
    node = CommandServer()
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
