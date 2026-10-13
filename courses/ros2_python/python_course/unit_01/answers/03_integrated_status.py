# 文件：answers/03_integrated_status.py
import math

raw_name = "  ROBOT_03  "
battery_text = "18"
speed_text = "0.45"
enabled_text = "false"
heading_deg = 180.0

# 本题给定文本均合法，实验三演示了任意输入的格式检查
robot_name = raw_name.strip().lower()
battery_percent = int(battery_text)
linear_speed_mps = float(speed_text)
is_enabled = enabled_text.strip().lower() == "true"

linear_speed_kmh = linear_speed_mps * 3.6
heading_rad = heading_deg * math.pi / 180.0
battery_valid = 0 <= battery_percent <= 100
speed_within_limit = 0.0 <= linear_speed_mps <= 0.50
can_move = is_enabled and battery_valid and (battery_percent > 20) and speed_within_limit

print(f"机器人：{robot_name}")
print(f"电量：{battery_percent}%")
print(f"线速度：{linear_speed_mps:.2f} m/s = {linear_speed_kmh:.2f} km/h")
print(f"朝向角：{heading_rad:.4f} rad")
print(f"已使能：{is_enabled}")
print(f"电量是否合法：{battery_valid}")
print(f"速度是否在范围内：{speed_within_limit}")
print(f"是否满足模拟许可条件：{can_move}")
