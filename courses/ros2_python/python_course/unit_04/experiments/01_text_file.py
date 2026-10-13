"""实验一：使用 with 读写文本文件。"""

from pathlib import Path


# 以当前脚本所在目录为基准，避免受终端当前目录影响。
data_dir = Path(__file__).resolve().parent / "data"
data_dir.mkdir(exist_ok=True)
file_path = data_dir / "robot_status.txt"

# "w" 会重新写入文件；with 代码块结束后文件会自动关闭。
with file_path.open("w", encoding="utf-8") as file:
    file.write("robot_name=robot_01\n")
    file.write("battery_percent=85\n")
    file.write("speed_mps=0.30\n")

# 再以只读模式打开，验证刚才保存的内容。
with file_path.open("r", encoding="utf-8") as file:
    content = file.read()

print(content, end="")
