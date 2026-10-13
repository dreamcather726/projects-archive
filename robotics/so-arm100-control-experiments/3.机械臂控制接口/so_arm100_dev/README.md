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
├── README.md                      # 项目说明
├── requirements.txt               # 依赖包
├── 安装说明.md                    # 安装指南
├── 使用指南.md                    # ID设置使用指南
├── 校准使用指南.md                # 校准使用指南
├── config/
│   └── motor_config.py           # 舵机配置
├── core/
│   ├── motor_bus.py              # 舵机总线通信
│   ├── motor_setup.py            # 舵机ID设置
│   └── calibration.py            # 舵机校准
├── examples/
│   ├── setup_motor_ids.py        # ID设置示例
│   ├── calibrate_motors.py       # 校准示例
│   └── test_calibration.py       # 校准测试
├── utils/
│   └── port_finder.py            # 串口查找工具
└── calibration/                  # 校准数据目录
    └── so_arm100_calibration.json
```

## 安装依赖
```bash
pip install -r requirements.txt
```

## 使用说明

### 快速开始

1. **安装依赖**
   ```bash
   pip install -r requirements.txt
   ```
   详见：`安装说明.md`

2. **设置舵机ID**
   ```bash
   python examples\setup_motor_ids.py
   ```
   详见：`使用指南.md`

3. **校准舵机**
   ```bash
   python examples\calibrate_motors.py
   ```
   详见：`校准使用指南.md`

4. **测试校准数据**
   ```bash
   python examples\test_calibration.py
   ```

### 开发流程

1. ✅ **舵机ID设置** - 为6个舵机分配正确的ID（1-6）
2. ✅ **舵机校准** - 记录零点位置和运动范围
3. ⏭️ **机械臂控制** - 实现位置控制和轨迹规划
4. ⏭️ **数据采集** - 录制演示轨迹
5. ⏭️ **模型训练** - 训练控制策略
