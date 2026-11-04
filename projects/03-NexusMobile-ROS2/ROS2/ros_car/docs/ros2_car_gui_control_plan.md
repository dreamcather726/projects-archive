# ROS2 小车 GUI 上位机完整方案

## 1. 目标

这个 GUI 的目标不是简单做一个遥控器，而是做一个“整车控制台”：

- 中间显示 RViz2 视图，用来观察雷达点云、地图、TF、小车模型、导航路径和代价地图。
- 左侧显示小车实时状态，例如速度、电池、电机 PWM、编码器、IMU、雷达、串口连接状态。
- 右侧提供控制区，例如游戏手柄式小车控制、速度限制、模式切换、PID 参数调节、机械臂控制。
- 底部提供安全状态，例如急停、通信延迟、当前控制来源、ROS2 topic 在线状态。
- 后续可以接入 PyBullet，让电脑端先在仿真里控制机械臂，再把目标角度发送给真实机械臂。

推荐把它做成 Ubuntu 上运行的 Python + ROS2 + Qt 上位机。电脑负责“大脑和界面”，MusePi 负责“车载 ROS2 桥接和传感器”，ESP32 负责“实时电机和舵机控制”。

## 2. 总体架构

```text
┌────────────────────────────────────────────────────────────┐
│ Ubuntu 电脑                                                  │
│                                                            │
│  ┌──────────────────────┐      ┌──────────────────────┐    │
│  │ PyQt/PySide GUI       │      │ RViz2                 │    │
│  │ - 控制小车             │      │ - /scan 雷达           │    │
│  │ - 显示参数             │      │ - /map 地图            │    │
│  │ - 控制机械臂           │      │ - /odom 里程计          │    │
│  │ - 急停                 │      │ - /tf 坐标树           │    │
│  └──────────┬───────────┘      └──────────┬───────────┘    │
│             │ ROS2 topic/service/action    │                │
└─────────────┼──────────────────────────────┼────────────────┘
              │                              │
              │ WiFi / Ethernet              │
              │                              │
┌─────────────▼──────────────────────────────▼────────────────┐
│ MusePi                                                       │
│ - 运行 ros_car 工作区                                        │
│ - 雷达驱动发布 /scan                                         │
│ - ESP32 串口桥接节点发布 /odom、/imu、/wheel_state             │
│ - 订阅 /cmd_vel、/arm/servo_targets、/emergency_stop          │
└─────────────┬───────────────────────────────────────────────┘
              │ USB 串口
              │
┌─────────────▼───────────────────────────────────────────────┐
│ ESP32S3 下位机                                                │
│ - 电机 PID                                                    │
│ - 编码器读取                                                  │
│ - IMU 读取                                                    │
│ - PCA9685 机械臂舵机控制                                      │
│ - 安全互锁：机械臂动作时限制或禁止小车运动                     │
└─────────────────────────────────────────────────────────────┘
```

## 3. 为什么推荐这种方案

### 3.1 不建议把所有控制都塞进 ESP32

ESP32 适合做实时控制，例如电机 PWM、编码器、PID、PCA9685 舵机输出。但它不适合负责复杂 GUI、RViz2、PyBullet、地图和导航。

如果 ESP32 同时处理 WiFi 网页、小车速度、机械臂、ROS2 通信和安全策略，后续会越来越乱，调试也会很痛苦。

### 3.2 不建议电脑直接分两路控制小车和机械臂

例如电脑通过 ROS2 控小车，同时又通过 UDP/WiFi 直接控机械臂，这样会出现两个控制入口：

- 小车速度由 ROS2 下发。
- 机械臂角度由 UDP 直接下发。
- 小车和机械臂之间的安全互锁很难统一。

更好的方式是：电脑 GUI 统一发 ROS2 指令，MusePi 统一转发给 ESP32，ESP32 只接收一个串口控制入口。

### 3.3 推荐电脑 Ubuntu 做主控

Ubuntu 里运行 ROS2、RViz2、PyBullet 和 Python GUI 最稳定。这样后续要做建图、导航、机械臂仿真、数据记录和可视化都比较顺。

## 4. GUI 界面布局

建议第一版界面使用三栏结构：

