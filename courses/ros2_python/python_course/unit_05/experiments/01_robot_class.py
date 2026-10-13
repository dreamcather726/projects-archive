"""实验一：定义 Robot 类。"""


class Robot:
    def __init__(self, name: str, battery: int, speed_mps: float = 0.0):
        # 实例属性保存这一台机器人自己的状态。
        self.name = name
        self.battery = battery
        self.speed_mps = speed_mps

    def set_speed(self, speed_mps: float) -> None:
        """更新当前对象的速度。"""
        self.speed_mps = speed_mps

    def stop(self) -> None:
        """将速度设为零，模拟停车。"""
        self.speed_mps = 0.0

    def show_status(self) -> None:
        print(f"机器人：{self.name}")
        print(f"电量：{self.battery}%")
        print(f"线速度：{self.speed_mps:.2f} m/s")


# 创建对象后，才能调用实例方法并保存具体状态。
robot = Robot("robot_01", 85)
robot.set_speed(0.30)
robot.show_status()
robot.stop()
print(f"停车后速度：{robot.speed_mps:.2f} m/s")
