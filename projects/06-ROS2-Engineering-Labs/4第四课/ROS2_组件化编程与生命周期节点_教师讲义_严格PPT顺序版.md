# ROS 2 组件化编程与生命周期节点：逐页教师讲义

对应课件：`2026.8.10 ROS2 组件化编程与生命周期节点学习手册.pptx`，共 28 页。

严格一页对应一节；讲稿、命令行实验与技术修正均放在原页面位置。

## 上课前准备

### 执行目录与课前准备

**本课所有命令均从整套讲义的根目录执行**，也就是同时能看到 `第一课`、`第二课`、`第三课` 等文件夹的位置。请先在 WSL Ubuntu 的 Bash 中进入此目录；每个新终端都保持在该目录。若终端已位于 `第四课`，先执行 `cd ..` 回到讲义根目录。

Python 程序直接使用已提供的 `第四课/课堂实验/` 文件，无需复制到另一份脚本目录。第三课配置直接使用 `第三课/课堂实验/config/`。只有需要 colcon 构建的 ROS 包才由准备脚本写入旁边的 `../ros2_ws/src/`，构建在子 Shell 中进行，完成后当前终端仍位于讲义根目录。

```bash
source /opt/ros/humble/setup.bash
test -d "第四课/课堂实验" || { echo "请先进入整套讲义根目录"; exit 1; }
mkdir -p tf_debug
sudo apt update && sudo apt install -y ros-humble-rclcpp-components ros-humble-lifecycle ros-humble-demo-nodes-cpp ros-humble-launch-ros python3-colcon-common-extensions python3-rosdep && \
python3 prepare_course.py 4 --workspace ../ros2_ws && \
(cd ../ros2_ws && colcon build --packages-select demo_component --symlink-install) && \
source ../ros2_ws/install/setup.bash
```

节点持续运行时，后面的查询命令应在另一个终端执行。各终端使用相同 ROS_DOMAIN_ID，先停止旧实验再启动新实验。

所有 Bash 命令都在 WSL Ubuntu 22.04、ROS 2 Humble 中执行。每个新终端先 `source /opt/ros/humble/setup.bash`；使用本地包时再 `source ../ros2_ws/install/setup.bash`。三个终端的 ROS_DOMAIN_ID 一致。

课堂终端：A 运行生产者/主体进程，B 控制或接收，C 查询与记录。持续命令用 `Ctrl+C` 结束；同一个终端不能在前台进程运行时继续执行下一条命令。实验切换前停止上个实验的发布者，避免同名话题、节点或 TF 混入。

随讲义交付的完整源码在 `第四课/课堂实验/`，直接使用这些文件。

前置知识：节点、话题、C++ 基础与构建/source。依赖：
```bash
sudo apt install ros-humble-rclcpp-components ros-humble-lifecycle ros-humble-demo-nodes-cpp
ros2 pkg executables rclcpp_components
ros2 pkg executables lifecycle
ros2 component load --help
ros2 lifecycle set --help
```
前半统一包 demo_component、容器 /lesson4_container、类 demo_component::Talker/Listener、话题 /component_chatter；后半官方节点 /lc_talker、话题 /lifecycle_chatter。课前完整准备已构建源码，第 10 页讲解构建规则；第 8、9 页可直接使用准备好的包。

---



使用 rosdep 前，先初始化并更新规则；已初始化时跳过 init。

```bash
if [ ! -f /etc/ros/rosdep/sources.list.d/20-default.list ]; then
  sudo rosdep init
fi
rosdep update --rosdistro humble
```

## 第 1 页｜ROS2 组件化编程与生命周期节点

这页你可以把它讲成：

> **今天解决两个不同问题：节点放在哪个进程里，以及进程存在时业务是否应该运行。组件化管理部署，生命周期管理状态。**

### 讲解与例子

用相机流水线开场：相机、矫正和识别频繁传图，可以考虑共享进程；相机设备需要先打开、再启动采集、再暂停和关闭，则需要明确的状态。两条设计轴可以组合，不能互相替代。承接前课节点、执行器与话题概念，本课会用加载和状态命令观察变化。

### 板书

```text
部署轴：独立进程 / 组件容器
状态轴：未配置 / 待命 / 运行 / 终结
```

### 课堂提问

> 同一容器里的节点是否一定都正在执行业务？

参考答案：不一定，加载只是创建实例；生命周期节点还可能未配置或未激活。

这页最后可以总结：

> **部署形态和业务状态要分别判断。**

---

## 第 2 页｜课程目录

这页你可以把它讲成：

> **先看怎样把节点变成可加载的库，再看怎样让节点按状态有序启停。**

### 讲解与例子

