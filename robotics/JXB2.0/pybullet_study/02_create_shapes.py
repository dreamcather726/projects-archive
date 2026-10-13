import time

import pybullet as p
import pybullet_data


def create_box(position, color):
    # createVisualShape 只负责“看起来是什么样”。
    # 它决定颜色、形状、大小，但不参与碰撞计算。
    visual = p.createVisualShape(
        p.GEOM_BOX,
        halfExtents=[0.25, 0.25, 0.25],
        rgbaColor=color,
    )

    # createCollisionShape 负责“物理碰撞是什么形状”。
    # 如果只有 visual，没有 collision，物体看得见但不会真实碰撞。
    collision = p.createCollisionShape(
        p.GEOM_BOX,
        halfExtents=[0.25, 0.25, 0.25],
    )

    # createMultiBody 把质量、碰撞形状、显示形状和初始位置组合成一个物体。
    # baseMass=1 表示它受重力影响，会掉落。
    # 如果 baseMass=0，它就是固定不动的静态物体。
    return p.createMultiBody(
        baseMass=1,
        baseCollisionShapeIndex=collision,
        baseVisualShapeIndex=visual,
        basePosition=position,
    )


def create_cylinder(position):
    # 圆柱体也同样需要 visual shape 和 collision shape。
    visual = p.createVisualShape(
        p.GEOM_CYLINDER,
        radius=0.18,
        length=0.5,
        rgbaColor=[0.2, 0.8, 1.0, 1],
    )
    collision = p.createCollisionShape(
        p.GEOM_CYLINDER,
        radius=0.18,
        height=0.5,
    )
    return p.createMultiBody(
        baseMass=1,
        baseCollisionShapeIndex=collision,
        baseVisualShapeIndex=visual,
        basePosition=position,
    )


def main():
    # 打开 3D 窗口，方便观察物体下落和碰撞。
    p.connect(p.GUI)

    # 使用 PyBullet 官方自带资源路径，这里主要用 plane.urdf。
    p.setAdditionalSearchPath(pybullet_data.getDataPath())

    # 开启重力。没有重力的话，物体会悬在空中。
    p.setGravity(0, 0, -9.8)

    # 加载官方地面模型。
    p.loadURDF("plane.urdf")

    # 摄像机参数也可以用简写形式：
    # resetDebugVisualizerCamera(distance, yaw, pitch, target)
    p.resetDebugVisualizerCamera(3, 40, -30, [0, 0, 0.4])

    # 创建 3 个物体，初始 z 坐标大于 0，所以运行后会落到地面上。
    create_box([-0.6, 0, 1.0], [1, 0.4, 0.2, 1])
    create_box([0, 0, 1.4], [0.4, 1, 0.4, 1])
    create_cylinder([0.6, 0, 1.0])

    # 每一步 stepSimulation 都会更新位置、速度、碰撞和重力。
    while p.isConnected():
        p.stepSimulation()
        time.sleep(1 / 240)


if __name__ == "__main__":
    main()
