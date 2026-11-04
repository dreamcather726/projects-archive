# SO-ARM100舵机校准使用指南（LeRobot方式）

## 🎯 校准方法说明

本项目采用**LeRobot**的校准方法，这是一种经过验证的、高效的校准流程。

### 核心方法

1. **`set_half_turn_homings()`** - 半圈零点法
   - 将机械臂移动到运动范围中间位置
   - 自动计算并设置零点偏移，使当前位置对应编码器的半圈值（2048）
   
2. **`record_ranges_of_motion()`** - 实时范围记录
   - 实时显示关节位置
   - 手动移动关节通过完整运动范围
   - 自动记录最小和最大位置

## 🚀 快速开始

### 运行校准程序

```bash
cd C:\Users\wuwu\Desktop\so_arm100_dev
python examples\calibrate_motors.py
```

### 校准流程（3步完成）

#### 步骤1：准备
- 连接机械臂到电脑
- 确保供电稳定（12V 5A以上）
- 运行校准程序

#### 步骤2：设置零点偏移
```
将机械臂移动到运动范围的中间位置，然后按回车键...
```
- 手动移动机械臂到运动范围的**中间位置**
- 所有关节都应该在中间位置
- 按回车键，程序自动设置**所有舵机**（包括wrist_roll）的零点偏移

#### 步骤3：记录运动范围
```
移动所有关节（除了'wrist_roll'）通过其完整的运动范围
实时记录位置，按 ENTER 键停止...
```
- 手动移动每个关节（除了wrist_roll）到其运动极限
- 观察实时表格确认范围已记录
- 按回车键完成
- **wrist_roll自动使用全范围（0-4095）**

#### 自动处理
程序会自动：
- 写入校准数据到舵机
- 保存校准文件

### 实时显示示例

```
-----------------------------------------------------------------
NAME            |    MIN |    POS |    MAX |    RANGE
-----------------------------------------------------------------
shoulder_pan    |    512 |   2048 |   3584 |     3072
shoulder_lift   |   1024 |   2100 |   3072 |     2048
elbow_flex      |    800 |   2000 |   3200 |     2400
wrist_flex      |   1200 |   2048 |   2896 |     1696
gripper         |    100 |    500 |   1000 |      900
```

## 📋 校准原理

### 半圈零点法（LeRobot方式）

**工作流程：**
1. 将机械臂移动到运动范围的中间位置
2. 记录当前位置 `Actual_Position`
3. 计算零点偏移：`homing_offset = Actual_Position - 2047`
4. 记录运动范围（除wrist_roll外）
5. wrist_roll使用全范围（0-4095）

**Feetech舵机位置关系：**
```
Present_Position = Actual_Position - Homing_Offset
```

**设置零点后：**
```
Present_Position = 2047（中间位置对应半圈值）
```

**优点：**
- 所有舵机都设置零点偏移（包括wrist_roll）
- 零点对应运动范围的中间位置
- 确保对称的正负运动范围
- wrist_roll可以360度连续旋转，使用全范围

### 实时范围记录

**工作方式：**
1. 程序持续读取关节位置（50Hz）
2. 实时更新最小值和最大值
3. 显示当前位置和记录范围
4. 用户按回车停止

**优点：**
- 直观可视化
- 实时反馈
- 确保完整覆盖运动范围

## 🔧 使用Python API

### 完整校准

```python
from core.motor_bus import MotorBus
from core.calibration import MotorCalibration

# 连接
bus = MotorBus("COM3", 1_000_000)
bus.connect()

# 创建校准工具
calibrator = MotorCalibration(bus)

# 执行完整校准
# 机械臂可以在任意初始位置
# 通过移动关节来标定运动范围
calibration_data = calibrator.calibrate_all_motors()

# 保存
calibrator.save_calibration()

# 断开
bus.disconnect()
```

### 单独使用各方法

