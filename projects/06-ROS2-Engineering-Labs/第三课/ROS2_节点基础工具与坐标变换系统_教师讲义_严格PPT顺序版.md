# ROS 2 节点基础工具与坐标变换系统：逐页教师讲义

对应课件：`2026.8.9 ROS2 节点基础工具与坐标变换系统学习手册.pptx`，共 38 页。

环境：沿用前两课的 WSL Ubuntu 22.04、ROS 2 Humble，演示代码使用 Python 3 与 rclpy。

前置知识：学生已经知道节点、话题、服务、消息类型与基本 QoS，能够加载 ROS 2 环境并在多个终端运行节点。本课不要求先掌握导航、机械臂或矩阵推导。

本讲义严格按实际 PPT 第 1～38 页展开，一页对应一节。讲稿采用可以直接面向学生讲的口语；原课件中的版本差异和过度绝对的说法，在原页的“教师说明”中修正。补充演示服务于对应页面，不另造课件页码。

## 上课前准备

### 执行目录与课前准备

**本课所有命令均从整套讲义的根目录执行**，也就是同时能看到 `第一课`、`第二课`、`第三课` 等文件夹的位置。请先在 WSL Ubuntu 的 Bash 中进入此目录；每个新终端都保持在该目录。若终端已位于 `第三课`，先执行 `cd ..` 回到讲义根目录。

Python 程序直接使用已提供的 `第三课/课堂实验/` 文件，无需复制到另一份脚本目录。第三课配置直接使用 `第三课/课堂实验/config/`。只有需要 colcon 构建的 ROS 包才由准备脚本写入旁边的 `../ros2_ws/src/`，构建在子 Shell 中进行，完成后当前终端仍位于讲义根目录。

```bash
source /opt/ros/humble/setup.bash
test -d "第三课/课堂实验" || { echo "请先进入整套讲义根目录"; exit 1; }
mkdir -p tf_debug
sudo apt update && sudo apt install -y ros-humble-rclpy ros-humble-rcl-interfaces ros-humble-rosgraph-msgs ros-humble-geometry-msgs ros-humble-tf2-ros ros-humble-tf2-tools ros-humble-tf2-geometry-msgs ros-humble-launch-ros graphviz python3-colcon-common-extensions python3-rosdep
```

节点持续运行时，后面的查询命令应在另一个终端执行。各终端使用相同 ROS_DOMAIN_ID，先停止旧实验再启动新实验。

### 1. 环境与终端

通常准备三个 WSL Ubuntu 终端；TF 实操需要同时运行几个发布进程，可以额外打开终端。

```text
终端 A：参数节点；后续换成时间观察节点或动态 TF 发布节点
终端 B：参数操作；后续发布 /clock 或运行静态 TF
终端 C：服务、日志、时钟与 TF 调试
额外终端：TF 章节中另一个静态 TF 发布节点、监听节点
```

每个新终端先执行：

```bash
source /opt/ros/humble/setup.bash
echo "$ROS_DISTRO"
echo "${ROS_DOMAIN_ID:-0}"
```

第一项应显示 `humble`，各演示终端的 Domain ID 应一致。本课直接运行已有 Python 文件，不依赖前课接口包；通常无需加载工作空间。若另用前课包，才执行：

```bash
if [ -f ../ros2_ws/install/setup.bash ]; then
  source ../ros2_ws/install/setup.bash
fi
```

本课脚本直接用 `python3` 运行，不修改前课接口包，也不需要重新构建前课接口。下文所有 Bash 命令都在 WSL Ubuntu 中执行，不能直接复制到 Windows PowerShell。

创建保存目录：

```bash
mkdir -p tf_debug
```

### 2. 本课统一名称

| 对象 | 名称或路径 | 用途 |
| --- | --- | --- |
| 执行目录 | 整套讲义根目录 | Python 文件直接使用第三课/课堂实验 |
| 参数脚本／节点 | `第三课/课堂实验/parameter_demo.py`／`/parameter_demo` | 声明、校验与动态读取参数 |
| 配置文件 | `第三课/课堂实验/config/parameter_demo.yaml` | 启动配置 |
| 参数 | `controller.max_speed`、`enabled`、`robot_id` | 速度上限、使能、只读编号 |
| Launch 文件 | `第三课/课堂实验/parameter_demo.launch.py` | 演示文件配置与显式覆盖 |
| 时间脚本／节点 | `第三课/课堂实验/clock_probe.py`／`/clock_probe` | 比较 ROS 时钟与稳态时钟 |
| 时钟话题 | `/clock`，`rosgraph_msgs/msg/Clock` | 人工推进仿真时间 |
| 动态 TF 脚本／节点 | `第三课/课堂实验/odom_tf_demo.py`／`/odom_tf_demo` | 模拟车身在里程计系中移动 |
| 监听脚本／节点 | `第三课/课堂实验/tf_point_demo.py`／`/tf_point_demo` | 将雷达点转换到车身坐标系 |
| 坐标系 | `map`、`odom`、`base_link`、`laser_frame` | 地图、里程计、车身、雷达 |
| TF 话题 | `/tf`、`/tf_static` | 动态与静态变换 |

`controller.max_speed` 的单位统一为米每秒，合法范围为 0.0～2.0。本课只模拟配置和位姿，不连接真实底盘。

### 3. 课前检查

```bash
python3 -c "import rclpy; import tf2_ros; import tf2_geometry_msgs"
ros2 interface show rcl_interfaces/msg/ParameterDescriptor
ros2 interface show rosgraph_msgs/msg/Clock
ros2 pkg executables tf2_ros
ros2 pkg executables tf2_tools
ros2 param list --help
```

Python 导入应无报错；接口应能列出字段；工具列表应包含 `static_transform_publisher`、`tf2_echo`、`tf2_monitor` 与 `view_frames`。若缺依赖，先确认已经加载 Humble；确实未安装时课前执行：

```bash
sudo apt update
sudo apt install ros-humble-rclpy ros-humble-rcl-interfaces \
  ros-humble-rosgraph-msgs ros-humble-geometry-msgs \
  ros-humble-tf2-ros ros-humble-tf2-tools ros-humble-tf2-geometry-msgs \
  ros-humble-launch graphviz
```

教师课前按第 8、9、22、33、34 页保存完整脚本和配置，再按各页验收步骤预演；课堂仍按 PPT 顺序讲。进入时间演示前停止无关的仿真器或 Bag 回放，确保 `/clock` 只有本课演示来源。进入 TF 演示时，停止时间观察节点，使用默认 `use_sim_time=false` 的新节点。

---

## 第 1 页｜ROS2 节点基础工具与坐标变换系统学习手册

这页你可以把它讲成：

> **前两课我们学会了让节点传数据、发请求和执行任务。今天继续问：节点的配置怎样改？故障发生后怎样找到原因？数据中的时间到底是哪一种时间？雷达看到的点又怎样变成车身前方的障碍物？**

用同一台巡检机器人串起课件的四个主题：

```text
想把速度上限从 0.5 调到 0.8 m/s → 参数
想知道为什么某次修改被拒绝     → 日志与响应
想让暂停的仿真不继续累计运动   → 时间
想把雷达点放进地图             → TF2
```

可以类比：参数是设备设置，日志是操作记录，时钟是大家共同使用的时间尺，TF2 是不同位置之间的坐标换算规则。每个类比都只解释职责，不代表具体实现。

### 课堂提问

> 雷达报“前方 1 米有一个点”，车身坐标系里一定也是前方 1 米吗？

参考答案：不一定，还要知道雷达相对车身的安装位置和朝向；机器人移动时，转换到地图还要考虑采样时间。

这页最后可以总结：

> **今天的目标，是让节点的配置、运行记录、时间基准和空间关系都能被解释和验证。**

---

## 第 2 页｜课程目录

这页你可以把它讲成：

> **今天四部分按课件顺序来：先管理配置，再记录运行，再统一时间，最后把不同坐标系连接起来。TF2 会用到时间戳，所以时间部分要先讲清楚。**

| 部分 | 主要问题 | 实际页码 |
| --- | --- | --- |
| 节点参数系统 | 参数归谁管理，怎样声明、修改和保存？ | 第 3～11 页 |
| 日志系统 | 怎样分级，怎样输出，怎样用记录定位故障？ | 第 12～18 页 |
| 时间与定时器 | 系统、稳态、ROS 时间怎样选，定时器为什么会延迟？ | 第 19～26 页 |
| TF2 | 空间关系怎样表示、传播、查询和调试？ | 第 27～37 页 |
| 总结 | 用完整问题检验四部分能力 | 第 38 页 |

板书：

```text
配置是否正确 → 运行能否看见 → 时间能否对齐 → 空间能否换算
```

### 课堂提问

> 如果两个传感器都写了时间戳，是不是就已经同步了？

先让学生保留判断，到第 26 页再回顾。教师参考答案：还需要相同时间基准、正确采样时刻和可接受的同步误差。

这页最后可以总结：

> **四部分各解决一个基础问题，最后会在同一条机器人数据链中配合使用。**

---

## 第 3 页｜节点参数系统

这是一张章节过渡页，可以这样讲：

> **先看最常见的需求：今天在教室里演示，希望机器人速度上限低一些；换到测试场又希望稍微提高。每次都改源码再编译很麻烦，我们把这种长期有效、偶尔调整的配置交给参数。**

板书：

```text
节点启动：声明并取得配置
节点运行：读取配置，接受合法修改
节点结束：运行时参数随节点结束
```

### 课堂提问

> 每秒更新 100 次的机器人当前位置，适合存成参数吗？

参考答案：不适合，这属于持续状态数据，通常通过话题发布。第 11 页再系统比较边界。

这页最后可以总结：

> **参数用来配置节点的行为，先弄清它归哪个节点管理。**

---

## 第 4 页｜参数系统核心机制：ROS1 vs ROS2

这页你可以把它讲成：

> **以前参数像放在公共管理处的配置，现在更像每个设备自带设置菜单。要修改谁的设置，就找到谁提供的参数接口。**

### 1. 保留课件的六个比较维度

| 维度 | ROS 1 参数服务器 | ROS 2 参数 |
| --- | --- | --- |
| 架构 | 参数服务器通常随 Master 提供，参数集中管理 | 每个节点维护自己的参数，默认提供参数服务 |
| 可靠性 | 中央参数服务不可用时，查询和修改受影响 | 一个节点失效主要影响该节点的参数；网络等共享依赖仍可能故障 |
| 生命周期 | 参数可独立于使用它的节点存在 | 通常与所属节点同寿命；持久化由应用实现 |
| 通信 | 参数 API 使用 XML-RPC | 使用 ROS 2 标准服务，经 RMW／DDS 等通信实现传递 |
| 类型 | 有明确的可表示值类型，但允许较灵活的配置结构 | 有固定类型集合；默认声明后限制类型变更 |
| 发现 | 参数访问依赖服务器地址 | 默认参数服务参与 ROS 图发现 |

### 2. 同名参数为什么不冲突

`/camera` 的 `enabled` 和 `/lidar` 的 `enabled` 是两个参数。外部操作要同时写出节点名与参数名，不能只问全系统“enabled 是多少”。

```text
/camera ── enabled=true
/lidar  ── enabled=false
```

### 课堂提问

> `/parameter_demo` 退出以后，另一个节点中的同名参数会跟着消失吗？

参考答案：不会。它们属于不同节点。

### 教师说明

