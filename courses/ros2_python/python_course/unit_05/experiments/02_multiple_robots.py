"""实验二：多个实例分别保存状态。"""


class Robot:
    # 类属性由同类对象共享读取，适合保存固定类别等公共信息。
    category = "mobile_robot"

    def __init__(self, name: str, battery: int):
        # 实例属性分别属于每个对象，不会因另一个对象更新而改变。
        self.name = name
        self.battery = battery
        self.speed_mps = 0.0

    def set_speed(self, speed_mps: float) -> None:
        self.speed_mps = speed_mps


robot_1 = Robot("robot_01", 85)
robot_2 = Robot("robot_02", 72)

# 两个实例调用同一方法，但各自保存自己的速度。
robot_1.set_speed(0.30)
robot_2.set_speed(0.45)

print(robot_1.name, robot_1.speed_mps, robot_1.category)
print(robot_2.name, robot_2.speed_mps, robot_2.category)
