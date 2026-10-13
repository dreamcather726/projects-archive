# ROS 2 Gazebo模型与传感器模拟：逐页教师讲义

对应课件：`2026.8.20 Gazebo + 模型 + 传感器模拟 完全指南.pptx`，共 28 页。

严格一页对应一节；讲稿、命令行实验与技术修正均放在原页面位置。

## 上课前准备

### 执行目录与课前准备

**本课所有命令均从整套讲义的根目录执行**，也就是同时能看到 `第一课`、`第二课`、`第三课` 等文件夹的位置。请先在 WSL Ubuntu 的 Bash 中进入此目录；每个新终端都保持在该目录。若终端已位于 `第七课`，先执行 `cd ..` 回到讲义根目录。

Python 程序直接使用已提供的 `第七课/课堂实验/` 文件，无需复制到另一份脚本目录。第三课配置直接使用 `第三课/课堂实验/config/`。只有需要 colcon 构建的 ROS 包才由准备脚本写入旁边的 `../ros2_ws/src/`，构建在子 Shell 中进行，完成后当前终端仍位于讲义根目录。

```bash
source /opt/ros/humble/setup.bash
test -d "第七课/课堂实验" || { echo "请先进入整套讲义根目录"; exit 1; }
mkdir -p tf_debug
sudo apt update && sudo apt install -y ros-humble-gazebo-ros-pkgs ros-humble-gazebo-plugins ros-humble-xacro ros-humble-robot-state-publisher ros-humble-tf2-ros ros-humble-launch-ros liburdfdom-tools python3-colcon-common-extensions python3-rosdep && \
python3 prepare_course.py 7 --workspace ../ros2_ws && \
(cd ../ros2_ws && colcon build --packages-select my_robot_description --symlink-install) && \
source ../ros2_ws/install/setup.bash
```

节点持续运行时，后面的查询命令应在另一个终端执行。各终端使用相同 ROS_DOMAIN_ID，先停止旧实验再启动新实验。

所有 Bash 命令都在 WSL Ubuntu 22.04、ROS 2 Humble 中执行。每个新终端先 `source /opt/ros/humble/setup.bash`；使用本地包时再 `source ../ros2_ws/install/setup.bash`。三个终端的 ROS_DOMAIN_ID 一致。

课堂终端：A 运行生产者/主体进程，B 控制或接收，C 查询与记录。持续命令用 `Ctrl+C` 结束；同一个终端不能在前台进程运行时继续执行下一条命令。实验切换前停止上个实验的发布者，避免同名话题、节点或 TF 混入。

随讲义交付的完整源码在 `第七课/课堂实验/`，直接使用这些文件。

前置知识：完成第六课模型包与 TF，知道 QoS 和仿真时间。本课环境明确是 Gazebo Classic 11；依赖不足时按此版本安装：
```bash
sudo apt install ros-humble-gazebo-ros-pkgs ros-humble-gazebo-plugins
gazebo --version
ros2 pkg prefix gazebo_ros
```
保留第六课包，添加本课文件，不覆盖原模型与 display 启动文件：
```bash
python3 "第七课/课堂实验/install_extensions.py" ../ros2_ws/src/my_robot_description
rosdep install --from-paths ../ros2_ws/src/my_robot_description --ignore-../ros2_ws/src --rosdistro humble -y
(cd ../ros2_ws && colcon build --packages-select my_robot_description --symlink-install)
source ../ros2_ws/install/setup.bash
ros2 run xacro xacro "$(ros2 pkg prefix --share my_robot_description)/urdf/my_robot_sim.xacro"   wheel_radius:=0.032 wheel_sep:=0.20 base_length:=0.24 base_width:=0.16 -o tf_debug/my_robot_sim.urdf
check_urdf tf_debug/my_robot_sim.urdf
```
统一实体 my_robot，world my_world；输入 /cmd_vel，输出 /odom、/scan、/imu、/joint_states、/clock；TF 为 odom→base_footprint→base_link→轮子与传感器。本课不启动第六课 JSP GUI；diff_drive 不发布轮 TF，RSP 根据真实仿真关节状态发布。完整 world 自包含地面、光照、墙，无需下载模型。

---



使用 rosdep 前，先初始化并更新规则；已初始化时跳过 init。

```bash
if [ ! -f /etc/ros/rosdep/sources.list.d/20-default.list ]; then
  sudo rosdep init
fi
rosdep update --rosdistro humble
```

## 第 1 页｜Gazebo + 模型 + 传感器模拟完全指南

这页你可以把它讲成：

> **今天不只把机器人画出来，还让模型受重力、接触地面、响应速度并产生传感器数据。**

### 讲解与例子

沿开场路径：模型接入、差速驱动、激光雷达、IMU，再用命令观察输入输出。实验限定本机 Gazebo Classic 11，不连接实体机器人。上一课模型保留为基线，扩展文件提供仿真属性和插件。

### 板书

```text
模型 + 物理属性 + 插件 → 运动与传感器数据
```

### 课堂提问

> RViz 中轮子 TF 转动就证明地面摩擦正常吗？

参考答案：不能，RViz 不计算地面接触。

这页最后可以总结：

> **动力学现象要在仿真链路里验证。**

---

## 第 2 页｜课程目录

这页你可以把它讲成：

> **按实际 28 页完成六个主题，目录中的完整实战和排查放进相关现有页。**

### 讲解与例子

