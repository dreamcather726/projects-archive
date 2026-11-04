# ROS2 小车项目节点式学习讲义

这份文档按“一个节点一个节点搭起来”的顺序讲解当前 ROS2 小车项目。讲课目标不是让学生一开始就记住所有参数，而是让他们知道：每个节点为什么存在、代码在哪里、订阅什么、发布什么、关键代码怎么读、启动后怎么验证。

推荐讲课主线：

```text
ROS2 基础概念
-> esp32_bridge_node 底盘桥接
-> keyboard_control_node 键盘控制
-> tf_tree_node 坐标树
-> ydlidar_ros2_driver_node 雷达
-> slam_gmapping 实时建图
-> Nav2 实时导航
-> xc_urdf / robot_state_publisher 机器人模型
-> VM RViz2 综合显示
```

## 当前整套启动命令

当前实车验证通过的完整流程如下，三条命令分别放在三个终端中运行。

终端 1：启动底盘、雷达、URDF 和键盘控制。

```bash
ros2 launch my_pkg bringup.launch.py port:=/dev/ttyUSB0 lidar_port:=/dev/ttyUSB1 use_lidar:=true use_urdf:=true use_urdf_rviz:=false use_keyboard_control:=true cmd_wheel_speed_sign:=-1.0 odom_wheel_speed_sign:=1.0
```

终端 2：启动实时建图。

```bash
ros2 launch my_pkg live_mapping.launch.py
```

终端 3：启动实时导航。

```bash
ros2 launch my_pkg live_navigation.launch.py use_rviz:=false
```

当前硬件连接：ESP32 底盘是 `/dev/ttyUSB0`，雷达是 `/dev/ttyUSB1`。Muse Pi 上不打开 RViz2，VM 中运行 RViz2 订阅这些 ROS2 数据。

## VM 显示前提

你的实际使用方式是：Muse Pi 在小车上运行所有 ROS2 节点，VM 只负责显示和发目标点。讲课时要先把这个边界说清楚，否则学生会以为 RViz2 必须运行在小车上。

| 位置 | 运行内容 | 不运行什么 |
| --- | --- | --- |
| Muse Pi | `bringup.launch.py`、`live_mapping.launch.py`、`live_navigation.launch.py` | 不打开 RViz2 |
| VM | `rviz2`、查看 topic、显示 RobotModel | 不连接 ESP32 串口、不连接雷达串口 |

VM 要能看到小车数据，需要满足三件事：

1. Muse Pi 和 VM 在同一网络，ROS2 DDS 能互相发现。
2. 两边 `ROS_DOMAIN_ID` 一致。
3. VM 也 source 了包含 `xc_urdf` 的工作区，否则 RobotModel 找不到 mesh。

VM 常用检查：

```bash
source ~/ros2_car/install/setup.bash
ros2 topic list
ros2 topic echo /scan --once
ros2 topic echo /robot_description --once
rviz2
```

## 课程总安排

下面是建议的 30 节课授课节奏。后面的正文仍按“节点和功能模块”展开，讲课时可以把一个模块拆成多节课来讲，例如底盘桥接拆成串口、`/odom`、IMU、`/cmd_vel`、方向标定几节。

| 课次 | 节点或主题 | 本节完成什么 | 验收标准 |
| --- | --- | --- | --- |
| 第 1 课 | ROS2 项目目标 | 讲清实车、Muse Pi、VM、ESP32、雷达分别做什么 | 能画出整车系统框图 |
| 第 2 课 | Workspace 和 package | 认识工作区、`src/`、`install/`、`package.xml` | 能运行 `colcon list` 并说出包的作用 |
| 第 3 课 | Node 和 topic | 理解节点和话题通信 | 能看懂 `ros2 node list`、`ros2 topic list` |
| 第 4 课 | Message 类型 | 讲 `Twist`、`Odometry`、`LaserScan`、`OccupancyGrid` | 能用 `ros2 interface show` 查看消息结构 |
| 第 5 课 | Service、action、parameter | 区分三种通信/配置方式 | 能说出 Nav2 目标为什么更像 action |
| 第 6 课 | Launch 文件 | 讲 `ros2 launch`、`name:=value`、多节点启动 | 能解释当前 bringup 命令每个参数 |
| 第 7 课 | 项目包结构 | 分析 `my_pkg`、雷达包、建图包、URDF 包 | 能说出每个包输入输出 |
| 第 8 课 | ESP32 串口协议 | 讲底盘串口、编码器帧、IMU 帧、轮速命令帧 | 能知道 `/dev/ttyUSB0` 是底盘串口 |
| 第 9 课 | 创建 `esp32_bridge_node` | 讲节点类、参数声明、串口打开 | `/esp32/status` 正常 |
| 第 10 课 | 发布 `/odom` | 讲编码器速度如何生成里程计 | `/odom` 持续更新 |
| 第 11 课 | 发布 `/imu/data` | 讲原始 IMU、修正 IMU、`imu_yaw_sign` | 左转时 RViz2 朝向一致 |
| 第 12 课 | 订阅 `/cmd_vel` | 讲 `/cmd_vel` 回调和左右轮速度换算 | 手动发布速度后车能动 |
| 第 13 课 | 底盘方向标定 | 讲 `cmd_wheel_speed_sign`、`odom_wheel_speed_sign` | 实车和 RViz2 前进方向一致 |
| 第 14 课 | `keyboard_control_node` | 讲键盘读取、组合键、50Hz 发布 | W/S/A/D 能稳定输出 `/cmd_vel` |
| 第 15 课 | 键盘控制调参 | 讲线速度、角速度、超时停车参数 | 能根据建图需求调慢速度 |
| 第 16 课 | `tf_tree_node` 基础 | 讲 `odom -> base_link` 动态 TF | `tf2_echo odom base_link` 正常 |
| 第 17 课 | 传感器静态 TF | 讲 `base_link -> laser_link`、IMU、camera frame | `tf2_echo base_link laser_link` 正常 |
| 第 18 课 | YDLidar 驱动节点 | 讲雷达串口、参数、`frame_id` | `/scan` 有稳定频率 |
| 第 19 课 | `/scan` 消息详解 | 讲 `LaserScan` 字段、`ranges`、角度和距离 | 能解释 `/scan` 不是 `PointCloud2` |
| 第 20 课 | VM/RViz2 显示雷达 | 讲 Fixed Frame、LaserScan Display、TF 链路 | VM 中能看到 `/scan` 点 |
| 第 21 课 | 雷达方向和质量排查 | 讲左右反、倒装、近距离噪声、反光 | 左侧障碍在 RViz2 中也在左侧 |
| 第 22 课 | `slam_gmapping` 输入输出 | 讲 `/scan`、`/odom`、TF 到 `/map` | `/map_metadata` 正常 |
| 第 23 课 | GMapping 参数 | 讲 `delta`、`particles`、更新阈值 | 能解释地图清晰度和 CPU 的取舍 |
| 第 24 课 | `live_mapping.launch.py` | 讲为什么建图单独启动 | `map -> odom` 出现 |
| 第 25 课 | Nav2 架构 | 讲 planner、controller、BT、behavior、lifecycle | Nav2 server 为 `active` |
| 第 26 课 | Nav2 路径显示 | 讲 `/goal_pose`、`/plan`、`/local_plan` | VM RViz2 点目标后出现路径 |
| 第 27 课 | Nav2 速度控制 | 讲 `/cmd_vel`、`velocity_smoother`、最小速度 | 能判断“有路径但车不动”的原因 |
| 第 28 课 | Costmap | 讲 local/global costmap、膨胀层、机器人半径 | 能解释墙体为什么变厚 |
| 第 29 课 | `xc_urdf` 和 RobotModel | 讲 URDF、mesh、`robot_state_publisher`、VM 显示 | VM RViz2 中 RobotModel 正常 |
| 第 30 课 | 综合验收和排查 | 三条命令跑完整实时建图导航 | 能按 topic、TF、map、Nav2、cmd_vel 顺序排查 |