```text
┌──────────────────────────────────────────────────────────────────────────────┐
│ 顶部状态栏：连接状态 | 当前模式 | 急停状态 | 电池 | 延迟 | 时间               │
├───────────────┬──────────────────────────────────────────────┬───────────────┤
│ 左侧状态面板    │ 中间 RViz2 大屏                               │ 右侧控制面板    │
│               │                                              │               │
│ 速度           │ /scan 雷达                                    │ 手柄控制        │
│ 电机 PWM       │ /map 地图                                     │ 速度限制        │
│ 编码器         │ /odom 轨迹                                    │ 模式切换        │
│ IMU            │ /tf 坐标                                      │ 机械臂控制      │
│ 雷达状态        │ RobotModel 小车模型                           │ PID 调参        │
│ 串口状态        │ Nav2 路径/代价地图                             │ 参数保存        │
│ ROS2 topic     │                                              │               │
├───────────────┴──────────────────────────────────────────────┴───────────────┤
│ 底部安全栏：急停按钮 | 小车速度上限 | 机械臂互锁状态 | 日志输出               │
└──────────────────────────────────────────────────────────────────────────────┘
```

### 4.1 顶部状态栏

显示整车当前是否健康：

- ROS2 Domain ID。
- MusePi 是否在线。
- ESP32 串口是否在线。
- 雷达是否在线。
- 当前控制模式：手动、建图、导航、机械臂。
- 急停状态。
- 最近一次数据更新时间。

### 4.2 左侧状态面板

用于显示车辆参数，不直接控制：

- 左轮速度、右轮速度，单位 `m/s` 或 `cm/s`。
- 线速度、角速度。
- 左右电机 PWM。
- 编码器计数。
- IMU 姿态角或加速度、角速度。
- 雷达频率、雷达点数。
- 里程计坐标 `x`、`y`、`yaw`。
- 串口收发统计。
- 电池电压，如果后续加电压检测模块。

### 4.3 中间 RViz2 大屏

第一版不建议直接“魔改 RViz2”，而是让 GUI 自动启动 RViz2，并加载固定配置文件。

RViz2 中建议固定显示：

- `LaserScan`：显示 `/scan`。
- `Map`：显示 `/map`。
- `Odometry`：显示 `/odom`。
- `TF`：显示 `map -> odom -> base_link -> laser_link`。
- `RobotModel`：显示小车 URDF。
- `Path`：显示 Nav2 规划路径。
- `Costmap`：显示局部和全局代价地图。

第一版可以让 RViz2 独立窗口运行，GUI 窗口放在旁边。第二版再考虑把 RViz2 嵌进 GUI 中。

### 4.4 右侧控制面板

控制面板建议分成几个 Tab：

```text
Tab 1：小车
- 虚拟摇杆
- 前进、后退、左转、右转、停止按钮
- 最大线速度
- 最大角速度
- 当前 cmd_vel 显示

Tab 2：机械臂
- 抓取
- 复位
- 停止
- 五个舵机角度滑条
- PyBullet 同步开关
- 机械臂忙碌状态

Tab 3：PID
- Kp
- Ki
- Kd
- 应用参数
- 恢复默认
- 保存当前参数

Tab 4：系统
- 连接 MusePi
- 启动/停止雷达
- 启动/停止建图
- 保存地图
- 启动/停止导航
- 日志窗口
```

## 5. 控制逻辑设计

### 5.1 小车控制

GUI 不应该直接控制左右轮 PWM，而是发布标准 ROS2 速度话题：

```text
/cmd_vel
类型：geometry_msgs/msg/Twist
```

例如：

- `linear.x` 表示前进后退速度，单位 `m/s`。
- `angular.z` 表示旋转速度，单位 `rad/s`。

MusePi 上的桥接节点订阅 `/cmd_vel`，再转换成 ESP32 串口命令。

### 5.2 机械臂控制

GUI 发布机械臂目标角度：

```text
/arm/servo_targets
类型：std_msgs/msg/Int16MultiArray
内容：[servo0, servo1, servo2, servo3, servo4]
```

也可以增加动作指令：

```text
/arm/command
类型：std_msgs/msg/String
内容：grab、reset、stop
```

MusePi 桥接节点把这些指令转换成 ESP32 串口帧。

### 5.3 急停控制

急停必须是最高优先级：

```text
/emergency_stop
类型：std_msgs/msg/Bool
```

