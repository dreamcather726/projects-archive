"""实验二：读取 JSON 配置并处理常见错误。"""

import json
from pathlib import Path


# JSON 文件保存节点将来可能用到的名称和阈值等普通配置。
config_path = Path(__file__).resolve().parent / "data" / "robot_config.json"

try:
    # json.load() 将 JSON 文本转换成 Python 字典、列表等对象。
    with config_path.open("r", encoding="utf-8") as file:
        config = json.load(file)

    print(f"机器人：{config['robot_name']}")
    print(f"最大速度：{config['max_speed_mps']:.2f} m/s")
    print(f"停车距离：{config['stop_distance_m']:.2f} m")
# 针对不同错误给出明确提示，便于定位是路径、格式还是字段问题。
except FileNotFoundError:
    print(f"配置文件不存在：{config_path}")
except json.JSONDecodeError:
    print("配置文件不是合法的 JSON 格式")
except KeyError as error:
    print(f"配置中缺少字段：{error}")