## 第 1 课：ROS2 基础概念

### 讲课目标

先让学生知道 ROS2 不是一个单独程序，而是一套机器人软件组织方式。一个机器人被拆成多个节点，每个节点只负责一件事，再通过 topic、service、action、parameter 组合起来。

### 基础概念

| 概念 | 小白理解 | 本项目例子 |
| --- | --- | --- |
| Workspace | 一个 ROS2 项目工作目录 | `~/ros2_car` |
| Package | 一个功能包 | `my_pkg`、`ydlidar_ros2_driver` |
| Node | 一个运行中的程序 | `esp32_bridge_node` |
| Topic | 持续传数据的频道 | `/cmd_vel`、`/odom`、`/scan` |
| Message | topic 里的数据格式 | `Twist`、`Odometry`、`LaserScan` |
| Parameter | 节点配置 | `port`、`wheel_base_m` |
| Launch | 一次启动多个节点的脚本 | `bringup.launch.py` |

### 演示命令

```bash
cd ~/ros2_car
source install/setup.bash
ros2 pkg list | grep -E "my_pkg|ydlidar|slam_gmapping|xc_urdf"
ros2 node list
ros2 topic list
```

### 讲解重点

`ros2 node list` 看到的是“正在运行的程序”；`ros2 topic list` 看到的是“正在传输的数据频道”。如果节点没有启动，对应 topic 通常也不会出现。

## 第 2 课：Topic、Service、Action、Parameter

### Topic

Topic 适合持续数据流。比如雷达一直发 `/scan`，底盘一直发 `/odom`，导航持续发 `/cmd_vel`。

| Topic | 发布者 | 订阅者 | 作用 |
| --- | --- | --- | --- |
| `/cmd_vel` | 键盘或 Nav2 | `esp32_bridge_node` | 控制小车速度 |
| `/odom` | `esp32_bridge_node` | `tf_tree_node`、GMapping、Nav2 | 里程计 |
| `/imu/data` | `esp32_bridge_node` | 调试或融合模块 | 修正后的 IMU |
| `/scan` | `ydlidar_ros2_driver_node` | GMapping、Nav2、RViz2 | 雷达扫描 |
| `/map` | `slam_gmapping` | Nav2、RViz2 | 实时地图 |
| `/plan` | Nav2 planner | RViz2、controller | 全局路径 |

常用命令：

```bash
ros2 topic list
ros2 topic info /cmd_vel
ros2 topic echo /cmd_vel
ros2 topic hz /scan
```

### 常见消息怎么读

讲课时不要只告诉学生 topic 名称，还要让他们能看懂消息内容。

| 消息类型 | 常见 topic | 重点字段 | 怎么解释 |
| --- | --- | --- | --- |
| `geometry_msgs/msg/Twist` | `/cmd_vel` | `linear.x`、`angular.z` | 线速度和角速度 |
| `nav_msgs/msg/Odometry` | `/odom` | `pose.pose`、`twist.twist` | 小车估计的位置和速度 |
| `sensor_msgs/msg/LaserScan` | `/scan` | `angle_min`、`angle_increment`、`ranges` | 一圈雷达每个角度方向的距离 |
| `nav_msgs/msg/OccupancyGrid` | `/map`、costmap | `info.resolution`、`data` | 栅格地图，每个格子表示空闲、占用或未知 |
| `nav_msgs/msg/Path` | `/plan`、`/local_plan` | `poses` | 一串路径点 |

可以演示：

```bash
ros2 interface show sensor_msgs/msg/LaserScan
ros2 interface show nav_msgs/msg/OccupancyGrid
ros2 interface show nav_msgs/msg/Path
```

重点讲清：RViz2 不是“凭空显示图像”，它只是把这些标准消息可视化出来。能不能显示，取决于 topic 是否存在、消息 frame 是否正确、TF 是否连通。

### Service

Service 是“问一次、答一次”。它不适合雷达这种连续数据，更适合临时请求，比如生命周期切换、保存地图等。

```bash
ros2 service list
ros2 service type /controller_server/get_state
```

### Action

Action 是“发一个长期任务，中间有反馈，最后有结果”。导航到目标点就是典型 action 思路：目标点发出去后，Nav2 会规划、控制、反馈状态，最后成功或失败。

```bash
ros2 action list
```

### Parameter

Parameter 是节点设置项。比如底盘串口、轮距、雷达 frame、导航速度、costmap 膨胀半径，都通过参数影响节点行为。

```bash
ros2 param list /controller_server
ros2 param get /controller_server FollowPath.max_vel_x
```

## 第 3 课：项目结构

