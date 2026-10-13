
raw_status = input("请输入机器人状态（格式：名称|电量|速度|是否启用）：")
fields = raw_status.split("|")

robot_name = fields[0].lower()
battery_percent = int(fields[1])
linear_speed_mps = float(fields[2])
is_enabled = fields[3].lower() == "true"

linear_speed_kmh = linear_speed_mps * 3.6
can_move = is_enabled and battery_percent > 20
print(f"机器人：{robot_name}")
print(f"电量：{battery_percent}%")
print(f"速度：{linear_speed_mps:.2f} m/s")
print(f"换算速度：{linear_speed_kmh:.2f} km/h")
print(f"满足模拟运动条件：{can_move}")