```python
# 1. 设置零点（机械臂已在中间位置）
homing_offsets = calibrator.set_half_turn_homings()

# 2. 记录范围
motors_to_record = ["shoulder_pan", "shoulder_lift", "elbow_flex"]
mins, maxes = calibrator.record_ranges_of_motion(motors_to_record)

# 3. 显示结果
print(f"最小值: {mins}")
print(f"最大值: {maxes}")
```

## 📊 校准数据格式

保存的JSON文件格式：

```json
{
  "shoulder_pan": {
    "id": 1,
    "drive_mode": 0,
    "homing_offset": 150,
    "range_min": 512,
    "range_max": 3584
  },
  "shoulder_lift": {
    "id": 2,
    "drive_mode": 0,
    "homing_offset": 52,
    "range_min": 1024,
    "range_max": 3072
  },
  ...
}
```

**字段说明：**
- `id`: 舵机ID
- `drive_mode`: 驱动模式（0=正向）
- `homing_offset`: 零点偏移值
- `range_min`: 运动范围最小值
- `range_max`: 运动范围最大值

## ⚠️ 注意事项

### 校准前
- ✅ 确保所有舵机ID已正确设置（1-6）
- ✅ 检查供电电压和电流（推荐12V 5A）
- ✅ 机械臂已完整组装
- ✅ USB连接稳定

### 校准中
- ⚠️ 扭矩会自动关闭，请手扶机械臂
- ⚠️ 移动关节时要平缓，避免碰撞
- ⚠️ 不要超出机械硬限位
- ⚠️ 观察实时数据确认范围正确

### 校准后
- 📁 校准文件保存在 `calibration/so_arm100_calibration.json`
- 📝 零点偏移已写入舵机EEPROM
- ✅ 建议验证校准数据
- ✅ 测试机械臂运动

## 🔍 验证校准

### 使用程序验证

```python
calibrator.load_calibration()
calibrator.verify_calibration()
```

### 手动验证

1. 读取当前位置
2. 检查是否在校准范围内
3. 测试关节运动

## 🆚 LeRobot校准特点

### 所有舵机都设置零点
- **包括wrist_roll**在内的所有6个舵机都设置零点偏移
- 即使wrist_roll可以360度旋转，也需要设置零点

### wrist_roll的特殊处理
- 设置零点偏移：与其他舵机相同
- 运动范围：不需要记录，直接使用全范围（0-4095）
- 原因：wrist_roll是连续旋转关节，没有机械限位

### 校准顺序
1. **先设置零点**（所有舵机）
2. **再记录范围**（除wrist_roll外）
3. **wrist_roll全范围**（自动设置）

## 🐛 故障排查

### 实时表格不显示

**Windows系统：**
- 确认终端支持ANSI转义序列
- 使用Windows Terminal或VS Code终端

**解决方法：**
```python
# 关闭实时显示
mins, maxes = calibrator.record_ranges_of_motion(display_values=False)
```

### 范围记录不准确

**问题：** 某些关节的范围太小

**解决：**
- 确保移动到真正的极限位置
- 重新运行范围记录
- 检查机械是否有卡顿

### 零点偏移异常

**问题：** 偏移值超出合理范围（-2048到2048）

**原因：** 初始位置不在中间

**解决：**
- 重新调整到中间位置
- 重新运行零点设置

## 💡 最佳实践

### 推荐校准顺序
1. 先校准底座和主要关节
2. 最后校准末端和夹爪
3. wrist_roll自动使用全范围

### 运动范围建议
- 不要完全顶到机械限位
- 预留10-20个编码值的余量
- 确保关节运动流畅

### 校准频率
- 首次使用必须校准
- 更换舵机后重新校准
- 机械松动后重新校准
- 定期验证（每月一次）

## 📚 参考资料

- LeRobot官方文档：https://github.com/huggingface/lerobot
- LeRobot校准实现：`lerobot/src/lerobot/motors/motors_bus.py`
- SO-100校准示例：`lerobot/src/lerobot/robots/so100_follower/so100_follower.py`

## 🎓 下一步

校准完成后，你可以：

1. ✅ 测试位置控制
2. ✅ 实现机械臂控制接口
3. ✅ 录制演示轨迹
4. ✅ 训练控制策略
