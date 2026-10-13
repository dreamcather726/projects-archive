"""实验一：用函数封装测距过滤和状态判断。"""


def filter_distances(raw_distances_m: list) -> list:
    """返回只包含正数测距的新列表。"""
    valid_distances_m = []
    for distance_m in raw_distances_m:
        if distance_m is None or distance_m <= 0:
            continue
        valid_distances_m.append(distance_m)
    return valid_distances_m


def choose_action(nearest_distance_m: float | None) -> str:
    """根据最近距离返回模拟运动状态。"""
    if nearest_distance_m is None:
        return "等待数据"
    if nearest_distance_m <= 0.40:
        return "停车"
    if nearest_distance_m <= 0.80:
        return "减速"
    return "前进"


raw_distances_m = [1.20, 0.55, None, -0.10, 0.32]
# 主流程按“过滤 → 求最近值 → 判断状态”的顺序组合函数。
valid_distances_m = filter_distances(raw_distances_m)
nearest_distance_m = min(valid_distances_m) if valid_distances_m else None
action = choose_action(nearest_distance_m)

print(f"有效数据：{valid_distances_m}")
print(f"最近距离：{nearest_distance_m:.2f} m")
print(f"模拟运动状态：{action}")