当 `/emergency_stop = true` 时：

- GUI 停止发送速度。
- MusePi 桥接节点向 ESP32 下发停车。
- ESP32 立即清空目标速度、清 PID、PWM 输出为 0。
- 机械臂可以根据实物安全策略选择保持当前位置或停止更新目标。

### 5.4 机械臂和小车互锁

你现在的需求有两个版本，需要明确区分：

第一种，最安全版本：

- 只有小车完全停下时才能控制机械臂。
- 机械臂运动期间小车不能动。
- 适合刚开始实机测试。

第二种，低速协同版本：

- 机械臂控制期间小车最大速度不超过 `0.2 m/s`。
- 超过速度上限时 GUI 和 MusePi 都要限速。
- 适合后续做边走边抓、巡检或慢速靠近目标。

建议第一版先做最安全版本，跑稳定后再开放第二种低速协同模式。

## 6. ROS2 话题设计

### 6.1 GUI 发布的话题

| 话题 | 类型 | 作用 |
| --- | --- | --- |
| `/cmd_vel` | `geometry_msgs/msg/Twist` | 控制小车速度 |
| `/arm/servo_targets` | `std_msgs/msg/Int16MultiArray` | 设置五路机械臂舵机角度 |
| `/arm/command` | `std_msgs/msg/String` | 抓取、复位、停止 |
| `/emergency_stop` | `std_msgs/msg/Bool` | 急停 |
| `/pid/set` | 自定义或 `Float32MultiArray` | 设置速度环 PID |

### 6.2 GUI 订阅的话题

| 话题 | 类型 | 作用 |
| --- | --- | --- |
| `/scan` | `sensor_msgs/msg/LaserScan` | 雷达数据 |
| `/odom` | `nav_msgs/msg/Odometry` | 小车里程计 |
| `/imu` | `sensor_msgs/msg/Imu` | IMU 数据 |
| `/wheel_state` | 自定义消息 | 左右轮速度、PWM、编码器 |
| `/arm/state` | 自定义或 `Int16MultiArray` | 当前机械臂角度和忙碌状态 |
| `/diagnostics` | `diagnostic_msgs/msg/DiagnosticArray` | 系统诊断 |

### 6.3 建议后续新增自定义消息

后续如果参数越来越多，建议建一个专门的消息包，例如 `ros_car_interfaces`：

```text
ros_car_interfaces/msg/WheelState.msg
ros_car_interfaces/msg/ArmState.msg
ros_car_interfaces/msg/CarStatus.msg
ros_car_interfaces/srv/SetPid.srv
```

第一版可以先不用自定义消息，先用标准消息跑通。

## 7. RViz2 集成方案

### 7.1 第一版：GUI 启动独立 RViz2 窗口

这是最推荐的第一版。

优点：

- 稳定。
- 开发快。
- 不需要研究 RViz2 插件机制。
- RViz2 崩了也不影响 GUI 本体。

GUI 启动 RViz2 的命令类似：

```bash
rviz2 -d ~/ros_car/src/my_pkg/rviz/ros_car.rviz
```

GUI 只负责：

- 启动 RViz2。
- 关闭 RViz2。
- 显示 ROS2 数据。
- 发布控制命令。

### 7.2 第二版：GUI 嵌入 RViz2 窗口

这种方式更像“完整游戏控制台”，但实现复杂。

可选做法：

- 使用 Qt 查找 RViz2 窗口 ID，然后嵌入 PyQt 界面。
- 或者写 RViz2 Panel 插件，把控制面板做成 RViz2 插件。

缺点：

- 不同系统窗口管理器行为不同。
- 调试成本高。
- RViz2 插件通常需要 C++ 和 Qt。

### 7.3 第三版：不用 RViz2，自己用 Python 画雷达和地图

这种适合轻量显示，但不建议第一版做。

Python 可以订阅 `/scan`，用 PyQtGraph 或 Matplotlib 画雷达点；也可以订阅 `/map` 画二维栅格地图。

缺点是需要自己实现很多 RViz2 已经做好的功能，例如 TF、地图坐标转换、RobotModel、Path、Costmap 显示。

## 8. 技术选型

### 8.1 GUI 框架

推荐：

```text
Python + PySide6
```

原因：

