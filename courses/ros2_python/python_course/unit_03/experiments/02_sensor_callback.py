"""实验二：把函数作为回调传给模拟传感器。"""


def print_reading(distance_m: float) -> None:
    """处理模拟器交来的一次测距。"""
    print(f"收到测距：{distance_m:.2f} m")


def sensor_simulator(distances_m: list, callback) -> None:
    """逐项产生数据，并在每次产生数据时调用回调。"""
    for distance_m in distances_m:
        callback(distance_m)


simulated_distances_m = [1.20, 0.75, 0.35]

# 传入函数本身，不加括号；ROS 2 注册回调时也是这种形式。
sensor_simulator(simulated_distances_m, print_reading)
