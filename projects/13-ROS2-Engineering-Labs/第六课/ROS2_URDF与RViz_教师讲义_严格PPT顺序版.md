# ROS 2 URDF与RViz：逐页教师讲义

对应课件：`2026.8.18 ROS2 中的 URDF  RViz 完全指南.pptx`，共 19 页。

严格一页对应一节；讲稿、命令行实验与技术修正均放在原页面位置。

## 上课前准备

### 执行目录与课前准备

**本课所有命令均从整套讲义的根目录执行**，也就是同时能看到 `第一课`、`第二课`、`第三课` 等文件夹的位置。请先在 WSL Ubuntu 的 Bash 中进入此目录；每个新终端都保持在该目录。若终端已位于 `第六课`，先执行 `cd ..` 回到讲义根目录。

Python 程序直接使用已提供的 `第六课/课堂实验/` 文件，无需复制到另一份脚本目录。第三课配置直接使用 `第三课/课堂实验/config/`。只有需要 colcon 构建的 ROS 包才由准备脚本写入旁边的 `../ros2_ws/src/`，构建在子 Shell 中进行，完成后当前终端仍位于讲义根目录。

```bash
source /opt/ros/humble/setup.bash
test -d "第六课/课堂实验" || { echo "请先进入整套讲义根目录"; exit 1; }
mkdir -p tf_debug
sudo apt update && sudo apt install -y ros-humble-xacro ros-humble-robot-state-publisher ros-humble-joint-state-publisher ros-humble-joint-state-publisher-gui ros-humble-rviz2 ros-humble-tf2-tools ros-humble-launch-ros liburdfdom-tools python3-colcon-common-extensions python3-rosdep && \
python3 prepare_course.py 6 --workspace ../ros2_ws && \
(cd ../ros2_ws && colcon build --packages-select my_robot_description --symlink-install) && \
source ../ros2_ws/install/setup.bash
```

节点持续运行时，后面的查询命令应在另一个终端执行。各终端使用相同 ROS_DOMAIN_ID，先停止旧实验再启动新实验。

所有 Bash 命令都在 WSL Ubuntu 22.04、ROS 2 Humble 中执行。每个新终端先 `source /opt/ros/humble/setup.bash`；使用本地包时再 `source ../ros2_ws/install/setup.bash`。三个终端的 ROS_DOMAIN_ID 一致。

课堂终端：A 运行生产者/主体进程，B 控制或接收，C 查询与记录。持续命令用 `Ctrl+C` 结束；同一个终端不能在前台进程运行时继续执行下一条命令。实验切换前停止上个实验的发布者，避免同名话题、节点或 TF 混入。

随讲义交付的完整源码在 `第六课/课堂实验/`，直接使用这些文件。

前置知识：TF、节点、话题与构建。第 7 页正式构建模型包；教师可课前准备。
```bash
sudo apt install ros-humble-xacro ros-humble-robot-state-publisher   ros-humble-joint-state-publisher ros-humble-joint-state-publisher-gui   ros-humble-rviz2 ros-humble-tf2-tools liburdfdom-tools
# 课前已复制；此处仅在包不存在时复制，重复执行仍成功
if [ ! -d ../ros2_ws/src/my_robot_description ]; then
  cp -r "第六课/课堂实验/my_robot_description" ../ros2_ws/src/
fi
ros2 pkg executables xacro
command -v check_urdf
```
统一包 my_robot_description；源模型 urdf/my_robot.xacro；入口 launch/display.launch.py；已提供 rviz/display.rviz。frame 为 base_footprint、base_link、left_wheel_link、right_wheel_link、caster_link、laser_link、imu_link。关节 left_wheel_joint/right_wheel_joint，默认半径 0.05m、轮距 0.32m。模型附惯性以便承接第七课，但本课不做动力学保证。

---

## 第 1 页｜ROS2 中的 URDF + RViz

这页你可以把它讲成：

> **今天把“机器人长什么样”和“每个部件当前在哪里”连起来，再用命令验证，而不是只凭一张渲染图判断。**

### 讲解与例子