- Python 和 ROS2 的 `rclpy` 配合方便。
- PySide6 做桌面 GUI 稳定。
- 后续接 PyBullet 方便。
- 可以做按钮、滑条、表格、仪表盘、日志窗口。

备选：

- PyQt5：兼容性好，但授权和版本要注意。
- PyQtGraph：适合实时曲线，例如速度、电压、IMU 曲线。
- DearPyGui：更像游戏工具界面，但和 ROS2/Qt/RViz2 集成不如 PySide6 顺。

### 8.2 数据可视化

建议：

- RViz2 负责地图、雷达、TF、导航。
- PyQtGraph 负责曲线，例如速度曲线、电机 PWM 曲线、IMU 曲线。
- QLabel/QTableWidget 负责状态文本和参数表。

### 8.3 控制输入

第一版：

- 键盘控制。
- GUI 按钮控制。
- 虚拟摇杆。

第二版：

- 接 Xbox/PS 手柄。
- 使用 ROS2 `joy` 包，把手柄输入变成 `/joy`。
- GUI 显示手柄连接状态。

## 9. 软件包规划

建议在当前 ROS2 工作区里新增一个 GUI 包：

```text
src/ros_car_gui/
├── package.xml
├── setup.py
├── resource/
│   └── ros_car_gui
├── ros_car_gui/
│   ├── __init__.py
│   ├── main_window.py
│   ├── ros_node.py
│   ├── car_control_panel.py
│   ├── arm_control_panel.py
│   ├── status_panel.py
│   ├── pid_panel.py
│   ├── rviz_launcher.py
│   └── styles.py
└── launch/
    └── gui.launch.py
```

职责划分：

- `main_window.py`：主窗口布局。
- `ros_node.py`：ROS2 发布和订阅。
- `car_control_panel.py`：小车控制。
- `arm_control_panel.py`：机械臂控制。
- `status_panel.py`：状态显示。
- `pid_panel.py`：PID 调参。
- `rviz_launcher.py`：启动和关闭 RViz2。
- `styles.py`：统一界面样式。

## 10. MusePi 桥接节点需要做什么

MusePi 是电脑和 ESP32 的中间层。

桥接节点建议负责：

- 订阅 `/cmd_vel`，转换为 ESP32 的左右轮目标速度。
- 订阅 `/arm/servo_targets`，转换为 ESP32 机械臂角度帧。
- 订阅 `/arm/command`，转换为抓取、复位、停止。
- 订阅 `/emergency_stop`，立刻发送停车。
- 读取 ESP32 返回的编码器、IMU、机械臂状态。
- 发布 `/odom`、`/imu`、`/wheel_state`、`/arm/state`。

这样 GUI 不需要知道 ESP32 串口协议细节。

## 11. ESP32 下位机需要配合什么

ESP32 侧建议保留实时控制职责：

- 接收小车速度命令。
- 接收机械臂角度命令。
- 接收 PID 参数命令。
- 执行机械臂和小车互锁。
- 周期返回编码器、IMU、速度、电机 PWM、机械臂状态。

机械臂互锁建议放在 ESP32 和 MusePi 两层：

- GUI 层：用户操作时给出提示和限速。
- MusePi 层：转换指令时做一次安全判断。
- ESP32 层：最终硬件保护，绝不完全依赖上位机。

## 12. 机械臂与 PyBullet 的结合

PyBullet 更适合放在 Ubuntu 电脑端。

推荐流程：

```text
PyBullet 计算机械臂目标角度
        │
        ▼
GUI 显示仿真姿态和目标角度
        │
        ▼
发布 /arm/servo_targets
        │
        ▼
MusePi 串口桥接
        │
        ▼
ESP32 控制 PCA9685
        │
        ▼
真实机械臂动作
```

这样做的好处：

- PyBullet 不需要直接知道 ESP32 的 IP。
- 安全互锁统一经过 ROS2。
- GUI 可以同时显示仿真角度和真实反馈角度。
- 后续可以做“仿真确认后再执行”。

## 13. 安全策略

必须优先做这些安全功能：

### 13.1 急停

GUI 上必须有明显急停按钮。

急停后：

- `/cmd_vel` 发送 0。
- ESP32 目标速度清 0。
- 电机 PWM 输出 0。
- PID 历史清空。
- GUI 显示红色急停状态。

### 13.2 控制权管理

