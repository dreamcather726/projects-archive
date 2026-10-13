# 文件：experiments/03_parse_status.py
# 自定义教学格式：名称|电量百分数|线速度m/s|使能文本
raw_status = "  Robot_01|85|0.30|TRUE  "
fields = raw_status.strip().split("|")

if len(fields) != 4:
    print("错误：状态文本必须包含 4 个字段，并使用 | 分隔。")
else:
    robot_name = fields[0].strip().lower()
    battery_text = fields[1].strip()
    speed_text = fields[2].strip()
    enabled_text = fields[3].strip().lower()

    # 先检查数字文本，避免直接转换无效字符串
    battery_format_ok = (
        battery_text.isascii()
        and battery_text.isdecimal()
        and len(battery_text) <= 3
    )
    speed_digits = speed_text.replace(".", "", 1)
    speed_format_ok = (
        speed_text.isascii()
        and speed_text.count(".") <= 1
        and speed_digits.isdecimal()
        and len(speed_text) <= 12
    )
    enabled_format_ok = enabled_text == "true" or enabled_text == "false"

    if not (robot_name and battery_format_ok and speed_format_ok and enabled_format_ok):
        print("错误：名称不能为空，电量或速度格式错误，或使能文本不是 true/false。")
    else:
        battery_percent = int(battery_text)
        linear_speed_mps = float(speed_text)
        is_enabled = enabled_text == "true"
        distance_m = None

        if not (0 <= battery_percent <= 100 and 0.0 <= linear_speed_mps <= 2.0):
            print("错误：电量须为 0～100%，速度须为 0.0～2.0 m/s。")
        else:
            linear_speed_kmh = linear_speed_mps * 3.6
            print("=== 解析后的机器人状态 ===")
            print(f"机器人：{robot_name}")
            print(f"电量：{battery_percent}%")
            print(f"线速度：{linear_speed_mps:.2f} m/s = {linear_speed_kmh:.2f} km/h")
            print(f"已使能：{is_enabled}")
            print(f"是否缺少测距：{distance_m is None}")
            print(f"电量类型：{type(battery_percent)}")
            print(f"速度类型：{type(linear_speed_mps)}")
            print(f"使能类型：{type(is_enabled)}")
