"""实验四：机器人对象组合测距传感器对象。"""


class DistanceSensor:
    """保存一个测距传感器的名称和最新读数。"""
    def __init__(self, name: str):
        self.name = name
        self.distance_m = None

    def update(self, distance_m: float) -> None:
        self.distance_m = distance_m


class Robot:
    """通过属性持有传感器对象，演示对象组合。"""
    def __init__(self, name: str, sensor: DistanceSensor):
        self.name = name
        self.sensor = sensor
        self.action = "等待数据"

    def update_action(self) -> None:
        """读取组合进来的传感器状态并更新机器人动作。"""
        distance_m = self.sensor.distance_m
        if distance_m is None:
            self.action = "等待数据"
        elif distance_m <= 0.40:
            self.action = "停车"
        elif distance_m <= 0.80:
            self.action = "减速"
        else:
            self.action = "前进"


front_sensor = DistanceSensor("front_sensor")
# 将已有传感器对象交给机器人，双方引用的是同一个对象。
robot = Robot("robot_01", front_sensor)

front_sensor.update(0.55)
robot.update_action()

print(f"机器人：{robot.name}")
print(f"传感器：{robot.sensor.name}")
print(f"测距：{robot.sensor.distance_m:.2f} m")
print(f"模拟运动状态：{robot.action}")
