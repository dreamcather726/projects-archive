"""没有 ROS 2 环境时，用普通 Python 演示事件与回调。"""


class TimerSimulator:
    """用普通循环模拟“保存回调，稍后调用”的关系。"""
    def __init__(self, callback):
        # 保存函数对象，不在初始化时执行。
        self.callback = callback

    def spin_three_times(self) -> None:
        # 下划线表示本循环不需要使用当前计数值。
        for _ in range(3):
            self.callback()


class RobotStatus:
    """保存会在多次回调之间持续存在的机器人状态。"""
    def __init__(self) -> None:
        self.robot_name = "robot_01"
        self.battery_percent = 85

    def timer_callback(self) -> None:
        # 每次调用都读取当前电量，再为下一次调用更新状态。
        print(f"{self.robot_name}：电量 {self.battery_percent}%")
        self.battery_percent -= 1


robot = RobotStatus()
# 传入绑定方法本身；真实 ROS 2 中由执行器在定时器到期后调用。
simulator = TimerSimulator(robot.timer_callback)
simulator.spin_three_times()
