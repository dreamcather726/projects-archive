from setuptools import find_packages, setup

package_name = 'patrol_communication_py'

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
    description='第 3 章服务与动作教学实验',
    license='Apache-2.0',
    entry_points={'console_scripts': [
        'patrol_server = patrol_communication_py.patrol_server:main',
        'patrol_client = patrol_communication_py.patrol_client:main',
    ]},
)
