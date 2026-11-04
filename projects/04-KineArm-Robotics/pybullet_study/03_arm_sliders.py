import time

import pybullet as p

from common_robot import connect_gui, create_simple_arm, add_joint_sliders
from common_robot import read_slider_degrees, set_joint_targets_deg


def main():
    # connect_gui 是我们自己在 common_robot.py 里封装的函数。
    # 它做了这些事：
    #   1. p.connect(p.GUI)
    #   2. 设置 pybullet_data 路径
    #   3. 设置重力
    #   4. 加载地面
    #   5. 设置摄像机
    connect_gui()

    # 创建一个“教学用”的 5 关节机械臂。
    # 这个机械臂不是从 URDF 文件加载的，而是用 createMultiBody 手动拼出来的。
    robot_id = create_simple_arm()

    # 在 PyBullet 左侧调试面板里创建 5 个滑条。
    # 每个滑条对应一个关节角度，单位是度。
    sliders = add_joint_sliders(robot_id)

    print("Drag the sliders in the PyBullet debug panel.")
    print("Each slider controls one robot arm joint.")

    while p.isConnected():
        # 读取所有滑条当前角度，例如 [90, 0, 180, 130, 90]。
        angles = read_slider_degrees(sliders)

        # 把“度”转换成 PyBullet 电机需要的“弧度”，并发送给关节电机。
        set_joint_targets_deg(robot_id, angles)

        # 推进仿真。没有这句，机械臂不会动。
        p.stepSimulation()
        time.sleep(1 / 240)


if __name__ == "__main__":
    main()
