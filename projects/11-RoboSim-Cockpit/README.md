# VMGUI

这个目录用于开发没有实车时的 ROS2 小车模拟数据工程和上位机 cockpit GUI 工程。

## 工程划分

- `ros_car_simulator`：模拟小车、雷达、IMU、里程计、电池、Topic 状态和摄像头图像数据。
- `ros_car_cockpit`：上位机 GUI 程序，后续做成类似游戏控制台的界面，中间显示 RViz2/摄像头，左右显示状态和控制面板。

## 推荐启动方式

```bash
cd ~/VMGUI
colcon build --symlink-install
source install/setup.bash
ros2 launch ros_car_simulator simulator.launch.py
ros2 launch ros_car_cockpit cockpit.launch.py
```

第一版先跑通模拟数据和 GUI 框架，后续再把真实 MusePi/ESP32 数据接进来。
