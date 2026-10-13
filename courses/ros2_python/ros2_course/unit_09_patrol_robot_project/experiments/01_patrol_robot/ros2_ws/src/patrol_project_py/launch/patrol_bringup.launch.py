"""复用第 8 章仿真，并启动第 9 章巡检控制器。"""

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node


def generate_launch_description():
    simulation_launch = os.path.join(
        get_package_share_directory('gazebo_patrol_py'),
        'launch',
        'simulation.launch.py',
    )
    config = os.path.join(
        get_package_share_directory('patrol_project_py'),
        'config',
        'patrol.yaml',
    )

    return LaunchDescription([
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(simulation_launch),
            launch_arguments={'start_controller': 'false'}.items(),
        ),
        Node(
            package='patrol_project_py',
            executable='patrol_controller',
            parameters=[config, {'use_sim_time': True}],
            output='screen',
        ),
    ])