URDF 描述结构，Xacro 维护可复用模型，robot_state_publisher 根据关节状态计算 TF，RViz 用这些数据显示。命令行可以检查树、字段和数值，外形、穿模、比例与渲染仍需要图形观察。

### 板书

```text
模型 → 关节状态 → 正运动学 → TF → RViz
```

### 课堂提问

> 只提供 URDF 就知道轮子转了多少角度吗？

参考答案：不，活动关节还需要实时 joint_states。

这页最后可以总结：

> **静态结构与运行状态共同决定可视化。**

---

## 第 2 页｜课程目录

这页你可以把它讲成：

> **按实际页序走四部分：结构、参数化、状态发布、显示排查。**

### 讲解与例子

PPT 目录列有完整实战和常见问题，但实际 19 页以第四部分检查清单收尾。本讲义将完整源码和问题排查安排到第 7、15、19 页，不另造第五章页码。先学模型含义，再展开和运行，顺序与原课件一致。

### 板书

```text
URDF → Xacro → RSP/JSP/Launch → RViz 与验收
```

### 课堂提问

> 目录里写了六部分，能否凭空添加第五部分的 PPT 页码？

参考答案：不能，以实际页面对应，补充实战放在相关现有页。

这页最后可以总结：

> **课程结构以真实页序为准。**

---

## 第 3 页｜ROS2 怎么知道机器人长什么样？

这页你可以把它讲成：

> **驱动发来的数值不会自己变成机器人外形，需要描述部件与它们的连接关系。**

### 讲解与例子

车体、轮子、雷达是刚体部件；关节定义相对连接与允许运动。ROS 并不从节点名自动猜模型尺寸。先把机器人画成一棵树：base_footprint 到 base_link，再到左右轮与传感器。

### 板书

```text
根 link → 车体 → 轮子/雷达/IMU
```

### 课堂提问

> 雷达位于车体前方 6cm 的信息由谁提供？

参考答案：模型固定关节或相应 TF 发布者提供，不能仅由话题名推出。

这页最后可以总结：

> **几何关系需要显式建模。**

---

## 第 4 页｜URDF：用 XML 描述机器人的结构

这页你可以把它讲成：

> **link 描述刚体，joint 描述连接；这两类元素构成模型树。**

### 讲解与例子

URDF 是统一机器人描述格式。visual、collision、inertial 分别服务显示、碰撞和动力学，它们不必是同一个几何体。joint 的 parent/child 建立树，模型通常有一个根，不支持直接以普通 URDF joint 表达闭链。真实机构有闭环时需要其他建模策略，不能偷偷再给 link 添加第二个父关节。

### 板书

```text
link：形状与物理属性
joint：父子连接、零位、运动轴
```

### 命令行实验

先只检查随附模型文件与元素，不启动节点：
```bash
head -n 20 ../ros2_ws/src/my_robot_description/urdf/my_robot.xacro
grep -nE '<link|<joint|<xacro:macro' ../ros2_ws/src/my_robot_description/urdf/my_robot.xacro
```
准备复制见课前步骤。观察 wheel 宏尚未展开，两次调用会生成不同名称。原始 Xacro 的 grep 结果不等于最终 link 数量。

### 课堂提问

> 给同一轮子连两个父 joint 能表示稳定支撑吗？

参考答案：普通 URDF 树不支持这种拓扑，应重新设计模型表达。

这页最后可以总结：

> **先保证结构合法，再讨论运行位姿。**

---

## 第 5 页｜link 元素：一个刚体的完整描述

这页你可以把它讲成：

> **能显示不代表能仿真：visual 看外形，collision 算接触，inertial 算动力学。**

### 讲解与例子

按 PPT 三个子元素与四种几何体解释。box size 用米；cylinder 的本地轴默认沿 z；sphere 用半径；mesh 需要资源路径和正确缩放。visual.origin 是几何相对 link 坐标系的偏移，不会替代 joint.origin 改变 link 间关系。RViz 纯显示可只有 visual；Gazebo 的动态接触则需要适合的 collision 和质量惯量。

### 板书

```text
visual.origin：几何相对 link
joint.origin：父坐标系到关节零位
```

