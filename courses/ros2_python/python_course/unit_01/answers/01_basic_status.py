# 文件：answers/01_basic_status.py
robot_name = "robot_02"
battery_percent = 72
linear_speed_mps = 0.256
is_enabled = False
distance_m = None

print(f"机器人：{robot_name}")
print(f"电量：{battery_percent}%")
print(f"线速度：{linear_speed_mps:.2f} m/s")
print(f"已使能：{is_enabled}")
print(f"是否缺少测距：{distance_m is None}")
print(f"原始速度：{linear_speed_mps}")
print(f"速度类型：{type(linear_speed_mps)}")