```text
ros_car/
├── docs/
├── src/
│   ├── my_pkg/
│   │   ├── launch/
│   │   ├── config/
│   │   └── my_pkg/
│   │       ├── esp32_bridge/
│   │       ├── keyboard_control/
│   │       └── tf_tree/
│   ├── ydlidar_ros2_driver/
│   ├── slam_gmapping/
│   ├── openslam_gmapping/
│   └── xc_urdf/
```

| 包 | 作用 |
| --- | --- |
| `my_pkg` | 当前项目主包，包含底盘桥接、键盘控制、TF、launch、Nav2 参数 |
| `ydlidar_ros2_driver` | 雷达驱动，发布 `/scan` |
| `slam_gmapping` | GMapping ROS2 节点，发布 `/map` |
| `openslam_gmapping` | GMapping 底层算法库 |
| `xc_urdf` | 小车 URDF 模型包，用于 RViz2 RobotModel |

构建命令：

```bash
cd ~/ros2_car
colcon build --symlink-install
source install/setup.bash
```

## 第 4 课：创建底盘桥接节点 `esp32_bridge_node`

### 节点职责

文件：[esp32_bridge_node.py](C:/Users/12547/Desktop/workspace/ROS-CAR/ROS2/ros_car/src/my_pkg/my_pkg/esp32_bridge/esp32_bridge_node.py:59)

`esp32_bridge_node` 是 ROS2 和 ESP32 底盘之间的桥：

1. 打开 ESP32 串口。
2. 订阅 `/cmd_vel`。
3. 把 `/cmd_vel` 转成左右轮目标速度。
4. 通过串口发送给 ESP32。
5. 接收 ESP32 回传的编码器和 IMU 数据。
6. 发布 `/odom`、`/imu/rawdata`、`/imu/data`、`/esp32/status`。

### 节点输入输出

| 类型 | 名称 | 说明 |
| --- | --- | --- |
| 订阅 | `/cmd_vel` | 上层速度命令 |
| 发布 | `/odom` | 编码器和 IMU 生成的里程计 |
| 发布 | `/imu/rawdata` | 原始 IMU |
| 发布 | `/imu/data` | 修正 yaw 方向后的 IMU |
| 发布 | `/esp32/status` | 串口连接状态 |
| 参数 | `port` | ESP32 串口 |
| 参数 | `cmd_wheel_speed_sign` | 修正命令方向 |
| 参数 | `odom_wheel_speed_sign` | 修正 odom 方向 |

### 关键代码 1：声明参数

```python
self.declare_parameter("port", DEFAULT_PORT)
self.declare_parameter("baudrate", DEFAULT_BAUDRATE)
self.declare_parameter("wheel_base_m", WHEEL_BASE_M)
self.declare_parameter("imu_yaw_sign", DEFAULT_IMU_YAW_SIGN)
self.declare_parameter("cmd_wheel_speed_sign", DEFAULT_CMD_WHEEL_SPEED_SIGN)
self.declare_parameter("odom_wheel_speed_sign", DEFAULT_ODOM_WHEEL_SPEED_SIGN)
```

讲解：这些参数让同一个节点可以适配不同硬件现场。比如串口变了，不需要改代码，只要启动时传 `port:=/dev/ttyUSB0`。

### 关键代码 2：创建发布者和订阅者

```python
self.odom_pub = self.create_publisher(Odometry, "/odom", 10)
self.imu_raw_pub = self.create_publisher(Imu, "/imu/rawdata", 10)
self.imu_pub = self.create_publisher(Imu, "/imu/data", 10)
self.status_pub = self.create_publisher(String, "/esp32/status", 10)
self.create_subscription(Twist, "/cmd_vel", self.cmd_vel_callback, 10)
```

讲解：这段是 ROS2 节点最核心的结构。发布者负责往 topic 发数据，订阅者负责从 topic 收数据并进入回调函数。

### 关键代码 3：打开串口

```python
self.serial_port = serial.Serial(
    port,
    baudrate,
    timeout=0.005,
    write_timeout=0.2,
)
```

讲解：`timeout` 很短，是为了让节点不断循环读取串口，不因为一次读取阻塞太久。串口打开失败时，节点会发布 `serial_disconnected`，并通过定时器重连。

### 关键代码 4：解析编码器并发布 odom

```python
left_speed = read_int16_le(payload, 6) / 100.0
right_speed = read_int16_le(payload, 8) / 100.0
self.publish_odom(left_speed, right_speed)
```

讲解：ESP32 回传的是整数编码后的速度，ROS2 节点把它还原成 cm/s，再交给 `WheelOdometry` 生成标准 `nav_msgs/Odometry`。

### 关键代码 5：处理 IMU yaw 方向

```python
imu_yaw_sign = float(self.get_parameter("imu_yaw_sign").value)
corrected_yaw = yaw * imu_yaw_sign
self.latest_imu_yaw = math.radians(corrected_yaw)
```

讲解：如果实车左转，但 RViz2 中向右转，通常就是 yaw 正方向反了。`imu_yaw_sign` 用来快速修正这个问题。

### 验证命令

```bash
ros2 launch my_pkg bringup.launch.py port:=/dev/ttyUSB0 use_lidar:=false use_keyboard_control:=false cmd_wheel_speed_sign:=-1.0 odom_wheel_speed_sign:=1.0
ros2 topic echo /esp32/status --once
ros2 topic echo /odom --once
ros2 topic echo /imu/data --once
```

### 这个节点的数据后面怎么用

| 输出 | 后续用途 |
| --- | --- |
| `/odom` | `tf_tree_node` 用它发布 `odom -> base_link`，GMapping 和 Nav2 用它估计小车运动 |
| `/imu/data` | 用于检查 yaw 方向，辅助里程计朝向 |
| `/esp32/status` | 判断底盘串口是否连上 |
| 串口轮速命令 | ESP32 根据左右轮目标速度控制电机 |

讲课时可以强调：底盘桥接节点是整车闭环的入口和出口。上层通过 `/cmd_vel` 告诉小车怎么动，底层通过 `/odom` 告诉上层小车实际怎么动了。

## 第 5 课：`/cmd_vel` 到左右轮速度

### 关键代码

文件：[esp32_bridge_node.py](C:/Users/12547/Desktop/workspace/ROS-CAR/ROS2/ros_car/src/my_pkg/my_pkg/esp32_bridge/esp32_bridge_node.py:348)

