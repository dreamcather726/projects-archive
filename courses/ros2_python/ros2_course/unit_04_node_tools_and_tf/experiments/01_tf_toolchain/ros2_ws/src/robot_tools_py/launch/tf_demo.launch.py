"""一次启动静态 TF、动态 TF 和监听节点。"""

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    package_share = get_package_share_directory('robot_tools_py')
    config_file = os.path.join(package_share, 'config', 'sensor_tf.yaml')

    return LaunchDescription([
        Node(
            package='robot_tools_py',
            executable='static_sensor_tf',
            name='static_sensor_tf',
            parameters=[config_file],
            output='screen',
        ),
        Node(
            package='robot_tools_py',
            executable='dynamic_base_tf',
            name='dynamic_base_tf',
            output='screen',
        ),
        Node(
            package='robot_tools_py',
            executable='tf_monitor',
            name='tf_monitor',
            output='screen',
        ),
    ])
