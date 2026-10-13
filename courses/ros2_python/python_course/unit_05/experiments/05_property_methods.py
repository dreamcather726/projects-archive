"""拓展实验：property、staticmethod 和 classmethod。"""


class Robot:
    category = "mobile_robot"

    def __init__(self, name: str, battery: int):
        self.name = name
        self._battery = 0
        # 通过公开属性赋值，会自动经过下面 setter 的范围检查。
        self.battery = battery

    @property
    def battery(self) -> int:
        """像普通属性一样读取内部保存的电量。"""
        return self._battery

    @battery.setter
    def battery(self, value: int) -> None:
        """写入电量前统一检查合法范围。"""
        if not 0 <= value <= 100:
            raise ValueError("电量必须在 0～100 之间")
        self._battery = value

    @staticmethod
    def is_valid_speed(speed_mps: float) -> bool:
        """不依赖某个对象状态，直接检查传入速度。"""
        return 0.0 <= speed_mps <= 0.50

    @classmethod
    def create_default(cls, name: str):
        """使用当前类创建具有默认满电状态的对象。"""
        return cls(name, 100)


# 类方法提供另一种含义明确的对象创建入口。
robot = Robot.create_default("robot_03")

print(f"机器人：{robot.name}")
print(f"电量：{robot.battery}%")
print(f"0.30 m/s 是否合法：{Robot.is_valid_speed(0.30)}")
