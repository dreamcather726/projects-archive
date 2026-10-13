from setuptools import find_packages, setup

package_name = 'executor_qos_py'

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
    description='第 6 章执行器、回调组与 QoS 教学实验',
    license='Apache-2.0',
    entry_points={'console_scripts': [
        'sensor_load = executor_qos_py.sensor_load:main',
        'single_thread_demo = executor_qos_py.executor_demo:main_single',
        'multi_thread_demo = executor_qos_py.executor_demo:main_multi',
    ]},
)
