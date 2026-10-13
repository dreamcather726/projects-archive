"""课堂练习参考答案。"""


def analyze_distances(raw_distances_m: list) -> tuple:
    """返回有效列表、最近距离和对应状态。"""
    valid_distances_m = []

    for distance_m in raw_distances_m:
        if distance_m is None or distance_m <= 0:
            continue
        valid_distances_m.append(distance_m)

    if not valid_distances_m:
        # 始终返回三个值，方便调用处使用相同的解包方式。
        return [], None, "等待数据"

    nearest_distance_m = min(valid_distances_m)

    if nearest_distance_m <= 0.40:
        action = "停车"
    elif nearest_distance_m <= 0.80:
        action = "减速"
    else:
        action = "前进"

    return valid_distances_m, nearest_distance_m, action


raw_data = [1.50, None, 0.75, -1.00, 0.35]
# 多返回值实际组成元组，可一次解包给三个变量。
valid_data, nearest_distance, action = analyze_distances(raw_data)

print(f"有效数据：{valid_data}")
print(f"最近距离：{nearest_distance:.2f} m")
print(f"模拟运动状态：{action}")
