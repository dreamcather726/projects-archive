# ROS2 小车建图操作步骤

本文记录 Muse Pi 上 ROS2 小车从导入工程到完成 GMapping 建图的完整操作流程。

## 0. 当前实车整套启动命令

下面三条命令是当前实车验证通过的完整启动流程。每条命令放在一个独立终端中运行。

终端 1：启动底盘、雷达、URDF 和键盘控制。

```bash
ros2 launch my_pkg bringup.launch.py port:=/dev/ttyUSB0 lidar_port:=/dev/ttyUSB1 use_lidar:=true use_urdf:=true use_urdf_rviz:=false use_keyboard_control:=true cmd_wheel_speed_sign:=-1.0 odom_wheel_speed_sign:=1.0
```

终端 2：启动实时建图。

```bash
ros2 launch my_pkg live_mapping.launch.py
```

终端 3：启动实时导航。Muse Pi 不运行 RViz2，所以这里保持 `use_rviz:=false`。

```bash
ros2 launch my_pkg live_navigation.launch.py use_rviz:=false
```

当前串口和方向参数含义：

| 参数 | 当前值 | 含义 |
| --- | --- | --- |
| `port` | `/dev/ttyUSB0` | ESP32 底盘串口 |
| `lidar_port` | `/dev/ttyUSB1` | YDLidar 雷达串口 |
| `cmd_wheel_speed_sign` | `-1.0` | 修正 `/cmd_vel` 发到底盘后的运动方向 |
| `odom_wheel_speed_sign` | `1.0` | ESP32 回传轮速生成 `/odom` 时不再反向 |

## 1. 导入全部文件

把 ROS2 小车工程完整放到 Muse Pi，例如：

```bash
~/ros2_car
```

ROS2 工作区目录建议保持下面的结构：

```text
~/ros2_car/
  src/
    my_pkg/
    openslam_gmapping/
    slam_gmapping/
    ydlidar_ros2_driver/
```

`ydlidar_ros2_driver` 是 ROS2 驱动包，需要放在 `src/` 下。

`YDLidar-SDK` 不是 ROS2 包，不建议放进 `src/`。建议单独放在：

```bash
~/YDLidar-SDK
```

## 2. 安装 YDLidar SDK

进入 SDK 源码目录并编译安装：

```bash
cd ~/YDLidar-SDK
mkdir -p build
cd build
cmake .. -DBUILD_TEST=OFF
make -j4
sudo make install
```

`-DBUILD_TEST=OFF` 用来关闭 SDK 自带测试，避免 gtest 或 C++ 标准版本导致无关编译失败。

安装完成后检查 SDK 头文件和库是否存在：

```bash
ls /usr/local/include/src/CYdLidar.h
ls /usr/local/lib/libydlidar_sdk.a
```

如果这两个文件都能看到，说明 YDLidar SDK 安装成功。

如果提示 `No such file or directory`，说明 SDK 没有安装成功，需要回到 `~/YDLidar-SDK/build` 重新执行：

```bash
cmake .. -DBUILD_TEST=OFF
make -j4
sudo make install
```

## 3. 安装 pyserial

底盘 ESP32 串口通信需要 Python 串口库 `pyserial`。

优先使用系统 apt 安装：

```bash
sudo apt update
sudo apt install -y python3-serial
```

安装后检查 Python 是否能导入 `serial`：

```bash
python3 -c "import serial; print(serial.__version__)"
```

如果 `apt` 源里没有 `python3-serial`，再使用 `pip` 安装：

```bash
python3 -m pip install --user pyserial
```

## 4. 配置串口权限

先查看当前串口设备：

```bash
ls -l /dev/ttyUSB*
```

当前实车串口分配：

```text
底盘 ESP32: /dev/ttyUSB1
激光雷达:   /dev/ttyUSB0
```

底盘和雷达不能使用同一个串口设备。

如果启动底盘或雷达时提示没有串口权限，例如：

```text
Permission denied: '/dev/ttyUSB0'
Permission denied: '/dev/ttyUSB1'
```

可以先用临时方式给当前设备加权限：

```bash
sudo chmod 666 /dev/ttyUSB0
sudo chmod 666 /dev/ttyUSB1
```

这个方法重启或重新插拔 USB 后可能失效，适合临时测试。

长期方式是把当前用户加入 `dialout` 组：