按 PPT 第一部分讨论必要性、容器、插件注册、进程内通信、开发与加载；第二部分讨论普通节点痛点、状态机、转换回调、选型及组合。本课的验证路径也按这个顺序：先看到组件 ID，再看到生命周期状态和消息变化。

### 板书

```text
库注册 → 容器加载 → 观察通信
状态查询 → 合法转换 → 观察启停
```

### 课堂提问

> 为什么不先讲 Nav2 自动编排？

参考答案：先掌握单个节点的接口和状态，才能解释编排器在调用什么。

这页最后可以总结：

> **先验证一个对象，再讨论系统组合。**

---

## 第 3 页｜组件化编程与动态加载

这页你可以把它讲成：

> **组件仍然是 ROS 节点，只是由容器帮它创建实例和运行回调。**

### 讲解与例子

把容器类比成共享工作室，组件是入驻岗位。岗位仍有名称、话题和参数，只是不用每个岗位单独租一个进程。后续先使用空容器，再加载我们自己的两个组件。

### 板书

```text
空容器 → 加载组件 → 节点加入执行器
```

### 课堂提问

> 加载组件是否相当于重新启动整个容器？

参考答案：不是，容器可在运行中创建新的组件实例。

这页最后可以总结：

> **节点功能与运行载体可以分开。**

---

## 第 4 页｜为什么需要组件化？

这页你可以把它讲成：

> **先问哪里有成本，再决定是否共用进程。不是节点一多就必须全部放一起。**

### 讲解与例子

沿课件四点讲资源开销、复用、通信和部署。进程有地址空间和运行资源，高频大消息跨进程通常有传输成本；组件库让同一类节点能够在不同容器里实例化。但普通节点也能复用代码，也可以由启动工具动态增减，不把传统方式描述成天然不能扩展。用两段独立 talker/listener 建立进程基线，只观察布局，不将字符串实验当图像吞吐基准。

### 板书

```text
问题：大消息交互 / 资源限制 / 动态组合
代价：共享故障边界
```

### 命令行实验

目的：建立两进程基线。A/B 分别运行官方 talker/listener，C 查进程；实验后停止 A/B。
```bash
# A
ros2 run demo_nodes_cpp talker
# B
ros2 run demo_nodes_cpp listener
# C
ros2 node list
ps -eo pid,comm,args | grep -E 'demo_nodes_cpp/(talker|listener)' | grep -v grep
```
预期：两个业务节点、两个独立程序。进程列表观察布局，不证明吞吐性能。没有节点先检查 source 与 Domain ID。

### 课堂提问

> 低频小消息一定值得组件化吗？

参考答案：未必，封装和调试成本可能超过收益，应先测实际瓶颈。

这页最后可以总结：

> **组件化解决具体部署成本，需要保留取舍。**

---

## 第 5 页｜组件化核心设计理念

这页你可以把它讲成：

> **一份动态库可以提供节点类，容器按类名实例化多个节点。磁盘上的库不等于运行中的节点。**

### 讲解与例子

对照 PPT 两侧：独立进程各自运行；容器中节点共享地址空间，并由执行器调度。组件类通常接受 NodeOptions，保留命名、参数、通信等能力。共享进程不自动启用进程内通信，更不保证所有消息路径零拷贝。

### 板书

```text
共享库（类定义） → 工厂 → 节点实例 → 执行器
```

### 课堂提问

> 只有 .so 文件，ros2 node list 会出现节点吗？

参考答案：不会，需要容器加载并实例化。

### 教师说明

课件中的内存下降比例属于特定测量，不能作为本实验必现结果。

这页最后可以总结：

> **库提供能力，实例才出现在计算图里。**

---

## 第 6 页｜普通节点 vs 组件节点对比

这页你可以把它讲成：

> **对照时抓住编译产物、运行载体、故障边界和调试方式，不背没有测量依据的纳秒数字。**

### 讲解与例子

普通入口是可执行程序的 main，组件入口通过库中的工厂；二者仍可使用标准话题接口。共享地址空间提高耦合的故障影响范围，同容器一处严重崩溃可能带走其他节点。进程隔离也不能消除上游数据缺失造成的业务影响。普通节点可以提供可复用类，组件也可另写独立入口。

### 板书

```text
独立程序：独立进程边界
组件库：容器进程边界
共同点：节点、参数、话题接口
```

### 课堂提问

> 能否只因共享进程就承诺性能提升 50 倍？

参考答案：不能，需固定消息大小、频率、拓扑、QoS 和测量方式再比较。

这页最后可以总结：

> **选型要比较真实负载和故障范围。**

---

## 第 7 页｜组件容器内部结构

这页你可以把它讲成：

> **容器里有一个管理器负责装卸节点，执行器负责调度，两个职责不能混在一起。**

