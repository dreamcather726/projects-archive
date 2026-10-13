# 文件：answers/02_convert_and_compare.py
speed_kmh_text = "1.8"
max_linear_speed_mps = 0.40

linear_speed_kmh = float(speed_kmh_text)
linear_speed_mps = linear_speed_kmh / 3.6
speed_within_limit = linear_speed_mps <= max_linear_speed_mps

print(f"输入速度：{linear_speed_kmh:.2f} km/h")
print(f"换算速度：{linear_speed_mps:.2f} m/s")
print(f"是否未超过 {max_linear_speed_mps:.2f} m/s：{speed_within_limit}")