第一部分物理环境，第二部分模型属性，第三部分插件，第四部分驱动，第五部分传感器，第六部分世界；实际末页总结。第 13 页手工生成、第 27 页一键集成，第 28 页验收和排查，不凭目录补造第七、八部分页码。

### 板书

```text
环境 → 模型 → 插件 → 运动 → 感知 → 世界 → 验收
```

### 课堂提问

> 为什么先调通空模型再讨论 SLAM？

参考答案：导航算法依赖底层里程计、传感器、TF 和时间链路。

这页最后可以总结：

> **底层链路先验证，上层算法才有可信输入。**

---

## 第 3 页｜Gazebo 物理仿真环境

这页你可以把它讲成：

> **先确认我们使用哪个仿真器和哪套插件，再开始实验。**

### 讲解与例子

本课固定 Classic 11 与 gazebo_ros_pkgs，术语和命令均在这条版本线。模型、世界、传感器和插件构成仿真数据生产链，ROS CLI 用来观察。无 GUI 启动适合演示数据，不代表所有渲染传感器都无需图形资源。

### 板书

```text
版本确认 → 世界启动 → ROS 数据观察
```

### 课堂提问

> 看到 gz 命令就能判定是哪个 Gazebo 版本吗？

参考答案：不能，不同安装也可能提供 gz 工具，先查版本与帮助。

这页最后可以总结：

> **先固定工具链，避免版本混用。**

---

## 第 4 页｜为什么 RViz 不够，还需要 Gazebo

这页你可以把它讲成：

> **RViz 显示别人提供的数据；Gazebo 计算模型的运动和传感器输出。两者可以共享同一套 ROS 话题接口。**

### 讲解与例子

按表比较物理、数据源、时钟和资源。RViz 也能设置 use_sim_time，并非只能用系统时间；Gazebo 中轮子运动并非模型自动获得，需要驱动或控制机制。迁移真机接口相同有助复用，但标定、噪声、延迟与硬件行为仍要适配，不能保证算法零修改。

### 板书

```text
Gazebo：物理/传感器生产者
RViz：模型与数据消费者
```

### 命令行实验

先使用完整仿真 launch（第 27 页解释集成），A：
```bash
ros2 launch my_robot_description gazebo.launch.py gui:=false
```
C：
```bash
ros2 topic list -t
ros2 topic echo /clock --once
ros2 topic echo /odom --once
```
预期 spawn 成功后产生数据。图形不是前提，CLI 输出来自仿真。若未出现 odom，先看 A 的库加载/生成日志。后续实验可保留此 A，手工生成实验前必须停止整套 launch。

### 课堂提问

> 暂停 Gazebo 后 RViz 一定停止渲染吗？

参考答案：不一定，界面仍可操作；仿真数据时间与更新可能暂停。

这页最后可以总结：

> **区分界面更新、物理推进与数据时间。**

---

## 第 5 页｜Gazebo 的核心概念

这页你可以把它讲成：

> **World 包含 Model，模型包含连杆、关节与传感器；插件把仿真行为接到 ROS 接口。**

### 讲解与例子

沿层级图讲 visual/collision/inertial 的职责，sensor 描述采样，plugin 描述扩展能力。SDF 是 Gazebo 原生格式，URDF 可转换为 SDF，但不应说两种独立规范是简单包含关系。没有驱动插件的动态模型仍会受重力和碰撞，不是物理上完全死物体。

### 板书

```text
World → Model → Link/Joint/Sensor
Plugin → 行为与 ROS 接口
```

### 课堂提问

> 只写 visual 的墙能挡住车吗？

参考答案：需要 collision 才参与接触计算。

这页最后可以总结：

> **世界层级与物理属性决定基础现象。**

---

## 第 6 页｜物理引擎与仿真步长

这页你可以把它讲成：

> **仿真时间是模型里的时间，机器的真实秒钟是另一条轴，暂停后就能看到差别。**

### 讲解与例子

max_step_size 表示一次推进的仿真秒数，update_rate 表示期望更新速率；乘积支持理解目标速率，实际 RTF 受负载影响。更小步长通常增加求解次数，但不能保证所有约束更稳定。时间一致性要求相关节点 use_sim_time，并检查 /clock；不要通过消息频率直接断言真实时间因子。

### 板书

```text
仿真步长 × 更新次数 → 目标推进量
/clock → ROS 仿真时间
实际 RTF → 真实测量
```

### 命令行实验

A 保持仿真。C 先查服务名和类型，再暂停/恢复：
```bash
ros2 param get /robot_state_publisher use_sim_time
ros2 service list -t | grep -E 'pause_physics|unpause_physics'
ros2 topic echo /clock
```
C 的 echo 持续观察，B 操作：
```bash
ros2 service call /pause_physics std_srvs/srv/Empty "{}"
ros2 service call /unpause_physics std_srvs/srv/Empty "{}"
```
预期暂停时仿真时间不再推进（是否继续重复旧 /clock 由插件配置决定），恢复后递增。名字有命名空间时用 list 的实际服务名，不凭空补服务。结束 C echo，确认恢复运行。

### 课堂提问

> paused 时 use_sim_time 节点的定时器一定按墙钟继续触发吗？

参考答案：使用 ROS 仿真时钟的定时器通常随仿真时间暂停，墙钟定时器另看实现。

这页最后可以总结：

> **时间源和定时器类型要一起检查。**

---

## 第 7 页｜Gazebo Classic vs 新一代 Gazebo

这页你可以把它讲成：

> **Classic 的 gazebo_ros 插件与新 Gazebo 的系统插件和桥接不能相互照抄。**