同一时间只能有一个控制源：

- GUI 手动控制。
- 键盘控制。
- Nav2 自动导航。
- 机械臂协同模式。

切换模式时必须先停车。

### 13.3 机械臂动作保护

第一版建议规则：

- 小车速度接近 0 时才允许抓取。
- 机械臂忙碌时 GUI 禁止发送小车速度。
- 如果小车正在动，点击抓取按钮只提示，不执行。

第二版可以开放：

- 机械臂控制期间小车速度上限 `0.2 m/s`。
- GUI、MusePi、ESP32 三层都做限速。

### 13.4 通信超时

如果 MusePi 超过一定时间没有收到 GUI 的速度命令，应自动停车。

建议：

- `/cmd_vel` 超时 `500 ms` 自动停车。
- ESP32 串口命令超时 `500 ms` 自动停车。
- GUI 超过 `1 s` 没有收到状态数据，显示通信异常。

## 14. 开发阶段计划

### 阶段 1：先做可运行 GUI 空壳

目标：

- 能打开 PySide6 主窗口。
- 有三栏布局。
- 有状态栏、控制面板和日志区域。
- 能启动 RViz2 独立窗口。

验收：

- Ubuntu 运行 `ros2 run ros_car_gui gui_app` 能打开界面。
- 点击按钮能启动 RViz2。

### 阶段 2：接入 ROS2 小车控制

目标：

- GUI 发布 `/cmd_vel`。
- 支持按钮控制前后左右停。
- 支持速度上限。
- 支持急停。

验收：

- `ros2 topic echo /cmd_vel` 能看到速度命令。
- 小车能按 GUI 控制运动。
- 急停能立即停车。

### 阶段 3：接入状态显示

目标：

- 订阅 `/odom`、`/imu`、`/scan`。
- 显示速度、位置、IMU、雷达在线状态。
- 显示通信延迟。

验收：

- 雷达启动时 GUI 显示在线。
- 推车或运行小车时里程计变化。
- IMU 数据能实时刷新。

### 阶段 4：接入机械臂控制

目标：

- GUI 提供抓取、复位、停止按钮。
- GUI 提供五路舵机滑条。
- 发布 `/arm/servo_targets` 和 `/arm/command`。
- 加入“小车未停稳禁止抓取”的逻辑。

验收：

- 小车停下时机械臂能抓取。
- 小车运动时抓取按钮不可执行。
- 机械臂动作期间小车速度命令被禁止或限速。

### 阶段 5：接入 PID 调参

目标：

- GUI 能显示当前 PID 参数。
- GUI 能修改 `Kp`、`Ki`、`Kd`。
- MusePi 桥接节点把参数发给 ESP32。

验收：

- 修改 PID 后 ESP32 串口返回确认。
- 速度响应变化符合预期。

### 阶段 6：接入 PyBullet

目标：

- GUI 启动 PyBullet。
- PyBullet 输出机械臂五路角度。
- GUI 显示仿真角度。
- 点击执行后发送真实机械臂目标角度。

验收：

- 仿真动作和真实动作角度一致。
- 可以先仿真、再执行真实机械臂动作。

### 阶段 7：做成一键启动

目标：

- 一个 launch 文件启动 GUI、RViz2、雷达、桥接节点。
- GUI 可以选择只启动部分模块。

验收：

```bash
ros2 launch ros_car_gui gui.launch.py
```

能够启动完整上位机。

## 15. 推荐第一版界面草图

```text
┌──────────────────────────────────────────────────────────────────────────────┐
│ ROS-CAR 控制台       MusePi 在线  ESP32 在线  雷达在线  手动模式  急停未触发     │
├───────────────┬──────────────────────────────────────────────┬───────────────┤
│ 状态           │ RViz2                                        │ 控制           │
│               │                                              │               │
│ 线速度 0.00    │  [雷达点云 / 地图 / 小车模型 / 路径]             │  小车           │
│ 角速度 0.00    │                                              │   ▲            │
│ 左轮  0.00     │                                              │ ◀ ■ ▶          │
│ 右轮  0.00     │                                              │   ▼            │
│ PWM L 0       │                                              │ 速度上限 0.2   │
│ PWM R 0       │                                              │               │
│ IMU 正常       │                                              │ 机械臂          │
│ 雷达 8.0Hz     │                                              │ 抓取 复位 停止  │
│ 串口 12ms      │                                              │ S0 [滑条] 90   │
│               │                                              │ S1 [滑条] 0    │
│               │                                              │ S2 [滑条] 180  │
│               │                                              │ S3 [滑条] 130  │
│               │                                              │ S4 [滑条] 90   │
├───────────────┴──────────────────────────────────────────────┴───────────────┤
│ 急停 | 当前控制源：GUI | 机械臂互锁：允许 | cmd_vel 超时：正常 | 日志输出        │
└──────────────────────────────────────────────────────────────────────────────┘
```