```python
left_speed_m_s = (
    linear_speed - angular_speed * wheel_base_m / 2.0
) * cmd_wheel_speed_sign
right_speed_m_s = (
    linear_speed + angular_speed * wheel_base_m / 2.0
) * cmd_wheel_speed_sign
```

讲解：差速小车只有左右轮，不能直接执行 `linear.x` 和 `angular.z`，必须换算成左右轮速度。

| `/cmd_vel` | 左右轮结果 | 实车现象 |
| --- | --- | --- |
| `linear.x > 0`、`angular.z = 0` | 左右轮同速 | 直行 |
| `linear.x = 0`、`angular.z > 0` | 左右轮反向或速度不同 | 原地左转 |
| `linear.x > 0`、`angular.z > 0` | 右轮比左轮快 | 前进左转 |

### 方向参数怎么讲

| 参数 | 改它的场景 |
| --- | --- |
| `cmd_wheel_speed_sign` | 发前进命令时实车后退 |
| `odom_wheel_speed_sign` | 实车前进，但 RViz2 模型后退 |
| `imu_yaw_sign` | 实车左转，但 RViz2 向右转 |

当前实车正确组合：

```bash
cmd_wheel_speed_sign:=-1.0 odom_wheel_speed_sign:=1.0
```

验证命令：

```bash
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist "{linear: {x: 0.10}, angular: {z: 0.0}}"
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist "{linear: {x: 0.0}, angular: {z: 0.9}}"
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist "{}"
```

## 第 6 课：编码器里程计 `WheelOdometry`

### 代码位置

文件：[odometry.py](C:/Users/12547/Desktop/workspace/ROS-CAR/ROS2/ros_car/src/my_pkg/my_pkg/esp32_bridge/odometry.py:8)

`WheelOdometry` 把左右轮实际速度积分成 `/odom`。

### 讲解重点

里程计不是外部定位，它是小车根据轮子转动“自己估计自己在哪里”。它短时间连续，但长期会漂移。轮子打滑、轮径误差、地面摩擦都会让 odom 越走越偏。

### 验证

```bash
ros2 topic echo /odom
ros2 run tf2_ros tf2_echo odom base_link
```

观察：小车真实向前时，RViz2 中模型也应向前；原地左转时，模型也应左转。

### `/odom` 在 VM 里怎么看

在 VM RViz2 中，`/odom` 本身通常不直接显示成一张图，而是通过 TF 和 RobotModel 间接体现：

1. `esp32_bridge_node` 发布 `/odom`。
2. `tf_tree_node` 订阅 `/odom`。
3. `tf_tree_node` 发布 `odom -> base_link`。
4. RViz2 根据 TF 把 RobotModel 放到正确位置。

VM 端验证：

```bash
ros2 topic echo /odom --once
ros2 run tf2_ros tf2_echo odom base_link
```

如果 `/odom` 有数据但 RViz2 模型不动，优先查 `tf_tree_node` 是否运行、`odom -> base_link` 是否存在，而不是先查 URDF。

## 第 7 课：键盘控制节点 `keyboard_control_node`

### 节点职责

文件：[keyboard_control_node.py](C:/Users/12547/Desktop/workspace/ROS-CAR/ROS2/ros_car/src/my_pkg/my_pkg/keyboard_control/keyboard_control_node.py:112)

`keyboard_control_node` 读取 W/A/S/D，发布 `/cmd_vel`，用于手动测试底盘、建图时慢速移动。

### 输入输出

| 类型 | 名称 | 说明 |
| --- | --- | --- |
| 输入 | 键盘 W/A/S/D/空格/X | 人手控制 |
| 发布 | `/cmd_vel` | 速度命令 |
| 参数 | `linear_speed_m_s` | 前进后退速度 |
| 参数 | `angular_speed_rad_s` | 左右转角速度 |
| 参数 | `publish_rate_hz` | 发布频率，当前 50Hz |
| 参数 | `key_timeout_sec` | 多久没有按键自动停车 |

### 关键代码 1：一次性读完按键缓冲

```python
while msvcrt.kbhit():
    keys.append(msvcrt.getwch().lower())
```

Linux 下使用 `select.select(..., 0.0)` 非阻塞读取。讲解：这样长按按键不会在终端缓冲里堆积，响应会更跟手。

### 关键代码 2：按键集合支持组合键

```python
self.pressed_keys = set()
if "w" in self.pressed_keys:
    linear_speed += linear_speed_m_s
if "a" in self.pressed_keys:
    angular_speed += angular_speed_rad_s
```

讲解：用集合记录当前有效按键，所以可以实现 `W+A`、`W+D` 这种边走边转。

### 关键代码 3：超时自动停车

```python
if time.monotonic() - self.last_key_time <= key_timeout_sec:
    return
self.stop_motion()
```

讲解：终端很难真正知道某个键何时释放，所以用“多久没收到新按键”作为停车保护。

### 启动和验证

```bash
ros2 launch my_pkg bringup.launch.py port:=/dev/ttyUSB0 use_lidar:=false use_keyboard_control:=true keyboard_linear_speed_m_s:=0.10 keyboard_angular_speed_rad_s:=0.90 keyboard_publish_rate_hz:=50.0 keyboard_key_timeout_sec:=0.30
ros2 topic echo /cmd_vel
```

## 第 8 课：坐标树节点 `tf_tree_node`

### 节点职责

文件：[tf_tree_node.py](C:/Users/12547/Desktop/workspace/ROS-CAR/ROS2/ros_car/src/my_pkg/my_pkg/tf_tree/tf_tree_node.py:71)

TF 告诉 ROS2 各个坐标系之间的关系。本项目最重要的链路是：

```text
map -> odom -> base_link -> laser_link
```

`tf_tree_node` 做两件事：

1. 订阅 `/odom`，把 odom 位姿发布成动态 TF：`odom -> base_link`。
2. 发布静态 TF：`base_link -> laser_link`、`base_link -> imu_link`、`base_link -> camera_link`。

### 关键代码 1：订阅 odom

```python
self.create_subscription(
    Odometry,
    odom_topic,
    self.odom_callback,
    10,
)
```

### 关键代码 2：发布静态 TF

```python
static_transforms = [
    build_transform_message(self.base_frame, self.get_parameter("laser_frame").value, ...)
]
self.static_tf_broadcaster.sendTransform(static_transforms)
```

