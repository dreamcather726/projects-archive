from setuptools import find_packages, setup

package_name = 'sensor_monitor_py'

setup(
    name=package_name,
    version='0.1.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages', ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='ROS 2 Python Teacher',
    maintainer_email='teacher@example.com',
    description='第 2 章话题与自定义消息教学实验',
    license='Apache-2.0',
    entry_points={'console_scripts': [
        'sensor_publisher = sensor_monitor_py.sensor_publisher:main',
        'safety_monitor = sensor_monitor_py.safety_monitor:main',
    ]},
)
