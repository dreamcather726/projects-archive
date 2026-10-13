from setuptools import setup

package_name = "ros_car_cockpit"

setup(
    name=package_name,
    version="0.0.1",
    packages=[package_name, package_name + ".ui"],
    data_files=[
        ("share/ament_index/resource_index/packages", ["resource/" + package_name]),
        ("share/" + package_name, ["package.xml"]),
        ("share/" + package_name + "/launch", ["launch/cockpit.launch.py"]),
    ],
    install_requires=["setuptools"],
    zip_safe=True,
    maintainer="ros-car",
    maintainer_email="user@example.com",
    description="ROS2 cockpit GUI skeleton for ROS-CAR.",
    license="MIT",
    entry_points={
        "console_scripts": [
            "cockpit_app = ros_car_cockpit.main:main",
        ],
    },
)
