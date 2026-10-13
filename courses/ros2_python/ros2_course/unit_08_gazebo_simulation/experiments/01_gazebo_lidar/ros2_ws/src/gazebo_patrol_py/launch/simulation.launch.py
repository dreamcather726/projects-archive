"""启动 Gazebo Harmonic、话题桥接和可选 Python 控制器。"""

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    package_share = get_package_share_directory('gazebo_patrol_py')
    world = os.path.join(package_share, 'worlds', 'patrol_world.sdf')
    gz_launch = os.path.join(
        get_package_share_directory('ros_gz_sim'), 'launch', 'gz_sim.launch.py'
    )

    return LaunchDescription([
        DeclareLaunchArgument('start_controller', default_value='true'),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(gz_launch),
            launch_arguments={'gz_args': f'-r {world}'}.items(),
        ),
        Node(
            package='ros_gz_bridge',
            executable='parameter_bridge',
            arguments=[
                '/clock@rosgraph_msgs/msg/Clock[gz.msgs.Clock',
                '/scan@sensor_msgs/msg/LaserScan[gz.msgs.LaserScan',
                '/odom@nav_msgs/msg/Odometry[gz.msgs.Odometry',
                '/cmd_vel@geometry_msgs/msg/Twist]gz.msgs.Twist',
            ],
            output='screen',
        ),
        Node(
            package='gazebo_patrol_py',
            executable='reactive_controller',
            parameters=[{'use_sim_time': True}],
            condition=IfCondition(LaunchConfiguration('start_controller')),
            output='screen',
        ),
    ])
