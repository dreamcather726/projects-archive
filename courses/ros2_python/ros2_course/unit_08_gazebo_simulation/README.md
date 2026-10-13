# 第 8 模块：机器人建模与 Gazebo 物理仿真

课程材料：[知识点详解](第8章_授课讲义.md) · [实验说明与代码](experiments/01_gazebo_lidar/实验说明.md)

## 内容范围

- RViz 可视化与 Gazebo 物理仿真的区别。
- 世界、动力学机器人和模拟激光雷达。
- `ros_gz` 的 Gazebo Transport/ROS 2 话题桥接。
- Python 仿真传感器读取和速度命令发布。

## 详细知识点

### 1. 仿真系统组成

- World、Model、Link、Joint、Collision、Visual、Sensor、System Plugin。
- URDF 更偏机器人结构描述，SDF 能完整描述仿真世界、模型、传感器和插件。
- Gazebo Sim 与旧版 Gazebo Classic 的命名和工具差异。
- Jazzy 课程环境使用 Gazebo Harmonic 与 `ros_gz`。

### 2. 物理参数

- 质量、质心、惯性矩阵影响加速和稳定性。
- 碰撞几何应比高精度外观网格简单。
- 摩擦系数过小会打滑，过大或接触参数不合理会造成抖动。
- 仿真步长、实时因子和传感器更新率的关系。
- 重力、地面和静态障碍物。

### 3. 差速驱动

- 左右轮速度如何形成直行与转向。
- 轮半径和轮距配置错误对里程计的影响。
- `cmd_vel` 的线速度 x 与角速度 z。
- DiffDrive system 接收速度命令并输出里程计。
- 控制命令超时与失控安全。

### 4. 传感器仿真

- 激光雷达角度范围、角分辨率、最小/最大距离和更新率。
- 噪声模型与“完美仿真数据”的局限。
- 摄像头、深度相机和 IMU 的基本配置位置。
- 传感器坐标系必须与机器人模型一致。

### 5. ros_gz 桥接

- Gazebo Transport 与 ROS 2 是两个通信系统。
- `ros_gz_bridge parameter_bridge` 负责消息转换。
- GZ_TO_ROS、ROS_TO_GZ 和双向桥接。
- 桥接 `/scan`、`/odom`、`/cmd_vel` 和 `/clock`。
- YAML 桥接配置适合较多话题；命令参数适合最小演示。
- `use_sim_time` 让 ROS 节点跟随仿真时钟。

### 6. 仿真调试

- 先确认 Gazebo 内部模型和传感器，再检查 bridge，最后检查 Python 节点。
- 使用 `gz topic -l` 与 `ros2 topic list` 分别观察两侧。
- 使用 `ros2 topic hz` 检查桥接后频率。
- 模型掉落、飞散、轮子打滑和传感器无数据的排查顺序。
- 仿真暂停时 `/clock` 和基于 ROS 时间的定时器可能停止。

## 课堂演示

1. 在空世界中加入地面、灯光和墙体。
2. 加载差速小车并观察重力、碰撞和惯性。
3. 从 ROS 2 命令行发布 `/cmd_vel` 控制机器人。
4. 桥接激光与里程计并用命令行查看。
5. 启动 Python 避障控制器，让小车遇障转向。

## 实验任务

[Gazebo 差速小车、激光雷达与 Python 避障](experiments/01_gazebo_lidar/实验说明.md)

## 常见问题

- 仍使用旧仿真包或旧插件名称，和当前环境不兼容。
- bridge 方向写反，ROS 能看到话题名但没有数据流。
- 轮子关节名称与驱动插件配置不一致。
- 没有 Sensors system，激光传感器不产生数据。
- Python 节点启用仿真时间但没有桥接 `/clock`。

## 验收标准

- 仿真小车不会异常掉落或飞散。
- ROS 2 能收到激光与里程计，并能发送速度命令。
- Python 节点能够根据真实 `/scan` 数据停车或转向。
- 能按 Gazebo 侧、桥接层、ROS 2 侧的顺序定位问题。
