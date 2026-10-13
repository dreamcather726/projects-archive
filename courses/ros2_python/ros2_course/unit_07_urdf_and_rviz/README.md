# 第 7 模块：ROS 2 中的 URDF、Xacro 与 RViz

课程材料：[知识点详解](第7章_授课讲义.md) · [实验说明与代码](experiments/01_xacro_rviz/实验说明.md)

## 内容范围

- 使用 URDF/Xacro 描述机器人几何结构与关节关系。
- 模型坐标树和关节状态到 TF 的转换关系。
- RViz2 中的模型、TF、传感器显示与错误排查。

## 详细知识点

### 1. URDF 模型结构

- `robot`、`link`、`joint` 的层级关系。
- 连杆表达刚体，关节表达两个刚体之间的约束。
- 根连杆、父连杆、子连杆和树形结构限制。
- `visual`、`collision`、`inertial` 的用途不同，不能只写外观。
- `origin xyz/rpy` 表达局部坐标偏移；旋转单位为弧度。
- box、cylinder、sphere 和 mesh 几何体。
- 材质颜色与网格资源 `package://` 路径。

### 2. 关节类型

- `fixed`：固定安装的传感器和部件。
- `continuous`：可无限旋转的车轮。
- `revolute`：带上下限的旋转关节。
- `prismatic`：沿轴线平移。
- `axis`、`limit`、`dynamics` 的意义。
- 关节 origin 是子连杆坐标系相对父连杆的安装位姿。

### 3. 惯性、碰撞与可视化

- RViz 只显示 visual，不代表模型能够正确物理仿真。
- Gazebo 使用 collision 和 inertial 参与碰撞与动力学计算。
- 质量、质心和惯性矩阵必须合理，不能全为零。
- 外观网格可以很复杂，碰撞模型应尽量简化。
- 左右车轮坐标、圆柱轴向和旋转方向的对应关系。

### 4. Xacro 参数化

- 用 property 管理尺寸、质量和颜色。
- 用 macro 复用左右车轮、传感器安装结构。
- 用参数和前缀支持多机器人模型。
- 数学表达式和条件结构。
- 使用 `xacro` 命令展开并检查最终 URDF。
- 避免重复数字造成模型各处尺寸不一致。

### 5. robot_state_publisher

- 读取 `robot_description` 参数中的 URDF。
- 固定关节可以直接形成 TF。
- 可动关节需要 `/joint_states` 提供关节位置。
- `joint_state_publisher` 与 GUI 版本的教学用途。
- 模型 TF 与第 4 章手工 TF 发布之间的关系和避免重复发布。

### 6. RViz2

- Fixed Frame 决定所有显示数据转换到哪个坐标系。
- RobotModel、TF、LaserScan、Odometry、Path 等 Display。
- Status 中常见的 No transform、模型资源失败和时间错误。
- 保存 `.rviz` 配置，让课堂环境可重复。
- RViz 是可视化工具，不是物理仿真器。

## 课堂演示

1. 从一个 `base_link` 方盒开始，逐步添加左右车轮和激光雷达。
2. 故意删掉一个父关节，观察 URDF 树断裂报错。
3. 把重复车轮代码改为 Xacro 宏。
4. 启动 `robot_state_publisher` 与关节状态 GUI。
5. 在 RViz 中修改 Fixed Frame，观察错误与恢复过程。

## 实验任务

[使用 Xacro 构建差速小车并在 RViz 显示](experiments/01_xacro_rviz/实验说明.md)

## 常见问题

- 圆柱车轮旋转轴方向错误，车轮横躺或转动异常。
- mesh 路径能在源码中找到，但资源未安装导致运行时找不到。
- 可动关节没有 `/joint_states`，模型部分连杆无法更新。
- 同一个 TF 同时由 URDF 和手写节点发布，出现冲突。
- RViz Fixed Frame 选择了不存在或不连通的坐标系。

## 验收标准

- Xacro 能无错误展开为 URDF。
- 模型树只有一个根，所有连杆通过关节连通。
- RViz 正确显示底盘、车轮和激光雷达，TF 无断裂。
- 学生能解释 visual、collision 和 inertial 各自的作用。
