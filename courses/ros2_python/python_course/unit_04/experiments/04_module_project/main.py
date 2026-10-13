"""模块化机器人测距项目入口。"""

import json
from pathlib import Path

from robot_tools import choose_action, filter_distances, load_config


def main() -> None:
    """读取配置，处理测距并输出最终状态。"""
    # 所有路径都以项目入口文件所在目录为基准。
    project_dir = Path(__file__).resolve().parent
    config_path = project_dir / "config" / "robot_config.json"
    raw_distances_m = [1.20, None, 0.65, -0.10]

    try:
        # 各模块各负其责：config 读取配置，processing 处理数据。
        config = load_config(config_path)
        valid_distances_m = filter_distances(raw_distances_m)

        if not valid_distances_m:
            # 没有数据时提前结束，避免调用 min([])。
            print("没有有效测距数据")
            return

        nearest_distance_m = min(valid_distances_m)
        # 阈值来自配置文件，因此修改规则时不需要修改程序代码。
        action = choose_action(
            nearest_distance_m,
            config["stop_distance_m"],
            config["slow_distance_m"],
        )

        print(f"机器人：{config['robot_name']}")
        print(f"有效数据：{valid_distances_m}")
        print(f"最近距离：{nearest_distance_m:.2f} m")
        print(f"模拟运动状态：{action}")
    except FileNotFoundError:
        print(f"配置文件不存在：{config_path}")
    except json.JSONDecodeError:
        print("配置文件不是合法的 JSON 格式")
    except KeyError as error:
        print(f"配置中缺少字段：{error}")


# 直接运行本文件时启动主流程；被其他模块导入时不会自动运行。
if __name__ == "__main__":
    main()
