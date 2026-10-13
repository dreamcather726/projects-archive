# 文件：experiments/01_robot_status.py
# 用独立变量表示机器人的一次状态快照
robot_name = "robot_01"
battery_percent = 85
linear_speed_mps = 0.30
is_enabled = True
distance_m = None

print("=== 机器人初始状态 ===")
print(f"机器人：{robot_name}")
print(f"电量：{battery_percent}%")
print(f"线速度：{linear_speed_mps:.2f} m/s")
print(f"已使能：{is_enabled}")
print(f"测距数据：{distance_m}")
print(f"是否缺少测距：{distance_m is None}")

# 模拟一次状态更新，不会连接或控制机器人
battery_percent -= 5
print("=== 一次更新之后 ===")
print(f"剩余电量：{battery_percent}%")
print(f"电量是否高于 20%：{battery_percent > 20}")
