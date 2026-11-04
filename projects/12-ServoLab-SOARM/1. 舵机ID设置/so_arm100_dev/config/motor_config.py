# -*- coding: utf-8 -*-
"""
SO-ARM100机械臂舵机配置文件
"""

# 舵机型号配置
MOTOR_MODEL = "sts3215"
MOTOR_RESOLUTION = 4096  # 12位编码器

# 通信协议配置
PROTOCOL_VERSION = 0  # Feetech Protocol 0
DEFAULT_BAUDRATE = 1_000_000  # 1Mbps
TIMEOUT_MS = 1000

# 舵机ID配置（按照机械臂关节顺序）
MOTOR_IDS = {
    "shoulder_pan": 1,   # 底座旋转关节
    "shoulder_lift": 2,  # 肩部俯仰关节
    "elbow_flex": 3,     # 肘部关节
    "wrist_flex": 4,     # 腕部俯仰关节
    "wrist_roll": 5,     # 腕部旋转关节
    "gripper": 6,        # 夹爪
}

# 控制表地址（STS3215舵机）
CONTROL_TABLE = {
    "Model_Number": (3, 2),
    "ID": (5, 1),
    "Baud_Rate": (6, 1),
    "Operating_Mode": (33, 1),
    "Torque_Enable": (40, 1),
    "Goal_Position": (42, 2),
    "Present_Position": (56, 2),
    "Present_Load": (60, 2),
    "Present_Voltage": (62, 1),
    "Present_Temperature": (63, 1),
    "Status": (65, 1),
    "Moving": (66, 1),
}

# 波特率表
BAUDRATE_TABLE = {
    1_000_000: 0,
    500_000: 1,
    250_000: 2,
    128_000: 3,
    115_200: 4,
    57_600: 5,
    38_400: 6,
    19_200: 7,
}

# 扫描波特率列表（按常用顺序）
SCAN_BAUDRATES = [
    1_000_000,
    115_200,
    57_600,
    500_000,
    250_000,
    128_000,
    38_400,
    19_200,
]

# 操作模式
class OperatingMode:
    POSITION = 0  # 位置伺服模式
    VELOCITY = 1  # 恒速模式
    PWM = 2       # PWM开环模式
    STEP = 3      # 步进模式

# 扭矩模式
class TorqueMode:
    DISABLED = 0
    ENABLED = 1
