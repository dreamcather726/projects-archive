import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import LifecycleNode


def generate_launch_description():
    config = os.path.join(
        get_package_share_directory('managed_monitor_py'),
        'config',
        'monitor.yaml',
    )
    return LaunchDescription([
        # 不自动转换状态，课堂上由学生用 ros2 lifecycle 手动观察。
        LifecycleNode(
            package='managed_monitor_py',
            executable='managed_monitor',
            name='managed_monitor',
            parameters=[config],
            output='screen',
        )
    ])
