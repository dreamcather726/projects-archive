# VMGUI 工程结构说明

## 1. 总体目标

`VMGUI` 下面先放两个独立 ROS2 Python 工程：

```text
VMGUI/
├── ros_car_simulator/      # 无实机模拟数据发布工程
├── ros_car_cockpit/        # 上位机 cockpit GUI 工程
├── docs/                   # 设计说明
└── README.md
```

## 2. ros_car_simulator

这个工程用来模拟真实小车会发布的数据，方便没有实机时开发 GUI。

计划发布的话题：

| 话题 | 类型 | 说明 |
| --- | --- | --- |
| `/scan` | `sensor_msgs/msg/LaserScan` | 模拟 2D 激光雷达 |
| `/odom` | `nav_msgs/msg/Odometry` | 模拟小车里程计 |
| `/imu/data` | `sensor_msgs/msg/Imu` | 模拟 IMU |
| `/battery_state` | `sensor_msgs/msg/BatteryState` | 模拟电池 |
| `/camera/image_raw` | `sensor_msgs/msg/Image` | 模拟摄像头画面 |
| `/simulator/status` | `std_msgs/msg/String` | 模拟器状态 JSON |
| `/tf` | `tf2_msgs/msg/TFMessage` | 后续补充 TF |

订阅的话题：

| 话题 | 类型 | 说明 |
| --- | --- | --- |
| `/cmd_vel` | `geometry_msgs/msg/Twist` | 接收 GUI 的小车速度命令 |
| `/simulator/camera_enabled` | `std_msgs/msg/Bool` | 开关摄像头数据发布 |

## 3. ros_car_cockpit

这个工程用于上位机 GUI。第一版先搭框架，后续逐步做成完整界面。

主要模块：

```text
ros_car_cockpit/
├── main.py                 # GUI 程序入口
├── ros_client.py           # ROS2 发布/订阅封装
├── app_state.py            # GUI 使用的状态数据模型
├── ui/
│   ├── main_window.py      # 主窗口布局
│   ├── status_panel.py     # 左侧状态面板
│   ├── control_panel.py    # 右侧控制面板
│   ├── camera_panel.py     # 摄像头显示面板
│   └── style.py            # 界面样式
└── __init__.py
```

GUI 需要具备这些区域：

- 顶部：标题、连接状态、模式、急停状态。
- 左侧：电池、速度、里程计、IMU、雷达、串口、ROS Topic 状态。
- 中间：RViz2 显示区或占位区；开启摄像头后显示摄像头图像。
- 右侧：手动控制、建图、导航、机械臂、速度限制、PID 参数。
- 底部：急停、连接延迟、当前 `/cmd_vel` 曲线或简化显示。

## 4. 摄像头显示策略

第一版 GUI 里设置“开启摄像头”按钮：

1. GUI 发布 `/simulator/camera_enabled = true`。
2. 模拟器开始发布 `/camera/image_raw`。
3. GUI 订阅 `/camera/image_raw`。
4. GUI 把 ROS Image 转成 Qt 图像并显示。

后续接真实摄像头时，只要真实相机也发布 `/camera/image_raw`，GUI 不需要大改。

## 5. 后续开发顺序

1. 跑通模拟器发布 `/scan`、`/odom`、`/imu/data`、`/battery_state`。
2. 跑通 GUI 窗口和基础状态显示。
3. 跑通 GUI 发布 `/cmd_vel` 控制模拟小车。
4. 跑通摄像头开关和图像显示。
5. 加 RViz2 启动按钮和 RViz2 配置文件。
6. 加机械臂控制区和安全互锁显示。
