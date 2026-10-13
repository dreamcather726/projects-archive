# 第 1 模块：ROS 2 工程基础

课程材料：[知识点详解](第一单元_知识点详解.md) · [实验说明与代码](experiments/01_robot_heartbeat/实验说明.md)

主题：工作空间、计算图、Python 节点与 `colcon` 编译规范。

## 内容范围

- ROS 2 解决的机器人软件通信与模块协作问题。
- 工作空间、Python 功能包和可执行节点的完整结构。
- 构建、加载环境、运行节点和命令行排错流程。

## 详细知识点

### 1. ROS 2 整体认识

- ROS 2 不是操作系统，而是机器人软件开发框架、通信中间件和工具集合。
- 分布式节点的含义：不同节点可以位于同一进程、不同进程或不同计算机。
- 计算图基本实体：节点、话题、服务、动作、参数。
- DDS/RMW 的分层概念：应用使用 `rclpy`，底层由 RMW 适配具体 DDS 实现。
- 自动发现的基本过程，以及同一网络、`ROS_DOMAIN_ID` 和防火墙对发现的影响。

### 2. 环境与终端

- ROS 发行版、Ubuntu 版本和软件包之间的对应关系。
- `source /opt/ros/jazzy/setup.bash` 的作用。
- 底层环境与覆盖工作空间的加载顺序。
- 常用环境变量：`ROS_DISTRO`、`ROS_DOMAIN_ID`、`RMW_IMPLEMENTATION`。
- 每个新终端都需要重新加载环境；如何写入 shell 配置以及何时不应永久写入。

### 3. 工作空间与功能包

- 工作空间的 `src/`、`build/`、`install/`、`log/` 四类目录。
- 源码空间、构建空间和安装空间的区别。
- `ros2 pkg create --build-type ament_python` 创建 Python 包。
- Python 功能包组成：`package.xml`、`setup.py`、`setup.cfg`、`resource/`、Python 模块目录。
- `package.xml` 中名称、版本、维护者、许可证及运行依赖的作用。
- `setup.py` 中 `data_files`、`install_requires`、`entry_points/console_scripts` 的作用。
- 包名、模块名、节点名、可执行入口名的区别。

### 4. colcon 构建规范

- `colcon build` 的基本流程和构建顺序。
- `--packages-select` 只构建指定包。
- `--symlink-install` 对 Python 开发调试的意义。
- 修改 Python 源码、入口、依赖或资源文件后，哪些情况需要重新构建。
- 构建成功不等于运行环境已加载；理解 `install/setup.bash`。
- 依赖声明、`rosdep` 与“本机能运行但换机器失败”的关系。

### 5. 第一个 rclpy 节点

- `rclpy.init()`、`Node`、`rclpy.spin()`、`destroy_node()`、`rclpy.shutdown()` 的生命周期。
- 继承 `Node` 与在构造方法中创建通信实体。
- 使用 `get_logger()` 输出不同级别的日志。
- 使用 `create_timer()` 创建周期任务，理解回调函数。
- `KeyboardInterrupt` 下的资源清理和安全退出。

### 6. ROS 2 命令行工具

- `ros2 node list/info` 查看节点与接口。
- `ros2 topic list/type/info/echo` 查看话题。
- `ros2 pkg list/prefix/executables` 检查包和入口。
- `ros2 run <包名> <入口名>` 的查找过程。
- 用 `--ros-args` 设置日志级别、节点名和命名空间。

## 课堂演示

1. 创建 `ros2_ws/src` 和 `robot_basics_py` 功能包。
2. 编写 `hello_node.py`，每秒输出一次机器人状态。
3. 注册 `console_scripts`，使用 `colcon build --symlink-install` 构建。
4. 运行节点，用 `ros2 node list` 和 `ros2 node info` 观察计算图。
5. 故意不加载工作空间，演示“找不到包”的原因和恢复方法。

## 实验任务

实现 `robot_heartbeat` 节点：定时输出节点名、运行秒数和当前状态，并支持通过 ROS 参数修改输出周期。参数的完整知识将在第 4 模块展开，本实验只完成基本使用。

## 常见问题

- 混淆功能包名称、Python 模块名称和节点名称。
- 忘记在 `setup.py` 中注册入口，导致 `ros2 run` 找不到可执行程序。
- 构建后忘记加载 `install/setup.bash`。
- `package.xml` 漏写依赖，导致换环境后运行失败。
- 在错误目录构建，产生多层嵌套工作空间。

## 验收标准

- 能从空目录创建工作空间和 `ament_python` 功能包。
- `colcon build` 无错误，`ros2 run` 可以启动节点。
- `ros2 node info` 能看到节点的通信实体。
- 能解释 `src`、`build`、`install`、`log` 的用途。
