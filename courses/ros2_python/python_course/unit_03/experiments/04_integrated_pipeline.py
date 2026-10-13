"""综合实验：生成数据、过滤数据并通过回调输出状态。"""


def filter_distance(distance_m: float | None) -> float | None:
    """无效数据返回 None，有效数据保持原值返回。"""
    if distance_m is None or distance_m <= 0:
        return None
    return distance_m


def choose_action(distance_m: float) -> str:
    """把有效距离转换成模拟运动状态。"""
    if distance_m <= 0.40:
        return "停车"
    if distance_m <= 0.80:
        return "减速"
    return "前进"


def show_status(distance_m: float, action: str) -> None:
    """作为回调显示一次处理结果。"""
    print(f"有效测距：{distance_m:.2f} m，模拟状态：{action}")


def process_sensor_data(raw_distances_m: list, callback) -> None:
    """组织过滤、判断和回调输出的完整流水线。"""
    for raw_distance_m in raw_distances_m:
        distance_m = filter_distance(raw_distance_m)
        if distance_m is None:
            print(f"跳过无效数据：{raw_distance_m}")
            continue

        action = choose_action(distance_m)
        # 处理函数不固定输出方式，而是把结果交给外部传入的回调。
        callback(distance_m, action)


simulated_data = [1.20, None, -0.10, 0.65, 0.30]
process_sensor_data(simulated_data, show_status)
