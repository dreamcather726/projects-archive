import math
import time

import pybullet as p

from common_robot import connect_gui, create_simple_arm, set_joint_targets_deg


def main():
    # 本阶段不再使用滑条，而是让程序自己生成关节角度。
    connect_gui()
    robot_id = create_simple_arm()

    # 记录程序开始时间，后面用 t 表示已经运行了多少秒。
    start = time.time()

    print("The arm is moving automatically with sine waves.")

    while p.isConnected():
        # t 会不断变大。
        # sin(t) 会在 -1 到 1 之间周期变化。
        # 所以 90 + 40 * sin(t) 会在 50 到 130 度之间来回摆动。
        t = time.time() - start

        # 5 个关节使用不同频率和幅度，这样运动看起来更自然。
        angles = [
            90 + 40 * math.sin(t),
            90 + 35 * math.sin(t * 0.8),
            90 + 45 * math.sin(t * 1.2),
            90 + 25 * math.sin(t * 1.6),
            90 + 20 * math.sin(t * 2.0),
        ]

        # 把自动生成的角度发给机械臂。
        set_joint_targets_deg(robot_id, angles)
        p.stepSimulation()
        time.sleep(1 / 240)


if __name__ == "__main__":
    main()
