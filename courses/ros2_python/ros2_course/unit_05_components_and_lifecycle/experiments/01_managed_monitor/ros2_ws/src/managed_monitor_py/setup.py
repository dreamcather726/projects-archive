import os
from glob import glob

from setuptools import find_packages, setup

package_name = 'managed_monitor_py'

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
    description='第 5 章模块化与生命周期教学实验',
    license='Apache-2.0',
    entry_points={'console_scripts': [
        'managed_monitor = managed_monitor_py.managed_monitor:main',
    ]},
)
