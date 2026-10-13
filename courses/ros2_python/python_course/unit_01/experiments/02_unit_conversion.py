# 文件：experiments/02_unit_conversion.py
import math

robot_name = "robot_01"
linear_speed_mps = 0.30
heading_deg = 90.0
angular_speed_degps = 45.0
travel_duration_s = 10.0

# 保持物理量含义不变，只转换单位
linear_speed_kmh = linear_speed_mps * 3.6
heading_rad = heading_deg * math.pi / 180.0
recovered_heading_deg = heading_rad * 180.0 / math.pi
angular_speed_radps = angular_speed_degps * math.pi / 180.0
travel_distance_m = linear_speed_mps * travel_duration_s

print(f"机器人：{robot_name}")
print(f"线速度：{linear_speed_mps:.2f} m/s = {linear_speed_kmh:.2f} km/h")
print(f"朝向角：{heading_deg:.1f} deg = {heading_rad:.4f} rad")
print(f"反向换算：{recovered_heading_deg:.1f} deg")
print(f"角速度：{angular_speed_degps:.1f} deg/s = {angular_speed_radps:.4f} rad/s")
print(f"匀速运动 {travel_duration_s:.1f} s 的路程：{travel_distance_m:.2f} m")
