"""加载 Xacro，发布机器人状态并启动 RViz2。"""

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.substitutions import Command
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue


def generate_launch_description():
    package_share = get_package_share_directory('robot_description_py')
    model = os.path.join(package_share, 'urdf', 'my_robot.urdf.xacro')

    # Command 在启动时执行 xacro；ParameterValue 明确结果是字符串参数。
    robot_description = ParameterValue(
        Command(['xacro ', model]), value_type=str
    )

    return LaunchDescription([
        Node(
            package='robot_state_publisher',
            executable='robot_state_publisher',
            parameters=[{'robot_description': robot_description}],
            output='screen',
        ),
        Node(
            package='joint_state_publisher_gui',
            executable='joint_state_publisher_gui',
            output='screen',
        ),
        Node(package='rviz2', executable='rviz2', output='screen'),
    ])
