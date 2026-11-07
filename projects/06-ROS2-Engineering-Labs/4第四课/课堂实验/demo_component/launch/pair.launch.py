from launch import LaunchDescription
from launch_ros.actions import ComposableNodeContainer
from launch_ros.descriptions import ComposableNode


def generate_launch_description():
    return LaunchDescription([
        ComposableNodeContainer(
            name='lesson4_container', namespace='',
            package='rclcpp_components', executable='component_container',
            output='screen', composable_node_descriptions=[
                ComposableNode(package='demo_component',
                               plugin='demo_component::Talker',
                               extra_arguments=[{'use_intra_process_comms': True}]),
                ComposableNode(package='demo_component',
                               plugin='demo_component::Listener',
                               extra_arguments=[{'use_intra_process_comms': True}]),
            ])
    ])
