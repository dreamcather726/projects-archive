import os
from glob import glob

from setuptools import find_packages, setup

package_name = 'patrol_project_py'

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
    description='第 9 章自主巡检移动机器人综合项目',
    license='Apache-2.0',
    entry_points={'console_scripts': [
        'patrol_controller = patrol_project_py.patrol_controller:main',
        'patrol_client = patrol_project_py.patrol_client:main',
    ]},
)
