def log(func):
    def wrapper(*arg,**kwargs):
        print(f"开始执行{func.__name__}函数")
        func(*arg,**kwargs)
        print("执行完毕")
    return wrapper

@log
def add(a,b):
    print(f"sum={a+b}")

add(12,14)