### 讲解与例子

本课命令 gazebo、gazebo_ros launch 和 libgazebo_ros_*.so 对应 Classic。新版本使用不同系统与 ros_gz 集成方案，Humble 也可搭配不同版本线，不能只按 ROS 发行版猜 Gazebo。Classic 已结束维护，本课为了复现现有课件仍限定旧环境，不把它当新项目通用推荐。

### 板书

```text
Classic：gazebo_ros + libgazebo_ros_*.so
新 Gazebo：系统插件 + ros_gz 集成
```

### 命令行实验

课前或 C 查询：
```bash
gazebo --version
ros2 pkg prefix gazebo_ros
ls /opt/ros/humble/lib/libgazebo_ros_*.so | head
gz --help
```
预期 Classic 11 与 gazebo_ros 安装目录。若只有 gz sim/ros_gz，不继续加载 Classic 插件，改用对应资料准备合适环境。

### 课堂提问

> Jazzy 项目能直接复制本课插件库名吗？

参考答案：不能，必须按实际仿真器与 ROS 集成版本核对。

### 教师说明

版本信息参见 [Gazebo 官方迁移资料](https://gazebosim.org/docs/latest/gazebo_classic_migration/)，本课不自动迁移原课件。

这页最后可以总结：

> **示例的可用范围由版本组合决定。**

---

## 第 8 页｜让 URDF 模型能在 Gazebo 中仿真

这页你可以把它讲成：

> **保留第六课结构，再补物理与插件配置，观察“能显示”和“能稳定接触”之间的区别。**

### 讲解与例子

先查 collision、mass 和 inertia，再查 gazebo 扩展。模型刚进入世界时的高度也会影响接触，不能仅看文件解析成功就认为动力学可靠。

### 板书

```text
结构基线 → 碰撞与惯性 → 扩展 → 动力学验收
```

### 课堂提问

> RSP 正常启动能证明 Gazebo 插件加载了吗？

参考答案：不能，RSP 不执行 Gazebo 插件。

这页最后可以总结：

> **模型解析和仿真能力分别验证。**

---

## 第 9 页｜URDF 里仿真必需的三件事

这页你可以把它讲成：

> **碰撞决定接触，质量和惯量决定受力后的响应。缺字段或数量级错误，会产生不同异常。**

### 讲解与例子

按课件表说明动态实体需要合适碰撞与惯性。碰撞体可比外观简化；不是每个固定传感器都必须有独立 collision，关键是所需接触和动力学表达。固定关节转换可能把连杆合并到父体；空 base_footprint 是坐标框架，不必人为给它重复加质量。

### 板书

```text
collision：接触几何
mass：平移响应
inertia：旋转响应
```

### 命令行实验

查看生成模型的物理字段，不依赖图形：
```bash
grep -nE 'collision|mass|inertia' tf_debug/my_robot_sim.urdf | head -n 25
check_urdf tf_debug/my_robot_sim.urdf
```
预期驱动轮、车体有碰撞与正质量惯量。空 base_footprint 与不需要独立接触的传感器可不同。解析成功不能证明参数数量级正确，下一页计算检查。

### 课堂提问

> 为所有 frame 随便写同一质量是否正确？

参考答案：不，frame 不一定是独立动态刚体，质量应对应实际模型。

这页最后可以总结：

> **物理参数要对应真实或明确近似的对象。**

---

## 第 10 页｜inertial 的数学原理与计算方法

这页你可以把它讲成：

> **用简单几何体公式得到合理起点，明确惯量关于哪个质心和哪个坐标轴。**

### 讲解与例子

长方体 m(b²+c²)/12 等公式关于质心、沿几何主轴；圆柱默认轴沿 z，轮子沿 y 时要旋转 inertial.origin 或变换矩阵。对角正值与三角不等式是检查线索，完整矩阵还要满足正定与物理合理性。τ=Iα 只是简化表达，一般旋转动力学还含角速度耦合项。

### 板书

```text
几何公式 → 质心坐标系 → 旋转到惯性 frame → 检查数量级
```

### 命令行实验

计算本课车体和轮子的参考惯量：
```bash
python3 - <<'PY'
m, a, b, h = 1.0, 0.24, 0.16, 0.04
print('base Ixx/Iyy/Izz:', m*(b*b+h*h)/12, m*(a*a+h*h)/12, m*(a*a+b*b)/12)
m, r, width = 0.1, 0.032, 0.02
print('cylinder own-z:', m*(3*r*r+width*width)/12, m*(3*r*r+width*width)/12, m*r*r/2)
PY
```
单位 kg·m²；轮子 inertial.origin 绕 x 转 90°，将该局部主轴表达转到 y 安装。不要把这些数无条件用于更重或更大的模型。

### 课堂提问

> 把圆柱 visual 转 90°，惯性矩阵自动跟着转吗？

参考答案：不会，惯性坐标系也要一致；本模型显式设置 inertial.origin rpy。

这页最后可以总结：

> **惯性参数必须连同表达坐标系说明。**

---

## 第 11 页｜惯性参数重要性 & URDF→SDF 转换

这页你可以把它讲成：

> **把展开的 URDF 转成 SDF，看 Gazebo 实际接收了哪些属性；别只查源文件里的标签。**

### 讲解与例子

fixed joint 可能被合并，sensor 和碰撞对象的最终挂接位置需检查转换结果。reference 拼写必须匹配源模型，错误扩展未必导致 XML 解析失败。URDF 已有视觉材质概念，gazebo material 是渲染扩展，不应讲成 URDF 完全没有材质。

### 板书

```text
Xacro → URDF → SDF → 仿真器加载
```

### 命令行实验

已完成课前展开；使用 Classic gz 的 sdf 工具：
```bash
gz sdf --help
gz sdf -p tf_debug/my_robot_sim.urdf > tf_debug/my_robot_sim.sdf
grep -nE '<plugin|<sensor|wheel|inertial' tf_debug/my_robot_sim.sdf | head -n 30
```
预期 SDF 中存在差速和传感器插件；fixed link 可能被合并，所以不要要求名字与 URDF link 清单逐字完全一致。转换失败先确认 gz 版本、URDF 有效和 xacro 已展开。

### 课堂提问

> URDF 解析成功但 sensor 没出现，下一步看什么？

参考答案：看转换后的 SDF 与 Gazebo 插件加载日志，核对 reference 和库。

这页最后可以总结：

> **检查仿真器实际使用的生成模型。**

---

## 第 12 页｜常用 gazebo 子标签与摩擦系数详解

这页你可以把它讲成：

> **摩擦系数改变抓地行为，但同样的数字在不同接触模型和工况下不保证同样效果。**

### 讲解与例子

材质影响外观，mu1/mu2 影响接触切向约束，selfCollide 影响同模型接触。本课驱动轮用 1.0、支撑球 0.1，作为仿真近似，不是物理材料规格。若观察打滑，要对比轮速与世界运动；默认真值里程计会掩盖编码器漂移。本模型明确选编码器 odometry_source=0。

### 板书

```text
轮子角速度 → 接触摩擦 → 真实运动
编码器里程计 ≠ 世界真值
```

### 课堂提问

> odom 前进就证明车体真实前进同样距离吗？

参考答案：编码器里程计可因打滑漂移，应与仿真世界位姿或观感比较。

这页最后可以总结：

> **摩擦实验需要明确里程计来源。**

---

## 第 13 页｜重力、实战配置与模型生成

这页你可以把它讲成：

> **先展开完整模型，再启动世界并生成实体。成功生成以后还要检查是否落地、时钟和插件数据。**

### 讲解与例子

base_footprint 是地面参考，模型生成 z=0.05 是让机器人略高于地面自然落下；不是轮子半径固定加 0.05 的普遍公式。过低可能初始穿入地面，过高会掉落。名字重复会阻止重复生成，重新实验可以只删除本课 my_robot 实体，不重置所有世界。

### 板书

```text
展开 → 世界就绪 → spawn → 落地 → 查询数据
```

### 命令行实验

手工分步实验前停止第 4 页完整 launch，避免第二个世界和同名实体。
```bash
# A：只启动世界
ros2 launch gazebo_ros gazebo.launch.py gui:=false world:="$(ros2 pkg prefix --share my_robot_description)/worlds/my_world.world"
# B：RSP 从已有 URDF 文件启动；Humble 支持此兼容入口
ros2 run robot_state_publisher robot_state_publisher tf_debug/my_robot_sim.urdf --ros-args -p use_sim_time:=true
# C：等待世界服务就绪后生成
ros2 service list -t | grep spawn_entity
ros2 run gazebo_ros spawn_entity.py -topic robot_description -entity my_robot -z 0.05
ros2 topic echo /odom --once
```
预期 SpawnEntity 成功；RSP 的文件入口可能提示兼容/弃用警告，推荐部署用第 27 页参数式 launch。若实体重复，不继续反复 spawn，可先按实际服务删除本课实体：
```bash
ros2 service call /delete_entity gazebo_msgs/srv/DeleteEntity "{name: my_robot}"
```
本实验后停止 A/B，后续恢复完整 launch，勿在旧实体上重复启动。

### 课堂提问

> spawn 返回成功能证明传感器也正常吗？

参考答案：不，实体生成、插件加载和话题数据要逐层检查。

这页最后可以总结：

> **生成是入口，数据与接触才是验收。**

---

## 第 14 页｜Gazebo 插件机制

这页你可以把它讲成：

> **插件既可能控制模型，也可能把传感器读数发布为 ROS 数据；挂载位置和参数依插件而异。**

### 讲解与例子

接下来不编写 C++ 插件，而使用已安装的标准插件，把参数与观察对象一一对应。选错 filename、reference 或接口会影响加载和数据输出。

### 板书

```text
模型/传感器 → 插件实例 → ROS 话题
```

### 课堂提问

> 只写 sensor 就一定出现 ROS /scan 吗？

参考答案：需要相应 ROS 发布插件或桥接机制。

这页最后可以总结：

> **仿真传感器与 ROS 输出之间需要连接层。**

---

## 第 15 页｜插件的本质与四种类型

这页你可以把它讲成：

> **Model 管整机运动，Sensor 管采样输出，World 管世界扩展，System 管进程级行为。**

### 讲解与例子

沿课件四类说明动态库与 Load/更新钩子。插件不一定都在每一帧处理，也可按传感器更新或设定周期工作。本课 diff_drive 是 Model，ray/imu 是 Sensor，init/factory 启动插件提供时钟与实体服务。

### 板书

```text
Model：驱动
Sensor：雷达/IMU
World/System：世界与进程扩展
```

### 课堂提问

> 雷达 update_rate=10，物理 1000Hz，ROS 扫描一定 1000Hz 吗？

参考答案：不，采样与物理推进速率不同。

这页最后可以总结：

> **插件职责与更新节奏分别配置。**

---

## 第 16 页｜plugin 标签格式与常用插件一览

这页你可以把它讲成：

> **filename 找库，name 标识实例，私有参数控制行为，ros remapping 决定对外名称。**

### 讲解与例子

本课安装并检查 diff_drive、ray_sensor、imu_sensor 与 joint_state_publisher 四个库。ROS 2 插件用标准 remapping 修改 cmd_vel/odom 名称；不要把其他版本的 command_topic XML 参数当本机一定识别。深度相机版本可能与单目相机共享实现，库名以本机文件与源码为准。

### 板书

```text
filename → 动态库
reference → 挂接对象
ros remapping → ROS 名称
```

### 命令行实验

只检查实际存在库与配置：
```bash
ls /opt/ros/humble/lib/libgazebo_ros_diff_drive.so
ls /opt/ros/humble/lib/libgazebo_ros_ray_sensor.so
ls /opt/ros/humble/lib/libgazebo_ros_imu_sensor.so
ls /opt/ros/humble/lib/libgazebo_ros_joint_state_publisher.so
grep -nE 'filename|remapping|frame_name|odometry_source' ../ros2_ws/src/my_robot_description/urdf/my_robot_sim.xacro
```
预期四库存在。缺库先安装 gazebo_plugins；A 日志出现 Failed to load 先查 filename 与库依赖。未知 XML 参数不一定报错，需以源码为准。

### 课堂提问

> 插件配置写了未知标签，仿真器一定报错吗？

参考答案：不一定，未读取的私有标签可能被忽略，需要查源码和输出。

### 教师说明

PPT 的 libgazebo_ros_joint_state_pub.so 应按实际安装核对，本课使用 libgazebo_ros_joint_state_publisher.so。diff_drive 常用 cmd_vel/odom 重映射；完整配置已提供。

这页最后可以总结：

> **存在于 XML 不等于被插件实际使用。**

---

## 第 17 页｜差速驱动插件

这页你可以把它讲成：

> **只给车体外形还不能响应 Twist。差速插件把 v、ω 转换为左右轮速度并提供里程计。**

### 讲解与例子

先确认唯一 cmd_vel 控制来源，再观察静止、直行和旋转三种状态。先使用小速度和短时间窗口，以便解释数据变化而不是追逐小车。

### 板书

```text
Twist(v, ω) → 左右轮目标 → 物理运动 → odom/TF
```

### 课堂提问

> 发送一次非零速度后发布者退出，会自动停车吗？

参考答案：本课插件不应依赖它自动清零，需要显式零速度。

这页最后可以总结：

> **每次运动实验都包含停止指令。**

---

## 第 18 页｜为什么不能再用 joint_state_publisher_gui

这页你可以把它讲成：

> **GUI 改的是关节状态消息，不是 Gazebo 中的真实电机关节。仿真应从物理关节提供状态。**

### 讲解与例子

JSP GUI 可以使 RSP 的轮子 TF 变化，却不替代差速插件施加物理控制。直接重复发布同名 joint_states 会让 RViz 和仿真脱节；本课不启动 GUI，使用 Gazebo joint_state_publisher 插件从关节读取。说 GUI 与物理引擎“打架”需限定为共享状态造成的可视化冲突，它本身不施加 Gazebo 力矩。

### 板书

```text
GUI 状态 → RSP/显示
驱动插件 → Gazebo 关节 → 状态插件 → RSP
```

### 命令行实验

A 恢复完整仿真；不启动 JSP/JSP GUI。C：
```bash
ros2 topic info /joint_states --verbose
ros2 topic echo /joint_states --once
ros2 node list
```
预期关节状态来源为 Gazebo 插件而非 JSP GUI。关节 position/velocity 随运动变化。若有旧 joint_state_publisher，停止对应上一课 launch，不能用 killall python 清场。

### 课堂提问

> 关节状态来自两个节点，应该取平均就好了么？

参考答案：不，应明确每个关节的权威状态来源，避免重复发布。

这页最后可以总结：

> **仿真状态必须从仿真物理对象读取。**

---

## 第 19 页｜差速驱动的运动学原理

这页你可以把它讲成：

> **v 决定两个轮子共同前进，ω 决定左右轮速差；轮距和半径决定转换比例。**

### 讲解与例子

公式 v=r(ωR+ωL)/2，ω=r(ωR−ωL)/L；逆算 ωR=(v+ωL/2)/r，ωL=(v−ωL/2)/r。里程计的积分有方法与来源差异，本课编码器估计，不能把 PPT 中点积分公式当所有插件实现一致。打滑和参数误差都能造成里程计偏差。

### 板书

```text
直行：左右同速
转弯：速度不同
原地旋转：大小相等方向相反
```

### 命令行实验

计算课件给定速度的目标轮速：
```bash
python3 - <<'PY'
v, w, L, r = 0.2, 0.3, 0.20, 0.032
left = (v-w*L/2)/r
right = (v+w*L/2)/r
print('left/right rad/s:', left, right)
print('recovered v,w:', r*(right+left)/2, r*(right-left)/L)
PY
```
预期左右约 5.3125 与 7.1875rad/s，恢复 v=0.2m/s、ω=0.3rad/s。物理实际受加速度限制与接触影响，不要求瞬间达到目标。

### 课堂提问

> 轮径误写成半径会影响哪些量？

参考答案：轮速换算和里程计比例都可能错误。

这页最后可以总结：

> **先核对单位和物理尺寸，再分析轨迹。**

---

## 第 20 页｜实际参数与插件配置

这页你可以把它讲成：

> **本课模型用半径 0.032m、轮距 0.20m，并通过同一 Xacro 参数驱动几何与插件，避免两份数字漂移。**

### 讲解与例子

左右关节 left_wheel_joint/right_wheel_joint，base_footprint 是里程计子 frame。显式设置发布 odom、odom TF、关闭轮 TF，让 RSP 唯一发布轮子关系。插件 max_wheel_torque 默认和 update_rate 在版本间可能不同，本课显式写出值。odometry_source=0 用编码器，默认源码可能用世界真值。

### 板书

```text
geometry r/sep = plugin diameter/sep
odom → base_footprint：驱动插件
车体与轮子：RSP
```

### 命令行实验

核对生成参数一致：
```bash
grep -nE 'wheel_separation|wheel_diameter|odometry_source|publish_odom|publish_wheel_tf' tf_debug/my_robot_sim.urdf
ros2 topic info /cmd_vel --verbose
ros2 topic echo /odom --once
```
预期轮距 0.20、轮径 0.064、odometry_source=0、轮 TF 不由 diff_drive 重复发布。发布开关与数值在本课显式设置，不照搬 PPT 的默认值表。

### 课堂提问

> publish_wheel_tf=true 再让 RSP 发轮 TF 为什么有风险？

参考答案：同一变换重复来源可能造成时间与数据冲突，应选择一个权威来源。

这页最后可以总结：

> **参数一致和 TF 唯一来源一起保证解释清晰。**

---

## 第 21 页｜参数速查表、测试与 TF 树变化

这页你可以把它讲成：

> **做三次小实验：静止、短直行、短旋转。每次对照 Twist、里程计、轮状态和 TF，再发送零速度。**

### 讲解与例子

字段检查先看 odom header.frame_id 和 child_frame_id，再看 position、orientation、twist；TF 是变换关系，不是单纯位置表。原地旋转时 x/y 应变化较小，姿态变化明显；接触、打滑和数值误差会带来偏差，不能要求精确为零。

### 板书

```text
cmd_vel → joint_states → odom → odom/base_footprint TF
```

### 命令行实验

A 仿真保持运行；B 先短直行，C 同时观察：
```bash
# C：持续观察位置
ros2 topic echo /odom --field pose.pose.position
# B：10Hz 发 20 次，最后显式停车
ros2 topic pub /cmd_vel geometry_msgs/msg/Twist "{linear: {x: 0.1}, angular: {z: 0.0}}" --rate 10 --times 20
ros2 topic pub /cmd_vel geometry_msgs/msg/Twist "{linear: {x: 0.0}, angular: {z: 0.0}}" --once
```
预期 x 增加，y 变化小；加速过程使距离不必精确为 0.2m。C Ctrl+C，改看 orientation；B 发短旋转再停车：
```bash
# C
ros2 topic echo /odom --field pose.pose.orientation
# B
ros2 topic pub /cmd_vel geometry_msgs/msg/Twist "{linear: {x: 0.0}, angular: {z: 0.3}}" --rate 10 --times 20
ros2 topic pub /cmd_vel geometry_msgs/msg/Twist "{linear: {x: 0.0}, angular: {z: 0.0}}" --once
```
姿态四元数变化，不把 z 分量直接当 yaw 角。运动中如提前 Ctrl+C，也必须补零速度。最后查 TF（持续命令结束用 Ctrl+C）：
```bash
ros2 run tf2_ros tf2_echo odom base_footprint
```
不要把中断速度发布当自动停车。

### 课堂提问

> 只看 angular.z 就能断定走的是完美圆吗？

参考答案：不能，还要查位置变化和真实接触状态。

这页最后可以总结：

> **运动验收用多个输出相互印证。**

---

## 第 22 页｜常用传感器插件

这页你可以把它讲成：

> **传感器配置要同时考虑采样参数、坐标系、噪声和 ROS 输出，不能只看话题名。**

### 讲解与例子

先检查 lidar 和 imu 固定安装，再读取扫描与惯性字段。用带墙的世界让雷达有可观察目标，不在空场景里把 inf 当故障。

### 板书

```text
采样几何 → 噪声 → 插件输出 → frame 与时间
```

### 课堂提问

> 雷达没有有限距离就一定没工作吗？

参考答案：可能探测范围内没有目标，还要看样本数量、角度和场景。

这页最后可以总结：

> **传感器数值要联系场景解释。**

---

## 第 23 页｜传感器插件通用结构与激光雷达

这页你可以把它讲成：

> **ray 定义扫描物理参数，插件定义 ROS 输出；两层都正确才会得到 LaserScan。**

### 讲解与例子

配置 360 个采样、约 ±π 扫描、0.12–8m 范围、10Hz、0.01m 高斯噪声。frame_name=laser_link 用于解释范围方向。output_type 在该插件 XML 中用 sensor_msgs/LaserScan，ROS 查询类型则显示 sensor_msgs/msg/LaserScan，不混用两套字符串。墙在 x=1.5m，前方扫描应出现约米级有限距离。

### 板书

```text
ray samples/range/noise → 插件 ~/out remap scan → LaserScan
```

### 命令行实验

保持仿真静止，C 检查字段：
```bash
ros2 topic type /scan
ros2 topic info /scan --verbose
ros2 topic echo /scan --qos-reliability best_effort --once
python3 第七课/课堂实验/scan_probe.py
```
最后命令持续打印 beam 数量、front/min 距离和有限样本数量，避免铺满 360 项。预期 360 beams、frame=laser_link，初始朝墙时 front 为约米级距离；已有运动会改变数值，不要求固定 1.39m。

与第 21 页配合：C 保持 scan_probe，B 以 0.1m/s 短直行 20 次并发零速度；预期前方墙距离总体变小。若转过弯，front 不再正对墙，用 min 与场景解释，不能伪称角度条件不变。测完 Ctrl+C。

### 课堂提问

> ranges 里的 inf 是否是合法读数？

参考答案：可以表示无有限回波等情况，需结合量程和场景判断。

这页最后可以总结：

> **扫描字段、场景和坐标系同时核对。**

---

## 第 24 页｜IMU、相机拓展与传感器噪声意义

这页你可以把它讲成：

> **IMU 静止也可能测到重力相关比力，不能把所有加速度都要求为零。**

### 讲解与例子

水平静止、z 向上时线加速度通常约 +9.8m/s²；转弯后角速度 z 有响应；姿态、方向和传感器约定改变时读数也改变。不能说任意状态下 z=0 都是故障。噪声影响滤波与阈值，协方差也要匹配模型。课件相机是选学，本模型不含 camera_link，不承诺 /camera/image_raw 自动存在。

### 板书

```text
姿态四元数 + 角速度(rad/s) + 比力(m/s²)
噪声与协方差必须配套
```

### 命令行实验

静止时查加速度，再短旋转查角速度；使用实际 QoS：
```bash
ros2 topic type /imu
ros2 topic echo /imu --qos-reliability best_effort --field linear_acceleration --once
ros2 topic echo /imu --qos-reliability best_effort --field angular_velocity
```
第三条持续观察，B 使用第 21 页短旋转与零速度。水平静止预期 z 约 9.8，旋转时 angular_velocity.z 有响应，结束后接近静止值。若无相机配置，/camera/image_raw 不存在是预期，不把选学占位当已实现实验。噪声参数先在副本里修改，重新展开和 spawn 才生效。

### 课堂提问

> 水平静止时 z≈9.8 就说明机器人向上加速吗？

参考答案：不，这是加速度计比力的解释，运动加速度需结合重力与姿态。

这页最后可以总结：

> **读传感器先核对物理量和坐标约定。**

---

## 第 25 页｜仿真世界与环境

这页你可以把它讲成：

> **墙不仅是画面里的背景，它给碰撞和雷达提供共同的实验目标。**

### 讲解与例子

本课 world 用完整 SDF 定义地面、光源和静态墙，避免依赖外部 model 下载。先用文件验证，下一页再介绍 GUI 的 Building Editor 作为复杂场景创建方式。

### 板书

```text
world：地面 + 光源 + 物理参数 + 静态障碍
```

### 课堂提问

> 墙只有 visual，雷达和碰撞会和预期一致吗？

参考答案：不一定，应提供合适碰撞和传感器可见几何。

这页最后可以总结：

> **环境也要作为实验输入固定下来。**

---

## 第 26 页｜World 文件本质与 Building Editor 建墙

这页你可以把它讲成：

> **先用命令把 world 里的墙位置读出来，再用雷达看到它，建立文件配置到输出数值的连接。**

### 讲解与例子

Building Editor 八步可以用于复杂房间：启动世界、进入编辑、画墙、门窗/楼梯可选、保存模型、退出、保存世界。当前命令行实验无需 GUI 手工建墙，完整 world 已提供。共享给学生时还需交付自定义模型资源，不只交一个引用本机路径的 world。

### 板书

```text
SDF 模型位置 → 场景几何 → 激光范围
```

### 命令行实验

命令行查看场景与墙：
```bash
WORLD_FILE="$(ros2 pkg prefix --share my_robot_description)/worlds/my_world.world"
grep -nE 'wall_1|pose|collision|static' "$WORLD_FILE"
gz sdf -p "$WORLD_FILE" > tf_debug/my_world_checked.sdf
```
预期墙在 x=1.5m，静态模型同时有碰撞与外观。可选 GUI 建墙：启动 gui:=true 的世界，按 PPT 八步操作后保存完整资源；结束旧世界再加载新世界。改变墙位置后必须重启世界才能观察配置变化，不能只改文件就期待现有世界自动改变。

### 课堂提问

> world 中引用 ~/.gazebo/models 的文件，换机器一定能找到吗？

参考答案：不，需同步模型资源或写成自包含场景。

这页最后可以总结：

> **可复现环境包含世界和依赖资源。**

---

## 第 27 页｜集成到功能包与加载自定义 world

这页你可以把它讲成：

> **把模型、世界和启动文件一起安装，运行时从包的 share 查找，不依赖当前目录。**

### 讲解与例子

随附安装脚本扩展第六课包而不覆盖原 Xacro/display.launch.py。CMake 新增 worlds，package.xml 新增 gazebo 运行依赖；launch 包含官方 Gazebo world、RSP 与 spawn，默认 gui=false，便于 CLI 实验。spawn 会等待实体服务和描述；仍需看其返回日志，不能用固定 sleep 推断完成。

### 板书

```text
资源文件 → install share → FindPackageShare → world + RSP + spawn
```

### 命令行实验

课前已通过 install_extensions.py 完成集成，这里让学生理解并重复构建：
```bash
grep -n worlds ../ros2_ws/src/my_robot_description/CMakeLists.txt
(cd ../ros2_ws && colcon build --packages-select my_robot_description --symlink-install)
source ../ros2_ws/install/setup.bash
ls "$(ros2 pkg prefix --share my_robot_description)/worlds"
ros2 launch my_robot_description gazebo.launch.py --show-args
```
停止旧的手工 Gazebo/RSP/其他 launch 后，A 一键启动：
```bash
ros2 launch my_robot_description gazebo.launch.py gui:=false
```
预期 world、描述、spawn 和插件输出一起就绪。需要图形复查时停止 A，再改 gui:=true；不要同时启动两套同名 world。

### 课堂提问

> world 放 src 里但没 install，ros2 launch 一定找到吗？

参考答案：从安装包查找时可能找不到，需正确安装并重新构建。

这页最后可以总结：

> **一键运行以资源和依赖齐全为前提。**

---

## 第 28 页｜课程总结

这页你可以把它讲成：

> **现在用命令证明车能接收控制、里程计与 TF 连通、雷达能看墙、IMU 能反映状态。**

### 讲解与例子

复盘 RViz 看、Gazebo 算；模型与 gazebo 扩展提供物理和插件能力；差速、雷达与 IMU 构成底层数据链。进入 SLAM/Nav2 还要核对时间、标定、数据质量和适合的 TF，不仅是列出四个话题。末页给出任务与排查，保持原 28 页结构。

### 板书

```text
输入正确 + 输出可信 + TF 连通 + 时间一致 → 上层算法基础
```

### 命令行实验

最终验收：
```bash
ros2 topic list -t
ros2 topic echo /odom --once
ros2 topic echo /scan --qos-reliability best_effort --once
ros2 topic echo /imu --qos-reliability best_effort --once
ros2 param get /robot_state_publisher use_sim_time
```
再分别运行 tf2_echo odom base_footprint 和 base_link laser_link，每条测完退出。任务：短直行使 odom x 增、前墙距离减；短旋转使姿态与 IMU z 角速度变化；每次运动后明确零速度。通过这些现象解释输入与输出关系。

| 现象 | 首查 | 下一步 |
| --- | --- | --- |
| 模型生成失败 | A spawn 日志、/spawn_entity、名字冲突 | 确认世界服务与描述，必要时仅删除 my_robot |
| 模型穿地或抖动 | collision、惯量、初始高度 | 查展开/SDF 和接触，图形复查几何 |
| 车不动 | cmd_vel 类型、订阅数、暂停状态 | 小速度实验、检查驱动关节和库 |
| 有 /scan 无输出 | 发布 QoS、sensor、场景 | best_effort echo，查 samples 和插件日志 |
| TF 冲突 | 重复 RSP、轮 TF 开关、JSP GUI | 停止旧链路，保留唯一来源 |
| 时间不一致 | use_sim_time 与 /clock | 查时钟是否推进，恢复暂停 |

清理先发零速度，退出 C/B 的观察命令，再 Ctrl+C 停止 A launch。保留模型资源供复习；不把清空整个工作空间当维护步骤。

### 课堂提问

> 四个话题都列出来，就算导航底层完成了吗？

参考答案：还要验证数据、频率、frame、时间、运动关系和停止行为。

这页最后可以总结：

> **底层验收依靠现象与因果，而不是话题数量。**

---

## 教师核对资料

使用 rosdep 前，先初始化并更新规则；已初始化时跳过 init。


- [gazebo_ros_pkgs ROS 2 源码](https://github.com/ros-simulation/gazebo_ros_pkgs/tree/ros2)：Classic 集成与库名。
- [diff_drive 源码](https://github.com/ros-simulation/gazebo_ros_pkgs/blob/ros2/gazebo_plugins/src/gazebo_ros_diff_drive.cpp)：cmd_vel/odom、odometry_source、TF 开关与默认值。
- [ray_sensor 源码](https://github.com/ros-simulation/gazebo_ros_pkgs/blob/ros2/gazebo_plugins/src/gazebo_ros_ray_sensor.cpp)：output_type、frame 与扫描发布。
- [IMU 源码](https://github.com/ros-simulation/gazebo_ros_pkgs/blob/ros2/gazebo_plugins/src/gazebo_ros_imu_sensor.cpp)：姿态参考、加速度与 QoS。
- [joint_state_publisher 插件](https://github.com/ros-simulation/gazebo_ros_pkgs/blob/ros2/gazebo_plugins/src/gazebo_ros_joint_state_publisher.cpp)：真实关节数据。
- [gazebo_ros_init 源码](https://github.com/ros-simulation/gazebo_ros_pkgs/blob/ros2/gazebo_ros/src/gazebo_ros_init.cpp)：时钟与暂停服务。
- [Gazebo Classic 迁移资料](https://gazebosim.org/docs/latest/gazebo_classic_migration/)：版本线与维护范围。
- 随附 [模型扩展](课堂实验/urdf/my_robot_sim.xacro)、[world](课堂实验/worlds/my_world.world)、[launch](课堂实验/launch/gazebo.launch.py)、[扫描观察脚本](课堂实验/scan_probe.py)、[资源集成脚本](课堂实验/install_extensions.py)。

验证记录：已读取对应 PPT 的全部页面文字并核对页序；完整 Python 源码和 launch 做语法检查，XML 模型和包描述做解析检查，模型参数与树结构做静态检查。关键命令与 API 对照 Humble 官方资料及源码。当前 Windows 环境不能访问 WSL 运行会话，因此未完成 ROS 节点运行、C++ ROS 构建、TF/QoS 网络实测及 Gazebo/RViz 显示预演。文中输出均为预期现象，需教师在目标环境预演；不把静态通过写成实际实验通过。
