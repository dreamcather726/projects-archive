import functools
import time

class Counter:
    def __init__(self, func):
        self.func = func
        self.time1=time.time() 
        func()
        self.time2=time.time()
        self.time=self.time2-self.time1 
        
    def __call__(self, *args, **kwargs):
        print(f"用时{self.time}")
        return self.func(*args, **kwargs)

@Counter
def work():
    time.sleep(1)
    print("任务执行完毕")
work()
