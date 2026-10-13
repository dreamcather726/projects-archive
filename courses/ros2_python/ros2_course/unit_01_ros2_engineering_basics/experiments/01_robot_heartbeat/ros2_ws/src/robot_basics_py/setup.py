from setuptools import find_packages, setup


package_name = 'robot_basics_py'


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
    description='第 1 章机器人心跳节点教学实验',
    license='Apache-2.0',
    entry_points={
        'console_scripts': [
            # 左边是 ros2 run 使用的入口名；右边是 Python 模块和 main 函数。
            'robot_heartbeat = robot_basics_py.heartbeat_node:main',
        ],
    },
)
