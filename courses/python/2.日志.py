str1=input("输入日志前缀1:")
str2=input("输入日志前缀2:")
print("\n")
def log(l="info"):
    def aaa(func):
        def wrapper(*arg,**kwargs):
            if l==str1:
                print(f"{str1}开始执行{func.__name__}")
                func(*arg,**kwargs)
                print("执行完毕")
            elif l==str2:
                print(f"{str2}开始执行{func.__name__}")
                func(*arg,**kwargs)
                print("执行完毕")
        return wrapper
    return aaa

@log(l= str1)
def add(a, b):
    print(f"sum={a+b}")
@log(l= str2)
def sss():
    print("sss")

add(12,14)
print("\n................\n")
sss()