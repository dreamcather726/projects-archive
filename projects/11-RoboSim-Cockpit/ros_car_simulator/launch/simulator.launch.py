from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription(
        [
            Node(
                package="ros_car_simulator",
                executable="simulator_node",
                name="ros_car_simulator",
                output="screen",
            )
        ]
    )