讲解：雷达相对小车的位置是固定的，所以用静态 TF。小车相对 odom 会不断变化，所以用动态 TF。

### 关键代码 3：把 odom 转为 TF

```python
pose = odom_message.pose.pose
transform_message = build_transform_message(
    parent_frame,
    child_frame,
    odom_message.header.stamp,
    (pose.position.x, pose.position.y, pose.position.z),
    (pose.orientation.x, pose.orientation.y, pose.orientation.z, pose.orientation.w),
)
self.tf_broadcaster.sendTransform(transform_message)
```

### 验证命令

```bash
ros2 run tf2_ros tf2_echo odom base_link
ros2 run tf2_ros tf2_echo base_link laser_link
```

如果这里不通，后面的 GMapping 和 Nav2 都会出问题。

### TF 和 `/scan` 的关系

雷达只知道“某个角度方向距离障碍物多远”，这个数据本身是在 `laser_link` 坐标系下的。GMapping 和 Nav2 需要知道这些点相对小车、相对地图在哪里，所以必须依赖 TF。

数据转换关系：

```text
/scan.header.frame_id = laser_link
base_link -> laser_link 告诉 ROS2 雷达装在车体哪里
odom -> base_link 告诉 ROS2 小车相对 odom 在哪里
map -> odom 告诉 ROS2 odom 相对地图在哪里
```

所以 VM RViz2 中 Fixed Frame 选 `map` 时，显示 `/scan` 实际需要完整链路：

```text
map -> odom -> base_link -> laser_link
```

如果 RViz2 的 LaserScan 显示红色错误，经常不是雷达没数据，而是 TF 链路不完整。

## 第 9 课：雷达节点 `ydlidar_ros2_driver_node`

### 节点职责

包：`src/ydlidar_ros2_driver`

这个节点连接 YDLidar 串口，把雷达数据发布成标准 `sensor_msgs/msg/LaserScan`，topic 是 `/scan`。

### 启动方式

通常由 `bringup.launch.py` 间接启动：

```bash
ros2 launch my_pkg bringup.launch.py port:=/dev/ttyUSB0 lidar_port:=/dev/ttyUSB1 use_lidar:=true use_urdf:=false use_keyboard_control:=false
```

### 关键参数

文件：`src/ydlidar_ros2_driver/params/ydlidar.yaml`

| 参数 | 当前讲法 |
| --- | --- |
| `port` | 当前实车雷达是 `/dev/ttyUSB1` |
| `frame_id` | 应为 `laser_link`，要和 TF 一致 |
| `baudrate` | X3 常用 115200 |
| `reversion` | 扫描方向左右反时尝试修改 |
| `inverted` | 雷达倒装或角度反时尝试修改 |
| `min_range_m` | 过滤过近噪声 |
| `max_range_m` | 过滤远距离噪声 |

### 验证命令

```bash
ros2 topic hz /scan
ros2 topic echo /scan --once
ros2 run tf2_ros tf2_echo base_link laser_link
```

课堂实验：在车左侧放一个障碍物，RViz2 中 LaserScan 也应该出现在车左侧。如果反了，先查雷达参数和 `base_link -> laser_link`，不要先改地图。

### `/scan` 消息怎么理解

`/scan` 的类型是 `sensor_msgs/msg/LaserScan`。它不是三维点云 `PointCloud2`，而是一圈二维激光距离数组。RViz2 的 `LaserScan` Display 会把这些距离数组画成一圈点，看起来像点云，但底层消息仍然是 LaserScan。

核心字段：

| 字段 | 含义 | 讲课时怎么解释 |
| --- | --- | --- |
| `header.frame_id` | 雷达坐标系 | 必须能通过 TF 连到 `base_link`、`map` |
| `angle_min` | 扫描起始角度 | 第一束激光的角度 |
| `angle_max` | 扫描结束角度 | 最后一束激光的角度 |
| `angle_increment` | 相邻激光角度间隔 | 决定一圈有多少个点 |
| `range_min` | 最小有效距离 | 太近的数据会被认为无效 |
| `range_max` | 最大有效距离 | 超过这个距离的数据无效 |
| `ranges` | 距离数组 | 每个角度方向测到的障碍物距离 |
| `intensities` | 反射强度 | 有些雷达会提供，有些为空 |

用命令观察：

```bash
ros2 topic echo /scan --once
ros2 interface show sensor_msgs/msg/LaserScan
```

讲解方式：假设 `angle_min=-3.14`、`angle_increment=0.01`，`ranges[0]` 就是第一个角度方向的距离，`ranges[100]` 就是向后偏移 100 个角度间隔后的距离。

### `/scan` 在项目里怎么被使用

| 使用者 | 怎么用 `/scan` |
| --- | --- |
| RViz2 | 把 `ranges` 根据角度画成可视化点 |
| GMapping | 用 `/scan` 和 `/odom` 匹配墙体，生成 `/map` |
| Nav2 local_costmap | 根据 `/scan` 实时标记小车附近障碍物 |
| Nav2 global_costmap | 可结合地图和传感器更新全局代价 |

这就是为什么雷达方向错会影响建图和导航：RViz2 可能只是显示反了，但 GMapping 会把墙建到错误方向，Nav2 也会把障碍物放错位置。

### VM 中怎么看 `/scan`

VM 端步骤：

1. 确认 VM 已 source 工作区。

```bash
source ~/ros2_car/install/setup.bash
```

2. 确认能收到 `/scan`。

```bash
ros2 topic hz /scan
ros2 topic echo /scan --once
```

3. 确认 TF 能把 `laser_link` 转到 RViz2 Fixed Frame。

```bash
ros2 run tf2_ros tf2_echo base_link laser_link
ros2 run tf2_ros tf2_echo map laser_link
```

4. 打开 RViz2。

```bash
rviz2
```

5. RViz2 中添加：

| RViz2 设置 | 值 |
| --- | --- |
| Global Options / Fixed Frame | 建图前可先用 `base_link`，建图后推荐 `map` |
| Add / By display type | `LaserScan` |
| LaserScan / Topic | `/scan` |
| LaserScan / Size | 可调大一点，便于讲课看清 |
| LaserScan / Style | Points 或 Flat Squares |

如果建图还没启动，Fixed Frame 选 `map` 可能无法显示 `/scan`，因为此时还没有 `map -> odom`。这时可以先把 Fixed Frame 选成 `base_link`，确认雷达点方向正确；启动 GMapping 后再切回 `map`。

