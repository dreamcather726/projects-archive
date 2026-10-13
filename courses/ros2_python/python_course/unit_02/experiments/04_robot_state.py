"""综合实验：使用字典组织机器人状态。"""

# 字典用有含义的键集中保存同一台机器人的不同状态。
robot = {
    "name": "robot_01",
    "battery": 85,
    "speed_mps": 0.30,
    "distances_m": [0.90, 0.70, 0.50],
}

# 通过键取出测距列表，再计算其中的最近距离。
nearest_distance_m = min(robot["distances_m"])

# 将判断结果写回字典，形成一份完整的状态记录。
if nearest_distance_m <= 0.40:
    robot["action"] = "停车"
elif nearest_distance_m <= 0.80:
    robot["action"] = "减速"
else:
    robot["action"] = "前进"

print("=== 机器人状态 ===")
print(f"机器人：{robot['name']}")
print(f"电量：{robot['battery']}%")
print(f"设定速度：{robot['speed_mps']:.2f} m/s")
print(f"最近距离：{nearest_distance_m:.2f} m")
print(f"模拟运动状态：{robot['action']}")
