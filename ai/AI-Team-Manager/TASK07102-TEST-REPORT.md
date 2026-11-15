# Task 07.10.2 阶段报告

本阶段完成统一 UI 外框、导航、顶部会话标签和前端路由。沿用原生 JavaScript、CSS、VS Code Webview 及原消息桥，没有接入新的 Team 执行业务。本阶段结束于 07.10.2。

## 实际修改文件

| 文件 | 变更 |
| --- | --- |
| src/webview/workspace/workspace.js | 统一导航及顶部标签；通用路由出口；切换、隐藏、重新打开；保留原聊天和业务组件 |
| src/webview/workspace/workspace.css | 深色蓝紫外框、选中导航、独立标签栏、通用页面容器及响应式布局 |
| src/webview/workspace/workspaceRouter.js（新增） | 独立页面注册表、路由解析及导航状态校验；不持有消息桥或业务服务 |
| src/webview/workspace/layoutManager.js | 将导航状态纳入原布局持久化；新工作区默认 GPT 单窗口，保留已保存布局 |
| src/workspace/workspaceLayout.ts | 校验并迁移旧版布局，持久化路由和隐藏标签 ID |
| src/providers/workspacePanel.ts | 仅增加本地路由脚本；保留 CSP、nonce、原消息桥及面板生命周期 |
| test/workspace.test.cjs | 新增四组导航、标签、恢复和业务消息隔离测试 |
| test/workspace-browser.cjs | 加载全部生产组件；路由、标签、尺寸拖动验证及截图 |
| test/task02-host.cjs | 增加关闭、重开及重启后的路由和隐藏标签恢复断言 |
| TASK07102-TEST-REPORT.md（新增） | 本报告 |

## 路由和扩展方式

| 路由 | 入口及当前实现 |
| --- | --- |
| collaboration | AI 协同开发；默认 GPT 总指挥；复用原聊天和业务面板 |
| collaboration/worker/<稳定 ID> | 根据已有面板 ID 选择会话；当前沿用 claude-01，其他保留槽不具有真实 Worker 能力 |
| task-center | 任务调度中心；已注册、可切换至通用路由出口 |
| review-integration | 代码审查与集成；已注册、可切换至通用路由出口 |
| projects | 项目管理；已注册、可切换至通用路由出口 |
| settings | 系统设置；已注册、可切换至通用路由出口 |

四个未来入口目前共享通用出口和标题，不提前实现四套独立占位组件或业务。后续可在 WorkspaceRouter 的独立页面注册项中扩展组件及渲染，保留现有路由 ID；独立占位组件留到 07.10.6。非法路由和未知 Worker 路由回退至 collaboration。

导航状态使用原 workspace.layout.v1 中的 navigation.route 和 navigation.hiddenTabs。沿用 activePanel 及 panels 的稳定 ID，不按标签顺序关联会话。旧布局无 navigation 时按原选中面板迁移。原窗口尺寸、网格布局、最大化、折叠机制继续使用。

顶部标签可切换、隐藏及通过“+ 会话”重新打开。隐藏标签不删除面板、历史或 Session；至少保留一个可见标签。未读及异常各预留隐藏标识，不填充虚构计数。标签过多时横向滚动，并支持方向键、Home、End。原“+ 窗口”仍仅增加明确标注的预留窗口，不创建 Worker。

左侧“已有功能”、顶部更多菜单和原业务面板保留 GPT/Claude 会话入口、会话历史、项目选择、审查、权限、修复、调度、日志和现有扩展设置。

## 实际验证

| 验证 | 实际结果及证据边界 |
| --- | --- |
| npm.cmd run typecheck | 通过 |
| npm.cmd test | 94/94 通过，包括原有 90 项和新增 4 组测试；覆盖会话、调度、权限、锁、Worktree、编辑保护、审查、修复、集成等自动测试；不等同于全量真实模型验收 |
| npm.cmd run test:host | 编译通过；专用 VS Code Extension Host 两次启动通过；真实 Webview 脚本/CSP 加载、面板复用、关闭重开和重启恢复验证通过。GPT 使用测试 Mock CLI，未声称真实模型已验证 |
| 浏览器 UI 验证 | 1680、1280、1024、760、640、390、320 宽度无页面横向溢出；五种原布局、全部分隔线拖动、最大化/折叠、小屏切换/任务面板通过；页面异常 0，远程资源请求 0 |
| 旧聊天及业务面板 | GPT/Claude 通道隔离、发送及停止的正确目标、输入草稿、Enter/Shift+Enter、安全 Markdown、日志筛选及仅清空展示、审查/权限/修复/调度控件自动回归通过 |
| Worker 生命周期保护 | 在 DOM 测试和浏览器中执行导航、标签选择、隐藏及重开，记录的桥消息全部为 layout；无 send/check/stop/new 等执行业务消息。隐藏期间后台快照仍更新原 DOM，重开保留原输入及 Session 标识 |
| 动态标签 | 使用明确的测试保留槽验证稳定 ID 路由、顺序无关和键盘切换；未将这些槽描述为真实运行的 Worker |
| 真实多 Worker + 真实模型后台执行 | 本阶段未启动；未验证。真实 Team 事件与多 Worker 工作台接入留待后续阶段 |

执行策略阻止 npm.ps1 后使用 npm.cmd 执行，未更改系统执行策略，未安装依赖。

## 截图和测试产物

截图为 Chromium 真实运行生产 Webview 资源的结果，采用本次 Extension Host 导出的真实空会话快照。截图未填入 Mock 对话、模拟进度或虚构 Worker；它们是浏览器中的 Webview 渲染截图，不是 VS Code 整窗截图。

- .tools/task07102-ui/workspace-dark.png：统一外框，1680×1000。
- .tools/task07102-ui/navigation.png：五个正式导航及已有功能入口。
- .tools/task07102-ui/session-tabs.png：独立顶部会话标签。
- .tools/task07102-ui/page-route.png：通用路由出口。
- .tools/task07102-ui/workspace-light.png、workspace-mobile.png：浅色及小屏回归。
- .tools/task07102-ui/browser-results.json：浏览器断言结果。
- .tools/task02-host-1794593236953/：本次 Host 测试记录和空快照。

运行快照、专用测试配置和截图均在现有忽略目录 .tools 中，不提交运行数据。

## 文件和 Git 保护

起点为 master / origin/master 的 7936cda。起始暂存区为空；原有 test/api.ts 修改、未跟踪的 src/team/teamSession.ts 和 src/team/teamTools.ts 全部保留。

提交前对这三个文件及 DispatchCoordinator、TeamWorkflow、IntegrationManager、ClaudeWorkerManager、FileLockManager、Permission Gateway 比对开工 SHA-256，结果 9/9 一致。仅显式暂存本报告列出的文件，检查暂存差异和统计，再提交并推送 origin/master。不会把前期三个文件纳入本次提交，也不更改 .gitignore 或业务模块。

现有 ai-team-manager-0.6.0.vsix、ai-team-manager-0.7.0.vsix 继续由 Git 跟踪；本阶段未重新打包发布 VSIX。验证使用当前源代码编译的 Development Host。

阶段提交哈希与最终 Push 结果在交付消息中报告，以避免在提交内写入自引用哈希。完成后等待授权，不进入 Task 07.10.3。