### `/scan` 常见问题排查

| 现象 | 优先检查 |
| --- | --- |
| `ros2 topic hz /scan` 没输出 | 雷达串口、驱动是否启动、雷达是否旋转 |
| RViz2 有 LaserScan 红色错误 | Fixed Frame 到 `laser_link` 的 TF 是否存在 |
| 左边障碍显示到右边 | `reversion`、`inverted`、雷达安装 yaw |
| 障碍物围着车转 | Fixed Frame 或 TF 链路不对 |
| 点很散、乱跳 | 反光、玻璃、黑色物体、供电、雷达高度 |
| 近处一圈杂点 | 适当调大 `min_range_m` |

## 第 10 课：总启动文件 `bringup.launch.py`

### 文件职责

文件：[bringup.launch.py](C:/Users/12547/Desktop/workspace/ROS-CAR/ROS2/ros_car/src/my_pkg/launch/bringup.launch.py:11)

`bringup.launch.py` 是第一阶段启动入口，用来组合启动底盘、TF、雷达、URDF、键盘控制。

### 讲解重点

| 参数 | 作用 |
| --- | --- |
| `port` | ESP32 底盘串口 |
| `lidar_port` | 雷达串口 |
| `use_lidar` | 是否启动雷达 |
| `use_urdf` | 是否启动机器人模型发布 |
| `use_urdf_rviz` | 是否在本机打开 RViz2，Muse Pi 上设为 `false` |
| `use_keyboard_control` | 是否启动键盘控制 |
| `cmd_wheel_speed_sign` | 修正实车前后方向 |
| `odom_wheel_speed_sign` | 修正 RViz2 里 odom 前后方向 |

### 当前完整命令

```bash
ros2 launch my_pkg bringup.launch.py port:=/dev/ttyUSB0 lidar_port:=/dev/ttyUSB1 use_lidar:=true use_urdf:=true use_urdf_rviz:=false use_keyboard_control:=true cmd_wheel_speed_sign:=-1.0 odom_wheel_speed_sign:=1.0
```

### 常见错误

多行命令中反斜杠后面不能有空格，否则会出现：

```text
malformed launch argument ' ', expected format '<name>:=<value>'
```

给小白讲课时建议先用一行命令。

## 第 11 课：实时建图节点 `slam_gmapping`

### 节点职责

包：`src/slam_gmapping`

GMapping 不是直接画雷达点，它融合 `/scan`、`/odom` 和 TF，输出实时地图 `/map`，并发布 `map -> odom`。

### 输入输出

| 类型 | 名称 | 说明 |
| --- | --- | --- |
| 输入 | `/scan` | 雷达扫描 |
| 输入 | `odom -> base_link` | 小车运动 |
| 输入 | `base_link -> laser_link` | 雷达安装位置 |
| 输出 | `/map` | 实时占据栅格地图 |
| 输出 | `/map_metadata` | 地图元信息 |
| 输出 | `map -> odom` | 修正 odom 漂移 |

### 启动

```bash
ros2 launch my_pkg live_mapping.launch.py
```

### 关键参数

文件：`src/slam_gmapping/params/slam_gmapping.yaml`

| 参数 | 修改效果 |
| --- | --- |
| `base_frame` | 小车本体 frame，通常 `base_link` |
| `odom_frame` | 里程计 frame，通常 `odom` |
| `map_frame` | 地图 frame，通常 `map` |
| `delta` | 地图分辨率，越小越细但更耗资源 |
| `particles` | 粒子数，越大越稳但 CPU 更高 |
| `linearUpdate` | 直线走多远更新一次 |
| `angularUpdate` | 转多少角度更新一次 |

### 验证命令

```bash
ros2 topic echo /map_metadata --once
ros2 topic hz /map
ros2 run tf2_ros tf2_echo map odom
```

如果没有 `map -> odom`，Nav2 在 `map` 坐标系下就不知道机器人位置。

### GMapping 怎么使用 `/scan`

GMapping 的核心思想可以这样讲：机器人移动时，雷达不断看到墙和障碍物。算法把当前这一帧 `/scan` 和已有地图、里程计运动估计进行匹配，找到最合理的小车位姿，再把雷达命中的位置写入占据栅格地图。

简化数据流：

```text
/scan 当前看到的墙
+ /odom 短时间运动估计
+ base_link -> laser_link 雷达安装位置
=> scan matching
=> /map
=> map -> odom
```

几个专业但适合讲课的点：

| 概念 | 讲法 |
| --- | --- |
| Scan Matching | 用当前雷达形状和已有地图对齐，修正小车位姿 |
| Particle | 多个可能位姿假设，谁更符合雷达观测就权重更高 |
| Occupancy Grid | 地图被切成很多小格子，每格记录空闲、占用、未知 |
| `map -> odom` | 用 SLAM 结果修正 odom 漂移，但不直接改底盘 odom |

这也解释了为什么 `/odom` 不准会影响地图，雷达方向不准也会影响地图。GMapping 同时依赖运动估计和雷达观测，两边有一个错，地图就可能重影、旋转、撕裂。

### VM 中怎么看 `/map`

在 VM RViz2 中：

| RViz2 设置 | 值 |
| --- | --- |
| Fixed Frame | `map` |
| Add / Map | Topic 选 `/map` |
| Add / LaserScan | Topic 选 `/scan` |
| Add / TF | 打开 TF |
| Add / RobotModel | 显示车体 |

命令验证：

```bash
ros2 topic echo /map_metadata --once
ros2 topic hz /map
ros2 run tf2_ros tf2_echo map odom
```

如果 `/map` 有数据但 RViz2 不显示，先看 Fixed Frame 是否为 `map`，再看 `map -> odom -> base_link -> laser_link` 是否连通。

### 地图质量怎么判断

| 现象 | 更可能的问题 |
| --- | --- |
| 墙体很清晰，转弯后仍重合 | 建图正常 |
| 墙体出现双层 | odom、IMU yaw 或雷达 TF 有误差 |
| 地图整体旋转错位 | yaw 方向或 `map -> odom` 异常 |
| 机器人走动但地图不扩展 | `/scan`、TF 或 GMapping 更新参数 |
| 地图更新很慢 | CPU、`map_update_interval`、雷达频率 |

