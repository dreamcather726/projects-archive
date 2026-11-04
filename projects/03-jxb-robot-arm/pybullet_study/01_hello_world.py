import time

import pybullet as p #PyBullet 的官方 Python 包，提供了物理仿真功能。
import pybullet_data 


def main():
    # 1. connect() 用来连接 PyBullet 物理服务器。
    #
    # p.GUI:
    #   打开一个可视化窗口，适合学习和调试。
    #
    # p.DIRECT:
    #   不打开窗口，只在后台运行物理仿真，适合自动测试或训练。
    p.connect(p.GUI)

    # 2. 设置官方资源路径。
    #
    # pybullet_data 是 PyBullet 安装时自带的一批官方示例资源，
    # 里面有 plane.urdf、r2d2.urdf、kuka_iiwa、franka_panda 等模型。
    #
    # 设置后，loadURDF("plane.urdf") 就不需要写完整路径。
    p.setAdditionalSearchPath(pybullet_data.getDataPath())

    # 3. 设置重力。
    #
    # 参数是 x、y、z 三个方向的重力加速度。
    # 地球表面重力大约是 -9.8 m/s^2，所以 z 方向写 -9.8。
    p.setGravity(0, 0, -9.8)

    # 4. 加载地面。
    #
    # URDF 是机器人仿真里常见的模型描述文件。
    # plane.urdf 是 PyBullet 官方数据包里的无限大地面。
    p.loadURDF("plane.urdf")

    # 5. 加载一个官方示例模型 R2D2。
    #
    # 第二个参数 [0, 0, 1] 是模型初始位置：
    #   x = 0
    #   y = 0
    #   z = 1
    #
    # z=1 表示先把 R2D2 放在空中，运行后你会看到它落到地面上。
    p.loadURDF("r2d2.urdf", [0, 0, 0.5])

    # 6. 设置摄像机视角。
    #
    # cameraDistance: 镜头距离目标点多远
    # cameraYaw:      水平旋转角度
    # cameraPitch:    俯仰角，负数表示从上往下看
    # cameraTargetPosition: 镜头看向哪个点
    p.resetDebugVisualizerCamera(
        cameraDistance=1,
        cameraYaw=45,
        cameraPitch=-35,
        cameraTargetPosition=[0, 0, 0],
    )

    # 7. 主循环。
    #
    # PyBullet 不会自己一直计算物理世界。
    # 每调用一次 stepSimulation()，仿真世界才前进一步。
    #
    # 这里用 1/240 秒，是因为 PyBullet 官方例子里常用 240Hz 作为仿真频率。
    # 如果没有 time.sleep()，仿真会跑得非常快，不方便观察。
    while p.isConnected():
        p.stepSimulation()
        time.sleep(1 / 600)


if __name__ == "__main__":
    main()
