from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription(
        [
            Node(
                package="ros_car_cockpit",
                executable="cockpit_app",
                name="ros_car_cockpit",
                output="screen",
            )
        ]
    )