## 第 12 课：实时建图启动文件 `live_mapping.launch.py`

### 文件职责

文件：[live_mapping.launch.py](C:/Users/12547/Desktop/workspace/ROS-CAR/ROS2/ros_car/src/my_pkg/launch/live_mapping.launch.py:8)

它只启动 GMapping，不启动底盘、不启动雷达、不启动 Nav2。这样课堂上可以分层验证：

1. 先启动 `bringup.launch.py`，确认 `/scan`、`/odom`、TF。
2. 再启动 `live_mapping.launch.py`，确认 `/map`。
3. 最后启动导航。

### 课堂验证顺序

```bash
ros2 topic hz /scan
ros2 topic echo /odom --once
ros2 run tf2_ros tf2_echo base_link laser_link
ros2 launch my_pkg live_mapping.launch.py
ros2 topic echo /map_metadata --once
```

## 第 13 课：Nav2 实时导航

### 节点组成

Nav2 不是一个节点，而是一组 server：

| 节点 | 作用 |
| --- | --- |
| `planner_server` | 规划全局路径 `/plan` |
| `controller_server` | 跟踪路径并输出 `/cmd_vel` |
| `bt_navigator` | 行为树调度导航任务 |
| `behavior_server` | 恢复行为，例如旋转、后退 |
| `velocity_smoother` | 平滑速度 |
| `local_costmap` | 小车附近避障地图 |
| `global_costmap` | 全局代价地图 |

### 启动

```bash
ros2 launch my_pkg live_navigation.launch.py use_rviz:=false
```

### 为什么不用 AMCL/map_server

本项目是实时建图导航，地图由 GMapping 正在发布 `/map`，`map -> odom` 也由 GMapping 发布。所以 `live_navigation.launch.py` 不启动保存地图模式下的 AMCL/map_server。

### lifecycle 验证

```bash
ros2 lifecycle get /planner_server
ros2 lifecycle get /controller_server
ros2 lifecycle get /bt_navigator
ros2 lifecycle get /behavior_server
```

正常应进入 `active`。如果还是 `unconfigured`，先查 `/map`、TF 和参数文件。

### Nav2 怎么使用这些数据

Nav2 的输入不只是地图。它同时依赖地图、雷达、里程计和 TF。

| 输入 | Nav2 用途 |
| --- | --- |
| `/map` | 全局规划的环境基础 |
| `/scan` | 更新 local_costmap，发现近处动态障碍 |
| `/odom` / TF | 判断小车当前位姿和速度 |
| `map -> odom -> base_link` | 把路径、障碍物、机器人位置放到同一坐标系 |
| `/goal_pose` | RViz2 发来的目标点 |

输出：

| 输出 | 用途 |
| --- | --- |
| `/plan` | 全局路径，给 RViz2 显示，也给 controller 跟踪 |
| `/local_plan` | 局部轨迹，反映小车眼前怎么走 |
| `/cmd_vel` | 最终控制底盘的速度 |
| `/global_costmap/costmap` | 全局代价地图 |
| `/local_costmap/costmap` | 局部代价地图 |

### VM 中怎么看 Nav2 路径

VM RViz2 添加：

| Display | Topic | 说明 |
| --- | --- | --- |
| `Path` | `/plan` | 全局路线 |
| `Path` | `/local_plan` | 局部轨迹 |
| `Map` | `/global_costmap/costmap` | 全局代价地图 |
| `Map` | `/local_costmap/costmap` | 局部代价地图 |
| `LaserScan` | `/scan` | 近处障碍来源 |
| `RobotModel` | `/robot_description` | 小车模型 |

发送目标点后，用命令确认：

```bash
ros2 topic echo /goal_pose --once
ros2 topic echo /plan --once
ros2 topic echo /cmd_vel
```

排查逻辑：

| 现象 | 优先查 |
| --- | --- |
| 点目标后没有 `/plan` | `planner_server`、global_costmap、目标点是否在可通行区域 |
| 有 `/plan` 但没有 `/cmd_vel` | `controller_server`、local_costmap、TF |
| 有 `/cmd_vel` 但车不动 | 底盘桥接、串口、电机死区、速度参数 |
| 路径离墙太远 | `inflation_radius`、`robot_radius` |
| 路径穿墙 | `/map`、costmap、footprint 或膨胀层 |

## 第 14 课：Nav2 速度和 costmap 参数

### 速度参数

文件：`src/my_pkg/config/nav2_params.yaml`

| 参数 | 作用 |
| --- | --- |
| `FollowPath.min_vel_x` | DWB 采样的最小前进速度 |
| `FollowPath.max_vel_x` | 最大前进速度 |
| `FollowPath.max_vel_theta` | 最大角速度 |
| `FollowPath.min_speed_theta` | 最小旋转采样速度 |
| `velocity_smoother.deadband_velocity` | 小速度死区 |

注意：最小速度参数不等于“把所有小速度强制抬高”。很多时候小于阈值的速度会被认为是停止或被 deadband 压成 0。

### costmap 参数

| 参数 | 作用 |
| --- | --- |
| `robot_radius` | 小车半径估计 |
| `inflation_radius` | 障碍物膨胀距离 |
| `cost_scaling_factor` | 代价衰减速度 |

讲解现象：RViz2 中 `/global_costmap/costmap` 或 `/local_costmap/costmap` 看起来墙更厚，不一定是地图错，而是 Nav2 为了安全把障碍物膨胀了。

## 第 15 课：URDF 模型节点 `xc_urdf`

### 节点职责

包：`src/xc_urdf`

URDF 用来描述机器人模型。当前主要作用是让 VM RViz2 显示真实车体，而不是只看到坐标轴。

启动由 `bringup.launch.py use_urdf:=true` 间接完成，会包含 `xc_urdf/display.launch.py`，通过 `robot_state_publisher` 发布 `/robot_description` 和模型 TF。

### 关键文件

| 文件 | 作用 |
| --- | --- |
| `src/xc_urdf/urdf/xc_urdf.urdf` | 机器人结构描述 |
| `src/xc_urdf/meshes/*.STL` | 车体 mesh |
| `src/xc_urdf/launch/display.launch.py` | 启动 robot_state_publisher |

### VM 为什么也要有 `xc_urdf`

RViz2 解析 URDF 中的：

```text
package://xc_urdf/meshes/base_link.STL
```