### 讲解与例子

沿图示解释三部分：ComponentManager 暴露加载、卸载和列表服务；动态库工厂创建实例；执行器驱动订阅、定时器等回调。管理服务能响应并不代表业务回调没有阻塞。容器默认单线程，多线程容器仍要配合回调组设计。

### 板书

```text
CLI → 管理服务 → 库/实例
                     ↓
                  Executor → 回调
```

### 命令行实验

A 启动空容器，后续第 11 页继续使用它：
```bash
# A
ros2 run rclcpp_components component_container --ros-args -r __node:=lesson4_container
# C
ros2 component list
ros2 service list -t | grep lesson4_container
ros2 node info /lesson4_container
```
预期：空容器已有管理节点和加载/卸载服务，但尚无业务组件。若容器不存在，先确认 A 未退出。

### 课堂提问

> 知道组件 ID 能判断它是否收到数据吗？

参考答案：ID 只标识实例，数据行为还要查话题和日志。

这页最后可以总结：

> **管理平面和数据平面分别观察。**

---

## 第 8 页｜底层插件实现原理

这页你可以把它讲成：

> **插件要能被加载，需要同时做好类注册、构建安装和资源索引。缺一环都会找不到组件。**

### 讲解与例子

编译期先实现接受 NodeOptions 的类，在源文件注册宏，CMake 生成 SHARED 库并登记类名，安装库及索引。运行期管理器根据资源索引找到库，class_loader 创建工厂并实例化节点，再加入执行器。库有符号但索引未安装时，类型列表仍可能找不到它。

### 板书

```text
源码宏注册 + CMake 类登记 + 安装
                    ↓
          ament 资源索引 → 库 → 实例
```

### 命令行实验

完成第 10 页构建后执行，不在本页提前要求编写源码：
```bash
ros2 component types | grep -A 3 demo_component
PREFIX=$(ros2 pkg prefix demo_component)
cat "$PREFIX/share/ament_index/resource_index/rclcpp_components/demo_component"
ls "$PREFIX/lib/libdemo_component.so"
```
预期：看到 Talker/Listener 类、索引和共享库。types 缺失先查构建与 source，不先修改容器。

### 课堂提问

> 为什么源码里有注册宏还可能无法加载？

参考答案：还可能漏了 CMake 登记、安装，或当前终端未 source 正确前缀。

### 教师说明

