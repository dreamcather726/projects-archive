"""实验二：根据最近障碍物距离判断模拟运动状态。"""

nearest_distance_m = 0.55

# 阈值要从最小范围向上判断，确保近距离首先触发停车。
if nearest_distance_m is None:
    action = "等待数据"
elif nearest_distance_m <= 0.40:
    action = "停车"
elif nearest_distance_m <= 0.80:
    action = "减速"
else:
    action = "前进"

# None 不能使用 .2f 格式化，所以输出时需要单独处理。
if nearest_distance_m is None:
    print("最近障碍物：暂无数据")
else:
    print(f"最近障碍物：{nearest_distance_m:.2f} m")

print(f"模拟运动状态：{action}")