是在 VM 本机查找 mesh 文件。所以 Muse Pi 和 VM 都要构建并 source `xc_urdf`。

```bash
colcon build --symlink-install --packages-select xc_urdf
source install/setup.bash
```

### 常见问题

| 现象 | 原因 | 处理 |
| --- | --- | --- |
| RobotModel 不显示 | VM 找不到 `xc_urdf` | VM 上构建并 source |
| mesh resource 加载失败 | STL 没装到 share 目录 | 重建 `xc_urdf` |
| 模型穿模 | URDF mesh 原点偏低 | 当前已上抬，最低点约 `z=0.005m` |
| 模型朝向反 | CAD 外观和 `base_link` 不一致 | 当前用 `base_link -> xc_model_link` 修正显示方向 |

### URDF 在课堂上怎么讲

URDF 不负责让小车导航，但它让学生更直观看到“机器人本体”和“坐标系”的关系。

| 内容 | 专业解释 | 课堂说法 |
| --- | --- | --- |
| `link` | 机器人刚体部件 | 车体、轮子、雷达支架 |
| `joint` | link 之间的连接关系 | 哪个部件安装在哪个部件上 |
| `visual` | RViz2 中显示的几何体 | 让模型看起来像真实小车 |
| `collision` | 碰撞几何 | 后续仿真或更精确 footprint 可用 |
| `robot_state_publisher` | 读取 URDF 并发布 TF/robot_description | 把模型结构广播给 RViz2 |

在本项目中，导航主要还是使用 `base_link`、`laser_link`、`odom`、`map` 这些 frame。URDF 当前更重要的作用是教学和可视化：让 VM RViz2 中看到真实小车姿态，帮助判断“车头是否反了”“模型是否跟随 odom 移动”。

### VM 中显示 RobotModel 的完整步骤

1. VM 上构建模型包。

```bash
cd ~/ros2_car
colcon build --symlink-install --packages-select xc_urdf
source install/setup.bash
```

2. Muse Pi 上启动时打开 URDF 发布，但不打开 RViz2。

```bash
ros2 launch my_pkg bringup.launch.py port:=/dev/ttyUSB0 lidar_port:=/dev/ttyUSB1 use_lidar:=true use_urdf:=true use_urdf_rviz:=false use_keyboard_control:=true cmd_wheel_speed_sign:=-1.0 odom_wheel_speed_sign:=1.0
```

3. VM 上确认能收到模型描述。

```bash
ros2 topic echo /robot_description --once
ros2 pkg prefix xc_urdf
```

4. RViz2 添加 `RobotModel`，Description Topic 使用 `/robot_description`。

如果 `RobotModel` 状态是 OK 但看不到车，通常是视角、Fixed Frame 或 TF 问题；如果报 mesh resource 错误，通常是 VM 找不到 `xc_urdf` 的 STL 文件。

## 第 16 课：VM RViz2 综合显示

Muse Pi 只运行节点，VM 运行 RViz2。

VM 端：

```bash
cd ~/ros2_car
source install/setup.bash
rviz2
```

RViz2 推荐显示项：

| Display | 设置 |
| --- | --- |
| Global Options | Fixed Frame 选 `map` |
| TF | 显示坐标树 |
| LaserScan | Topic 选 `/scan` |
| Map | Topic 选 `/map` |
| RobotModel | 使用 `/robot_description` |
| Path | Topic 选 `/plan` |
| Path | Topic 选 `/local_plan` |
| Map | Topic 选 `/local_costmap/costmap` |
| Map | Topic 选 `/global_costmap/costmap` |

如果 Fixed Frame 选 `map` 后报错，先确认 GMapping 已经发布 `map -> odom`。

### 推荐课堂 RViz2 配置顺序

不要一开始就把所有 Display 打开。小白看到太多图层会分不清每层意义，建议按节点进度逐步添加：

1. 只讲底盘和 TF 时：

| Display | 设置 |
| --- | --- |
| TF | 打开坐标轴 |
| RobotModel | 看车体是否跟随 odom |

2. 讲雷达时：

| Display | 设置 |
| --- | --- |
| LaserScan | `/scan` |
| Fixed Frame | 先用 `base_link`，确认雷达相对车体方向 |

3. 讲建图时：

| Display | 设置 |
| --- | --- |
| Fixed Frame | `map` |
| Map | `/map` |
| LaserScan | `/scan` |
| TF | 确认 `map -> odom -> base_link -> laser_link` |

4. 讲导航时：

| Display | 设置 |
| --- | --- |
| Path | `/plan` |
| Path | `/local_plan` |
| Map | `/local_costmap/costmap` |
| Map | `/global_costmap/costmap` |

### 每个图层看什么

| 图层 | 用来判断什么 |
| --- | --- |
| TF | 坐标系是否连通，方向是否正确 |
| RobotModel | 模型是否显示、车头方向是否对 |
| LaserScan | 雷达方向、障碍物位置、数据质量 |
| `/map` | GMapping 建图质量 |
| `/plan` | 全局路径是否生成 |
| `/local_plan` | 控制器眼前轨迹是否合理 |
| local_costmap | 近处障碍物是否被识别 |
| global_costmap | 全局规划是否受膨胀层影响 |

## 综合排查顺序

遇到问题不要直接改 Nav2 参数，按这个顺序排查：

1. 查节点：`ros2 node list`
2. 查话题：`ros2 topic list`
3. 查雷达：`ros2 topic hz /scan`
4. 查底盘：`ros2 topic echo /odom --once`
5. 查 TF：`ros2 run tf2_ros tf2_echo odom base_link`
6. 查雷达 TF：`ros2 run tf2_ros tf2_echo base_link laser_link`
7. 查地图：`ros2 topic echo /map_metadata --once`
8. 查 `map -> odom`：`ros2 run tf2_ros tf2_echo map odom`
9. 查 Nav2：`ros2 lifecycle get /controller_server`
10. 查速度：`ros2 topic echo /cmd_vel`

## 讲课建议

每讲一个节点，都固定回答七个问题：

1. 这个节点解决什么问题？
2. 代码文件在哪里？
3. 它订阅什么？
4. 它发布什么？
5. 它有哪些关键参数？
6. 关键代码是哪几段？
7. 启动后用什么命令验证？

这样学生会形成稳定的 ROS2 调试习惯：先看数据流，再看 TF，再看参数，最后才修改代码。
