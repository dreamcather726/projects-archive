"""实验三：把模拟传感器数据写入 CSV。"""

import csv
from pathlib import Path


# 每个内层列表表示 CSV 中的一行：编号、距离、状态。
records = [
    [1, 1.20, "前进"],
    [2, 0.65, "减速"],
    [3, 0.30, "停车"],
]

# 输出目录不存在时先创建；exist_ok=True 允许目录已经存在。
data_dir = Path(__file__).resolve().parent / "data"
data_dir.mkdir(exist_ok=True)
csv_path = data_dir / "sensor_log.csv"

with csv_path.open("w", newline="", encoding="utf-8") as file:
    writer = csv.writer(file)
    # 先写表头，再一次写入全部测量记录。
    writer.writerow(["sample_id", "distance_m", "action"])
    writer.writerows(records)

with csv_path.open("r", newline="", encoding="utf-8") as file:
    reader = csv.reader(file)
    # reader 每次迭代返回一行字段列表，字段默认都是字符串。
    for row in reader:
        print(row)
