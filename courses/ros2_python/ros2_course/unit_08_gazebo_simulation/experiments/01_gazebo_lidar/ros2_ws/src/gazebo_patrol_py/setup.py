import os
from glob import glob

from setuptools import find_packages, setup

package_name = 'gazebo_patrol_py'

setup(
    name=package_name,
    version='0.1.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages', ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        (os.path.join('share', package_name, 'launch'), glob('launch/*')),
        (os.path.join('share', package_name, 'worlds'), glob('worlds/*')),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='ROS 2 Python Teacher',
    maintainer_email='teacher@example.com',
    description='第 8 章 Gazebo 与 Python 避障教学实验',
    license='Apache-2.0',
    entry_points={'console_scripts': [
        'reactive_controller = gazebo_patrol_py.reactive_controller:main',
    ]},
)
