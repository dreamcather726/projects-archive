import time

import pybullet as p

from common_robot import connect_gui, create_simple_arm, add_joint_sliders
from common_robot import read_slider_degrees, set_joint_targets_deg


def main():
    connect_gui()
    robot_id = create_simple_arm()
    sliders = add_joint_sliders(robot_id)

    # poses 用来保存多个姿态。
    # 每个姿态都是一个角度列表，例如 [90, 0, 180, 130, 90]。
    poses = []

    # addUserDebugParameter 原本是滑条，但也可以当成“按钮”使用。
    # 这里 min=1, max=0 是一个常见写法，点击后它的值会变化。
    record_button = p.addUserDebugParameter("record pose", 1, 0, 0)
    play_button = p.addUserDebugParameter("play poses", 1, 0, 0)

    # 用 last_record/last_play 判断按钮有没有被点击。
    last_record = 0
    last_play = 0

    print("1. Move joint sliders.")
    print("2. Click 'record pose' several times.")
    print("3. Click 'play poses' to replay your action.")

    while p.isConnected():
        # 平时仍然由滑条控制机械臂。
        angles = read_slider_degrees(sliders)
        set_joint_targets_deg(robot_id, angles)

        # 读取两个“按钮”的当前值。
        record_now = p.readUserDebugParameter(record_button)
        play_now = p.readUserDebugParameter(play_button)

        # 如果 record_button 的值发生变化，就认为用户点击了一次录制。
        if record_now != last_record:
            poses.append(angles)
            print("Recorded pose:", [round(a, 1) for a in angles])
            last_record = record_now

        # 如果 play_button 的值发生变化，就依次播放所有已录制姿态。
        if play_now != last_play:
            print(f"Playing {len(poses)} poses")
            for pose in poses:
                # 每个姿态保持 120 个仿真步。
                # 240Hz 下约等于 0.5 秒。
                for _ in range(120):
                    set_joint_targets_deg(robot_id, pose)
                    p.stepSimulation()
                    time.sleep(1 / 240)
            last_play = play_now

        p.stepSimulation()
        time.sleep(1 / 240)


if __name__ == "__main__":
    main()
