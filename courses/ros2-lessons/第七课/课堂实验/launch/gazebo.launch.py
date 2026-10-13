from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import Command, FindExecutable, LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    share = FindPackageShare('my_robot_description')
    description = ParameterValue(Command([
        FindExecutable(name='xacro'), ' ',
        PathJoinSubstitution([share, 'urdf', 'my_robot_sim.xacro']),
        ' wheel_radius:=0.032 wheel_sep:=0.20 base_length:=0.24 base_width:=0.16'
    ]), value_type=str)
    gazebo_launch = PathJoinSubstitution([FindPackageShare('gazebo_ros'), 'launch', 'gazebo.launch.py'])
    return LaunchDescription([
        DeclareLaunchArgument('gui', default_value='false'),
        IncludeLaunchDescription(PythonLaunchDescriptionSource(gazebo_launch),
            launch_arguments={'world': PathJoinSubstitution([share, 'worlds', 'my_world.world']),
                              'gui': LaunchConfiguration('gui')}.items()),
        Node(package='robot_state_publisher', executable='robot_state_publisher',
             parameters=[{'robot_description': description, 'use_sim_time': True}], output='screen'),
        Node(package='gazebo_ros', executable='spawn_entity.py',
             arguments=['-topic', 'robot_description', '-entity', 'my_robot', '-z', '0.05'],
             output='screen'),
    ])
