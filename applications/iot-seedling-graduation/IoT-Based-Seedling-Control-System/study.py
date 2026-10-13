import threading
import time
import os
g_count = 0
def sum():
    for i in range(10000):
        global g_count
        g_count += 1
    print(f"G_sum1: {g_count}")

def sum2():
    for i in range(10000):
        global g_count
        g_count += 1
    print(f"G_sum2: {g_count}")



if __name__ == "__main__":
     
    sum_thread = threading.Thread(target=sum )        
    sum2_thread = threading.Thread(target=sum2 )        
    sum_thread.start()  
    sum2_thread.start()  
