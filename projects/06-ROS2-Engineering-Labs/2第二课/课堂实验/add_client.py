import rclpy
from rclpy.node import Node
from example_interfaces.srv import AddTwoInts


def main():
    rclpy.init()
    node = Node('add_client')
    client = node.create_client(AddTwoInts, '/add_two_ints')
    future = None

    try:
        if not client.wait_for_service(timeout_sec=3.0):
            node.get_logger().error('服务端尚未可用')
            return

        request = AddTwoInts.Request()
        request.a = 2
        request.b = 3
        future = client.call_async(request)
        node.get_logger().info('请求已发出，当前持有的是 Future')

        rclpy.spin_until_future_complete(node, future, timeout_sec=3.0)
        if not future.done():
            client.remove_pending_request(future)
            node.get_logger().error('等待响应超时，服务端可能仍在执行')
            return

        response = future.result()
        node.get_logger().info(f'2 + 3 = {response.sum}')
    except Exception as exc:
        node.get_logger().error(f'调用异常：{exc}')
    finally:
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == '__main__':
    main()
