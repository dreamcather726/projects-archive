from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, GroupAction
from launch.conditions import IfCondition, UnlessCondition
from launch.substitutions import Command, FindExecutable, LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    share = FindPackageShare('my_robot_description')
    description = ParameterValue(Command([
        FindExecutable(name='xacro'), ' ',
        PathJoinSubstitution([share, 'urdf', 'my_robot.xacro'])
    ]), value_type=str)
    return LaunchDescription([
        DeclareLaunchArgument('gui', default_value='true'),
        DeclareLaunchArgument('rviz', default_value='true'),
        DeclareLaunchArgument('joint_states', default_value='true'),
        Node(package='robot_state_publisher', executable='robot_state_publisher',
             parameters=[{'robot_description': description}], output='screen'),
        GroupAction(condition=IfCondition(LaunchConfiguration('joint_states')), actions=[
            Node(package='joint_state_publisher_gui', executable='joint_state_publisher_gui',
                 condition=IfCondition(LaunchConfiguration('gui'))),
            Node(package='joint_state_publisher', executable='joint_state_publisher',
                 condition=UnlessCondition(LaunchConfiguration('gui'))),
        ]),
        Node(package='rviz2', executable='rviz2',
             arguments=['-d', PathJoinSubstitution([share, 'rviz', 'display.rviz'])],
             condition=IfCondition(LaunchConfiguration('rviz'))),
    ])