## 16. Ubuntu 环境准备

推荐环境：

- Ubuntu 22.04。
- ROS2 Humble。
- Python 3.10。
- PySide6。
- PyQtGraph。
- PyBullet。

安装命令参考：

```bash
sudo apt update
sudo apt install ros-humble-desktop ros-humble-rviz2 ros-humble-teleop-twist-keyboard
sudo apt install python3-pip python3-colcon-common-extensions
pip3 install PySide6 pyqtgraph pybullet
```

如果使用手柄：

```bash
sudo apt install ros-humble-joy ros-humble-teleop-twist-joy
```

## 17. 启动流程建议

第一版推荐手动分窗口启动，方便排查：

```bash
# 终端 1：启动底盘、雷达、TF、建图可选项
cd ~/ros_car
source install/setup.bash
ros2 launch my_pkg bringup.launch.py

# 终端 2：启动 RViz2
rviz2

# 终端 3：启动 GUI
ros2 run ros_car_gui gui_app
```

稳定后再做一键启动：

```bash
ros2 launch ros_car_gui gui.launch.py
```

## 18. 文件和代码落点建议

当前工程可以这样放：

```text
ROS-CAR/ROS2/ros_car/
├── docs/
│   └── ros2_car_gui_control_plan.md
└── src/
    ├── my_pkg/
    │   └── 继续保留底盘桥接、launch、已有节点
    ├── ydlidar_ros2_driver/
    │   └── 雷达驱动
    ├── slam_gmapping/
    │   └── 建图
    └── ros_car_gui/
        └── 新增 GUI 上位机包
```

不建议把 GUI 代码塞进已有 `my_pkg` 里。`my_pkg` 继续负责底盘桥接和机器人基础功能，`ros_car_gui` 单独负责界面。

## 19. 后续需要确认的问题

正式写代码前需要确认这些点：

- 电脑和 MusePi 是否在同一个局域网。
- MusePi 是否固定 IP，或者是否使用主机名连接。
- 当前 ROS2 版本是否为 Humble。
- RViz2 当前配置文件是否已经保存。
- ESP32 串口协议是否要新增机械臂功能码。
- 机械臂控制期间到底采用“完全停车”还是“最大 `0.2 m/s` 低速协同”。
- 是否需要接入实体手柄。
- GUI 是否需要打包成桌面图标或开机自启动。

## 20. 最推荐的落地路线

第一步不要追求一口气做成完整游戏控制台。建议先做这个最小可用版本：

1. PySide6 主窗口。
2. 独立启动 RViz2。
3. GUI 发布 `/cmd_vel` 控车。
4. GUI 显示 `/odom`、`/imu`、`/scan` 是否在线。
5. GUI 提供急停。
6. GUI 发送机械臂抓取、复位、停止。
7. ESP32 和 MusePi 做机械臂互锁。

这个版本跑通后，再继续加入：

1. 机械臂五路角度滑条。
2. PID 调参。
3. PyBullet 仿真同步。
4. 手柄控制。
5. 一键 launch。
6. GUI 嵌入 RViz2 或 RViz2 Panel 插件。

## 21. 结论

这个 GUI 方案完全可以做，而且推荐做在 Ubuntu 电脑上。

最稳的路线是：

```text
Ubuntu 电脑：GUI + RViz2 + PyBullet + ROS2 主控
MusePi：雷达 + ESP32 串口桥接 + ROS2 节点
ESP32：电机 PID + 编码器/IMU + PCA9685 机械臂实时控制
```

第一版先让 GUI 控车、看状态、启动 RViz2、做急停和机械臂基本控制。等底层数据链路稳定后，再把界面做得更像游戏控制台。
