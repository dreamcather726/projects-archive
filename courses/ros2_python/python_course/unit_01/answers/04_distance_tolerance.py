# 文件：answers/04_distance_tolerance.py
import math

distance_m = 0.1 + 0.2
target_distance_m = 0.3

exact_match = distance_m == target_distance_m
close_match = math.isclose(
    distance_m,
    target_distance_m,
    rel_tol=0.0,
    abs_tol=1e-9,
)

print(f"原始计算结果：{distance_m}")
print(f"两位小数显示：{distance_m:.2f} m")
print(f"直接比较：{exact_match}")
print(f"容差比较：{close_match}")