```bash
sudo usermod -aG dialout $USER
```

执行后需要退出 SSH 或重启 Muse Pi，让组权限重新加载：

```bash
sudo reboot
```

重新登录后检查当前用户是否已经在 `dialout` 组：

```bash
groups
```

如果输出里能看到 `dialout`，通常就可以直接访问 `/dev/ttyUSB0` 和 `/dev/ttyUSB1`。

## 5. 编译 ROS2 工作区环境

回到 ROS2 工作区根目录：

```bash
cd ~/ros2_car
source /opt/ros/humble/setup.bash
colcon build --symlink-install
```

`--symlink-install` 的作用是让 Python、launch、yaml 等文件在 `install/` 中尽量使用软链接。开发调试时，修改这些文件后通常只需要重新启动节点，不一定每次都重新编译。

编译完成后加载工作区环境：

```bash
source install/setup.bash
```

如果最后看到类似输出：

```text
Summary: 4 packages finished
```

说明工作区包已经编译完成。

如果修改了 C++ 源码，例如 `ydlidar_ros2_driver` 或 `slam_gmapping` 的 `.cpp` 文件，需要重新执行：

```bash
colcon build --symlink-install
source install/setup.bash
```

## 6. 启动底盘和雷达

终端 1 启动底盘、TF 和雷达，不启动 GMapping：

```bash
cd ~/ros2_car
source /opt/ros/humble/setup.bash
source install/setup.bash

ros2 launch my_pkg bringup.launch.py port:=/dev/ttyUSB0 lidar_port:=/dev/ttyUSB1 use_lidar:=true use_urdf:=true use_urdf_rviz:=false use_keyboard_control:=true cmd_wheel_speed_sign:=-1.0 odom_wheel_speed_sign:=1.0
```

`cmd_wheel_speed_sign:=-1.0` 用来修正前进命令发到底盘后的运动方向；`odom_wheel_speed_sign:=1.0` 表示 ESP32 回传轮速生成 `/odom` 时不再反向。它们只修正底盘速度方向，不修改雷达 `/scan` 或地图 `/map`。

这个命令会启动：

```text
esp32_bridge_node
tf_tree_node
ydlidar_ros2_driver_node
```

## 7. 检查底盘、雷达和 TF 数据

另开终端检查 `/scan`、`/odom` 和 TF：

```bash
cd ~/ros2_car
source /opt/ros/humble/setup.bash
source install/setup.bash

ros2 topic hz /scan
ros2 topic hz /odom
ros2 run tf2_ros tf2_echo odom base_link
ros2 run tf2_ros tf2_echo base_link laser_link
```

GMapping 需要下面这条 TF 链正常：

```text
odom -> base_link -> laser_link
```

如果出现：

```text
Message Filter dropping message: frame 'laser_link'
discarding message because the queue is full
```

通常说明 `/scan` 有数据，但 GMapping 等不到匹配时间戳的 TF。优先检查 `/odom` 是否有频率，以及两个 `tf2_echo` 是否正常。

## 8. 启动 GMapping 建图

确认 `/scan`、`/odom` 和 TF 稳定后，另开终端启动 GMapping：

```bash
cd ~/ros2_car
source /opt/ros/humble/setup.bash
source install/setup.bash

ros2 run slam_gmapping slam_gmapping
```

也可以使用项目里的 launch 文件启动：

```bash
ros2 launch slam_gmapping gmapping_x3_launch.py
```

如果能看到类似输出，说明 GMapping 已经开始处理雷达数据：

```text
Registering First Scan
Registering Scans:Done
```

## 9. 启动键盘控制

另开终端启动键盘控制节点：

```bash
cd ~/ros2_car
source /opt/ros/humble/setup.bash
source install/setup.bash

ros2 run my_pkg keyboard_control_node --ros-args \
  -p linear_speed_m_s:=0.10 \
  -p angular_speed_rad_s:=0.90 \
  -p publish_rate_hz:=50.0 \
  -p key_timeout_sec:=0.30
```

按键说明：

```text
W: 前进
S: 后退
A: 左转
D: 右转
Space 或 X: 停止
```

当前键盘控制会一次性读完缓冲区中的按键，支持 `W+A`、`W+D` 组合键。`publish_rate_hz:=50.0` 表示 20ms 发布一次 `/cmd_vel`，会比原来的 10Hz 更灵敏。

