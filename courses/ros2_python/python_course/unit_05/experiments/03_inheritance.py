"""实验三：使用继承、方法重写和多态。"""


class Robot:
    """提供所有机器人共用的基本状态和方法。"""

    def __init__(self, name: str, battery: int):
        self.name = name
        self.battery = battery
        self.speed_mps = 0.0

    def move(self) -> None:
        print(f"{self.name}：使用通用方式移动")

    def stop(self) -> None:
        self.speed_mps = 0.0
        print(f"{self.name}：已停止")

    def show_status(self) -> None:
        print(f"{self.name}：电量 {self.battery}%，速度 {self.speed_mps:.2f} m/s")


class MobileRobot(Robot):
    """继承 Robot，并增加移动机器人特有的轮子信息。"""

    def __init__(self, name: str, battery: int, wheel_count: int):
        # 先复用父类初始化，再初始化子类新增的属性。
        super().__init__(name, battery)
        self.wheel_count = wheel_count

    def move(self) -> None:
        # 重写父类方法，提供轮式机器人的移动行为。
        print(f"{self.name}：转动 {self.wheel_count} 个轮子前进")

    def show_status(self) -> None:
        # 重写方法时仍可调用父类版本，避免重复基本状态输出。
        super().show_status()
        print(f"轮子数量：{self.wheel_count}")


class AerialRobot(Robot):
    """继承 Robot，并增加飞行机器人特有的高度信息。"""

    def __init__(self, name: str, battery: int, altitude_m: float):
        super().__init__(name, battery)
        self.altitude_m = altitude_m

    def move(self) -> None:
        # 同样重写 move()，但实现的是飞行动作。
        print(f"{self.name}：提升旋翼，飞向 {self.altitude_m:.1f} m 高度")

    def show_status(self) -> None:
        super().show_status()
        print(f"目标高度：{self.altitude_m:.1f} m")


def run_mission(robot: Robot) -> None:
    """用同一套代码操作不同种类的机器人。"""
    print("\n--- 开始任务 ---")
    robot.show_status()
    robot.move()  # 根据对象的实际类型，调用各自重写后的方法。
    robot.stop()  # 两个子类都没有重写 stop()，所以复用父类方法。


mobile_robot = MobileRobot("mobile_01", 85, 4)
mobile_robot.speed_mps = 0.30

aerial_robot = AerialRobot("drone_01", 76, 5.0)
aerial_robot.speed_mps = 1.20

# 同一个列表中保存不同子类对象，同一个函数调用表现出不同的行为。
robots = [mobile_robot, aerial_robot]
for current_robot in robots:
    run_mission(current_robot)
