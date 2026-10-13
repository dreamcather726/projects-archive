"""实验三：使用生成器产生数据，用装饰器记录函数调用。"""


def log_call(func):
    """为函数调用增加开始和结束提示。"""
    def wrapper(*args, **kwargs):
        # 原函数的参数原样转交，返回值也要继续交还调用者。
        print(f"开始调用：{func.__name__}")
        result = func(*args, **kwargs)
        print(f"调用完成：{func.__name__}")
        return result

    return wrapper


def generate_distances():
    """每次迭代产生一个模拟测距。"""
    for distance_m in [1.20, 0.75, 0.35]:
        yield distance_m



def choose_action(distance_m: float) -> str:
    """根据单次测距选择状态。"""
    if distance_m <= 0.40:
        print("停车")
        return "停车"
    if distance_m <= 0.80:
        print("减速")
        return "减速"
    print("前进")   
    return "前进"


# for 每次向生成器索取一个值，不需要先创建完整的新列表。
for distance_m in generate_distances():
    action = choose_action(distance_m)
    print(f"{distance_m:.2f} m -> {action}")
