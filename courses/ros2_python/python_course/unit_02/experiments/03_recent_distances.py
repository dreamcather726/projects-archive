"""实验三：使用队列保存最近三次测距。"""

from collections import deque


# maxlen=3 表示只保留最近三项；加入第四项时会自动移除最早一项。
recent_distances_m = deque(maxlen=3)
new_distances_m = [1.20, 0.90, 0.70, 0.50]

for distance_m in new_distances_m:
    recent_distances_m.append(distance_m)
    print(f"加入 {distance_m:.2f} m：{list(recent_distances_m)}")

# 对缓存中的近期数据做统计，可用于平滑或趋势判断。
nearest_distance_m = min(recent_distances_m)
print(f"最近三次测量中的最小值：{nearest_distance_m:.2f} m")
