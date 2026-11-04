import pybullet as p


def main():
    # 这个文件只做一件事：确认 pybullet 能正常 import 和连接。
    #
    # p.connect() 是使用 PyBullet 的第一步。
    #
    # p.DIRECT:
    #   后台模式，不打开 3D 窗口。
    #   适合检查环境、自动测试、强化学习训练。
    #
    # p.GUI:
    #   会打开 3D 窗口。
    #   适合我们后面学习机械臂仿真。
    physics_client = p.connect(p.DIRECT)

    # 如果上面没有报错，说明 pybullet 安装成功。
    # physics_client 是连接编号，通常从 0 开始。
    print("PyBullet is installed.")
    print("Physics client id:", physics_client)

    # getAPIVersion() 可以看到当前 PyBullet API 版本。
    print("API version:", p.getAPIVersion())

    # 用完后断开连接。养成这个习惯，后面写复杂程序会更稳。
    p.disconnect()


if __name__ == "__main__":
    main()
