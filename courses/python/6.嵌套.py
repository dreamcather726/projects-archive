def log1(func):
    def wrapper():
        print("函数开始执行1：\n")
        func()
        print("结束执行1\n")
    return wrapper

def log2(func):
    def wrapper():
        print("函数开始执行2：\n")
        func()
        print("结束执行2\n")
    return wrapper

def log3(func):
    def wrapper():
        print("函数开始执行3：\n")
        func()
        print("结束执行3\n")
    return wrapper

@log1
@log2
@log3
def hi():
    print("hello\n")

hi()
    