### 课堂提问

> 把 visual 往上移，TF 坐标系也一起往上移吗？

参考答案：不会，visual 偏移改变几何位置，link 的父子 TF 仍由关节定义。

这页最后可以总结：

> **几何原点和连杆坐标系要分别理解。**

---

## 第 6 页｜joint 元素：连接与运动的定义

这页你可以把它讲成：

> **最常见错误是把父子方向和旋转轴的坐标系想反了。**

### 讲解与例子

origin 定义关节坐标系相对父 link 的位姿，零关节位时子 link 与关节坐标系一致；axis 在关节坐标系中表达。fixed 不运动，revolute 有界旋转，continuous 无界旋转，prismatic 平移，floating/planar 描述更高自由度但常用工具支持有限。普通驱动轮适合 continuous，不代表所有轮子关节都只能如此。

### 板书

```text
父 link --origin--> joint frame --q--> 子 link
axis 在 joint frame 表达
```

### 课堂提问

> 关节 origin 有旋转时，axis 一定仍沿父坐标系同一方向吗？

参考答案：不一定，axis 需要经过 joint frame 方向转换。

### 教师说明

PPT 的 axis 在父坐标系说法应修正。参考 [URDF joint 规范](https://wiki.ros.org/urdf/XML/joint)。

这页最后可以总结：

> **关节轴的含义必须连同坐标系解释。**

---

## 第 7 页｜最小可用 URDF 示例（车体 + 两轮）

这页你可以把它讲成：

> **本页课件展示的是结构片段；我们用随附完整模型展开成 URDF，再查树和左右对称性。**

### 讲解与例子

完整课堂模型含 base_footprint、车体、两轮、支撑球、雷达和 IMU，便于承接下一课。左右轮用同一宏生成，关节 y 分别为 ±wheel_sep/2，圆柱几何绕 x 旋转 90°。本课默认轮半径 0.05m、轮距 0.32m。车体几何使用随附模型声明的尺寸，不把它冒充 PPT 所有片段完全一致。

### 板书

```text
Xacro 完整模型 → 展开 URDF → check_urdf → 检查名称和数值
```

### 命令行实验

在准备终端构建资源包，再展开：
```bash
(cd ../ros2_ws && colcon build --packages-select my_robot_description --symlink-install)
source ../ros2_ws/install/setup.bash
MODEL_SHARE=$(ros2 pkg prefix --share my_robot_description)
ros2 run xacro xacro "$MODEL_SHARE/urdf/my_robot.xacro" -o tf_debug/my_robot.urdf
check_urdf tf_debug/my_robot.urdf
grep -E '<link|<joint|<cylinder' tf_debug/my_robot.urdf
```
预期根 base_footprint、左右轮名称不同，解析成功。缺 check_urdf 安装 liburdfdom-tools；缺模型查 pkg prefix 与 CMake 安装。PPT 的最小片段只有车体与两轮，本实验使用随附扩展完整模型，名称和参数以本讲义为准。

### 课堂提问

> PPT 片段里 cylinder r 和 l 可以直接照抄吗？

参考答案：标准属性应为 radius 和 length，片段需要修正并补齐。

### 教师说明

PPT 的省略号、cylinder r/l 不是可运行完整代码；本课交付完整源文件代替猜测补齐。

这页最后可以总结：

> **可运行模型要完整且符合 XML 元素规范。**

---

## 第 8 页｜本章小结：URDF

这页你可以把它讲成：

> **让学生解释同一处偏移写在 visual 与 joint 中，会产生什么不同结果。**

### 讲解与例子

复盘 link 三种属性、joint 父子与 axis、树结构和检查工具。check_urdf 能检查解析与结构，但不证明惯量准确、形状不穿模或设备安装正确。next 部分用参数化减少左右轮复制代码。

### 板书

```text
结构检查通过 ≠ 动力学或外观全部正确
```

### 课堂提问

> 纯 RViz 模型一定要写三种子元素吗？

参考答案：显示可只用 visual；完整动力学模型需要适当碰撞和惯性属性。

这页最后可以总结：

> **验证层次随用途不同而不同。**

---

## 第 9 页｜URDF 重复代码太多怎么办？

这页你可以把它讲成：

> **左右轮只差前缀和左右符号，把公共结构抽出来，就能避免只改左轮忘记右轮。**

### 讲解与例子

Xacro 是模型预处理工具，最终生成标准 URDF。属性表达共享尺寸，宏表达重复结构，条件表达可选配置。它不会替我们修正不合理的物理参数，只改善维护方式。

### 板书

```text
共同尺寸 → property
重复结构 → macro
可选部件 → if/unless
```

### 课堂提问

> 用宏以后轮子位置一定正确吗？

参考答案：不一定，错误公式也会被宏重复展开。

这页最后可以总结：

> **参数化提高一致性，仍需验证展开结果。**

---

## 第 10 页｜Xacro 属性：用变量代替硬编码

这页你可以把它讲成：

> **改变一个参数，观察生成的两处轮子几何和位置一起变化。**

### 讲解与例子

随附模型用 arg 接收命令行输入，再用 property 和表达式计算；property 是内部复用变量，arg 适合外部配置。展开结果应当没有 xacro 标签和未求值表达式。不要把错误变量名描述成只产生空字符串，未定义表达式通常会报错，应检查命令返回码。

### 板书

```text
命令行 arg → property → 数学表达式 → URDF 常数
```

### 命令行实验

同一源文件生成两个版本：
```bash
MODEL_SHARE=$(ros2 pkg prefix --share my_robot_description)
ros2 run xacro xacro "$MODEL_SHARE/urdf/my_robot.xacro" wheel_sep:=0.32 -o tf_debug/wide.urdf
ros2 run xacro xacro "$MODEL_SHARE/urdf/my_robot.xacro" wheel_sep:=0.40 -o tf_debug/wider.urdf
check_urdf tf_debug/wider.urdf
diff -u tf_debug/wide.urdf tf_debug/wider.urdf
```
预期左右轮关节 y 从 ±0.16 变 ±0.20；diff 返回 1 表示存在差异，不是生成失败。轮径没有改变。每次只改轮距，避免同时改半径影响解释。

### 课堂提问

> 改变轮距应该同时改左右 y 吗？

参考答案：应该，通过同一 sep 变量和 reflect 计算保证对称。

### 教师说明

PPT 的“变量拼错不报错”和把管道符漏写的命令均需修正；采用输出到文件再 check_urdf。

这页最后可以总结：

> **改参数之后检查展开值，而不只看源文件。**

---

## 第 11 页｜Xacro 宏：封装重复代码，调用时传参

这页你可以把它讲成：

> **同一 wheel 宏被调用两次，prefix 保证名称唯一，reflect 决定左右。**

### 讲解与例子

宏的 link 与 joint 名都必须用 prefix 区分；只改位置不改名字会重复定义。条件控制雷达时必须同时包含 link 和 joint，避免半条结构；后续仿真引用它的 sensor 配置也要同步。生成无雷达模型可以用于纯结构实验，不直接套有 laser_link 插件的下一课仿真文件。

### 板书

```text
wheel(prefix=left, reflect=1)
wheel(prefix=right, reflect=-1)
可选部件：link + joint 成对
```

### 命令行实验

验证可选雷达与宏名称：
```bash
ros2 run xacro xacro "$MODEL_SHARE/urdf/my_robot.xacro" use_lidar:=false -o tf_debug/no_lidar.urdf
check_urdf tf_debug/no_lidar.urdf
grep -n laser tf_debug/no_lidar.urdf
grep -n wheel_link tf_debug/no_lidar.urdf
```
预期无 laser 相关结构，左右轮仍各一份。grep 无匹配返回 1 属于预期。恢复默认生成 tf_debug/my_robot.urdf；不要把无雷达版本直接用于第七课含雷达插件的仿真。

### 课堂提问

> 雷达 link 删除了但 joint 仍引用它，会怎样？

参考答案：结构解析失败或模型不完整，需要成对维护。

这页最后可以总结：

> **复用需要唯一名称，可选结构需要完整连接。**

---

## 第 12 页｜本章小结：Xacro

这页你可以把它讲成：

> **把宏语言当生成步骤，确认生成结果才进入运行流程。**

### 讲解与例子

总结属性、宏和条件，使用同一组命令展开、解析、查字段。直接 URDF 对简单模型也可以合理，课件“项目始终只用 Xacro”是建议不是工具限制。Launch 中动态展开和预先展开两种方法都能用，关键是 robot_description 得到合法 URDF。

### 板书

```text
源模型 → 展开成功 → 标准 URDF → 运行
```

### 课堂提问

> 提前生成 URDF 文件是否就不允许使用？

参考答案：允许，部署时可以固定生成产物；动态展开便于开发。

这页最后可以总结：

> **模型生成方式服务维护与部署需求。**

---

## 第 13 页｜ROS2 怎么算出每个部件的位置？

这页你可以把它讲成：

> **模型给出零位和连接，关节状态给出当前值；正运动学把两者变成 TF。**

### 讲解与例子

固定关节不需要不断变化的关节角度；可动关节需要带正确 joint 名的状态值。RSP 不是状态估计器，不会从 cmd_vel 直接推断所有关节状态。我们先用命令行发布角度，观察轮子的 TF。

### 板书

```text
URDF + joint_states(q) → 正运动学 → 父子 TF
```

### 课堂提问

> 只有速度命令没有 joint_states，轮子 TF 一定跟着转吗？

参考答案：不，需驱动/状态来源提供关节位置等数据。

这页最后可以总结：

> **模型结构与关节测量共同参与计算。**

---

## 第 14 页｜两个核心节点的工作流程

这页你可以把它讲成：

> **JSP 给测试关节值，RSP 根据模型计算 TF；仿真或真机时应换成真实状态来源。**

### 讲解与例子

按 PPT 流程区分：RSP 加载 robot_description、订阅 joint_states、计算并发布动态与静态 TF；JSP/JSP GUI 为模型调试提供值。fixed 在 tf_static，活动关节在 tf。两套 joint_states 来源同时给同一关节会污染实验，所以手工发布时关闭自动 JSP。

### 板书

```text
关节来源 → /joint_states → RSP
固定关节 → /tf_static
活动关节 → /tf
```

### 命令行实验

只运行 RSP，关闭自动 JSP 与 RViz。A：
```bash
ros2 launch my_robot_description display.launch.py gui:=false rviz:=false joint_states:=false
```
B 手工发布关节状态，C 查变换：
```bash
# B
ros2 topic pub /joint_states sensor_msgs/msg/JointState   "{header: auto, name: [left_wheel_joint, right_wheel_joint], position: [0.0, 0.0]}" --rate 5
# C
ros2 run tf2_ros tf2_echo base_link left_wheel_link
```
预期平移约 (0, 0.16, -0.015)m，零角旋转为单位四元数。B Ctrl+C 后把 position 改 [1.0, 0.0] 重启，预期左轮旋转改变、平移不变。Humble pub 的 header:auto 在每次发布时填当前时间，避免零时间戳影响 TF 更新；正式驱动应填实际采集时刻。先退出 C，再退出 B/A。

### 课堂提问

> JSP GUI 滑块改变后会直接控制真实电机吗？

参考答案：它只发布关节状态，不是电机控制器。

### 教师说明

PPT 的 continuous 不显示滑块说法不适用于常见 joint_state_publisher_gui 实现；continuous 通常用约 ±π 范围调试，fixed 不提供运动滑块。

这页最后可以总结：

> **状态发布与运动执行不能混淆。**

---

## 第 15 页｜display.launch.py：一键启动可视化

这页你可以把它讲成：

> **把重复启动写成 Launch，但保留 CLI 开关，能只启动模型链路而不依赖图形界面。**

### 讲解与例子

随附完整 launch 使用 Command 调 xacro，并用 ParameterValue(value_type=str) 明确描述参数是字符串；RSP 读取模型，按开关启动 JSP/GUI 和 RViz。rviz 文件真实存在并已安装，不沿用课件 path/to 占位。joint_states:=false 允许手工发布作为唯一来源。

### 板书

```text
FindPackageShare → Xacro Command → description 字符串
RSP + 可选 JSP + 可选 RViz
```

### 命令行实验

验证可重复启动与参数：
```bash
ros2 launch my_robot_description display.launch.py --show-args
# A：命令行链路，无 GUI
ros2 launch my_robot_description display.launch.py gui:=false rviz:=false
# C
ros2 topic echo /joint_states --once
ros2 topic echo /robot_description --qos-durability transient_local --once
ros2 param get /robot_state_publisher robot_description
```
预期 JSP 发布两轮状态，RSP 提供模型。完成查询后停止 A；需要外观时再启动默认 display.launch.py（GUI/RViz 为 true），不要同时启动第二套 RSP/JSP。

### 课堂提问

> 只复制 launch 但没 install urdf/rviz 会怎样？

参考答案：安装前缀找不到资源，需保留 CMake 的目录安装规则并重建。

这页最后可以总结：

> **Launch 一键运行依赖完整资源安装。**

---

## 第 16 页｜本章小结：状态发布

这页你可以把它讲成：

> **用查询验证两条链路：固定安装无需实时关节值，轮子旋转需要活动关节状态。**

### 讲解与例子

RSP 使用模型关系做正运动学；JSP 是调试数据源；正式运行由仿真或硬件驱动发布。robot_description 既是 RSP 参数，也有用于工具接收的描述话题，检查时注意 transient_local。

### 板书

```text
静态安装：模型 → tf_static
可动关节：模型 + joint_states → tf
```

### 命令行实验

保持第 15 页无 GUI 链路运行，查询：
```bash
ros2 topic info /tf --verbose
ros2 topic info /tf_static --verbose
ros2 run tf2_ros tf2_echo base_link laser_link
```
预期固定雷达平移 (0.06, 0, 0.05)m。tf2_echo 持续运行，测完 Ctrl+C。静态 TF 不要求持续收到新消息；描述/静态 topic echo 需要适当持久性。

### 课堂提问

> tf_static 没有持续 Hz 输出是否表示坏了？

参考答案：不，静态变换不是靠高频重复发布保持有效，要用兼容的晚加入订阅或 TF 查询。

这页最后可以总结：

> **按对象特性选择观察方式。**

---

## 第 17 页｜RViz 里机器人不显示怎么办？

这页你可以把它讲成：

> **先从命令确认模型和 TF，再回到 RViz 查 Fixed Frame 与显示项，不凭“空白”认定 RSP 没工作。**

### 讲解与例子

本课默认根是 base_footprint，纯模型显示可选它或树中连通 frame。不存在的 map 没有连接时模型无法变换到 map。TF 正常不代表 RobotModel 正常，描述来源、mesh 路径、透明度和显示开关都要检查。课件 90% 只是经验性说法，本课不作为故障统计。

### 板书

```text
模型？→ 状态？→ TF 连接？→ RViz 描述和显示项？
```

### 命令行实验

先启动第 15 页链路，C 做数据检查：
```bash
ros2 node list
ros2 topic echo /robot_description --qos-durability transient_local --once
ros2 run tf2_ros tf2_echo base_footprint base_link
```
预期 base_link 相对 footprint 上移 0.065m。退出 echo 后可故意查不存在的 map：
```bash
timeout --signal=INT 5s ros2 run tf2_ros tf2_echo map base_link
```
预期 frame 不存在或无法连接提示。该失败是参考系条件变化；不要添加假 map TF 掩盖模型显示问题。

### 课堂提问

> Fixed Frame 写 map 就一定比 base_link 正确吗？

参考答案：本模型没有 map 时不正确，必须选存在并连通的参考系。

这页最后可以总结：

> **显示排查从数据证据走向配置。**

---

## 第 18 页｜RViz 配置四步走

这页你可以把它讲成：

> **先选参考系，再加模型，再看 TF，最后保存配置；CLI 为每一步提供数据检查。**

### 讲解与例子

随附 display.rviz 已设置 base_footprint、RobotModel 的 /robot_description 和 TF。可以由 CLI 用 -d 加载，GUI 中检查 Display 状态与错误信息；保存新配置前确认写入源目录并重建安装。添加 TF 后部分红色可能来自时间或连接问题，不一律归因于 Fixed Frame。

### 板书

```text
Fixed Frame → RobotModel → TF → 保存配置
```

### 命令行实验

A 保持模型链路，C 加载已提供配置：
```bash
ros2 run rviz2 rviz2 -d "$(ros2 pkg prefix --share my_robot_description)/rviz/display.rviz"
```
在 RViz 观察模型、TF 和 Status；将 Fixed Frame 临时改为不存在的 map，观察错误，再恢复 base_footprint。图形不可用时只能完成 CLI 检查，不能声称外观已验收。退出 RViz，避免重复实例。

### 课堂提问

> 描述话题存在就一定能显示吗？

参考答案：还要收到合法描述，资源路径可用，相关 TF 完整且时间条件满足。

这页最后可以总结：

> **显示需要模型、资源和变换同时成立。**

---

## 第 19 页｜RViz 验证检查清单

这页你可以把它讲成：

> **收尾既查数值，也看几何；学生要解释轮距、角度和 TF 的对应关系。**

### 讲解与例子

沿课件清单检查部件完整、相对位置、轮子朝向、比例、可动关节、TF、重叠和传感器安装。CLI 能验树、描述和变换；轮子是否竖直、visual/collision 是否合适要图形复查。课堂不把 XML 解析通过当成模型所有属性正确。

### 板书

```text
结构 → 数值 → 运动 → 外观 → 资源 → 清理
```

### 命令行实验

C 在单独目录生成 TF 图并查状态：
```bash
(cd tf_debug && ros2 run tf2_tools view_frames)
ros2 topic echo /joint_states --once
ros2 run tf2_ros tf2_echo base_link right_wheel_link
```
预期右轮零位 y=-0.16m、z=-0.015m，与左轮对称。最后一条持续命令 Ctrl+C。

验收一：第 10 页只改 wheel_sep，提交 diff 中左右对称变化。验收二：第 14 页只改左角度，提交平移不变与旋转改变。验收三：图形检查轮子沿 y 安装、车体比例、雷达位置和模型状态。若模型不显示，依次查 description、joint_states、TF、Fixed Frame、资源路径。结束 A/B/C，下一课仿真不保留 JSP GUI。

### 课堂提问

> check_urdf 成功能证明机器人在 Gazebo 不会炸飞吗？

参考答案：不能，动力学接触、惯性与插件需要下一课单独验证。

这页最后可以总结：

> **模型验证要覆盖结构、运行和用途三个层次。**

---

## 教师核对资料

使用 rosdep 前，先初始化并更新规则；已初始化时跳过 init。


- [URDF link](https://wiki.ros.org/urdf/XML/link)、[URDF joint](https://wiki.ros.org/urdf/XML/joint)：几何、坐标系与轴。
- [Humble URDF 教程](https://docs.ros.org/en/humble/Tutorials/Intermediate/URDF/URDF-Main.html)：建模、Xacro 与运动关节。
- [Xacro 官方项目](https://github.com/ros/xacro)：属性、宏、条件和展开行为。
- [robot_state_publisher Humble](https://github.com/ros/robot_state_publisher/tree/humble)：joint_states 与静态/动态 TF。
- [joint_state_publisher 官方实现](https://github.com/ros/joint_state_publisher/tree/ros2)：continuous 自由关节与 GUI 行为。
- 随附 [完整模型包](课堂实验/my_robot_description/)：Xacro、CMake、package.xml、Launch 和 RViz 配置。

验证记录：已读取对应 PPT 的全部页面文字并核对页序；完整 Python 源码和 launch 做语法检查，XML 模型和包描述做解析检查，模型参数与树结构做静态检查。关键命令与 API 对照 Humble 官方资料及源码。当前 Windows 环境不能访问 WSL 运行会话，因此未完成 ROS 节点运行、C++ ROS 构建、TF/QoS 网络实测及 Gazebo/RViz 显示预演。文中输出均为预期现象，需教师在目标环境预演；不把静态通过写成实际实验通过。