课件的“声明期定义类型，编译期校验”应改为：默认参数类型约束主要在运行时检查，外部 YAML 和 CLI 配置也在运行时处理；Python 更不能按 C++ 编译期类型检查来讲。分布式管理也不等于整个机器人不存在单点故障。[Humble 参数概念](https://github.com/ros2/ros2_documentation/blob/humble/source/Concepts/Basic/About-Parameters.rst)

这页最后可以总结：

> **ROS 2 参数首先是节点自己的配置状态，访问时必须明确所属节点。**

---

## 第 5 页｜参数生命周期：三个阶段

这页你可以把它讲成：

> **把参数的一生分成三段就容易记：启动时建立规则，运行时按规则使用和修改，结束时释放运行状态。要下次继续用，得主动保存配置。**

### 1. 声明阶段

声明名称、默认值与描述符。外部启动配置可以覆盖默认值，因此“默认值写了 0.5”不代表这次启动实际一定是 0.5。

### 2. 运行阶段

内部通过 API 读取；外部通过参数服务操作。类型、只读属性等有框架约束，但“开了 enabled 才能用某功能”这类业务规则需要应用自己实现。

### 3. 销毁与保存

节点销毁后，其运行时参数不会自动写回源码或 YAML。可用第 10 页的 `dump` 保存，下次启动显式加载。节点对象也可以在进程仍存在时销毁，不必等整个进程退出。

板书：

```text
默认值 + 启动覆盖 → 本次初值 → 合法运行时修改 → 本次当前值
                                                │
                                          dump 到 YAML
                                                │
                                       下次启动显式加载
```

### 课堂提问

> 终端把速度改成 0.8，关掉节点再按默认方式启动，速度仍是 0.8 吗？

参考答案：本课节点会恢复代码默认值 0.5；除非把 0.8 保存并作为启动配置加载。

### 教师说明

“未声明参数一律不能访问或修改”只适用于默认的声明策略。Humble 允许显式启用 `allow_undeclared_parameters`；本课坚持先声明，但不能把默认策略说成不可改变的系统规则。没有默认持久化也不会自动消除配置漂移，部署仍需版本管理。

这页最后可以总结：

> **修改的是本次运行状态，保存和重新加载才把配置带到下一次启动。**

---

## 第 6 页｜参数类型与声明规范

这页你可以把它讲成：

> **设置菜单不能什么东西都往里塞。ROS 2 参数支持哪些值，是明确规定的；名称可以分层，但一个参数值并不是任意复杂对象。**

### 1. 实际类型集合

| 类别 | 类型 | 本课例子 |
| --- | --- | --- |
| 标量 | 布尔、64 位整数、64 位浮点、字符串 | `enabled=true`、速度 `0.5`、编号 `demo_01` |
| 数组 | 字节数组、布尔数组、整数数组、浮点数组、字符串数组 | PID 系数 `[1.0, 0.1, 0.05]` |

数组元素须同类型。`[1, 2.0]` 不应当作标准参数数组使用。参数也有“未设置”状态，它不等于额外增加一种业务配置对象。

### 2. 点号命名与只读描述符

`controller.max_speed` 表达配置分组。`controller` 并不是一个可整体取得的结构体参数。

以下是节点构造函数中的局部片段，完整程序见第 8 页：

```python
self.declare_parameter('controller.max_speed', 0.5)
self.declare_parameter('enabled', True)
self.declare_parameter(
    'robot_id', 'demo_01',
    ParameterDescriptor(description='课堂机器人编号', read_only=True))
```

`robot_id` 在声明初始化后只读；可以在启动时给它初始覆盖值，不能把“只读”误解成“连启动配置也不准传”。

### 课堂提问

> 想配置宽、长、高，是声明一个 Struct 参数，还是分别声明三个浮点参数？

参考答案：分别声明，如 `robot.width`、`robot.length`、`robot.height`；也可用同类型数组，但需明确各位置的含义。

### 教师说明

PPT 的“Struct 结构体”不是 ROS 2 原生参数值类型。YAML 层级可以展开成带点号的参数名，不能据此认为参数 API 接收任意字典或结构体。[Humble 参数类型列表](https://github.com/ros2/ros2_documentation/blob/humble/source/Concepts/Basic/About-Parameters.rst)、[ParameterDescriptor 定义](https://github.com/ros2/rcl_interfaces/blob/humble/rcl_interfaces/msg/ParameterDescriptor.msg)

这页最后可以总结：

> **值遵守固定类型集合，名称表达分组，描述符表达约束和说明。**

---

## 第 7 页｜声明参数的四大核心价值

这页你可以把它讲成：

> **声明不是多写一行代码，而是给外部使用者一份配置约定：可以改什么，值是什么类型，不配置时是多少，哪些修改不接受。**

### 1. 类型安全

本课速度声明为浮点数。CLI 中 `0.8` 与 `1` 分别可能被解析为浮点与整数；设置速度时统一写 `1.0`，不要把类型拒绝误判成数值越界。

### 2. 默认兜底

默认速度用 0.5。默认值要适合本应用；需要硬件连接才能安全运行的节点，也可以选择启动失败并明确报错，不能为了“零配置启动”掩盖缺失配置。

### 3. 可发现与自说明

外部工具能列出参数，`describe` 能查看说明和约束。给别人一句“速度上限，单位 m/s”，比只给一个 `max_speed` 名称更容易正确使用。

### 4. 统一校验

数值范围可用描述符表达，跨参数关系和字符串枚举等规则由业务回调实现。以下是描述符示例，完整导入和使用见第 8 页：

```python
speed_descriptor = ParameterDescriptor(
    description='速度上限，单位 m/s',
    floating_point_range=[FloatingPointRange(
        from_value=0.0, to_value=2.0, step=0.0)])
```

板书：

```text
参数契约 = 名称 + 类型 + 默认值 + 描述/约束 + 应用使用方式
```

### 课堂提问

> 把“只允许 auto 或 manual”写进 `additional_constraints` 文本，框架就会自动拒绝其他字符串吗？

参考答案：不会。这是说明文本，枚举等规则需要自己校验。

### 教师说明

课件的“范围、长度、枚举均自动校验”不应照读。标准描述符提供整数和浮点范围等字段，没有通用字符串枚举或数组长度检查字段。更复杂的规则由代码实现。[描述符字段](https://github.com/ros2/rcl_interfaces/blob/humble/rcl_interfaces/msg/ParameterDescriptor.msg)

这页最后可以总结：

> **声明建立可理解的配置契约，框架负责基础规则，应用补齐业务规则。**

---

## 第 8 页｜动态修改的底层逻辑：5 步流程

这页你可以把它讲成：

> **上一课服务是“请求—处理—响应”。参数修改也一样：外部提出新值，节点检查，接受后更新，最后明确告诉调用者结果。这里特别要分清普通批量修改和原子批量修改。**

### 1. 五步流程与原子边界

```text
外部请求：参数名与候选值
          ↓
框架检查：声明、类型、只读、范围等
          ↓
业务检查：开发者注册的校验回调
          ↓
成功提交：更新参数存储并发布参数事件
          ↓
返回结果：successful 与 reason
```

这是便于讲解的职责顺序；具体检查和回调先后由客户端库实现决定，不把板书当成精确源码调用栈。

| 接口 | 批量结果 |
| --- | --- |
| `set_parameters` | 逐项返回结果，可能部分成功 |
| `set_parameters_atomically` | 整组作为一次提交，拒绝时不提交这组新值 |

校验回调只检查候选值，不提前控制电机、改业务成员或覆盖文件。后续校验仍可能失败。参数存储的原子提交也不等于外部设备动作有事务保证。

### 2. 完整参数节点

保存位置：`第三课/课堂实验/parameter_demo.py`。这是完整程序。

```python
import math

import rclpy
from rclpy.node import Node
from rclpy.parameter import Parameter
from rcl_interfaces.msg import (
    FloatingPointRange, ParameterDescriptor, SetParametersResult)


class ParameterDemo(Node):
    def __init__(self):
        super().__init__('parameter_demo')
        self.add_on_set_parameters_callback(self.validate_parameters)
        self.declare_parameter(
            'controller.max_speed', 0.5,
            ParameterDescriptor(
                description='速度上限，单位 m/s，范围 0.0～2.0',
                floating_point_range=[FloatingPointRange(
                    from_value=0.0, to_value=2.0, step=0.0)]))
        self.declare_parameter('enabled', True)
        self.declare_parameter(
            'robot_id', 'demo_01',
            ParameterDescriptor(description='课堂机器人编号', read_only=True))
        self.last_config = None
        self.timer = self.create_timer(1.0, self.observe_config)

    def validate_parameters(self, parameters):
        for parameter in parameters:
            if parameter.name == 'controller.max_speed':
                value = parameter.value
                if (parameter.type_ != Parameter.Type.DOUBLE or
                        not math.isfinite(value) or not 0.0 <= value <= 2.0):
                    return SetParametersResult(
                        successful=False,
                        reason='max_speed 必须是 0.0～2.0 的有限浮点数')
        return SetParametersResult(successful=True)

    def observe_config(self):
        speed = self.get_parameter('controller.max_speed').value
        enabled = self.get_parameter('enabled').value
        robot_id = self.get_parameter('robot_id').value
        config = (speed, enabled, robot_id)
        if config != self.last_config:
            self.get_logger().info(
                f'config robot_id={robot_id} enabled={enabled} '
                f'max_speed={speed:.2f} m/s')
            self.last_config = config


def main(args=None):
    rclpy.init(args=args)
    node = None
    try:
        node = ParameterDemo()
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        if node is not None:
            node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == '__main__':
    main()
```

先讲三件事：`declare_parameter` 建规则；`validate_parameters` 只审候选值；`observe_config` 读取已经提交的配置。定时器只用于每秒观察一次，具体原理放到第 24～25 页讲。

本例没有把参数值永久抄进启动时的成员变量，而是在使用时重新读取。因此合法修改之后，下一次观察就会反映新值。动态“业务生效”仍要由节点代码落实，不是 CLI 自动改写算法。

### 3. 课堂演示

终端 A：

```bash
python3 第三课/课堂实验/parameter_demo.py
```

终端 B 依次执行：

```bash
ros2 param set /parameter_demo controller.max_speed 0.8
ros2 param get /parameter_demo controller.max_speed
ros2 param set /parameter_demo controller.max_speed 3.0
ros2 param get /parameter_demo controller.max_speed
ros2 param set /parameter_demo robot_id changed_id
```

预期：0.8 被接受，终端 A 下一次观察输出 0.80；3.0 被拒绝，读取仍为 0.8；只读编号修改被拒绝。错误文本可能来自描述符检查或自定义回调，判断重点是成功标志和拒绝后值未改变。

终端 C：

```bash
ros2 service list
ros2 interface show rcl_interfaces/srv/SetParameters
ros2 interface show rcl_interfaces/srv/SetParametersAtomically
```

应看到 `/parameter_demo/set_parameters` 和 `/parameter_demo/set_parameters_atomically`。`~/set_parameters` 中的 `~` 表示节点私有命名空间，不是 Linux 家目录。

如命令一直等待，先检查节点是否运行、各终端环境和 Domain ID 是否一致，及节点是否仍在 `spin`。

### 课堂提问

> 一组参数里 enabled 合法、max_speed 非法，普通批量接口是否保证 enabled 也不改变？

参考答案：不保证。需要整组一致时使用原子接口，并在回调中按整组候选配置校验。

### 教师说明

PPT 把普通 `set_parameters` 流程描述成“全部成功或全部保持原样”，应在这里修正。Humble 的参数校验回调应避免副作用；本例日志来自提交后的周期读取。[参数接口与回调说明](https://github.com/ros2/ros2_documentation/blob/humble/source/Concepts/Basic/About-Parameters.rst)、[Humble rclpy 实现](https://github.com/ros2/rclpy/blob/humble/rclpy/rclpy/node.py)

### 补充命令行对照实验

目的：对照普通批量与原子批量的失败行为。A 保持 parameter_demo。B 每轮先恢复基线 enabled=true、max_speed=0.8：
```bash
ros2 param set /parameter_demo enabled true
ros2 param set /parameter_demo controller.max_speed 0.8
ros2 service call /parameter_demo/set_parameters rcl_interfaces/srv/SetParameters   "{parameters: [{name: enabled, value: {type: 1, bool_value: false}}, {name: controller.max_speed, value: {type: 3, double_value: 3.0}}]}"
ros2 param get /parameter_demo enabled
ros2 param get /parameter_demo controller.max_speed
```
预期逐项结果：enabled 可能已为 false，非法速度仍 0.8。再恢复基线并用原子接口：
```bash
ros2 param set /parameter_demo enabled true
ros2 service call /parameter_demo/set_parameters_atomically rcl_interfaces/srv/SetParametersAtomically   "{parameters: [{name: enabled, value: {type: 1, bool_value: false}}, {name: controller.max_speed, value: {type: 3, double_value: 3.0}}]}"
ros2 param get /parameter_demo enabled
ros2 param get /parameter_demo controller.max_speed
```
预期整组被拒绝，两项保持基线。type=1 是 bool，type=3 是 double，可先 interface show rcl_interfaces/msg/ParameterType 对照。最终恢复 enabled=true、速度0.8。

这页最后可以总结：

> **候选值先检查，已提交值再使用；一组配置要共同生效，必须明确使用原子接口。**

---

## 第 9 页｜四种参数配置方式与优先级

这页你可以把它讲成：

> **现在知道怎样改参数了，接着问它最初从哪里来。代码默认值是兜底，文件保存场景配置，Launch 组织启动，运行时修改改变本次状态。不要只背四个名字的排名，要看这次启动到底怎样传参。**

### 1. 本课明确的覆盖顺序

```text
代码默认 0.5
    ↓ 由启动参数文件覆盖
YAML 配置 0.6
    ↓ 本课 Launch 在文件之后传入显式覆盖
启动覆盖 0.7
    ↓ 成功的运行时 set
本次当前值 0.8
```

Launch 也可以只传 YAML，或者调整多个配置源的传入顺序。它是启动组织方式，不是天然压过任何 YAML 的独立等级。

### 2. YAML 配置与验证

保存为 `第三课/课堂实验/config/parameter_demo.yaml`：

```bash
mkdir -p tf_debug
cat > 第三课/课堂实验/config/parameter_demo.yaml <<'YAML'
parameter_demo:
  ros__parameters:
    controller.max_speed: 0.6
    enabled: true
    robot_id: demo_01
YAML
```

停止旧参数节点后，在终端 A 启动：

```bash
python3 第三课/课堂实验/parameter_demo.py \
  --ros-args --params-file 第三课/课堂实验/config/parameter_demo.yaml
```

终端 B：

```bash
ros2 param get /parameter_demo controller.max_speed
```

预期为 0.6。YAML 的节点键要匹配实际名称；重命名节点或加命名空间后，也要核对文件匹配规则。

### 3. 可直接运行的 Launch 示例

保存为 `第三课/课堂实验/parameter_demo.launch.py`。Humble 支持直接传入存在的 Launch 文件绝对路径（文件不存在时会按包名解析，可能显示 Package not found）。本课脚本未安装成 ROS 包，因此用 Launch 的 `ExecuteProcess` 启动 Python；安装成包后可以改用 `launch_ros.actions.Node`，但本页不要求新增包。

```python
from pathlib import Path

from launch import LaunchDescription
from launch.actions import ExecuteProcess


def generate_launch_description():
    experiment = Path(__file__).resolve().parent
    script = experiment / 'parameter_demo.py'
    config = experiment / 'config/parameter_demo.yaml'
    for path in (script, config):
        if not path.is_file():
            raise RuntimeError(
                f'缺少配套文件：{path}。请检查第三课/课堂实验中的源码与 config 目录。')
    return LaunchDescription([
        ExecuteProcess(
            cmd=[
                'python3', str(script),
                '--ros-args',
                '--params-file', str(config),
                '-p', 'controller.max_speed:=0.7'],
            output='screen')
    ])
```

确认现有 `第三课/课堂实验/` 包含 Python 节点、Launch 和 config/parameter_demo.yaml；无需移动文件。停止终端 A 旧进程，再启动：

```bash
source /opt/ros/humble/setup.bash
# 两个文件均须由课前准备或本页创建步骤生成
test -f 第三课/课堂实验/parameter_demo.py && \
  test -f 第三课/课堂实验/config/parameter_demo.yaml && \
  ros2 launch 第三课/课堂实验/parameter_demo.launch.py
```

终端 B 查询，应为 0.7；再 `set` 为 0.8，应读到 0.8。停止 Launch 后按同一文件重启，重新得到 0.7。

### 课堂提问

> YAML 写 0.6，Launch 没有另加速度值，最终一定会出现一个“Launch 值”吗？

参考答案：不会，Launch 只组织已有配置；本课显式传 `-p` 才有额外的 0.7 覆盖。

### 教师说明

PPT 的固定四级优先级可以用来理解本课示例，但不能当成所有启动场景的普遍排序；应核对具体传入顺序与最后运行值。运行时修改也可能被拒绝，并非无条件“优先级最高”。

这页最后可以总结：

> **启动来源决定初值，成功修改决定当前值，重启后仍要重新按启动配置计算。**

---

## 第 10 页｜参数调试工具集

这页你可以把它讲成：

> **调参不能只会 set。先看有哪些参数，再看当前值和约束，修改后确认结果，满意以后保存；恢复配置时也要确认每一项是否真的成功。**

### 1. 对照课件的五类工具

终端 A 保持第 8 页参数节点运行；终端 B 执行：

```bash
ros2 param list /parameter_demo
ros2 param list /parameter_demo --param-type
ros2 param get /parameter_demo controller.max_speed
ros2 param describe /parameter_demo controller.max_speed
ros2 param set /parameter_demo controller.max_speed 0.8
```

应能看到名称、类型、当前值和描述信息。不同安装补丁可能影响输出形式，先以本机 `--help` 为准。

### 2. 导出与恢复

```bash
ros2 param dump /parameter_demo > 第三课/课堂实验/config/parameter_demo_backup.yaml
ros2 param set /parameter_demo controller.max_speed 1.2
ros2 param load /parameter_demo 第三课/课堂实验/config/parameter_demo_backup.yaml
ros2 param get /parameter_demo controller.max_speed
```

备份时是 0.8，恢复后速度应为 0.8。`dump` 也会导出只读项和 `use_sim_time` 等已声明参数；`load` 尝试修改只读 `robot_id` 时可能返回失败。逐项看结果，不能因为命令执行结束就说整份配置完全恢复。

本课若只需要恢复可调项，可以保存以下文件为 `第三课/课堂实验/config/parameter_demo_runtime.yaml`：

```bash
mkdir -p tf_debug
cat > 第三课/课堂实验/config/parameter_demo_runtime.yaml <<'YAML'
parameter_demo:
  ros__parameters:
    controller.max_speed: 0.8
    enabled: true
YAML
```

```bash
ros2 param load /parameter_demo 第三课/课堂实验/config/parameter_demo_runtime.yaml
```

`load` 是对运行节点调用修改接口，不会替你重启节点，也不保证一组参数的原子回滚。

### 课堂提问

> 备份文件存在，是不是证明刚才那次 dump 保存成功了？

参考答案：还要检查内容是否有目标节点的 `ros__parameters` 和期望值；重定向可能创建空文件，不能只检查文件名。

### 教师说明

课件写“配合 -t 显示类型”，Humble 的对应选项是 `--param-type`，不照搬未核对的短选项。`load` 也不是数据库事务式的一键回滚。[list 源码](https://github.com/ros2/ros2cli/blob/humble/ros2param/ros2param/verb/list.py)、[dump 源码](https://github.com/ros2/ros2cli/blob/humble/ros2param/ros2param/verb/dump.py)、[load 源码](https://github.com/ros2/ros2cli/blob/humble/ros2param/ros2param/verb/load.py)

### 补充命令行对照实验

目的：检查配置文件里的实际变化。B：
```bash
ros2 param dump /parameter_demo > tf_debug/params_before.yaml
ros2 param set /parameter_demo controller.max_speed 0.9
ros2 param dump /parameter_demo > tf_debug/params_after.yaml
diff -u tf_debug/params_before.yaml tf_debug/params_after.yaml
ros2 param set /parameter_demo controller.max_speed 0.8
```
预期 diff 指出速度变化；返回 1 表示有差异。文件里仍含只读和系统项，不将整份 dump 无条件视为可完全恢复的运行配置。

这页最后可以总结：

> **列出、读取、描述、修改、导出、加载形成调参闭环，每次修改都以结果和当前值验收。**

---

## 第 11 页｜适用边界与最佳实践

这页你可以把它讲成：

> **有了方便的参数工具，也不能把所有信息都塞进去。判断方法是：这是长期配置，还是持续数据，还是一次业务请求？**

### 1. 保留课件的场景比较

| 场景 | 参数是否合适 | 更具体的选择依据 |
| --- | --- | --- |
| PID 系数、速度上限、避障阈值 | 合适 | 值较稳定，需要配置和低频调试 |
| 串口、设备 IP、设备编号 | 合适 | 是否允许运行时修改由设备能力决定 |
| 功能使能与算法模式 | 可用 | 切换前要检查业务状态与约束 |
| 传感器标定、安装偏移配置 | 可用 | 参数保存配置，空间关系还需由程序发布到 TF |
| 图像、IMU、实时位姿 | 不合适 | 用 Topic 连续发布 |
| 一次状态查询或设备操作 | 通常不用参数 | 用 Service 明确请求和响应 |
| 大模型权重、地图文件 | 通常保存路径而非大数据本身 | 文件、资源加载或专门数据接口 |

### 2. 四项工程原则

统一声明，让配置入口可查；默认值明确，让缺失配置的行为可预测；提交前校验，让非法值不进入应用；集中保存 YAML，让不同部署场景可以追踪。

“集中保存文件”与“参数分布式归属”不冲突：文件可以统一管理，各节点仍拥有自己的运行参数。

### 课堂提问

> 导航取消、每秒 50 次速度指令、速度上限，能否都用参数？

参考答案：不能。导航取消沿上一课的 Action 生命周期处理；连续速度指令通常用 Topic；速度上限可用参数。

### 教师说明

PPT 将“急停、运动控制”统一推荐 Service，范围过宽。持续运动指令与紧急停止都有各自的时效和确认要求，不能仅凭需要响应就认定普通 Service 提供实时保证。本课重点是把低频配置与实时交互区分开。

这页最后可以总结：

> **先判断数据和交互的性质，再选择参数、话题、服务或动作。**

---

## 第 12 页｜日志系统

这是一张章节过渡页，可以这样讲：

> **刚才 CLI 告诉我们参数被拒绝了。真正的机器人还有更多事情：设备连接、任务开始、传感器超时。出了问题，我们不能只说“它不动了”，要留下足够的信息说明什么时候、哪个节点、做了什么、结果怎样。**

板书：

```text
发生事件 → 记录上下文与结果 → 故障时重建过程
```

### 课堂提问

> 日志只有一句“失败了”，能帮助另一个同学定位问题吗？

参考答案：很难；缺少操作对象、失败原因、相关值和时间。

这页最后可以总结：

> **日志把运行过程变成可检查的记录，下一步先看应该记录什么。**

---

## 第 13 页｜日志系统的四大核心价值

这页你可以把它讲成：

> **日志不是每行代码后面都加一句 print。它要回答工程问题：哪里出错，系统是否健康，做过什么，多个节点怎样一起走到这个结果。**

### 1. 故障定位

“读取串口超时，设备 `/dev/ttyUSB0`，等待 0.5 s”比“读取失败”更有用。记录实际观察到的原因；如果只知道超时，就不要直接断言硬件已经损坏。

### 2. 运行监控

记录启动、连接恢复、异常计数等状态。日志能提供观测线索，但仅打印一堆文本不会自动形成统计图和告警。

### 3. 操作回溯

关键配置变化、任务编号和执行结果帮助事后还原。需要严格审计时，还要考虑保存期限、完整性和权限；普通 ROS 日志不是天然不可篡改的审计系统。

### 4. 分布式调试

让客户端和服务端都记录同一个请求编号，可以沿编号找出“发送—接收—处理—响应”。时间戳提供顺序线索，但跨设备还要检查授时。

板书：

```text
请求 id=42：客户端发出 → 服务端收到 → 处理失败 → 返回失败响应
```

### 课堂提问

> 客户端超时，服务端日志显示稍后完成，能否说服务端完全没处理？

参考答案：不能。客户端没有按时收到，与服务端没有执行，是两种情况；需要结合请求编号和时序判断。

这页最后可以总结：

> **有用的日志能还原行为与结果，让别人依据证据定位问题。**

---

## 第 14 页｜日志系统三层架构

这页你可以把它讲成：

> **我们在代码里调用的是一个简单接口，后面还要判断等级、组织内容，再送到不同输出端。课件用三层帮助理解，实际 ROS 2 实现里还有更细的组件。**

### 1. 业务接口层

C++ 使用 `RCLCPP_INFO` 等宏；Python 使用 `self.get_logger().info(...)` 等方法。接口把节点身份和日志处理机制连接起来。

### 2. 抽象与分发

日志器根据有效等级决定是否处理消息；日志器名称有层级关系，子日志器未单独设等级时可以继承父级设置。

### 3. 输出后端

控制台、文件和 `/rosout` 各有不同路径。课堂可以画成：

```text
rclcpp 宏 / rclpy logger
             ↓
等级判断与日志基础设施（rcutils、rcl）
             ↓
       ┌─────┼─────┐
     控制台  文件  /rosout
```

其中 `rcutils` 负责基础日志和控制台格式等，`rcl` 组织输出，默认文件实现使用 `rcl_logging_spdlog`。这是职责图，不声称每条日志严格沿一个单线调用栈通过所有框。

### 课堂提问

> 停用 `/rosout` 后，终端日志也必然消失吗？

参考答案：不会，输出目标可以分别启用或禁用。

### 教师说明

PPT 的“业务接口编译期优化，无运行时开销”范围过宽：被实际输出的日志有判断、格式化、同步与 I/O 开销。C++ 编译期裁剪只针对被裁掉的语句，也不能直接套到 Python。可插拔后端也不代表默认支持任意运行时热切换。[Humble 日志架构](https://github.com/ros2/ros2_documentation/blob/humble/source/Concepts/Intermediate/About-Logging.rst)

这页最后可以总结：

> **日志接口统一了写法，过滤和输出仍是需要理解的实际运行过程。**

---

## 第 15 页｜日志分级使用指南：什么时候用，什么时候不用

这页你可以把它讲成：

> **等级不是字体颜色，而是你对这件事的判断。别人只看告警时，应该能挑出需要处理的情况；打开调试时，又能看到解释过程的细节。**

| 等级 | 含义与本课场景 | 不宜这样用 |
| --- | --- | --- |
| DEBUG | 排查用的中间值、分支和细节 | 默认持续输出每个传感器样本 |
| INFO | 节点启动、配置提交、任务完成 | 所有异常都标 INFO |
| WARN | 当前可恢复或存在风险的异常 | 把一次正常配置读取都标 WARN |
| ERROR | 本次功能或操作失败，需要处理 | 将所有可恢复重试都标 ERROR |
| FATAL | 应用认为关键能力已不可继续 | 以为打印后框架会自动退出 |

以下是节点方法内的局部写法示例；不要把故障演示语句插入正常运行程序后说它真的故障了：

```python
self.get_logger().debug('读取配置，准备更新算法状态')
self.get_logger().info('配置已提交，速度上限为 0.80 m/s')
self.get_logger().warning('传感器暂时超时，继续尝试重连')
self.get_logger().error('本次采集失败，设备未返回有效数据')
self.get_logger().fatal('关键设备不可用，应用进入停止流程')
```

### 课堂提问

> 第一次连接失败，第二次重试成功，最终功能恢复，应不应该把每次重试都写成系统崩溃？

参考答案：不应该。可用 WARN 记录可恢复异常，恢复时用 INFO，最终无法提供功能时再按影响程度用 ERROR 或 FATAL。

### 教师说明

`fatal()` 或 `RCLCPP_FATAL` 只记录严重等级，不自动调用 `shutdown()`，也不自动让机器人安全停止。应用需实现停止、清理和退出逻辑。PPT 的“触发后立即停机”应理解为应用策略。[日志等级与 API](https://github.com/ros2/ros2_documentation/blob/humble/source/Concepts/Intermediate/About-Logging.rst)

这页最后可以总结：

> **等级表达影响程度，真实恢复、停止和退出由业务逻辑完成。**

---

## 第 16 页｜输出等级过滤机制：三级过滤

这页你可以把它讲成：

> **日志语句写在源码里，不代表每次都能看到。它可能在编译时已经被裁掉，也可能低于日志器阈值，还可能对应输出端被禁用了。**

### 1. 三个观察位置

```text
源码日志语句
    ↓ C++ 可选编译期裁剪
是否生成对应日志代码
    ↓ 日志器等级阈值
是否接受这条消息
    ↓ 已启用的输出目标与后端能力
实际输出到哪里
```

等级按 `DEBUG < INFO < WARN < ERROR < FATAL` 排列。阈值为 WARN 时，WARN、ERROR、FATAL 通过，DEBUG 和 INFO 不通过。

### 2. 课堂演示：启动时设置等级

停止第 8 页旧参数节点，在终端 A 运行：

```bash
python3 第三课/课堂实验/parameter_demo.py \
  --ros-args --log-level parameter_demo:=WARN
```

终端 B：

```bash
ros2 param get /parameter_demo controller.max_speed
ros2 param set /parameter_demo controller.max_speed 0.8
```

参数依然可以正常读取和修改，但节点的 INFO 配置观察日志不再显示。停止后改为以下命令重启：

```bash
python3 第三课/课堂实验/parameter_demo.py \
  --ros-args --log-level parameter_demo:=INFO
```

应重新看到 INFO。重启会重新加载参数初值，不把这次比较误读成参数持久化。

### 3. 运行时调整的条件

Python 节点代码可调用 `rclpy.logging.set_logger_level` 调整自身日志器。下面仅为程序内部的局部片段，不是从另一个进程远程修改节点的命令：

```python
from rclpy.logging import LoggingSeverity, set_logger_level
set_logger_level(self.get_logger().name, LoggingSeverity.DEBUG)
```

### 课堂提问

> DEBUG 语句在编译时已被删除，运行时把等级设成 DEBUG 能找回来吗？

参考答案：不能，需重新编译包含该语句的程序。

### 教师说明

Humble 不应默认存在一个通用的 `ros2` 热修改任意节点日志等级命令；远程调整需要节点提供相应接口。默认日志配置也不能据此承诺“终端 WARN、文件 INFO、话题 DEBUG”三套阈值可独立任意设置。尤其日志器已经过滤掉 DEBUG 后，后端不能恢复它；输出端独立开关和独立等级是两回事。[Humble 日志配置](https://github.com/ros2/ros2_documentation/blob/humble/source/Concepts/Intermediate/About-Logging.rst)

这页最后可以总结：

> **先确认代码存在，再确认等级通过，最后确认目标输出已启用。**

---

## 第 17 页｜三大输出通道

这页你可以把它讲成：

> **同一条日志可以现场看、落到文件里，或者通过 ROS 网络被其他节点接收。三个通道各有用途，也各有成本。**

| 通道 | 主要用途 | 需要检查 |
| --- | --- | --- |
| 控制台 | 本地开发时立即观察 | 终端、等级、缓冲与输出开关 |
| 文件 | 保留运行历史、离线排查 | 目录、磁盘空间、保留与轮转策略 |
| `/rosout` | 汇聚同一 ROS 通信域里的日志 | 网络发现、QoS、话题开关和采集端 |

### 课堂演示

终端 A 以 INFO 启动参数节点。终端 C：

```bash
ros2 topic info /rosout -v
ros2 topic echo /rosout rcl_interfaces/msg/Log
```

终端 B 修改速度为 0.9。终端 C 应看到新日志中的 `name`、`level`、`msg` 等字段；各字段组织形式以实际输出为准。先启动监听再修改，避免依赖历史日志是否仍保留。

若需要明确指定文件目录，停止旧参数节点，再在终端 A：

```bash
mkdir -p logs
ROS_LOG_DIR="$HOME/ros2_ws/logs" \
  python3 第三课/课堂实验/parameter_demo.py
```

终端 C：

```bash
ls -la logs
```

节点正常退出后检查文件内容，不把特定文件名写死。Launch 捕获的进程输出文件与节点日志后端文件也可能不同。

停用话题输出的演示命令为：

```bash
python3 第三课/课堂实验/parameter_demo.py \
  --ros-args --disable-rosout-logs
```

先停止已有同名节点，再运行这条命令。此时本节点新日志仍可出现在终端；其他节点仍可能向 `/rosout` 发布，不能靠话题是否存在判断本节点开关。

### 课堂提问

> `/rosout` 里没有记录，就能证明节点没有执行吗？

参考答案：不能，还可能是等级过滤、话题输出关闭、网络或订阅配置问题。

### 教师说明

控制台默认通常输出到 stderr，缓冲可配置，不保证“零延迟”。`/rosout` 的“全局”指可通信域内的汇聚，并非跨所有机器人网络自动共享。日志轮转需核对具体后端和部署方案，不是所有 Humble 输出默认都有按大小／时间轮转。[输出端与环境变量](https://github.com/ros2/ros2_documentation/blob/humble/source/Concepts/Intermediate/About-Logging.rst)

### 补充命令行对照实验

目的：观察单个节点关闭 rosout 后，其他日志仍可能存在。先停止旧 parameter_demo，再 A：
```bash
python3 第三课/课堂实验/parameter_demo.py --ros-args --disable-rosout-logs
```
B 修改合法速度，A 控制台有新观察日志；C echo /rosout 并检查 name 字段，不能靠 /rosout 是否还在判断开关。结束后停止 A，恢复正常启动，重新修改并确认该节点的新 rosout 日志出现。

这页最后可以总结：

> **控制台看现场，文件留历史，rosout 做汇聚；每个通道都要单独验收。**

---

## 第 18 页｜工程使用规范与最佳实践

这页你可以把它讲成：

> **知道接口以后，还要让记录既有用又不过量。想象凌晨接手故障的人没有你的课堂上下文，怎样才能只靠日志把过程看懂？**

### 1. 使用原则

节点运行信息优先使用 ROS 日志 API，保持身份和等级一致。命令行工具展示计算结果可以有正常标准输出，不能把课件的“禁用 printf/cout”扩展成所有软件绝不允许标准输出。

### 2. 内容规范

把“操作、对象、输入关键值、结果、已知原因”放在一起。例如：

```text
不清楚：failed
更清楚：request=42 operation=connect device=/dev/ttyUSB0
        result=timeout wait=0.5s
```

不要把未知根因编进日志，也不要输出课件列出的密码、密钥等敏感内容。

### 3. 性能注意

高频循环采用节流、计数摘要或状态变化时记录。Python 中 f-string 在调用前已经求值，不能指望日志等级过滤自动免掉昂贵计算。需要时先判断等级，再生成复杂内容。

以下是节点方法中的局部片段：

```python
self.get_logger().warning(
    '采集尚未恢复，继续重试', throttle_duration_sec=2.0)
```

它限制该日志调用点的重复输出频率，不会降低采集业务本身的频率。C++ 的 `_THROTTLE` 宏有自己的参数形式，不与 Python 关键字混用。

### 4. 排查顺序

先确定时间范围和任务编号，筛选 WARN／ERROR，然后结合正常 INFO 查前因后果；必要时针对性开启 DEBUG，复现后恢复。仅筛错误而丢掉正常上下文，也可能漏掉关键线索。

### 课堂提问

> 每秒打印 1000 条 INFO，会比每秒一条异常统计更容易排查吗？

参考答案：未必，大量重复日志会淹没关键信息，并增加 I/O 与格式化成本。

### 教师说明

“全链路 100% 打点”“性能零损耗”不当作普遍保证。应覆盖关键路径和异常分支，并按系统负载评估记录成本。日志时间也不天然等于消息采样的 ROS 时间，仿真中不要直接混算；下一部分会区分时钟。[Python 日志行为与节流参数](https://github.com/ros2/ros2_documentation/blob/humble/source/Concepts/Intermediate/About-Logging.rst)

这页最后可以总结：

> **记录关键事实和上下文，用合适等级与频率让日志能被读懂。**

---

## 第 19 页｜时间机制与定时器

这是一张章节过渡页，可以这样讲：

> **日志里有时间，传感器消息里也有时间，但它们未必在同一条时间轴上。仿真暂停了，电脑的日历时间还在走；我们先分清时钟，才能解释定时器和后面的 TF 查询。**

板书：

```text
现在是哪一个时刻？ → Time，时间点
两件事相隔多久？   → Duration，时间间隔
按什么时间推进？   → Clock，时钟
```

### 课堂提问

> 仿真暂停五分钟，模拟机器人应该继续按真实五分钟计算运动距离吗？

参考答案：通常不应该，算法要依据仿真的时间推进；外部看门狗等需求可能仍需按真实经过时间计时。

这页最后可以总结：

> **先选时间基准，再谈计时、调度和同步。**

---

## 第 20 页｜为什么需要独立的时间系统？

这页你可以把它讲成：

> **同样一秒，在实车里是墙钟的一秒，在加速仿真里可能只经过了半秒墙钟，在暂停回放里可能根本没有推进。算法需要能跟随它所在场景的时间。**

### 1. 仿真解耦

节点设置 `use_sim_time=true` 后，其接入 TimeSource 的 ROS 时钟使用 `/clock`。仿真器暂停时间推进，依赖该 ROS 时钟的积分和周期任务也要按相应语义处理。

```text
仿真器 / 人工时钟 / Bag 回放
            ↓ /clock
使用仿真时间的节点 ROS 时钟
            ↓
算法时间戳与关联定时器
```

`use_sim_time` 是每个节点的参数，不是执行一次就切换全系统。程序中的 `time.time()` 或显式创建的系统时钟不会因此自动变成仿真时间。

### 2. 多传感器对齐

实车上的设备可能使用各自晶振。PTP、NTP、硬件触发或驱动时间映射负责建立可比较的时间基准；ROS 消息携带时间戳，但不会自动校准所有传感器硬件时钟。

### 3. 离线回放

Bag 播放可以提供 `/clock`，让订阅节点按回放进度理解时间。需要显式启用相应播放选项，并让参与节点使用仿真时间；不是所有 Bag 播放都自动接管节点时钟。

本课不要求先创建 Bag。教师如已有录包，可先看本机 `ros2 bag play --help`，再核对 `--clock`；第 22、25 页用人工 `/clock` 就能验证核心原理。

### 课堂提问

> 只把一个融合节点设为仿真时间，其他节点还按真实时间打时间戳，能直接融合吗？

参考答案：不能假定可以，时间轴不一致；要统一参与节点和数据来源的时间基准。

### 教师说明

“一套代码无需任何修改”要求业务确实使用 ROS 时钟并正确处理暂停与跳变。设置参数不会自动改写直接读取操作系统时钟的代码，也不会自动完成多传感器同步。[ROS 2 时间设计](https://design.ros2.org/articles/clock_and_time.html)

这页最后可以总结：

> **时间抽象让算法适应不同推进方式，硬件同步和业务语义仍需明确实现。**

---

## 第 21 页｜时钟抽象层设计

这页你可以把它讲成：

> **算法尽量问“节点现在几点”，而不是自己决定去哪个硬件时钟取值。这样底层换成仿真时间，上层也能沿同一个接口工作。**

### 1. 对照课件三层关系

```text
业务代码：取得时间，计算本业务的间隔
                   ↓
Clock 接口：提供时间点与时钟类型
                   ↓
System / Steady / ROS 时钟
```

ROS 时钟可由 TimeSource 接入 `/clock`。定时器属于使用时钟的调度对象，不能把所有定时器管理和速率控制都等同为 `Clock` 本身的方法。

### 2. 本课正确的取时方式

以下是节点方法中的局部片段：

```python
stamp = self.get_clock().now().to_msg()
```

`now()` 得到 rclpy 时间对象，`to_msg()` 转成消息里的秒和纳秒字段。仅创建一个未连接 TimeSource 的 ROS Clock 对象，不保证它会自动订阅 `/clock`；本课优先使用节点自带时钟。

### 3. 依赖倒置怎样理解

可以这样类比：

> 算法像看表的人，Clock 是读表接口，TimeSource 决定表怎样走。用同一接口读表，并不代表看表的人可以忽略表暂停或倒拨。

### 课堂提问

> `use_sim_time=true` 会把显式创建的 Steady Clock 也切换成 `/clock` 吗？

参考答案：不会；稳态时钟用于真实经过时间等需求，和节点 ROS 时钟是不同对象。

### 教师说明

课件的“所有时间源均支持动态热切换”“业务逻辑绝对稳定”应改为：节点 ROS 时间可以在系统来源和仿真来源间切换；系统、稳态和 ROS 时钟类型有不同语义，时间跳变仍需应用处理。相关 rclpy 行为见 [Node 的时钟与 TimeSource 接入](https://github.com/ros2/rclpy/blob/humble/rclpy/rclpy/node.py)。

这页最后可以总结：

> **统一取时接口提高复用性，正确处理时钟变化才能保证算法行为。**

---

## 第 22 页｜三类时间源详细对比

这页你可以把它讲成：

> **三种时钟不是谁更高级，而是回答不同的问题：日历时刻、真实耗时、仿真进度。选错以后，代码即使没有语法错误，算出的时间差也可能没有意义。**

### 1. 保留课件六个比较维度并修正时间源

| 维度 | System 系统时间 | Steady 稳态时间 | ROS 时间 |
| --- | --- | --- | --- |
| 来源 | 操作系统墙钟 | 操作系统单调时钟 | 默认跟随系统；仿真启用后使用 `/clock` |
| 单调性 | 可校准、回拨或跳变 | 不回退；连续两次读取可能相同 | 可暂停、前跳，也可回退 |
| 系统授时影响 | 受 NTP／手动调整影响 | 不随墙钟回拨 | 非仿真时受影响；仿真时取决于来源 |
| 适用场景 | 真实日期时刻 | 性能耗时、硬件超时 | 节点数据时间戳、仿真、回放 |
| 暂停／倍速 | 通常不按仿真控制 | 不按仿真控制 | 可由时钟发布源控制 |
| 配置关联 | 显式系统时钟不受 `use_sim_time` 切换 | API 显式创建 | 节点 ROS 时钟受 `use_sim_time` 控制 |

### 2. 完整对照脚本

保存为 `第三课/课堂实验/clock_probe.py`：

```python
import rclpy
from rclpy.clock import Clock, ClockType
from rclpy.node import Node


class ClockProbe(Node):
    def __init__(self):
        super().__init__('clock_probe')
        self.system_clock = Clock(clock_type=ClockType.SYSTEM_TIME)
        self.steady_clock = Clock(clock_type=ClockType.STEADY_TIME)
        self.start_steady = self.steady_clock.now().nanoseconds
        self.ros_ticks = 0
        self.ros_timer = self.create_timer(1.0, self.on_ros_tick)
        self.observer = self.create_timer(
            1.0, self.observe, clock=self.steady_clock)

    def on_ros_tick(self):
        self.ros_ticks += 1
        self.get_logger().info(f'ROS timer tick={self.ros_ticks}')

    def observe(self):
        ros_ns = self.get_clock().now().nanoseconds
        system_ns = self.system_clock.now().nanoseconds
        elapsed_ns = self.steady_clock.now().nanoseconds - self.start_steady
        self.get_logger().info(
            f'ros={ros_ns / 1e9:.3f}s '
            f'system={system_ns / 1e9:.3f}s '
            f'steady_elapsed={elapsed_ns / 1e9:.3f}s '
            f'ros_ticks={self.ros_ticks}')


def main(args=None):
    rclpy.init(args=args)
    node = ClockProbe()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == '__main__':
    main()
```

终端 A：

```bash
python3 第三课/课堂实验/clock_probe.py --ros-args -p use_sim_time:=true
```

终端 B：

```bash
ros2 param get /clock_probe use_sim_time
ros2 topic info /clock -v
```

在没有 `/clock` 发布者的情况下，应看到仿真时间为 true、ROS 时间尚为 0，稳态观察输出继续增长，ROS 定时器不正常按墙钟每秒触发。每次启动观察节点会创建自己的时钟订阅，不能因为 `/clock` 话题名存在就认为已有时钟源。

第 25 页继续用这个节点演示人工推进；本页先让学生观察两条时间轴分开了。

### 课堂提问

> 仿真暂停后，一个硬件连接等待最多 2 秒的超时该不该也永远停住？

参考答案：若要求真实经过 2 秒后返回，应使用稳态计时；算法仿真节拍才适合 ROS 时间。

### 教师说明

PPT 写 `/ros_time`，正确标准话题是 `/clock`；ROS 时间不保证递增。稳态的保证是单调不回退，不是每次读数都严格大于前一次。开启仿真时间但没有收到时钟消息时，0 表示尚未初始化的状态。[时间设计](https://design.ros2.org/articles/clock_and_time.html)、[Humble 定时器的显式时钟参数](https://github.com/ros2/rclpy/blob/humble/rclpy/rclpy/node.py)

这页最后可以总结：

> **日历用系统时钟，真实耗时用稳态时钟，仿真与节点数据使用约定好的 ROS 时间。**

---

## 第 23 页｜标准时间戳结构：为什么是 sec + nanosec？

这页你可以把它讲成：

> **时间戳要传给别的节点，得有统一格式。我们把整数秒和剩余纳秒分开，保留可表示范围内的细小时间差。这里的纳秒是表示单位，不是设备真的测到了纳秒精度。**

### 1. 字段与归一化

```text
builtin_interfaces/msg/Time
int32  sec
uint32 nanosec，范围 0～999,999,999

时间值 = sec + nanosec × 10^-9 秒
```

例子：12.25 秒表示为 `sec=12, nanosec=250000000`。负时间 -1.7 秒按标准归一化为 `sec=-2, nanosec=300000000`，不是让无符号纳秒写负数。

### 2. 为什么不把大时间戳全部放进 double

浮点数有效位有限。秒数较大时，能够区分的最小时间差变大；两个只差 1 ns 的 Unix 大时间戳，转为 double 后可能相同。

Python 中可用整数纳秒算差值，下面是时间处理的局部片段：

```python
timestamp_ns = stamp.sec * 1_000_000_000 + stamp.nanosec
```

只在最后显示时转成小数秒；消息传输仍用标准字段。C++ 使用足够宽的整数并注意溢出，不能把所有运算都压进 32 位。

### 3. 范围、精度和语义

int32 正秒范围约 68 年；若把原点取为 Unix epoch，正向上界在 2038 年附近，不能讲成“从今天起还能随意用 68 年”。仿真可以使用不同起点，但必须保持语义一致。

消息字段不附带时钟类型。同样的 `sec=12`，可能是仿真第 12 秒，不可能仅凭数值知道来自哪个时钟。

### 课堂演示

终端 C：

```bash
ros2 interface show builtin_interfaces/msg/Time
ros2 interface show std_msgs/msg/Header
```

引导观察 `Header` 中的 `stamp` 和 `frame_id`：前者回答何时，后者回答在哪个坐标系，第 34 页会一起使用。

### 课堂提问

> 消息能存 1 ns 的变化，是不是传感器同步误差也已经小于 1 ns？

参考答案：不是。表示分辨率、真实时钟准确度、采样延迟和授时误差是不同指标。

### 教师说明

PPT 的“绝对精确”“完全一致”需要限定为标准整数表示和不溢出的相关运算，不能推广到实际同步误差、所有算法或无限时间范围。[Time 消息定义与负值示例](https://github.com/ros2/rcl_interfaces/blob/humble/builtin_interfaces/msg/Time.msg)

### 补充命令行对照实验

目的：验证 sec/nanosec 与小数时刻。先停止 Gazebo 或其他 /clock 生产者，仅 A 的 clock_probe(use_sim_time=true) 运行。B：
```bash
ros2 topic pub /clock rosgraph_msgs/msg/Clock "{clock: {sec: 10, nanosec: 500000000}}" --once
```
预期 A 的稳态观察输出 ROS 时间约 10.500s；不是 10.05 或 10.0005。再发布 sec=11,nanosec=0，观察推进。单次 pub 等待匹配时 A 必须已创建 clock 订阅；避免旧 clock 同时写入。

这页最后可以总结：

> **秒加纳秒保存时间值，正确的时间基准和采样语义才让这个值有用。**

---

## 第 24 页｜定时器的底层原理与实现

这页你可以把它讲成：

> **定时器不是另开一个线程，每秒 sleep 一次。它记录下一次何时应就绪，执行器在等待其他事件时也关注它；到期之后，还得有机会调度回调。**

### 1. 正确理解等待集合

```text
定时器：关联时钟 + 周期 + 下次到期时刻
                          ↓
Wait Set：订阅、服务、定时器等是否就绪
                          ↓
Executor：按执行器与回调组规则选择可执行回调
                          ↓
业务函数真正开始运行
```

没有运行 `spin`，仅创建定时器不会自动执行回调。底层就绪不代表立即获得 CPU，也不代表所有回调按到达先后严格排队。

### 2. 五类延迟因素

操作系统调度、CPU 竞争、执行器处理其他回调、时钟本身与时间消息推进情况、业务回调耗时，都会影响实际执行时刻。可以区分：

```text
计划到期时刻 ── 调度延迟 ── 回调开始 ── 业务执行耗时 ── 结束
```

在默认单线程执行器中，耗时回调会阻塞同一执行器上的其他工作；多线程也要满足回调组允许并发等条件，第五课会进一步展开。

### 3. 适用边界

心跳、周期发布与一般超时检查适合定时器。本课默认桌面／WSL 环境不提供硬实时期限保证；要求严格期限的控制环需要专门的实时架构和测量验证，不能只靠把周期填小。

### 课堂提问

> 周期填 0.01 秒，但另一个回调一次执行 0.2 秒，在单线程里能保证准时每 0.01 秒执行吗？

参考答案：不能，执行器忙时会延迟；必须减少阻塞或调整调度结构。

### 教师说明

课件把 ROS 2 定时器统一描述为 Linux `timerfd + epoll`，不符合 Humble 的通用实现。`rcl` 定时器维护时钟与到期信息，等待集合根据定时器计算等待时间并检查就绪；不能把某种平台机制当成跨平台固定原理。也不把执行器讲成严格“谁先触发先执行”。[rcl timer 实现](https://github.com/ros2/rcl/blob/humble/rcl/src/rcl/timer.c)、[rcl wait 实现](https://github.com/ros2/rcl/blob/humble/rcl/src/rcl/wait.c)

这页最后可以总结：

> **定时器到期只是变为可执行，真正开始还取决于执行器和系统调度。**

---

## 第 25 页｜定时器三大核心要素与控制操作

这页你可以把它讲成：

> **写定时器时先回答三句话：多久一次，执行什么，依据哪个时钟。然后再区分暂停、重新计时和销毁资源。**

### 1. 三个要素

| 要素 | 本课例子 | 注意 |
| --- | --- | --- |
| 周期 | `1.0` 秒，对应 1 Hz | 周期 T 与频率 f 的关系为 f=1/T |
| 回调 | `on_ros_tick` 或 `observe` | 保持短小，不在单线程回调里等待很久 |
| 时钟 | 节点 ROS Clock 或显式 Steady Clock | 默认值取决于 API；C++ wall timer 不等于 ROS timer |

第 22 页 `create_timer(1.0, ...)` 默认用节点时钟，而 `clock=self.steady_clock` 明确选择稳态时钟。不要把 Python 的默认规则直接套给所有 C++ 定时器 API。

### 2. 课堂演示：推进与暂停

前提：第 22 页节点在终端 A 以 `use_sim_time=true` 运行，没有其他 `/clock` 发布者。

终端 B 逐条执行，每条之后留几秒让学生观察：

```bash
ros2 topic pub --once /clock rosgraph_msgs/msg/Clock \
  "{clock: {sec: 10, nanosec: 0}}"
ros2 topic pub --once /clock rosgraph_msgs/msg/Clock \
  "{clock: {sec: 11, nanosec: 0}}"
ros2 topic pub --once /clock rosgraph_msgs/msg/Clock \
  "{clock: {sec: 12, nanosec: 0}}"
```

预期：ROS 时间依次显示 10、11、12 秒；推进后 ROS 定时器获得触发机会。停止发布后，ROS 时间停在 12，稳态观察继续输出，而 ROS tick 不再按真实秒数增长。大幅跳时间不意味着补执行每个遗漏的周期；不要规定第一次跳到 10 后必须已经执行十次。

再发 5 秒：

```bash
ros2 topic pub --once /clock rosgraph_msgs/msg/Clock \
  "{clock: {sec: 5, nanosec: 0}}"
```

应观察到 ROS 时间回退，而稳态经过时间继续增长。此处只观察时钟，不用回退后的值继续作普通正向积分。

若 ROS 时间不变，先查节点参数、`/clock` 类型和发布连接；不要因为日志行首的系统时间在走，就误判节点 ROS 时间也在走。

### 3. Humble Python 控制 API

以下为节点方法内的局部片段，作用对象为第 22 页的 `self.ros_timer`：

```python
self.ros_timer.cancel()                      # 暂停触发，保留对象
is_paused = self.ros_timer.is_canceled()
self.ros_timer.reset()                       # 恢复并重新安排下一周期
remaining_ns = self.ros_timer.time_until_next_call()
self.destroy_timer(self.ros_timer)           # 真正移除定时器资源
```

### 课堂提问

> 定时器 cancel 后还能 reset 吗？destroy 后还能当原定时器继续用吗？

参考答案：cancel 保留对象，可以 reset；destroy 后应重新创建，不继续访问已销毁对象。

### 教师说明

PPT 的 `time_until_trigger()` 不是本课 rclpy 方法；Humble 使用 `time_until_next_call()`，返回整数纳秒或无有效值，不是 Duration 对象。`cancel()` 不是资源释放。回调过慢会延迟或错过期望节拍，但不应笼统说每个过期周期都无限排队，或必然死锁；具体行为要结合执行器和阻塞逻辑。[Humble Timer API](https://github.com/ros2/rclpy/blob/humble/rclpy/rclpy/timer.py)

这页最后可以总结：

> **周期决定计划节拍，时钟决定怎样推进，cancel、reset 和 destroy 分别处理暂停、重启与释放。**

---

## 第 26 页｜时间戳的工程意义

这页你可以把它讲成：

> **时间戳不是给消息加一个好看的数字。机器人运动时，旧数据和新位姿混在一起，即使空间公式完全正确，结果也可能错。**

### 1. 多传感器同步

雷达在 12.0 秒采样，相机在 12.1 秒采样。如果车以 1 m/s 前进，两次采样间约移动 0.1 m。融合前必须考虑采样时刻与运动，而不是只看消息何时收到。

### 2. 延迟与性能评估

区分采样、发送、接收和处理完成。单进程算法耗时可用稳态时钟测量；从采样到处理的延迟要确保两端时间可比较。系统时间与仿真时间不能直接相减。

### 3. 插值

已知 12.0 与 12.1 秒的状态，可以在合理模型下估算 12.05 秒状态。插值在已有样本之间进行，不等于能准确预测还没有的数据。

### 4. 时序异常

检测乱序、重复和明显异常；但回放重置允许时间回退，此时需要按场景清理或重置状态，而不是一律认定时钟坏了。

### 5. 故障回溯

时间戳联合节点名、任务编号和来源重建事件顺序。两个事件可能有相同时间戳，时间值本身不是全系统唯一 ID。

板书：

```text
数据：何时采样？在哪个 frame？
变换：同一时刻该 frame 在哪里？
                  ↓
时刻相容，空间换算才有意义
```

### 课堂提问

> 雷达数据是 0.1 秒前的，为什么不能总用机器人最新位姿去转换？

参考答案：机器人在这段时间内可能移动；正确做法通常是用数据采样时间查询变换，或做明确的运动补偿。

### 教师说明

本页回到第 2 页的问题：有时间戳只是具备表示时刻的字段，同步还需要共同基准、采样语义和误差控制。PPT 的“唯一锚点”“自动消除数据跳变”都是过强说法，时间戳和插值只能提供必要信息与处理手段。

这页最后可以总结：

> **先让数据的时间可比较，再用同一时刻的空间关系处理它。**

---

## 第 27 页｜TF2 坐标变换系统

这是一张章节过渡页，可以这样讲：

> **刚才解决了“这条数据是什么时刻的”。现在继续解决“它是从哪里量出来的”。雷达、车身、地图各有自己的原点和方向，TF2 帮我们维护它们之间的关系。**

板书：

```text
雷达坐标里的点 → 车身坐标里的点 → 地图坐标里的点
          同一个物理点，不同的坐标表示
```

### 课堂提问

> 改变参考坐标系，会把障碍物真的移动到另一个地方吗？

参考答案：不会，改变的是描述同一个点所使用的坐标。

这页最后可以总结：

> **TF2 维护参考系之间的空间关系，让同一数据能在不同坐标系中使用。**

---

## 第 28 页｜为什么需要坐标变换？三大痛点

这页你可以把它讲成：

> **假设雷达装在车身原点前方 0.2 米、高 0.1 米的位置。雷达测到前方 1 米的点，车身看见的是哪里？只有平移时不难，但再加朝向、多个传感器和运动时间，就很容易算错。**

### 1. 计算易错

平移和旋转要按正确顺序组合，矩阵乘法不可随意交换。多个坐标系之间还要处理正向与逆向变换。

### 2. 维护困难

若每个算法都把 0.2 米偏移写在自己的源码里，调整雷达安装位置就要改很多模块。让一个明确的发布源维护安装变换，使用方查询关系，可以减少重复配置。

### 3. 时间缺失

雷达与车身的安装关系固定，车身相对地图的位置却会变化。处理旧数据必须能查询对应时刻的关系，而不是只存一份“当前位置”。

板书：

```text
固定安装：base_link → laser_frame
车身运动：odom → base_link，带时间戳
地图定位：map → odom，可能修正
```

### 课堂提问

> 雷达安装偏移只在一个模块更新，其他模块仍保留旧偏移，会有什么结果？

参考答案：各模块对同一障碍物的位置理解不一致，可能导致感知和规划结果冲突。

### 教师说明

TF2 负责组织、组合与查询已有变换，不负责自动测量传感器安装误差，也不会自动估计机器人位置。正确标定与正确时间戳仍由来源节点负责。

这页最后可以总结：

> **把空间关系集中定义、按时间维护，可以减少重复运算和重复配置。**

---

## 第 29 页｜TF2 三大核心特性

这页你可以把它讲成：

> **TF2 的能力可以记成三件事：表达三维位姿，沿坐标链组合关系，在已有历史里查询对应时间。每件事都有前提。**

### 1. 六自由度位姿

三个平移量 x、y、z，加上三维朝向，共六自由度。消息用平移向量和单位四元数表示旋转；四元数字段顺序是 x、y、z、w。

```text
无平移：x=y=z=0
无旋转：qx=qy=qz=0，qw=1
单位四元数要求：qx²+qy²+qz²+qw²≈1
```

四元数不是四个独立旋转自由度，全零四元数也不是“没有旋转”。四元数适合旋转组合与插值，不能据此讲成所有计算中的绝对最优表示。

### 2. 自动链式计算

输入源坐标系和目标坐标系，TF2 沿连接路径组合变换。树结构中两点间路径唯一；关键是连通且时间数据可用，不是需要在多条合法路线中选择“最短路线”。

### 3. 缓存与插值

动态历史默认缓存窗口通常为 10 秒，可配置。两个已有时刻之间可以对平移线性插值、对旋转进行球面插值；超出可用历史不能凭空补齐。

```text
已有：t=1.0 ───── t=1.1
查询：       t=1.05 → 可在满足条件时插值
查询：t=0.5 或 t=1.2 → 可能外插失败
```

### 课堂提问

> 节点刚启动只收到一秒数据，默认缓存十秒能否查九秒前？

参考答案：不能。十秒是保留窗口，不是自动生成十秒历史；新监听器通常也不会从动态话题自动补回此前全部历史。

### 教师说明

缓存和插值不等于“完美解决所有传感器同步”，也不保证轨迹永远无跳变；错误数据、定位修正或时钟回退仍需处理。[BufferCore 默认缓存](https://github.com/ros2/geometry2/blob/humble/tf2/include/tf2/buffer_core.hpp)、[动态缓存和插值实现](https://github.com/ros2/geometry2/blob/humble/tf2/src/cache.cpp)

这页最后可以总结：

> **TF2 能组合和插值已有空间关系，查询能力受拓扑、数据和时间范围约束。**

---

## 第 30 页｜坐标系树结构：三条铁则

这页你可以把它讲成：

> **看 PPT 右边这棵树：map 连 odom，odom 连车身，车身再连雷达、相机和 IMU。箭头表示父子归属；它不代表消息只能沿这个方向转换。**

### 1. 按原图保留关系

```text
map
 └─ odom
     └─ base_link
         ├─ laser_frame   激光雷达
         ├─ camera_frame  深度相机
         └─ imu_frame     IMU
```

完整机器人按课件图组织。本课实际运行只建立 `map→odom→base_link→laser_frame`，不假定相机与 IMU 发布节点已经存在。

### 2. 三条规则的正确边界

每个连通的树有一个根；同一个子坐标系的关系应有明确的父坐标系和唯一权威发布来源；不能有环路。

多个互不连接的树可以同时存在。各树内部仍可能查询成功，跨树查询没有连接路径；不能讲成一出现多根，所有 TF 都彻底失效。

不要让两个节点同时发布 `odom→base_link`，也不要一个说 `map→base_link`、另一个说 `odom→base_link`，造成父关系竞争。

### 课堂提问

> 雷达和相机都以 base_link 为父，是否违反单父规则？

参考答案：不违反。一个父可以有多个子；要求的是每个子有一致的父关系。

### 教师说明

PPT 的 `rosrun rqt_tf_tree rqt_tf_tree` 是 ROS 1 写法，本课使用第 36 页 `ros2 run tf2_tools view_frames`。环路违反模型，可能造成查询失败或异常，但不声称必然无限递归、栈溢出；TF2 实现有遍历深度等防护。[BufferCore 拓扑与深度限制](https://github.com/ros2/geometry2/blob/humble/tf2/include/tf2/buffer_core.hpp)

这页最后可以总结：

> **保持单父、无环和需要查询的链路连通，才能得到一致的坐标关系。**

---

## 第 31 页｜REP 105 深度解读：为什么有 map 和 odom 两个全局坐标系？

这页你可以把它讲成：

> **导航同时需要两种性质：相对地图的位置要有长期参考，短时间运动又希望连续平滑。一次定位修正可能突然改变地图位置，所以我们把这两种职责分到 map 和 odom。**

### 1. 准确性与连续性的区别

| 坐标系 | 职责 | 局限 |
| --- | --- | --- |
| `map` | 提供长期全局定位参考，修正累计漂移 | 定位更新可能跳变，仍有误差 |
| `odom` | 为短时运动提供连续参考 | 会随运动累计漂移 |
| `base_link` | 固连机器人本体 | 相对 map 或 odom 的位姿由估计结果决定 |

不是 map “绝对精准”，也不是 odom 永远没有噪声，而是它们的期望性质与用途不同。

### 2. 三层分工

```text
map ── 定位修正 ──→ odom ── 连续里程计估计 ──→ base_link
```

里程计来源发布 `odom→base_link`；定位来源结合全局位置和里程计关系发布 `map→odom`，将修正放在上层。不要为了额外输出 map 位置，又给 base_link 增加一个独立父关系。

可以类比：

> 里程计像按步数记录你一路走了多远，地图定位像看到路标后修正“原来我在这里”。修正地图参考，不必让每一步的连续估计跟着突然倒退。

### 课堂提问

> AMCL 重定位后 map 中车身位置发生变化，是否一定是 TF 数据损坏？

参考答案：不一定，可能是预期的定位修正；要看哪条边跳变、定位状态和 odom 的连续性。

### 教师说明

REP 105 描述参考系语义，不保证具体定位算法准确，也不自动确保控制器安全平滑。PPT 的“完全吸收漂移”“底层绝对连续”应按设计目标和实际估计质量讲。[REP 105 原文](https://github.com/ros-infrastructure/rep/blob/master/rep-0105.rst)

这页最后可以总结：

> **map 管长期参考，odom 管短时连续，把定位修正放到 map 与 odom 之间。**

---

## 第 32 页｜静态 vs 动态坐标变换

这页你可以把它讲成：

> **雷达固定装在车身上，车身却在地图里运动。前一种关系应作为静态变换管理，后一种需要带时间持续更新。静态不代表写进整个系统后永远不会丢。**

### 1. 保留课件的八个比较维度

| 维度 | 静态 TF | 动态 TF |
| --- | --- | --- |
| 核心特性 | 相对位姿在本模型中不随时间改变 | 位姿随时间变化 |
| 典型场景 | 固定雷达安装 | 车身运动、关节运动、云台转动 |
| 发布工具 | `StaticTransformBroadcaster` | `TransformBroadcaster` |
| 话题 | `/tf_static`，使用 transient local 等 QoS | `/tf`，接收后由 Buffer 管理时间历史 |
| 资源 | 不必高频重复同一固定值 | 需按运动和应用需求持续更新 |
| 缓存 | 已接收的静态关系不按动态历史窗口过期 | 动态样本通常按可配置历史窗口管理 |
| 来源停止 | 已有监听器可能仍保留；新监听器不保证还能获取 | 旧缓存可能仍可查，最新数据的新鲜度下降 |
| 插值 | 不需要按时间插值 | 有合适前后样本时可插值，不是每次查询必插值 |

### 2. 静态安装关系演示

在终端 B 运行并保持进程存活：

```bash
ros2 run tf2_ros static_transform_publisher \
  --x 0.2 --y 0.0 --z 0.1 \
  --roll 0.0 --pitch 0.0 --yaw 0.0 \
  --frame-id base_link --child-frame-id laser_frame \
  --ros-args -r __node:=laser_static
```

终端 C：

```bash
ros2 run tf2_ros tf2_echo base_link laser_frame
```

预期平移为 `[0.2, 0.0, 0.1]`，四元数约为 `[0.0, 0.0, 0.0, 1.0]`。位置单位为米，欧拉角命令参数单位为弧度。采用具名参数避免位置参数顺序混淆。

这里 `tf2_echo base_link laser_frame` 返回雷达原点在车身系中的位姿；对应变换可把雷达坐标数据转换进车身坐标系。交换两个名字就得到逆关系。

### 课堂提问

> 静态发布一次后，立刻退出发布进程，半小时后新启动的监听器一定收到吗？

参考答案：不保证。transient local 不是永久磁盘存储，应保持发布实体存活或按需要重新发布。

### 教师说明

ROS 2 用 transient local QoS 解释静态历史交付，比直接沿用 ROS 1 的 Latch 术语更准确。PPT 的“重启后仍有效”“不依赖发布节点存活”需要修正；动态 TF 也不是发布者一退出所有缓存立即失效。常规工程中静态边用静态发布器可减少开销，但用动态发布器发送恒定值不等于系统必然崩溃。[静态 QoS](https://github.com/ros2/geometry2/blob/humble/tf2_ros/include/tf2_ros/qos.hpp)、[静态缓存](https://github.com/ros2/geometry2/blob/humble/tf2/src/static_cache.cpp)、[静态发布器命令](https://github.com/ros2/geometry2/blob/humble/tf2_ros/src/static_transform_broadcaster_program.cpp)

### 补充命令行对照实验

目的：只交换查询方向，验证逆变换。B 保持本页静态发布器，C 先查询 base_link→laser_frame，退出后再查反向：
```bash
ros2 run tf2_ros tf2_echo base_link laser_frame
ros2 run tf2_ros tf2_echo laser_frame base_link
```
两条是依次运行的持续命令，中间 Ctrl+C。零旋转时预期平移分别约 (0.2,0,0.1)m 与 (-0.2,0,-0.1)m。有旋转时逆平移需要旋转矩阵，不能一般化为直接逐项取负。最后退出 C。

这页最后可以总结：

> **静态描述固定关系，动态维护时间序列；历史交付和运行期缓存都不是跨重启永久保存。**

---

## 第 33 页｜广播与监听机制：分布式架构

这页你可以把它讲成：

> **TF2 没有一个让所有算法远程查询的中央坐标服务器。发布者把变换发到话题，每个使用者的监听器接收，再放进自己的 Buffer；查询主要在这个本地缓存里完成。**

### 1. 三个角色

```text
Broadcaster                   Listener + Buffer
父/子、平移、旋转、时间戳
        ├── /tf ─────────────→ 动态历史
        └── /tf_static ──────→ 静态关系
                                      ↓
                              本节点查询与组合
```

广播器决定发布哪些关系，话题承担传播，监听器负责接收，Buffer 负责存储和查询。只创建 Buffer，没有监听器也不主动 `set_transform`，不会自动出现变换。

数据中的 `header.frame_id` 是父坐标系，`child_frame_id` 是子坐标系；平移是子原点在父系中的位置，旋转表达子坐标轴相对父系的朝向。不是“平移矩阵”字段，而是三个向量分量。

### 2. 完整动态广播程序

保存为 `第三课/课堂实验/odom_tf_demo.py`：

```python
import rclpy
from geometry_msgs.msg import TransformStamped
from rclpy.node import Node
from tf2_ros import TransformBroadcaster


class OdomTFDemo(Node):
    def __init__(self):
        super().__init__('odom_tf_demo')
        self.broadcaster = TransformBroadcaster(self)
        self.start_time = self.get_clock().now()
        self.timer = self.create_timer(0.1, self.publish_transform)

    def publish_transform(self):
        now = self.get_clock().now()
        elapsed = (now - self.start_time).nanoseconds / 1e9
        transform = TransformStamped()
        transform.header.stamp = now.to_msg()
        transform.header.frame_id = 'odom'
        transform.child_frame_id = 'base_link'
        transform.transform.translation.x = 0.1 * elapsed
        transform.transform.translation.y = 0.0
        transform.transform.translation.z = 0.0
        transform.transform.rotation.x = 0.0
        transform.transform.rotation.y = 0.0
        transform.transform.rotation.z = 0.0
        transform.transform.rotation.w = 1.0
        self.broadcaster.sendTransform(transform)


def main(args=None):
    rclpy.init(args=args)
    node = OdomTFDemo()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == '__main__':
    main()
```

本例按默认系统来源的 ROS 时间运行，模拟车身沿 odom 的 x 轴以 0.1 m/s 移动，不控制真实机器人。它不是里程计算法，也没有实现任意时间回退后的状态重置；本页不切换仿真时间。

### 3. 组成本课坐标链

停止第 22 页时间观察节点，终端 A：

```bash
python3 第三课/课堂实验/odom_tf_demo.py
```

终端 B 保留第 32 页的雷达静态发布器。额外终端 D 启动：

```bash
ros2 run tf2_ros static_transform_publisher \
  --x 0.0 --y 0.0 --z 0.0 \
  --roll 0.0 --pitch 0.0 --yaw 0.0 \
  --frame-id map --child-frame-id odom \
  --ros-args -r __node:=map_odom_static
```

课堂用固定恒等 `map→odom` 简化演示；真实导航中这条边通常由定位模块动态维护，不能把本课替代节点和真实定位节点同时启动。

终端 C：

```bash
ros2 run tf2_ros tf2_echo map laser_frame
```

预期得到完整链的结果：x 随时间增加，z 约为 0.1，旋转为单位四元数。x 大约等于本次模拟车身位置加 0.2；不要求和人为计时完全精确相等。

如果连不上，先分别检查 `map↔odom`、`odom↔base_link`、`base_link↔laser_frame`，定位是哪条边缺失。

### 课堂提问

> 两个节点各自有 Buffer，是不是它们在同一瞬间一定包含完全相同的数据？

参考答案：不是，发现、网络延迟和处理调度不同，局部缓存可能暂时不同。

### 教师说明

分布式减少中央查询依赖，但某条关键边的唯一来源停止后仍会影响业务。查询在本地不代表承诺微秒级耗时，等待缺失数据的查询也可能等待或失败。时间旅行仅限已收到的有效历史。[Humble Listener](https://github.com/ros2/geometry2/blob/humble/tf2_ros_py/tf2_ros/transform_listener.py)、[Buffer 查询 API](https://github.com/ros2/geometry2/blob/humble/tf2_ros_py/tf2_ros/buffer.py)

这页最后可以总结：

> **发布者传播关系，监听器填充本地缓存，使用者在数据可用时查询。**

---

## 第 34 页｜变换计算原理：齐次矩阵与链式相乘

这页你可以把它讲成：

> **工具能替我们计算，但方向不能猜。先约定一个记号：T_AB 表示把 B 中的坐标转换成 A 中的坐标。这样连乘顺序就能靠中间字母对上。**

### 1. 齐次矩阵表达

```text
T_AB = [ R_AB(3×3)  t_AB(3×1) ]
       [ 0  0  0       1      ]

p_A = R_AB p_B + t_AB
[p_A; 1] = T_AB [p_B; 1]
```

R 描述旋转，t 是 B 原点在 A 坐标系里的位置。最后补一个 1，让平移和旋转可以统一写成矩阵乘法。

### 2. 三条计算规则

链式相乘：

```text
T_AC = T_AB × T_BC
p_A  = T_AB × T_BC × p_C
```

逆变换：

```text
R_BA = R_AB 的转置
t_BA = -R_AB 的转置 × t_AB
```

逆变换的平移不总是简单取负，还要考虑旋转。时间插值必须对同一时间需求下可用的样本进行，不能在链上任取各边不同的“最新时刻”拼成一次精确采样时刻查询。

### 3. 先用简单数值建立方向感

第 32 页雷达在车身前方 0.2 米、高 0.1 米，朝向相同。雷达点：

```text
p_laser = (1.0, 0.0, 0.0)
p_base  = (1.2, 0.0, 0.1)
```

车身在 odom 中前进 s 米时，这个点在 odom 中的 x 为 `s+1.2`。这里是每次假想在雷达前方重新取一个点，不是声称一个固定障碍物始终跟着机器人移动。

### 4. 完整监听与点转换程序

保存为 `第三课/课堂实验/tf_point_demo.py`：

```python
import rclpy
from geometry_msgs.msg import PointStamped
from rclpy.duration import Duration
from rclpy.node import Node
from rclpy.time import Time
from tf2_geometry_msgs import do_transform_point
from tf2_ros import Buffer, TransformException, TransformListener


class TFPointDemo(Node):
    def __init__(self):
        super().__init__('tf_point_demo')
        self.buffer = Buffer(cache_time=Duration(seconds=10.0), node=self)
        self.listener = TransformListener(self.buffer, self)
        self.timer = self.create_timer(1.0, self.convert_point)

    def convert_point(self):
        point = PointStamped()
        point.header.frame_id = 'laser_frame'
        point.header.stamp = self.get_clock().now().to_msg()
        point.point.x = 1.0
        point.point.y = 0.0
        point.point.z = 0.0
        try:
            transform = self.buffer.lookup_transform(
                'base_link', 'laser_frame',
                Time.from_msg(point.header.stamp))
            converted = do_transform_point(point, transform)
            # 本次只改变参考系，保留模拟采样时间。
            converted.header.stamp = point.header.stamp
            self.get_logger().info(
                f'frame={converted.header.frame_id} '
                f'point=({converted.point.x:.2f}, '
                f'{converted.point.y:.2f}, {converted.point.z:.2f})')
        except TransformException as error:
            self.get_logger().warning(
                f'等待或检查 TF: {error}', throttle_duration_sec=2.0)


def main(args=None):
    rclpy.init(args=args)
    node = TFPointDemo()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == '__main__':
    main()
```

运行前提：至少保留第 32 页 `base_link→laser_frame` 的静态发布器；完整链可继续运行，但这个转换只需要其中一条边。额外终端 E：

```bash
python3 第三课/课堂实验/tf_point_demo.py
```

预期日志含：

```text
frame=base_link point=(1.20, 0.00, 0.10)
```

逐段解释：PointStamped 同时声明点的 frame 和采样时刻；Listener 持续接收；`lookup_transform(target, source, time)` 先写想转换到的坐标系，再写数据原坐标系；`do_transform_point` 完成数值转换。这个例子的边是静态边，查询当前采样时刻不需要等待未来动态样本。

启动初期可能先出现等待警告。程序使用不带阻塞等待的查询，失败后下个周期再试，让同一单线程执行器有机会接收 TF；不要在此回调中加长时间阻塞等待而妨碍 Listener。

### 课堂提问

> 要把雷达点变成车身点，应该写 `lookup_transform('laser_frame', 'base_link', ...)` 吗？

参考答案：应写目标 `base_link` 在前，源 `laser_frame` 在后。把顺序倒过来得到逆变换。

### 教师说明

PPT 的“A→C”在图上的父子方向和数据变换方向可能混淆；讲数学公式时先给出本页 T_AB 的定义。`tf2_echo` 命令行中两个参数的术语也容易混淆，以实际结果含义为准：`tf2_echo base_link laser_frame` 用于将 laser 的数据转换到 base。[Buffer 参数定义](https://github.com/ros2/geometry2/blob/humble/tf2_ros_py/tf2_ros/buffer.py)、[点转换实现](https://github.com/ros2/geometry2/blob/humble/tf2_geometry_msgs/src/tf2_geometry_msgs/tf2_geometry_msgs.py)、[tf2_echo 的方向说明](https://github.com/ros2/geometry2/blob/humble/tf2_ros/src/tf2_echo.cpp)

这页最后可以总结：

> **先定义源、目标和时间，再组合变换；点的位置要连同 frame 和 stamp 一起理解。**

---

## 第 35 页｜TF2 四大核心应用场景

这页你可以把它讲成：

> **现在把刚才一条雷达点转换扩展到真实机器人。TF2 是很多应用共同使用的空间关系工具，但它不代替感知、定位或机械臂规划算法。**

### 1. 传感器数据转换

点、点云等具有明确三维坐标语义的数据，可以依对应消息类型进行转换。二维图像检测框不能只靠 TF2 直接变成世界中的三维位置，还需要深度、相机模型或其他几何信息。

向量、姿态和带协方差的消息有各自转换规则，也不能都照着“点加平移”的方式处理。

### 2. 导航与定位

定位和里程计维护不同边，规划读取需要的坐标关系。更换算法时，如果 TF、地图、时间戳等接口约定保持一致，可减少上层改动；并非任何算法换上去都自动完全兼容。

### 3. 机械臂运动学

URDF 描述关节与连杆几何，关节状态给出当前关节值，`robot_state_publisher` 根据模型计算并发布关系；TF2 再查询和组合已有链。

```text
URDF 模型 + joint_states
             ↓ robot_state_publisher
         各连杆 TF
             ↓ TF2 查询
        末端位姿
```

没有机器人几何模型，TF2 不会自动知道关节轴和连杆长度；它也不自动求逆运动学或生成运动轨迹。相关模型细节会在后续 URDF 课程展开。

### 4. 多传感器融合

TF2 提供空间转换和已有历史变换查询；采样同步、测量模型、噪声和融合估计仍由对应系统负责。时间对齐与空间对齐都需要正确来源。

### 课堂提问

> 给一幅二维图片加上 camera_frame，TF2 是否已经知道图中每个人的三维世界位置？

参考答案：不知道，还缺像素到三维的几何关系和距离信息。

### 教师说明

课件的“自动完成正运动学、无需模型推导”应改为：模型和关节值由机器人描述与状态发布系统处理，TF2 消费、组合已发布变换。不要把“查询关节链”讲成不需要运动学模型。

这页最后可以总结：

> **TF2 统一空间关系，上层算法仍负责产生正确模型、数据和估计结果。**

---

## 第 36 页｜调试工具三件套

这页你可以把它讲成：

> **排查顺序先看连接，再核数值，最后查时间和发布情况。不要一看到模型不对，就直接提高频率或重写矩阵。**

前提：第 32、33 页的两个静态发布器和一个动态发布器保持运行；终端 C 执行调试，持续运行的命令用 Ctrl+C 结束后再执行下一条。

### 1. view_frames：看结构

```bash
(cd tf_debug && ros2 run tf2_tools view_frames)
ls -lt tf_debug
```

采集片刻后应生成坐标树 PDF 等文件，名称依工具版本而定。打开新生成的 PDF，应看到：

```text
map → odom → base_link → laser_frame
```

它反映采集窗口内观察到的关系，不是未来运行永远正确的证明。若图没生成，检查 Graphviz、当前目录写权限和实际工具输出。

### 2. tf2_echo：核数值与方向

```bash
ros2 run tf2_ros tf2_echo base_link laser_frame
ros2 run tf2_ros tf2_echo odom base_link
ros2 run tf2_ros tf2_echo map laser_frame
```

第一条固定 `[0.2, 0.0, 0.1]`；第二条 x 随模拟移动增长；第三条连乘后 x 约比车身位置大 0.2，z 约为 0.1。三条依次运行，比较时考虑车身还在移动。

### 3. tf2_monitor：查发布情况

```bash
ros2 run tf2_ros tf2_monitor odom base_link
```

观察统计中的频率、平均与最大延迟。动态源计划约 10 Hz，实际统计受负载、采集时长和工具算法影响，不要求精确为 10.000。

也可不加坐标系参数查看整体：

```bash
ros2 run tf2_ros tf2_monitor
```

静态关系不持续高频发布，其统计不能按动态源的心跳标准判断。延迟常包含接收时刻与变换时间戳之间的差异，不是纯网络耗时；跨主机需要时钟同步才能合理解释。

### 课堂提问

> 图上链路完整，但雷达点偏向车后方，下一步先做什么？

参考答案：用 `tf2_echo` 核对变换方向、单位和安装旋转；链路存在不等于数值正确。

### 教师说明

Humble 中使用实际注册的 `view_frames`，不默认沿用课件其他页的 `view_frames.py`。`tf2_monitor` 提供诊断统计，不会自动修复链路或保证标记所有业务超时。[view_frames 实现](https://github.com/ros2/geometry2/blob/humble/tf2_tools/tf2_tools/view_frames.py)、[tf2_echo 实现](https://github.com/ros2/geometry2/blob/humble/tf2_ros/src/tf2_echo.cpp)、[tf2_monitor 实现](https://github.com/ros2/geometry2/blob/humble/tf2_ros/src/tf2_monitor.cpp)

这页最后可以总结：

> **结构回答连没连，数值回答对不对，发布与时间统计帮助解释为什么不稳定。**

---

## 第 37 页｜三大常见问题与解决方法

这页你可以把它讲成：

> **报错先分类：名字不知道、两个树不连通、还是查询时间没有数据？这几种问题的处理方向不同，不能都用“多等几秒、提高频率”解决。**

### 1. 查找失败与连接失败

| 现象 | 先检查 | 方法与下一步 |
| --- | --- | --- |
| frame 不存在 | 名称、来源节点、发现和启动时间 | 查看节点与话题，核对 `view_frames`，再检查大小写与空格 |
| 两个 frame 都在但不连通 | 中间边是否缺失 | 逐边 echo，找到缺失发布源 |
| 新监听器没有静态安装关系 | 静态发布实体是否仍活着、QoS 是否正确 | 重启正确静态发布器，检查 `/tf_static` 端点 |

终端 C：

```bash
ros2 node list
ros2 topic info /tf -v
ros2 topic info /tf_static -v
```

本课应该看到 `odom_tf_demo`、`laser_static`、`map_odom_static` 等节点。节点名与 frame 名属于不同命名对象，不要求二者相同。

### 2. Extrapolation：时间窗口不满足

```text
查询太新：该时刻的动态变换尚未收到
查询太旧：需要的历史未收到或已超出缓存窗口
时间域错：真实时间与仿真时间混用
```

Python 最新可用查询写法如下，是已存在 Buffer 的局部片段：

```python
transform = self.buffer.lookup_transform(
    'map', 'laser_frame', Time())
```

`Time()` 的零值在该查询 API 中表示最新可用／链路最新共同时间，并非要求用仿真第 0 秒的样本。处理实际传感器消息时，通常应查询 `Time.from_msg(msg.header.stamp)`；改用最新值只是明确的工程取舍，不能悄悄丢弃采样时间。

太新的数据可以在允许的延迟内等待或采用异步策略，但执行器要能持续接收 TF。太旧的数据可能需更长缓存或调整处理延迟；延长等待不会重新生成已经丢失的历史。提高发布频率也不能解决两个时钟原点不同。

### 3. 跳变与卡顿

先定位具体哪条边变化，再检查时间戳、发布来源和模型。`map→odom` 的定位修正可能合理；`base_link→laser_frame` 突然变化可能是错误配置或竞争发布。

| 检查项 | 可能发现 | 处理方向 |
| --- | --- | --- |
| 同一子 frame 的发布来源 | 多个节点竞争发布 | 保留唯一权威来源 |
| frame 与角度单位 | 名称写错、度被当作弧度 | 修正命名与单位 |
| 动态 stamp | 时间域不一致、乱序、明显回退 | 统一时间来源，按场景处理重置 |
| 发布与回调负载 | 更新过慢、回调阻塞 | 按运动速度和误差需求调整，不只增加 Hz |
| 定位状态 | 重定位造成预期修正 | 区分全局修正和底层连续运动 |

### 课堂演示：只改变一个条件

停止终端 A 的动态发布器，保持两个静态发布器运行。在已经运行的 `tf2_echo map laser_frame` 中观察时间戳是否停止更新，再新启动工具比较行为。

预期：已有缓存可能仍能返回最后关系；新监听器缺少动态边，不能假定也能查到完整链。重新启动动态源后，查询应恢复。不要声称旧缓存十秒后必然按墙钟自动消失；动态窗口主要按数据时间管理，最新查询可能持续返回陈旧值，应用需自己检查新鲜度。

### 课堂提问

> lookup 返回成功，是否就证明变换足够新，可以直接用于当前运动控制？

参考答案：不证明。还要核对返回时间戳、业务允许延迟和时间来源。

### 教师说明

PPT 的 `ros::Time(0)` 是 ROS 1 C++ 写法；本课 Python 用 `rclpy.time.Time()`。固定“提高到 10 Hz／30 Hz”不是通用修复阈值，课件“90%”也未给出统计依据，不作为量化结论。[Buffer 查询与等待](https://github.com/ros2/geometry2/blob/humble/tf2_ros_py/tf2_ros/buffer.py)、[动态缓存管理](https://github.com/ros2/geometry2/blob/humble/tf2/src/cache.cpp)

### 补充命令行对照实验

目的：让错误名称产生可识别现象。保持正确 TF 来源，C：
```bash
timeout --signal=INT 5s ros2 run tf2_ros tf2_echo base_link laser_typo
```
预期 frame 不存在而不是角度计算错；timeout=124 属于等待结束。改回 laser_frame 应恢复。不要通过新增错误名 frame 来“修复”真实传感器消息里的拼写问题。结束本课所有时钟与 TF 演示，下一课重新明确来源。

这页最后可以总结：

> **先分拓扑和时间问题，再查来源与数据新鲜度；查询成功也要判断是否适合本次业务。**

---

## 第 38 页｜课程总结：四大基础工具

这页你可以把它讲成：

> **回到第一课的那台巡检机器人：配置要查得清，操作要有记录，数据要在同一时间轴，空间要能沿正确链路换算。今天这四种工具，是把节点拼成可调试机器人系统的基础。**

### 1. 四条主线

| 工具 | 今天应能回答 | 需要保留的边界 |
| --- | --- | --- |
| 参数 | 谁拥有、怎样声明、怎样修改和保存？ | 普通批量修改不保证原子，业务生效要自己实现 |
| 日志 | 怎样分级、过滤、输出和重建过程？ | FATAL 不自动停机，输出和格式化有成本 |
| 时间 | 用哪个时钟、怎样表示 stamp、为什么回调延迟？ | ROS 时间可暂停／回退，纳秒字段不等于纳秒同步精度 |
| TF2 | 谁发布、怎样成树、怎样按时间换算和排查？ | 需要正确模型与数据，缓存不是无限历史或永久保存 |

课件总结中的“所有参数修改具备原子性”“固定优先级”“所有坐标系必须全局单根”，分别按第 8、9、30 页修正后的条件回顾，不在结束时又恢复成绝对结论。

### 2. 课堂提问：回答封面问题

> 雷达报告前方 1 米，怎样得到车身和地图中的位置？

参考答案：数据要带正确的 `frame_id` 和采样时间；先用雷达安装变换，再结合对应时刻的车身／地图关系进行换算。本课无旋转的静态安装例中，车身坐标为 `(1.2, 0.0, 0.1)`；地图坐标还取决于该时刻完整链的位姿。

再问：

> 参数修改成功、日志存在、TF 查询成功，是不是机器人业务一定正确？

参考答案：还要验证应用真的使用新参数，日志内容准确，变换方向、时间和新鲜度符合业务需要。工具结果需要和业务行为一起验收。

### 3. 课后练习一：参数完整闭环

```text
任务：完成速度上限的读取、合法修改、非法拒绝与保存。
操作：启动 parameter_demo；设置 0.8，再设置 3.0；
      导出配置，重启时显式加载，另测只读 robot_id。
验收：0.8 成功，3.0 拒绝且值不变；运行时编号修改拒绝；
      默认重启与文件重启的值分别符合其配置。
解释：区分类型、范围、只读和持久化；指出普通批量接口的边界。
```

扩展：向 `/parameter_demo/set_parameters_atomically` 同时提交 `enabled=false` 与非法速度 3.0。用 `ros2 interface show rcl_interfaces/srv/SetParametersAtomically` 查看字段，确认整组失败后两项都保持提交前的值。不要在测试前先单独修改 enabled，否则无法判读本次原子结果。

具体操作：先读出并记录两项当前值，再发出一次整组请求，最后重新读取。

```bash
ros2 param get /parameter_demo enabled
ros2 param get /parameter_demo controller.max_speed
ros2 service call /parameter_demo/set_parameters_atomically \
  rcl_interfaces/srv/SetParametersAtomically \
  "{parameters: [{name: enabled, value: {type: 1, bool_value: false}}, {name: controller.max_speed, value: {type: 3, double_value: 3.0}}]}"
ros2 param get /parameter_demo enabled
ros2 param get /parameter_demo controller.max_speed
```

`ParameterValue` 的类型常量中 1 表示 BOOL，3 表示 DOUBLE，可用 `ros2 interface show rcl_interfaces/msg/ParameterType` 核对。预期整组返回 `successful=false`，两项读取结果与提交前相同。若 enabled 提交前已是 false，先通过合法单项修改设为 true，再记录基线并执行本次实验。

### 4. 课后练习二：日志与时间

```text
任务 A：解释“节点正常运行但没有 INFO 输出”。
操作：分别以 INFO 和 WARN 启动参数节点，查询参数并修改；
      比较终端记录；开启 rosout 后检查新日志字段。
验收：WARN 下参数仍可操作，INFO 被过滤；说明哪条记录被过滤。

任务 B：解释“仿真停住但电脑时间继续走”。
操作：以 use_sim_time=true 启动 clock_probe，发布 10、11、12 秒；
      停止推进，再发布 5 秒。
验收：ROS 时间能停住并回退，稳态观察继续增长；
      分别说明算法节拍和真实超时应该用哪个时钟。
```

提交观察字段和解释，不要求把日志行首的系统时刻当作仿真时刻。

### 5. 课后练习三：TF2 数值与排障

```text
任务：建成 map→odom→base_link→laser_frame，并完成点转换。
操作：运行两条静态边和一条动态边；生成树图；
      查询安装关系；运行 tf_point_demo；停止动态源后比较新旧监听器。
验收：树图链路完整；静态平移为 (0.2, 0.0, 0.1)；
      雷达点 (1.0, 0.0, 0.0) 转成车身点 (1.2, 0.0, 0.1)；
      能解释旧缓存可查为何不等于数据仍在更新。
```

扩展安装方向练习：先停止原 `laser_static`，重新发布同一子 frame，改为 `--yaw 1.57079632679`，其他参数不变。最好重启点监听节点，避免把旧缓存和新配置混淆。雷达正 x 轴点旋转到车身正 y，预期车身点约为 `(0.2, 1.0, 0.1)`。不得让新旧两个发布器同时争用 `laser_frame`。

### 本课最后一句话

> **配置看归属和提交结果，日志看事实与上下文，时间看基准，空间看方向、链路和时刻。**

---

## 教师核对资料

使用 rosdep 前，先初始化并更新规则；已初始化时跳过 init。


技术核对以 ROS 2 Humble 的文档分支、接口定义和实现为准；ROS 2 设计文档与 REP 用于说明语义，不把其他发行版新增 API 当作本课默认功能。以下官方仓库链接也可在文档网站访问受限时使用。

- [Humble：参数概念、类型、服务与回调](https://github.com/ros2/ros2_documentation/blob/humble/source/Concepts/Basic/About-Parameters.rst)
- [Humble：rclpy Node，声明、描述符、参数提交与定时器创建](https://github.com/ros2/rclpy/blob/humble/rclpy/rclpy/node.py)
- [Humble：ParameterDescriptor](https://github.com/ros2/rcl_interfaces/blob/humble/rcl_interfaces/msg/ParameterDescriptor.msg)
- [Humble：ros2 param list](https://github.com/ros2/ros2cli/blob/humble/ros2param/ros2param/verb/list.py)
- [Humble：ros2 param dump](https://github.com/ros2/ros2cli/blob/humble/ros2param/ros2param/verb/dump.py)
- [Humble：ros2 param load](https://github.com/ros2/ros2cli/blob/humble/ros2param/ros2param/verb/load.py)
- [Humble：日志与配置](https://github.com/ros2/ros2_documentation/blob/humble/source/Concepts/Intermediate/About-Logging.rst)
- [ROS 2：Clock and Time 设计](https://design.ros2.org/articles/clock_and_time.html)
- [Humble：builtin_interfaces/Time](https://github.com/ros2/rcl_interfaces/blob/humble/builtin_interfaces/msg/Time.msg)
- [Humble：rclpy Timer](https://github.com/ros2/rclpy/blob/humble/rclpy/rclpy/timer.py)
- [Humble：rcl 定时器实现](https://github.com/ros2/rcl/blob/humble/rcl/src/rcl/timer.c)
- [Humble：rcl 等待集合实现](https://github.com/ros2/rcl/blob/humble/rcl/src/rcl/wait.c)
- [REP 103：单位与坐标轴约定](https://github.com/ros-infrastructure/rep/blob/master/rep-0103.rst)
- [REP 105：map、odom、base_link 的职责](https://github.com/ros-infrastructure/rep/blob/master/rep-0105.rst)
- [Humble：TF2 BufferCore 默认历史窗口与树结构](https://github.com/ros2/geometry2/blob/humble/tf2/include/tf2/buffer_core.hpp)
- [Humble：动态变换缓存与插值](https://github.com/ros2/geometry2/blob/humble/tf2/src/cache.cpp)
- [Humble：静态变换缓存](https://github.com/ros2/geometry2/blob/humble/tf2/src/static_cache.cpp)
- [Humble：TF 发布／监听 QoS](https://github.com/ros2/geometry2/blob/humble/tf2_ros/include/tf2_ros/qos.hpp)
- [Humble：Python TransformListener](https://github.com/ros2/geometry2/blob/humble/tf2_ros_py/tf2_ros/transform_listener.py)
- [Humble：Python Buffer 查询 API](https://github.com/ros2/geometry2/blob/humble/tf2_ros_py/tf2_ros/buffer.py)
- [Humble：PointStamped 转换](https://github.com/ros2/geometry2/blob/humble/tf2_geometry_msgs/src/tf2_geometry_msgs/tf2_geometry_msgs.py)
- [Humble：static_transform_publisher 参数](https://github.com/ros2/geometry2/blob/humble/tf2_ros/src/static_transform_broadcaster_program.cpp)
- [Humble：view_frames](https://github.com/ros2/geometry2/blob/humble/tf2_tools/tf2_tools/view_frames.py)
- [Humble：tf2_echo](https://github.com/ros2/geometry2/blob/humble/tf2_ros/src/tf2_echo.cpp)
- [Humble：tf2_monitor](https://github.com/ros2/geometry2/blob/humble/tf2_ros/src/tf2_monitor.cpp)

验证记录：已读取课件全部 38 页的文字、表格与图形对象关系，并核对逐页顺序与主要信息覆盖；对讲义内 Python、YAML 和 Bash 示例进行静态检查，对无旋转和 90° 旋转的点转换作独立数值核算。未在 ROS 2 Humble 中实际启动节点、进行通信测试或生成 TF 图；当前 Windows 会话无法访问可运行的 WSL ROS 环境。因此课堂前仍须按每页“预期”在目标环境预演，特别核对本机工具版本、输出格式、发现等待和暂停／回退行为。静态语法检查不等于 ROS 2 运行测试通过。
