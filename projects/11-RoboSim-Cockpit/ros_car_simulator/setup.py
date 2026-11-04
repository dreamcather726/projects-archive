from setuptools import setup

package_name = "ros_car_simulator"

setup(
    name=package_name,
    version="0.0.1",
    packages=[package_name],
    data_files=[
        ("share/ament_index/resource_index/packages", ["resource/" + package_name]),
        ("share/" + package_name, ["package.xml"]),
        ("share/" + package_name + "/launch", ["launch/simulator.launch.py"]),
    ],
    install_requires=["setuptools"],
    zip_safe=True,
    maintainer="ros-car",
    maintainer_email="user@example.com",
    description="Simulated ROS2 data publishers for cockpit GUI development.",
    license="MIT",
    entry_points={
        "console_scripts": [
            "simulator_node = ros_car_simulator.simulator_node:main",
        ],
    },
)
