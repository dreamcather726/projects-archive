raw_distances_m = [1.20, 0.55, None, -0.10, 0.32]
valid_distances_m = []

for distance_m in raw_distances_m:
    if distance_m is None or distance_m <= 0: 
        break      
    print(f"有效数据：{distance_m:.2f} m")
        