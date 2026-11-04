import time

import pybullet as p

from common_robot import connect_gui, create_simple_arm


def main():
    connect_gui()
    robot_id = create_simple_arm()

    # 末端执行器一般是机械臂最后一个 link。
    # PyBullet 的关节编号从 0 开始，所以最后一个关节编号是 getNumJoints() - 1。
    end_effector = p.getNumJoints(robot_id) - 1

    # 这 3 个滑条不是控制关节，而是控制目标点的位置。
    # 你可以把它理解成：“我希望机械臂末端到达这里”。
    x_slider = p.addUserDebugParameter("target X", 0.2, 1.3, 0.8)
    y_slider = p.addUserDebugParameter("target Y", -0.8, 0.8, 0.0)
    z_slider = p.addUserDebugParameter("target Z", 0.1, 1.0, 0.4)

    # 创建一个红色小球，用来显示目标点。
    marker_visual = p.createVisualShape(
        p.GEOM_SPHERE,
        radius=0.04,
        rgbaColor=[1, 0.2, 0.2, 1],
    )
    marker_id = p.createMultiBody(baseMass=0, baseVisualShapeIndex=marker_visual)

    print("Move the target sliders. IK will calculate joint angles.")

    while p.isConnected():
        # 读取目标点坐标。
        target = [
            p.readUserDebugParameter(x_slider),
            p.readUserDebugParameter(y_slider),
            p.readUserDebugParameter(z_slider),
        ]

        # 把红色小球移动到目标点位置，方便观察。
        p.resetBasePositionAndOrientation(marker_id, target, [0, 0, 0, 1])

        # 逆运动学 IK:
        #   输入：末端目标位置 target
        #   输出：每个关节应该转到的角度
        #
        # 注意：IK 结果是弧度，不是度。
        joint_angles = p.calculateInverseKinematics(robot_id, end_effector, target)

        # 把 IK 算出来的角度发送给每个关节电机。
        for joint, angle in enumerate(joint_angles[: p.getNumJoints(robot_id)]):
            p.setJointMotorControl2(
                robot_id,
                joint,
                p.POSITION_CONTROL,
                targetPosition=angle,
                force=2.5,
                positionGain=0.08,
            )

        p.stepSimulation()
        time.sleep(1 / 240)


if __name__ == "__main__":
    main()
