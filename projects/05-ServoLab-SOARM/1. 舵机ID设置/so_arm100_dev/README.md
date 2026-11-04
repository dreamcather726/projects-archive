# SO-ARM100 6轴机械臂独立开发项目

## 项目说明
本项目为SO-ARM100 6轴机械臂的独立控制系统，使用STS3215飞特舵机。

## 硬件配置
- 6个STS3215舵机（12位分辨率，4096）
- Feetech Protocol 0通信协议
- 默认波特率：1,000,000 bps

## 舵机ID分配
- ID1: shoulder_pan（底座旋转）
- ID2: shoulder_lift（肩部俯仰）
- ID3: elbow_flex（肘部）
- ID4: wrist_flex（腕部俯仰）
- ID5: wrist_roll（腕部旋转）
- ID6: gripper（夹爪）

## 项目结构
```
so_arm100_dev/
├── README.md
├── requirements.txt
├── config/
│   └── motor_config.py       # 舵机配置
├── core/
│   ├── motor_bus.py          # 舵机总线通信
│   └── motor_setup.py        # 舵机ID设置
├── examples/
│   └── setup_motor_ids.py    # ID设置示例
└── utils/
    └── port_finder.py        # 串口查找工具
```

## 安装依赖
```bash
pip install -r requirements.txt
```

## 使用说明
参见 examples/ 目录下的示例代码
