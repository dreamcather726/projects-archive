"""拓展实验：通过命令行参数提供机器人状态。"""

import argparse


# --name 和 --speed 都是可选参数；未提供时使用 default。
parser = argparse.ArgumentParser(description="显示机器人状态")
parser.add_argument("--name", default="robot_01")
parser.add_argument("--speed", type=float, default=0.30)
# 解析启动命令中的参数，并把结果保存到 args 对象。
args = parser.parse_args()

print(f"机器人：{args.name}")
print(f"线速度：{args.speed:.2f} m/s")