Humble rclcpp_components 使用 ament 的 rclcpp_components 资源索引，不能机械照抄“必然生成 plugins.xml”。参考 [Composition](https://docs.ros.org/en/humble/Tutorials/Intermediate/Composition.html)。

这页最后可以总结：

> **注册是源码、构建和运行查找共同完成的。**

---

## 第 9 页｜进程内通信：零拷贝原理

这页你可以把它讲成：

> **我们用消息指针地址观察所有权转移，而不是凭感觉说速度变快了。**

### 讲解与例子

发布端构造 unique_ptr，publish(std::move(...)) 交出所有权；单个接收者按 UniquePtr 接收时可以观察相同对象。前提包括同进程、开启进程内通信和合适的拓扑及 QoS。多个拥有者、外部订阅者和缓冲方式会改变复制路径。指针地址相同只能支持本次对象传递现象，不能证明系统所有路径都没有复制。

### 板书

```text
同一对象：PUB ptr → SUB ptr
约束：同进程 + 开启 IPC + 支持的拓扑
```

### 命令行实验

前提：第 10 页已构建。为纯粹观察一对一进程内路径，先停止第 7 页空容器和其他 echo，再由 A 启动完整 pair：
```bash
# A
ros2 launch demo_component pair.launch.py
# C 只查询，不运行 topic echo
ros2 component list /lesson4_container
```
观察 A：同一编号的 PUB/SUB 日志中指针地址预期相同，且来自同一容器进程。不要比较不同编号，因为分配器可能复用地址。停止 launch 后，后续第 11 页重启空容器。这是对象传递演示，不是延迟基准。

### 课堂提问

> 跨进程看到两个相同数值的地址能证明零拷贝吗？

参考答案：不能，不同进程的虚拟地址空间不同。

这页最后可以总结：

> **观察对象地址时必须同时确认运行边界。**

---

## 第 10 页｜节点组件开发规范

这页你可以把它讲成：

> **把能运行的完整包打开看，明确哪个文件负责业务，哪个文件负责注册和安装。**

### 讲解与例子

随讲义的 demo_component 包包括 package.xml、CMakeLists.txt、src/demo_component.cpp 与 launch/pair.launch.py。Talker 创建 Publisher 和 Timer，Listener 创建 Subscription；二者构造函数把 NodeOptions 传给基类。宏导出两种节点类型，CMake 把它们登记到同一库。为突出加载链路，本示例在一个源文件实现两个类，生产工程可以再拆头文件。

### 板书

```text
业务实现 → 宏导出 → SHARED 编译
依赖声明 → 资源索引 → 安装 → source
```

### 命令行实验

保存位置：随讲义 `课堂实验/demo_component/`，复制到 Linux 空间；已有同名包时先检查，不覆盖前课工程。
```bash
# 课前已复制；此处仅在包不存在时复制，重复执行仍成功
if [ ! -d ../ros2_ws/src/demo_component ]; then
  cp -r "第四课/课堂实验/demo_component" ../ros2_ws/src/
fi
(cd ../ros2_ws && colcon list)
rosdep install --from-paths ../ros2_ws/src/demo_component --ignore-../ros2_ws/src --rosdistro humble -y
(cd ../ros2_ws && colcon build --packages-select demo_component --symlink-install --event-handlers console_direct+)
source ../ros2_ws/install/setup.bash
ros2 component types | grep -A 3 demo_component
```
预期：构建成功，类型列表出现两个插件。需先初始化 rosdep；依赖缺失先安装，首个 C++ 错误看编译日志。打开 `src/demo_component.cpp`，对应构造函数、Publisher/Subscription、Timer、两处宏；再对照 CMake 的 SHARED、register_nodes、install 三处配置。完整文件已提供，不需要补省略代码。

### 课堂提问

> add_executable 和 add_library(SHARED) 在这里有什么区别？

参考答案：前者生成独立程序，后者生成供容器加载的动态库；本示例使用后者。

这页最后可以总结：

> **组件工程的每个配置都能对应一个运行检查。**

---

## 第 11 页｜组件的加载与运行管理

这页你可以把它讲成：

> **先加载发布者，再加载订阅者，最后只卸载一个，观察其他对象是否还存在。**

### 讲解与例子

加载命令需要容器名、包名和插件类名三个位置参数。返回的数字 ID 是容器分配的，不要假设永远是 1。节点重命名与命名空间会改变实例的管理路径，但包和插件类名仍用于找到工厂。Launch 负责重复部署时统一配置，CLI 负责演示和排查。

### 板书

```text
load → 返回 ID → list → unload 指定 ID
```

### 命令行实验

A 重新启动第 7 页空容器。B 与 C 都 source 本课 workspace。B 的变量只在 B 有效：
```bash
# B：加载并捕获本次 ID
CID=$(ros2 component load /lesson4_container demo_component demo_component::Talker --quiet | awk 'NR==1{print $1}')
echo "$CID"
ros2 component load /lesson4_container demo_component demo_component::Listener
ros2 component list /lesson4_container
# C
ros2 topic echo /component_chatter --once
# B：确认 CID 是刚才输出的数字后卸载
ros2 component unload /lesson4_container "$CID"
ros2 component list /lesson4_container
```
预期：先收到递增文本；卸载后 Talker 消失、Listener 保留且不再收到新数据。CID 为空时停止卸载，先看 load 的错误与 types。结束 A 容器，避免后面状态实验混入。

### 课堂提问

> 卸载 Talker 后 Listener 必须消失吗？

参考答案：不必，Listener 实例还在，但没有数据源时可能不再打印消息。

### 教师说明

PPT 的 load 命令遗漏包名，正确示例见下方三个参数；Node 名使用本课统一的 /lesson4_container。

这页最后可以总结：

> **装卸实例要用实际 ID，数据是否存在另查。**

---

## 第 12 页｜适用场景与不适用场景

这页你可以把它讲成：

> **把频繁共享数据的功能放近，把需要故障隔离的功能分开。**

### 讲解与例子

高频感知流水线、资源受限设备和动态插件组合是组件化的候选。不同机器上的节点不可能靠同一地址空间进行进程内传递；要求隔离的模块应设置合适进程边界。功能安全还涉及硬件和系统设计，不能只靠分进程达成。

### 板书

```text
高频交互 → 候选同容器
隔离需求 → 候选独立进程
跨机器 → 仍走分布式通信
```

### 课堂提问

> 导航和急停应不应该一律放进同一容器？

参考答案：不能一律决定，要根据故障影响、调度和安全需求设计边界。

这页最后可以总结：

> **部署边界依据需求，而不是依据节点数量。**

---

## 第 13 页｜工程最佳实践

这页你可以把它讲成：

> **复用、单一职责、故障边界和回调耗时要一起考虑，不能只打开一个 IPC 开关。**

### 讲解与例子

逐项解释课件实践：组件保持职责清楚，使用标准接口，按高频交互与隔离需求划进程；选择受支持的进程内配置；保留独立调试入口；关注容器内慢回调。共享线程环境中，单线程长回调会影响其他节点，后课会用两个定时器观察。

### 板书

```text
标准接口 + 进程边界 + 回调耗时 + 可观测日志
```

### 课堂提问

> 改成 component_container_mt 就能保证所有回调并行吗？

参考答案：不能，回调组仍控制同组执行许可。

这页最后可以总结：

> **性能配置必须配合调度与故障设计。**

---

## 第 14 页｜第一部分小结

这页你可以把它讲成：

> **现在让学生独立完成一次装载闭环，再解释每条命令看到的是哪一层。**

### 讲解与例子

复盘库的类型列表、空容器、加载返回 ID、计算图出现节点、收到数据以及卸载后的变化。将同进程复用与故障范围并列，不把课件性能倍数当统一保证。

### 板书

```text
磁盘库 → 类型索引 → 容器实例 → 数据流
```

### 命令行实验

验收任务：学生独立启动 pair，查两个组件与同一话题，然后按实际 ID 卸载 Listener。观察发布端仍持续打印，解释“订阅者停止不强迫发布者停止”。最后 Ctrl+C 停止 pair。

### 课堂提问

> component types 和 component list 查询的对象相同吗？

参考答案：types 查已注册可用类，list 查运行容器中的实例。

这页最后可以总结：

> **注册可用与实例运行是两种状态。**

---

## 第 15 页｜生命周期节点与状态机管理

这页你可以把它讲成：

> **前面问节点放在哪里，现在问节点准备好了没有、业务应该开始还是暂停。**

### 讲解与例子

把相机从“有进程”细分成“未打开设备、设备就绪、采集中、已终结”。生命周期节点提供标准状态服务，但硬件恢复、依赖编排和业务门控仍要开发者实现。

### 板书

```text
进程存在 ≠ 资源就绪 ≠ 业务激活
```

### 课堂提问

> 节点在 node list 里出现，就可以认为设备可用吗？

参考答案：不能，需查状态和资源检查结果。

这页最后可以总结：

> **可管理状态让运行判断更具体。**

---

## 第 16 页｜普通节点的固有痛点

这页你可以把它讲成：

> **普通节点也能做启停，但每个团队各写一套接口，运维就很难统一判断。**

### 讲解与例子

围绕课件四个痛点解释：依赖启动是否就绪、初始化失败是否能回滚、暂停是否必须退出、外部如何观察状态。话题订阅者可以先于发布者启动，不会仅因话题不存在必然崩溃；真正的问题是业务是否把数据就绪当成前置条件。

### 板书

```text
初始化混合 → 回滚难
启停私有 → 编排难
状态不统一 → 诊断难
```

### 命令行实验

A 启动官方生命周期节点，C 先观察“进程存在但业务未运行”：
```bash
# A
ros2 run lifecycle lifecycle_talker
# C
ros2 node list
ros2 lifecycle get /lc_talker
ros2 lifecycle list /lc_talker
```
预期初始 unconfigured，允许 configure/shutdown。本课使用官方包，缺失先安装 ros-humble-lifecycle。保留 A 直到第 21 页。

### 课堂提问

> 先启动订阅者一定报错吗？

参考答案：不一定，订阅者可以等待发布端，业务代码需要处理未收到数据的阶段。

这页最后可以总结：

> **标准状态解决的是可控性和一致性。**

---

## 第 17 页｜生命周期节点的核心价值

这页你可以把它讲成：

> **我们把初始化和运行拆成两步，就能观察“配置完成，但还没有业务输出”。**

### 讲解与例子

configure 申请配置资源，activate 启动输出，deactivate 暂停业务，cleanup 释放配置资源；外部查询可知道处于哪一步。框架提供转换结构，资源对称、失败回滚和停止输出仍依赖回调实现。

### 板书

```text
configure：备好资源
activate：业务开始
deactivate：业务暂停
cleanup：资源释放
```

### 命令行实验

B 配置，C 查询后只等待一条消息：
```bash
# B
ros2 lifecycle set /lc_talker configure
ros2 lifecycle get /lc_talker
# C
ros2 topic echo /lifecycle_chatter --once
```
预期 Inactive；即使定时器回调打印尝试发布，LifecyclePublisher 不输出业务消息。C 等待几秒后 Ctrl+C，保留 A。不能把 echo 等待误认为 CLI 崩溃。

### 课堂提问

> 生命周期会自动替我们关闭所有设备吗？

参考答案：不会，开发者需要在相应回调中正确关闭资源。

这页最后可以总结：

> **框架标准化步骤，业务代码落实步骤。**

---

## 第 18 页｜状态机全景图

这页你可以把它讲成：

> **沿状态图走一圈，先记稳态，再记允许的转换；不能想从哪里跳到哪里。**

### 讲解与例子

四个主要稳态是 Unconfigured、Inactive、Active、Finalized，另有 configuring、activating 等转换态和错误处理阶段。configure 成功到 Inactive，activate 成功到 Active；Active 先 deactivate，再 cleanup。Finalized 是生命周期终态，不等于 C++ 对象或 OS 进程已经自动销毁。

### 板书

```text
Unconfigured --configure--> Inactive --activate--> Active
Unconfigured <--cleanup--- Inactive <--deactivate-- Active
各主要运行态 --shutdown--> Finalized
```

### 命令行实验

B 从 Inactive 激活，C 观察收到消息；B 去激活后 C 应不再接收新的业务数据：
```bash
# B
ros2 lifecycle set /lc_talker activate
ros2 lifecycle get /lc_talker
# C：持续观察
ros2 topic echo /lifecycle_chatter
# B
ros2 lifecycle set /lc_talker deactivate
ros2 lifecycle get /lc_talker
```
预期 Active 时有消息，Inactive 时暂停输出。A 进程仍在。结束 C 的 echo。

### 课堂提问

> Active 时直接 cleanup 是正常路径吗？

参考答案：不是，先 deactivate 到 Inactive，再 cleanup。

这页最后可以总结：

> **依据当前状态选择合法转换。**

---

## 第 19 页｜完整生命周期流转

这页你可以把它讲成：

> **先做正常流程，再故意提交非法转换，观察状态没有按我们的想象跳转。**

### 讲解与例子

正常路径的每一步都查询 get_state，不只看 change_state 服务请求发出。转换回调返回 SUCCESS、FAILURE 或 ERROR 有不同语义；ERROR 进入错误处理，而普通订阅或定时器的任意异常并不会自动变成全局生命周期错误钩子。错误后的业务恢复也不等于框架自动重试。

### 板书

```text
请求转换 → 执行回调 → 检查返回 → 查询新状态
```

### 命令行实验

当前 Inactive，B 清理后故意直接 activate：
```bash
ros2 lifecycle set /lc_talker cleanup
ros2 lifecycle get /lc_talker
ros2 lifecycle set /lc_talker activate
ros2 lifecycle get /lc_talker
ros2 service call /lc_talker/get_state lifecycle_msgs/srv/GetState "{}"
```
预期回到 Unconfigured；非法 activate 不进入 Active。恢复用 configure，再 activate。服务响应包含 id/label，可与 CLI 对照。

### 课堂提问

> 收到服务响应，就能认定转换成功吗？

参考答案：要看 success 和最终状态；收到响应只证明交互完成。

### 教师说明

FAILURE 与 ERROR 区分讲解；不能把运行时任何异常都说成自动触发 on_error。参考 [Managed nodes](https://design.ros2.org/articles/node_lifecycle.html)。

这页最后可以总结：

> **管理命令要用状态查询闭环验收。**

---

## 第 20 页｜回调编写规范（上）

这页你可以把它讲成：

> **配置负责准备，激活负责启动，去激活负责暂停；资源的生命周期要与业务输出分别管理。**

### 讲解与例子

打开官方 lifecycle_talker 思考：配置中创建定时器也可以，但非激活阶段 LifecyclePublisher 不输出消息。工程上可 cancel/reset 定时器或额外控制业务门控；普通订阅不会自动停止。activate 通常应轻量，deactivate 应等待或隔离正在运行的业务，防止未完成回调继续输出。

### 板书

```text
configure 创建资源
activate 启用业务
deactivate 停止业务，保留配置
```

### 命令行实验

执行前 B 已 configure/activate。C 先启动 echo，B 再 deactivate，观察停止；B activate，观察恢复。按第 18 页命令复现。问学生“本次变化的是配置资源还是发布许可”。官方示例源码见资料链接，定时器在 configure 创建，证明创建时机与业务门控需要分别解释。

### 课堂提问

> 定时器一创建就必然违背生命周期语义吗？

参考答案：不是，关键是非激活状态不得启动不应运行的业务；可创建后取消或用状态门控。

这页最后可以总结：

> **控制的是业务行为，不能只按对象创建位置判断。**

---

## 第 21 页｜回调编写规范（下）

这页你可以把它讲成：

> **清理释放配置资源，终结处理最终退出需求，错误处理先止损再决定是否可重新配置。**

### 讲解与例子

cleanup 正常由 Inactive 转入 Unconfigured，需要与配置资源对应；shutdown 可从主要稳态请求，要兼容尚未完整配置的资源；on_error 处理转换错误，清理残留并返回结果。幂等清理是代码设计要求，不表示状态机允许任意状态重复 cleanup。Finalized 以后进程仍可能留着用于检查，需要单独退出。

### 板书

```text
资源申请/释放成对
转换 ERROR → on_error
终态 ≠ 进程自动消失
```

### 命令行实验

从 Active 依次去激活、清理、终结：
```bash
ros2 lifecycle set /lc_talker deactivate
ros2 lifecycle set /lc_talker cleanup
ros2 lifecycle get /lc_talker
ros2 lifecycle set /lc_talker shutdown
ros2 lifecycle get /lc_talker
ros2 node list
```
预期 Unconfigured 后到 Finalized；进程是否退出取决于程序，官方示例可保持可查询。最后在 A Ctrl+C。终态无法再 configure，后续练习需重启 A。

### 课堂提问

> on_cleanup 正常由 Active 直接调用吗？

参考答案：标准正常路径由 Inactive 触发，Active 先去激活。

这页最后可以总结：

> **资源幂等和转换合法性是两种约束。**

---

## 第 22 页｜适用场景与不适用场景

这页你可以把它讲成：

> **设备准备复杂、需要统一编排时收益明显；简单打印工具不必先搭复杂状态机。**

### 讲解与例子

沿课件场景讲驱动、模型加载、系统启停和多节点依赖。纯计算模块也可能需要标准管理，不能说绝对不适用；安全认证和故障自愈更不能由生命周期继承关系自动获得。把相机设备打开失败的清理与重试作为具体判断例子。

### 板书

```text
初始化复杂度 + 运维需求 → 是否采用生命周期
```

### 课堂提问

> 使用 LifecycleNode 就能保证高可用吗？

参考答案：不能，还需要监控、重试、资源与故障策略。

这页最后可以总结：

> **状态机增加可管理性，可靠性需要完整实现。**

---

## 第 23 页｜工程最佳实践

这页你可以把它讲成：

> **每次转换都记录日志、检查资源，并用查询命令确认结果。反复启停能够发现一次启动看不到的泄漏和重复定时器。**

### 讲解与例子

课件七点归纳为状态语义、轻量转换、对称释放、错误处理、日志、LifecyclePublisher 和部署组合。反复 activate/deactivate 看消息是否启停，反复 configure/cleanup 看是否重复创建对象。用有限次循环做课堂对照，不承诺它能证明没有内存泄漏。

### 板书

```text
日志 + 状态 + 数据输出
三者一起验收转换
```

### 命令行实验

重新启动 A 的 lifecycle_talker。B configure 后有限次重复启停：
```bash
ros2 lifecycle set /lc_talker configure
for round in 1 2 3; do
  echo "round=$round"
  ros2 lifecycle set /lc_talker activate
  ros2 lifecycle get /lc_talker
  sleep 2
  ros2 lifecycle set /lc_talker deactivate
  ros2 lifecycle get /lc_talker
done
```
C 在独立终端运行 echo 验证启停。预期每轮状态与数据一致；单次循环未报错不能证明无资源泄漏。最后 cleanup/shutdown 并停止 A。

### 课堂提问

> 只查 Active 不查消息能证明运行正常吗？

参考答案：不能，仍要查输出与业务条件。

这页最后可以总结：

> **管理状态与数据行为需要互相印证。**

---

## 第 24 页｜组件化 + 生命周期组合

这页你可以把它讲成：

> **一个生命周期节点也能被做成组件；组件 ID 和生命周期状态分别属于不同管理接口。**

### 讲解与例子

沿图说明容器放置感知、定位和导航实例，生命周期管理器按依赖调用配置与激活。不要把管理器看成 ROS 2 自带的全局 Master，也不要假设加载会自动解析所有业务依赖。先查询单节点状态，再讨论编排器怎样等待上游就绪与处理失败。

### 板书

```text
容器管理：实例装卸
生命周期管理：配置启停
编排器：依赖顺序与失败策略
```

### 命令行实验

检查接口如何区分：对组件运行 `ros2 component list /lesson4_container`（需启动 pair），对 lifecycle_talker 运行 `ros2 lifecycle get /lc_talker`（需独立启动）。本课这两个示例分别验证两套接口，不伪称普通 Talker 已成为生命周期组件。组合实现需要把 LifecycleNode 类注册为组件。

### 课堂提问

> 卸载组件和 deactivate 是否等价？

参考答案：不等价，前者销毁实例，后者通常保留配置资源并暂停业务。

这页最后可以总结：

> **组合架构仍要区分两套控制接口。**

---

## 第 25 页｜三类节点选型对比

这页你可以把它讲成：

> **普通、组件和生命周期不是三个互斥盒子，而是可组合的能力。**

### 讲解与例子

普通 Node 是业务基础；组件描述装载形式；LifecycleNode 增加管理状态。可以有独立生命周期进程，也可以有生命周期组件。对照性能、灵活性和可靠性时避免绝对评级：状态回调不自动消除故障，IPC 不自动消除复制，独立进程也有依赖风险。

### 板书

```text
基础节点 × 部署方式 × 状态管理
按负载与运维需求组合
```

### 课堂提问

> 硬件驱动必须既组件化又生命周期吗？

参考答案：未必，根据性能、隔离和管理需求选择。

这页最后可以总结：

> **按设计维度组合，不按宣传标签排序。**

---

## 第 26 页｜课程总结

这页你可以把它讲成：

> **现在请把“看得到类型、装得出实例、读得到数据、控得住状态”四件事串起来。**

### 讲解与例子

组件部分复盘类型索引、动态库、实例 ID、进程内消息；生命周期部分复盘主要稳态、转换回调、输出启停与资源释放。回答开场相机问题：哪些模块同进程由负载决定，何时采集由业务状态与管理逻辑决定。

### 板书

```text
types → load/list → topic
lifecycle get/list → set → 输出检查
```

### 课堂提问

> 今天最容易混淆的两种“停止”是什么？

参考答案：卸载实例和去激活业务，应分别查组件列表与状态。

这页最后可以总结：

> **能查询、能解释，才能稳定管理。**

---

## 第 27 页｜课后思考与实践

这页你可以把它讲成：

> **练习不仅要提交命令截图，还要说明改变的条件和结果之间的关系。**

### 讲解与例子

保留课件四个任务：比较进程布局；设计相机状态机；分析组合架构；制定选型规则。用本课实验完成最小可验证部分，不要求学生凭空搭整个 Nav2 系统。每个结论注明测量对象或设计假设。

### 板书

```text
任务 → 操作 → 现象 → 解释 → 恢复
```

### 命令行实验

任务一：完成第 11 页装卸，记录实际 ID 与剩余节点。任务二：重做第 19 页非法转换，记录状态未变化。任务三：为相机列出 configure/activate/deactivate/cleanup 的资源操作；模拟打开失败时说明回滚对象。验收：命令、状态、数据三份证据一致，并能说明为何 unload 不等于 deactivate。

### 课堂提问

> 为什么同一容器里的两个节点不能靠节点名区分 OS 进程？

参考答案：节点名属于 ROS 对象，容器中多个节点共享同一进程。

这页最后可以总结：

> **实践用可观察证据支撑架构解释。**

---

## 第 28 页｜感谢聆听 / Q & A

这页你可以把它讲成：

> **用一个问题收尾：进程活着但没数据，下一条命令应该查什么？**

### 讲解与例子

让学生先判断是组件加载问题、生命周期未激活、话题匹配问题，还是回调调度问题。要求每组报出一条检查命令和一个预期结果，讨论时保持名称与本课示例一致。

### 板书

```text
实例？状态？匹配？回调？
```

### 课堂提问

> lc_talker 已 Finalized，却仍能在 node list 看到，算矛盾吗？

参考答案：不矛盾，生命周期终态不等于进程自动退出。

这页最后可以总结：

> **区分对象、状态与进程是排查的基础。**

---

## 教师核对资料

使用 rosdep 前，先初始化并更新规则；已初始化时跳过 init。


- [Humble Composition](https://docs.ros.org/en/humble/Tutorials/Intermediate/Composition.html)：容器、加载、卸载、参数与 IPC。
- [Humble load CLI 源码](https://github.com/ros2/ros2cli/blob/humble/ros2component/ros2component/verb/load.py)：三个位置参数与 quiet 输出。
- [进程内通信示例](https://docs.ros.org/en/humble/Tutorials/Demos/Intra-Process-Communication.html)：UniquePtr 所有权与拓扑条件。
- [Managed nodes 设计](https://design.ros2.org/articles/node_lifecycle.html)：稳态、转换态、错误处理。
- [Humble lifecycle_talker 完整源码](https://github.com/ros2/demos/blob/humble/lifecycle/src/lifecycle_talker.cpp)：配置、定时器、激活与清理。
- 随附 [组件完整包](课堂实验/demo_component/)：构建配置、双节点实现和 pair.launch.py。

验证记录：已读取对应 PPT 的全部页面文字并核对页序；完整 Python 源码和 launch 做语法检查，XML 模型和包描述做解析检查，模型参数与树结构做静态检查。关键命令与 API 对照 Humble 官方资料及源码。当前 Windows 环境不能访问 WSL 运行会话，因此未完成 ROS 节点运行、C++ ROS 构建、TF/QoS 网络实测及 Gazebo/RViz 显示预演。文中输出均为预期现象，需教师在目标环境预演；不把静态通过写成实际实验通过。
