class Robot:
    def stop(self):
        self._speed_mps = 0.0

    def show_status(self):
        print(self.name, self.speed_mps)
    def __init__(self):
        self.name = "R2-D2"
        self._speed_mps = 0.0
    def get_speed_mps(self):
        return self._speed_mps

robot = Robot()
robot.get_speed_mps()
