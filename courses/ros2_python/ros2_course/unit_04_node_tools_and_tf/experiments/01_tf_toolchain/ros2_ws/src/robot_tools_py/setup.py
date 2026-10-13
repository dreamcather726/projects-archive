import os
from glob import glob

from setuptools import find_packages, setup

package_name = 'robot_tools_py'

setup(
    name=package_name,
    version='0.1.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages', ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        (os.path.join('share', package_name, 'launch'), glob('launch/*')),
        (os.path.join('share', package_name, 'config'), glob('config/*')),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='ROS 2 Python Teacher',
    maintainer_email='teacher@example.com',
    description='第 4 章参数、Launch 与 TF2 教学实验',
    license='Apache-2.0',
    entry_points={'console_scripts': [
        'static_sensor_tf = robot_tools_py.static_sensor_tf:main',
        'dynamic_base_tf = robot_tools_py.dynamic_base_tf:main',
        'tf_monitor = robot_tools_py.tf_monitor:main',
    ]},
)
