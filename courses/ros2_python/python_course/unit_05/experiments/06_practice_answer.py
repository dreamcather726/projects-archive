"""课堂练习参考答案。"""


class Robot:
    
    def __init__(self, name: str, battery: int):
        # 每个实例单独保存名称、电量和速度。
        self.name = name
        self.battery = battery
        self.speed_mps = 0.0

    def set_speed(self, speed_mps: float) -> None:
        # 只接受教学规定范围内的速度，非法值不修改原状态。
        if 0.0 <= speed_mps <= 0.50:
            self.speed_mps = speed_mps
        else:
            print("速度超出范围")

    def stop(self) -> None:
        self.speed_mps = 0.0

    def show_status(self) -> None:
        print(f"{self.name}：电量 {self.battery}%，速度 {self.speed_mps:.2f} m/s")


robot_1 = Robot("robot_01", 85)
robot_2 = Robot("robot_02", 72)

# 分别更新两个对象，验证实例状态互不影响。
robot_1.set_speed(0.30)
robot_2.set_speed(0.45)

robot_1.show_status()
robot_2.show_status()

robot_1.stop()
# robot_1 停车后，robot_2 的速度仍保持原值。
robot_1.show_status()
robot_2.show_status()
