from pathlib import Path

from launch import LaunchDescription
from launch.actions import ExecuteProcess


def generate_launch_description():
    experiment = Path(__file__).resolve().parent
    script = experiment / 'parameter_demo.py'
    config = experiment / 'config/parameter_demo.yaml'
    for path in (script, config):
        if not path.is_file():
            raise RuntimeError(
                f'缺少配套文件：{path}。请检查第三课/课堂实验中的源码与 config 目录。')
    return LaunchDescription([
        ExecuteProcess(
            cmd=[
                'python3', str(script),
                '--ros-args',
                '--params-file', str(config),
                '-p', 'controller.max_speed:=0.7'],
            output='screen')
    ])
