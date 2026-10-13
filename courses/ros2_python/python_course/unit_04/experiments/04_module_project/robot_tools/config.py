"""配置读取模块。"""

import json
from pathlib import Path


def load_config(config_path: Path) -> dict:
    """读取 JSON 配置，并以 Python 字典返回。"""
    # 异常交给调用者处理，使本函数只负责读取和解析。
    with config_path.open("r", encoding="utf-8") as file:
        return json.load(file)
