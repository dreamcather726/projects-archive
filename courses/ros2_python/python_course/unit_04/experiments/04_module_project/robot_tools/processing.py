"""测距数据处理模块。"""


def filter_distances(raw_distances_m: list) -> list:
    """使用列表推导式保留有效的正数测距。"""
    return [
        distance_m
        for distance_m in raw_distances_m
        if distance_m is not None and distance_m > 0
    ]


def choose_action(distance_m: float, stop_m: float, slow_m: float) -> str:
    """按照配置中的停车和减速阈值返回状态。"""
    # 从更小的停车阈值开始判断，避免近距离被归入减速范围。
    if distance_m <= stop_m:
        return "停车"
    if distance_m <= slow_m:
        return "减速"
    return "前进"

if __name__ == "__main__":
    # 直接运行本文件时执行测试。
    test_distances = [1.20, None, 0.65, -0.10]
    print(f"原始数据：{test_distances}")
    print(f"有效数据：{filter_distances(test_distances)}")
