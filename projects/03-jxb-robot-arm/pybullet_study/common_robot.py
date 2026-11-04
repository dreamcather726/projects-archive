import math
import time

import pybullet as p
import pybullet_data


LINK_LENGTHS = [0.45, 0.38, 0.32, 0.24, 0.18]
DEFAULT_POSE_DEG = [90, 0, 180, 130, 90]


def connect_gui():
    """打开 PyBullet GUI，并创建一个最基础的仿真世界。"""
    # GUI 模式会打开 3D 窗口，适合学习、调试、观察机械臂运动。
    p.connect(p.GUI)

    # pybullet_data 是官方自带资源库，里面有 plane.urdf 等模型。
    p.setAdditionalSearchPath(pybullet_data.getDataPath())

    # 设置重力，否则物体不会自然下落。
    p.setGravity(0, 0, -9.8)

    # 加载官方地面模型。
    p.loadURDF("plane.urdf")
    set_camera()


def set_camera():
    """设置一个适合观察机械臂的摄像机角度。"""
    p.resetDebugVisualizerCamera(
        cameraDistance=2.1,
        cameraYaw=45,
        cameraPitch=-32,
        cameraTargetPosition=[0.45, 0, 0.35],
    )


def create_simple_arm():
    """创建一个教学用 5 关节机械臂。

    真实项目更常用 URDF 文件描述机械臂。
    这里为了学习 PyBullet API，故意用 createMultiBody 手动拼一个机械臂。
    """
    # 下面这些列表是一一对应的：
    # 第 0 个元素描述第 0 个 link，第 1 个元素描述第 1 个 link，以此类推。
    visual_shapes = []
    collision_shapes = []
    masses = []
    positions = []
    orientations = []
    inertial_positions = []
    inertial_orientations = []
    parent_indices = []
    joint_types = []
    joint_axes = []

    for index, length in enumerate(LINK_LENGTHS):
        # 每节机械臂用一个细长方块表示。
        # halfExtents 是半尺寸，所以 x 方向写 length / 2。
        color = [0.2, 0.85 - index * 0.08, 1.0, 1.0]
        half_extents = [length / 2, 0.035, 0.035]

        # visual shape 控制外观。
        visual_shapes.append(
            p.createVisualShape(p.GEOM_BOX, halfExtents=half_extents, rgbaColor=color)
        )

        # collision shape 控制碰撞。
        collision_shapes.append(
            p.createCollisionShape(p.GEOM_BOX, halfExtents=half_extents)
        )

        # link 质量。质量越大，电机需要更大的 force 才能推动。
        masses.append(0.2)

        # linkPositions 是每个 link 相对于它父关节的位置。
        # 这里把 link 中心放在 x 方向 length / 2 处，让关节位于 link 的一端。
        positions.append([length / 2, 0, 0])

        # 四元数 [0,0,0,1] 表示没有旋转。
        orientations.append([0, 0, 0, 1])

        # 惯性坐标系先保持默认即可。初学阶段可以先不深究。
        inertial_positions.append([0, 0, 0])
        inertial_orientations.append([0, 0, 0, 1])

        # In createMultiBody, link 0 uses parent index 0 for the base.
        # 这个参数容易让人困惑：
        #   第 0 节 link 的父级是 base，所以写 0。
        #   第 1 节 link 的父级是第 0 节 link，所以写 1。
        #   第 2 节 link 的父级是第 1 节 link，所以写 2。
        parent_indices.append(index)

        # 关节类型：JOINT_REVOLUTE 表示旋转关节，类似舵机/电机轴。
        joint_types.append(p.JOINT_REVOLUTE)

        # Alternate axes to make the arm move in 3D.
        # 关节轴决定“绕哪个方向旋转”。
        # [0,0,1] 是绕 z 轴转，[0,1,0] 是绕 y 轴转。
        joint_axes.append([0, 0, 1] if index in (0, 3) else [0, 1, 0])

    # 创建一个圆柱底座。
    # baseMass=0 时，底座是固定的，不会被重力拉走。
    base_collision = p.createCollisionShape(p.GEOM_CYLINDER, radius=0.11, height=0.1)
    base_visual = p.createVisualShape(
        p.GEOM_CYLINDER,
        radius=0.11,
        length=0.1,
        rgbaColor=[0.9, 0.9, 0.95, 1],
    )

    # createMultiBody 是手动创建机器人结构的核心 API。
    # 它把 base 和多个 link/关节一次性创建出来。
    robot_id = p.createMultiBody(
        baseMass=0,
        baseCollisionShapeIndex=base_collision,
        baseVisualShapeIndex=base_visual,
        basePosition=[0, 0, 0.08],
        linkMasses=masses,
        linkCollisionShapeIndices=collision_shapes,
        linkVisualShapeIndices=visual_shapes,
        linkPositions=positions,
        linkOrientations=orientations,
        linkInertialFramePositions=inertial_positions,
        linkInertialFrameOrientations=inertial_orientations,
        linkParentIndices=parent_indices,
        linkJointTypes=joint_types,
        linkJointAxis=joint_axes,
    )

    # 默认情况下 PyBullet 会给关节一些默认电机行为。
    # 这里先关闭速度控制，后面我们自己用 POSITION_CONTROL 控制它。
    for joint in range(p.getNumJoints(robot_id)):
        p.setJointMotorControl2(robot_id, joint, p.VELOCITY_CONTROL, force=0)

    return robot_id


def add_joint_sliders(robot_id):
    """为每个关节创建一个 GUI 滑条。"""
    sliders = []
    joint_count = p.getNumJoints(robot_id)
    for index, value in enumerate(DEFAULT_POSE_DEG[:joint_count]):
        # addUserDebugParameter 会在 PyBullet 左侧调试面板里加一个滑条。
        # 参数分别是：名字、最小值、最大值、默认值。
        slider = p.addUserDebugParameter(f"J{index + 1} angle", 0, 180, value)
        sliders.append(slider)
    return sliders


def read_slider_degrees(sliders):
    """读取所有滑条的当前值，返回单位是度。"""
    return [p.readUserDebugParameter(slider) for slider in sliders]


def set_joint_targets_deg(robot_id, angles_deg, force=2.0):
    """把角度命令发送给每个关节电机。

    我们给用户看的角度单位是“度”，比如 90 度。
    PyBullet 内部电机需要“弧度”，所以这里要转换。
    """
    for joint, angle_deg in enumerate(angles_deg[: p.getNumJoints(robot_id)]):
        # 这里减 90 是为了让滑条 90 度对应机械臂的中间姿态。
        angle_rad = math.radians(angle_deg - 90)

        # POSITION_CONTROL 表示位置控制：
        #   targetPosition: 目标角度，单位弧度
        #   force: 电机最大力，太小会推不动，太大可能过猛
        #   positionGain: 位置控制增益，越大越积极
        p.setJointMotorControl2(
            robot_id,
            joint,
            p.POSITION_CONTROL,
            targetPosition=angle_rad,
            force=force,
            positionGain=0.08,
        )


def step_simulation(seconds=None):
    """推进仿真。seconds=None 表示一直运行到窗口关闭。"""
    start = time.time()
    while p.isConnected():
        p.stepSimulation()
        time.sleep(1 / 240)
        if seconds is not None and time.time() - start >= seconds:
            break
