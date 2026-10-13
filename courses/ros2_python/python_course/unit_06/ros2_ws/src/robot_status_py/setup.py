from setuptools import find_packages, setup


package_name = "robot_status_py"

# setup() 描述 Python 包如何安装到 ROS 2 工作空间。
setup(
    name=package_name,
    version="0.0.1",
    packages=find_packages(exclude=["test"]),
    data_files=[
        # 注册功能包，并把 package.xml 安装到 share 目录。
        (
            "share/ament_index/resource_index/packages",
            ["resource/" + package_name],
        ),
        ("share/" + package_name, ["package.xml"]),
    ],
    install_requires=["setuptools"],
    zip_safe=True,
    maintainer="teacher",
    maintainer_email="teacher@example.com",
    description="Python examples for robot status nodes.",
    license="Apache-2.0",
    entry_points={
        # 左侧是 ros2 run 使用的名称，右侧指向模块中的 main()。
        "console_scripts": [
            "timer_logger = robot_status_py.timer_logger:main",
            "status_publisher = robot_status_py.status_publisher:main",
            "status_subscriber = robot_status_py.status_subscriber:main",
        ],
    },
)
