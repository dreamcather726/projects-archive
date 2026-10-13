"""课堂练习参考答案。"""

raw_distances_m = [1.50, None, 0.75, -1.00, 0.35]
valid_distances_m = []

# 第一步：逐项过滤无效数据。
for distance_m in raw_distances_m:
    if distance_m is None or distance_m <= 0:
        continue
    valid_distances_m.append(distance_m)

# 第二步：只有存在有效数据时，才计算最近距离并判断状态。
if valid_distances_m:
    nearest_distance_m = min(valid_distances_m)

    # 第一个满足的分支会决定最终状态。
    if nearest_distance_m <= 0.40:
        action = "停车"
    elif nearest_distance_m <= 0.80:
        action = "减速"
    else:
        action = "前进"

    print(f"有效数据：{valid_distances_m}")
    print(f"最近距离：{nearest_distance_m:.2f} m")
    print(f"模拟运动状态：{action}")
else:
    print("没有有效的测距数据")