键盘控制节点所在终端需要保持在前台，按键才会被节点接收。建图时建议慢速移动小车，避免快速原地旋转，否则地图容易拉伸或变形。

## 10. 用 RViz 查看地图

如果是在 VM 上查看地图，就在 VM 终端启动 RViz2。Muse Pi 只负责发布 `/scan`、`/odom`、TF 和 `/map`，不需要打开 RViz2。

```bash
rviz2
```

RViz 中设置：

```text
Fixed Frame: map
Add -> TF
Add -> By topic -> /scan -> LaserScan
Add -> By topic -> /map -> Map
```

如果地图刷新太慢，可以临时提高 GMapping 的 `/map` 发布频率：

```bash
ros2 run slam_gmapping slam_gmapping --ros-args -p map_update_interval:=0.5
```

查看 `/map` 实际刷新频率：

```bash
ros2 topic hz /map
```

## 11. 启动实时建图导航

如果需要一边 GMapping 实时建图，一边使用 Nav2 导航，保持前面的底盘、雷达和 GMapping 终端继续运行，再另开一个终端启动导航：

```bash
cd ~/ros2_car
source /opt/ros/humble/setup.bash
source install/setup.bash

ros2 launch my_pkg live_navigation.launch.py use_rviz:=false
```

启动前应先确认 GMapping 已经发布 `/map`，并且 `map -> odom` TF 正常：

```bash
ros2 topic hz /map
ros2 run tf2_ros tf2_echo map odom
```

Nav2 启动后检查生命周期状态：

```bash
ros2 lifecycle get /planner_server
ros2 lifecycle get /controller_server
ros2 lifecycle get /bt_navigator
ros2 lifecycle get /behavior_server
```

正常应显示：

```text
active [3]
```

如果 RViz2 在 VM 上运行，在 VM 终端启动：

```bash
rviz2
```

RViz 中设置：

```text
Fixed Frame: map
Add -> By topic -> /map -> Map
Add -> By topic -> /scan -> LaserScan
Add -> TF
Add -> Path -> /plan
Add -> Path -> /plan_smoothed
Add -> Path -> /local_plan
Add -> Map -> /global_costmap/costmap
Add -> Map -> /local_costmap/costmap
```

在 RViz 顶部工具栏选择 `Nav2 Goal`，在地图上点击目标点。发出目标后可用下面命令验证路径和速度输出：

```bash
ros2 topic echo /plan --once
ros2 topic echo /cmd_vel
```

如果 `/plan` 有 `poses`，说明全局路径已经规划出来；如果 `/cmd_vel` 有线速度或角速度输出，说明 Nav2 已经在向底盘发送导航速度。

如果还要显示小车 URDF 模型，Muse Pi 上启动底盘时加上 `use_urdf:=true use_urdf_rviz:=false`：

```bash
ros2 launch my_pkg bringup.launch.py use_lidar:=true use_urdf:=true use_urdf_rviz:=false
```

VM 上也需要有 `xc_urdf` 包并 source 对应工作区，否则 RViz2 的 `RobotModel` 可能找不到 `package://xc_urdf/meshes/...` 下的 STL 文件。

如果更新过 URDF，例如修正车体穿模高度，Muse Pi 和 VM 都需要重新构建 `xc_urdf`：

```bash
cd ~/ros2_car
colcon build --symlink-install --packages-select xc_urdf
source install/setup.bash
```

然后 Muse Pi 重新启动 `bringup.launch.py`，VM 重新打开 RViz2 或删除后重新添加 `RobotModel`。当前 `xc_urdf` 已把模型整体抬高，最低点约为 `z=0.005m`，正常情况下地图平面不应再从车体中间穿过。车模外观通过 `base_link -> xc_model_link` 固定关节旋转 180 度来对齐显示方向；这只影响 RViz2 里的 RobotModel，不修改 `/scan`、`/map` 或导航用的 `base_link`。

如果需要单独启动 RViz，手动运行：

```bash
rviz2
```

## 12. 保存地图

地图看起来可用后保存：

```bash
mkdir -p ~/ros2_car/maps
ros2 run nav2_map_server map_saver_cli -f ~/ros2_car/maps/first_map
```

生成文件：

```text
~/ros2_car/maps/first_map.yaml
~/ros2_car/maps/first_map.pgm
```
