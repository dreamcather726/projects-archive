# AI Team Manager · 多智能体开发工作台

VS Code 桌面扩展，使用 TypeScript、Node.js 和 Webview，将 Codex GPT 规划、Claude Code 隔离执行、权限审批、测试与独立代码审查组织到同一工作台。

- **源码仓库**：[dreamcather726/AI-Team-Manager](https://github.com/dreamcather726/AI-Team-Manager)
- **归档版本**：0.8.3，提交 `3c4fd05565473ce7761ee276491ea7b09ae6847b`，分支 `fix/task0711-r1-safe-retry`。
- **收录日期**：2026-11-15。快照不自动同步。保留 MIT 许可证与第三方声明。
- **功能重点**：自适应任务拆分和依赖调度、隔离 Worker 与文件所有权、基于证据的测试/审查门禁、会话恢复、真实用量统计和任务进度。
- **运行环境**：Node.js 22+、VS Code 1.95+；真实模型功能需要本机已有且已配置的 Codex / Claude Code CLI。
- **快照范围**：已提交源码、必要配置、回归测试和历史报告；排除 VSIX 安装包、依赖、构建输出及运行结果 JSON。本地未提交的 0.8.4 源码迁移未收录。报告中的历史本地证据路径可能不可用。

`npm ci` 后运行 `npm run typecheck`、`npm test` 和 `npm run compile`；按 F5 启动扩展开发宿主。真实模型验收脚本需手动执行并消耗现有账户用量。临时测试工程清理见 [清理说明](docs/TEST-CLEANUP.md)。

以下保留原项目说明及阶段验证记录。

---

# AI Team Manager - Multi Agent

## 阶段交付约定

每个获得授权的开发阶段完成后先执行对应验证。Task 07.11 统一提交本次源码、测试和报告；验证安装包及运行证据放在忽略的 `test-results/`，不纳入源码提交。此前发布包保留。只接入用户明确授权的两个历史文件，其他历史修改及运行数据不提交。

当前版本：0.8.3，增加[真实任务进度可视化](TASK0711-PROGRESS-REPORT.md)，保留 Task 07.11-R1-01 自动重试与简约工作台，以及[审查证据修复](TASK0711-REVIEW-EVIDENCE-REPORT.md)。[R1 报告](TASK0711-R1-01-REPORT.md)独立保留；Task 07.11 的 LARGE 真实模型验收仍待用户确认网关恢复后续行。两个授权历史文件的原始内容和 SHA-256 见 [历史成果接入基线](TASK0710-HISTORY-TEST-REPORT.md)。

VS Code 桌面插件。GPT 沿用 Codex CLI 多轮会话，保持只读，通过真实 MCP 工具启动隔离 Claude 团队。插件复用既有规划、权限、文件锁、测试、独立审查和集成业务层。旧单 Worker 通道保留兼容。

### Task 07.11 自适应调度（0.8.0）

0.8.3 的 GPT、Worker 和任务中心使用同一后端进度投影。进度条表示“已验证阶段”，不是开发时间或工作量估算：每项任务依次验证候选采集、当前快照测试、独立审查，整体还须验证集成和用户交付。Worker 显示自身五个验证节点，整体按各任务前三个节点加两个公共节点计数；Worker 复用后按任务 ID 隔离。阶段未知或计划数据不足时显示不确定进度。CLI 结束、测试通过、审查通过和实际交付分别处理，只有所有证据绑定且交付已记录才显示 100%。候选版本变化可能使旧测试/审查节点失效，进度允许回退。

步骤时间线默认折叠，显示记录中的开始/结束时间和阻塞原因，缺少时间标为“未记录”。测试进行状态来自认证测试工具事件，测试结果来自当前快照记录；暂停、取消、失败、重试仅改变展示状态，不按等待时间增加完成量。进度条用轻量 CSS 过渡，支持减少动画设置；页面切换与关闭只改变展示，不启动或停止业务。验证：`npm run test:progress`、`npm run test:progress-ui`。界面测试使用生产后端投影和 Webview 资源，测试场景明确标记为夹具，不调用模型。

R1 增加 Claude 临时错误的保守自动重试，默认最多 3 次（含首次共 4 次），基础等待 2/4/8 秒并加正向抖动。遵守有效 Retry-After；429 缺失等待信息或等待超过上限时转人工。只有 CLI 已退出、没有助手执行文本/工具/权限活动、无已知 Token 消耗且候选文件指纹未变，才允许沿原 Session、任务、租约及 MCP 重试。认证、权限、CLI 不兼容、Session 异常不盲目重试；等待期间暂停/停止会取消下一次请求，重启不重放。配置为 `claudeRetryMaxRetries` 和 `claudeRetryBaseDelayMs`。重试不重复创建用户消息或 RequestRecord，未知用量仍按未知记录。

三个核心页面共用深灰蓝外框与低饱和强调色。GPT 对话为主体，任务详情可收起；Worker 列表一次只展示一个独立工作区，五个标签切换，详情和诊断默认折叠；任务中心提供列表、状态/搜索筛选、详情抽屉及按需展开的依赖与事件。重试倒计时只更新显示，取消走真实后端接口；关闭标签不停止任务。五个原有导航和布局持久化保留，项目/会话切换会隔离 Worker 数据。

R1 验证：`npm run test:retry`、`npm run test:ui:r1`（使用本机 Edge，或 `ATM_BROWSER_CHANNEL` 指定已安装的 Playwright 浏览器通道）。测试使用隔离 CLI 故障夹具和生产 Webview 资源，不等同真实上游服务可用性验证。本轮不调用真实模型或修改原 LARGE 检查点。

0.8.2 修复小任务被仓库无关测试撑满审查上下文的问题：优先包含实际变更、直接依赖、相关测试与规则，必要上下文超限仍阻塞，可选背景省略会明确提示。审查包提供完整已采集文件范围的前后哈希对比和认证写入证明，严格保留越界修改、失败测试和证据缺口门禁。缺证据或审查服务异常显示“等待补证复审”，原因可在任务详情查看；点击“重新审查”先校验文件版本和权限，再复审保留候选，不重新开发。原检查点的历史结论不会被改写；只有新的独立审查通过才继续。验证：`npm run test:review-evidence`。

GPT 先输出四档任务规模、技术复杂度、文件/接口依赖、风险及 Worker 分配理由；并发受 `aiTeamManager.teamMaxConcurrent`（默认 3，范围 1–8）控制。简单任务单 Worker；独立模块并行，上游审查通过后才启动下游，空闲 Worker 保留原 Session 复用。实际写入仍强制检查所有权、路径和文件锁；冲突任务可取消地等待，不能抢占存活写进程。

`aiTeamManager.teamMaxRepairRounds` 控制新计划自动返工总轮数（默认 2，范围 0–5）；普通测试反馈重试与 GPT 定向返工共享此上限。每个已核实问题形成修复记录，范围外问题转人工，源码无进展停止。独立审查基于真实差异和当前快照测试；已审查依赖通过哈希、接口与测试证明复用。多批次仍要求人工整体复核，不自动放行。

GPT 右侧显示规模、分配依据、依赖关系、关键监督事件、修复轮数和真实用量；Worker 显示来源、依赖与审查反馈。普通流式输出只刷新 UI，决策等待可合并 60 秒的普通更新。Token 预算是请求前门禁，单轮的未来用量无法预知，返回后可能超额；80% 提示接近预算，达到预算或新增未知用量后阻止新请求。

中断保存实际文件、完成写入、测试与 Session；恢复显式校验工作树、文件版本、依赖哈希、权限及存活写锁。相同内容写入直接记录跳过，完整候选在 Worker 复用前归档。不会自动恢复中断任务、重放旧人工审批或把 READY 当已交付。

验证：`npm run test:task0711`；真实 A–D：`node test/task0711-live.cjs`；真实 E/F：`node test/task0711-safety-live.cjs`；打包后原生宿主：`node test/run-task0711-host.cjs`。真实测试消耗现有账户用量，仅修改隔离夹具；完整报告见 [Task 07.11 验收](TASK0711-TEST-REPORT.md)。

### 环境检测（0.7.4）

环境检测与聊天入口共用 CLI 定位规则：显式设置优先，其次 PATH，再查询已安装的 Codex/Claude 扩展。日志显示真实版本、来源和路径，仅执行 --version，不要求手动把扩展目录加入系统 PATH。验收见 [0.7.4 修复报告](CLI-DIAGNOSTICS-TEST-REPORT.md)。

### 中断后的用量确认（0.7.3）

若 GPT 提示“等待用量确认”，说明历史中断请求未返回 usage，或累计已知用量达到预算。点击 GPT 面板“用量与预算”，输入高于已知用量的预算，并在存在未知用量时明确确认后续请求。取消不更改账本；未知用量继续保留，不能按零推算。确认后只重新检测 CLI，不自动重发历史消息，请主动发送下一条。

修复验收见 [0.7.3 恢复报告](CODEX-RECOVERY-TEST-REPORT.md)。回归入口：`npm run test:recovery`；打包后真实宿主验收：`npm run test:recovery-host`（使用现有账户用量，在独立 VS Code 配置中运行）。

### Task 07.10 工作台

GPT 默认页面显示对话及六类真实任务详情。执行有效计划后，每个实际 Worker 自动获得独立标签，左侧列表可重新打开隐藏的会话；切换、查看历史及日志不调用模型，不中断后台执行。

Worker 工作台包含对话、文件变更、执行日志、测试结果和任务信息。日志每页最多 100 条，支持搜索、复制、清空显示及暂停滚动。Diff 只能来自该 Worker 已采集的变更证据。暂停会停止当前 CLI 并保留 Session 与候选；暂停后可保存补充指令，再通过原调度器恢复，权限范围不变。

GPT 和 Claude 均支持格式化、原始和调试模式。完整 JSON 经本地解析后按字段展示，保留全部字段及原文；不完整 JSON 回退展示。Markdown 使用文本节点渲染，工具权限批准与实际执行结果分开展示。原始模式沿用既有脱敏规则，不额外采集工具敏感参数。

任务调度中心展示真实队列、Worker 分配与 DAG；代码审查与集成、项目管理和系统设置保留既有独立路由。现有设置、历史、审查和修复操作仍在“已有功能”及“业务操作与历史”中。

`READY` 只表示集成候选通过验证。先查看当前实际补丁，再明确授权安全交付；交付时重新检查用户编辑、基线及候选。隔离团队不自动提交或推送用户项目。

新增验证入口：`npm run test:task0710`；真实模型验收：`npm run test:task0710-live`。后者会使用现有账户用量，在独立 `test-results/` 工程中运行。

## 安装与使用

在 VS Code 扩展面板菜单中选择“从 VSIX 安装”，打开本地验证包 `test-results/ai-team-manager-0.8.3.vsix`，按提示重新加载窗口。源码克隆后先运行 `npm ci`、`npm run typecheck`、`npm run compile`，再运行 `npm run package:vsix` 生成安装包。

1. 打开并信任一个已有 Git 仓库文件夹。
2. 点击左侧 AI Team Manager 图标，再点击“打开完整工作台”，或运行 `AI Team Manager: Open Dashboard`。
3. 点击顶部“启动”检测现有 Codex CLI；此按钮不会自动发送消息。
4. 输入消息并发送；Enter 发送，Shift+Enter 换行。
5. 点击“停止”终止本轮进程树；已收到内容及会话 ID 保留。
6. “新建会话”保留旧历史；“历史”可恢复本项目的旧会话。

多文件夹工作区通过“选择项目”明确选取分析根目录。无文件夹、非 Git 仓库、未受信任工作区或不支持只读的 CLI 会显示中文错误，不自动初始化 Git、跳过检查或扩大权限。

### GPT 主动派发与自动修复（Task 06）

Task 07.1–07.9 的业务能力由 Task 07.10 正式团队入口复用。以下 Task 06 内容描述保留的旧单 Worker 通道；隔离团队的新界面及验收以本页 Task 07.10 说明和最新报告为准。历史进度见 [Task 07 报告](TASK07-TEST-REPORT.md)。

隔离 Worker 采用 `--bare` 加空内置工具集，仅加载插件显式提供的有限 MCP 工具；每次文件操作先检查精确授权、真实路径和文件锁，测试放进 WSL 只读快照执行。既有连接由 CLI 自行使用，插件不读取或复制凭据。隔离执行暂只支持 WSL2、x86_64 命名空间和精确 Python 脚本，其他运行时拒绝宿主回退。

1. GPT 与 Claude 选择同一本地 Git 工作区，确认两个 CLI 可用并沿用已有授权。
2. 在 **GPT 总指挥**窗口输入：`让 Claude 在当前项目创建 test.py，实现一个加法函数，并运行测试。`
3. GPT 调用本地 MCP 的 `dispatch_claude_task`。Claude 先只读生成计划，独立 GPT 审批后才能执行；随后每个工具请求先经过权限 Hook。用户无需复制任务到 Claude。
4. 插件收集真实工具事件、文件变化、Git Diff，再独立执行任务已授权的测试并交给 Task 04 引擎审查。发现有证据的问题时复用 Task 05 方案生成与验证逻辑，在原文件范围内进入下一轮；默认最多 **2 轮，包含首轮开发**，可配置 1–3 轮。
5. 审查、测试、执行及逐问题修复验证全部通过才标记 PASSED。GPT 不可用、缺证据、代码过期、越权、高风险待确认或达到轮数上限时停止/暂停，不声称完成、不自动扩大范围、不破坏性回滚。

右侧“GPT → Claude 调度”显示实际任务、阶段、Session、文件范围、独立审批/审查 Token 与历史。点击“完整记录”查看每轮计划、审批、事件、源码快照、Diff、测试及关联审查报告。主 GPT 对话 Token 在原聊天窗口另计，缺失 usage 显示为未知，不按零推算。清空聊天展示不删除这些记录。

“审批模式 / 轮数”支持 MANUAL、GPT_HYBRID（默认）及 GPT_SCOPED_AUTO。文件路径须精确、无通配符；只有当前用户消息中明确创建或明确允许修改的普通文件可直接进入范围审批，推断出的文件范围需要原生人工确认，用户明确禁止的文件不可由 GPT 放行。L0 读取/有限只读命令允许；L1 授权源码和测试由 GPT 审批；L2 配置、依赖和不确定操作转人工；L3 敏感路径、危险/未知命令硬性拒绝。三种模式都不能覆盖硬性拒绝。

自动测试命令限 `node --test`、`npm test`、`npm run compile`、`python -B <精确相对路径>.py`，Python 脚本应使用 unittest 并实际运行测试。额外依赖命令须在计划 `expected_commands` 中逐项列出 `npm install --ignore-scripts <包>@<精确版本>`，同时批准 package.json 与 package-lock.json 范围，并逐次人工审批。复合命令、重定向、后台运行、任意 Shell/PowerShell 表达式均不放行。其他语言可通过明确授权的项目 npm 编译/测试脚本接入。

MCP 服务仅监听 127.0.0.1，每个 GPT 请求具有独立随机凭据与生命周期；凭据通过该子进程环境传递，不写配置或日志。仅五个网关工具允许进入插件的审批流程，不开放普通 Shell 写权限。Claude 保持非交互 manual 权限模式，复用 PreToolUse/PermissionRequest HTTP Hooks；不使用全局绕过、不修改 Claude/Codex/CC Switch 持久配置。支持相同工具调用两个 Hook 阶段之间的单次精确授权传递，最长 60 秒，不跨 Session、参数变化或后续调用复用。

“停止任务”、聊天停止按钮或 GPT 请求结束会停止仍在运行的任务、审批、测试和审查；已发生的文件修改保留。退出/重启后历史与 Session 恢复，中断任务标记 INTERRUPTED，不自动重发。PAUSED/FAILED 后需检查完整记录并明确发起新任务，不自动无限重试。权限 Hooks 不是操作系统沙箱：已授权测试/构建/依赖命令会运行项目代码，其副作用不能全部通过工具 Hook 隔离；实时语义风险判断也不保证发现所有缺陷。

已验证 Codex CLI `0.167.0-alpha.4.1` 与 Claude Code `2.1.322`。需要 Codex 的 Streamable HTTP MCP 配置和原生工具调用能力；不假设其他 Provider 的函数调用协议相同。不支持时请求失败并保留原历史，不退化成文本模拟派发。

真实验收：`npm run test:dispatch-host-live`；权限及两轮修复验收：先 `npm test`，再分别运行 `node test/dispatch-permissions-live.cjs`、`node test/dispatch-loop-live.cjs`。这些命令会调用真实模型并消耗现有账户用量，写入独立 `.tools` 测试工程。详见 [Task 06 测试报告](TASK06-TEST-REPORT.md)。

### Claude Worker 01

1. 在本机 Claude Code 中完成原有登录，打开并信任一个本地项目文件夹。Claude 不要求该文件夹已经初始化 Git。
2. 打开工作台，点击 Claude Agents，进入 **Claude Worker 01**。
3. 点击该面板“检测 CLI”，确认实际版本和路径；检测只运行版本及帮助命令，不调用模型。
4. 输入明确任务并发送。Claude 可真实修改所选项目，先在测试项目使用。文本逐步显示，工具区显示 Read、Edit、Bash 等接口实际返回的活动和结果状态。
5. 下一轮自动使用同一 Session ID；“历史”选择原会话，“新建”创建独立会话，首轮成功启动后获得新的 CLI Session ID。
6. “清空显示”只隐藏当前 Webview 中的消息，保留本地历史及原 Session。“停止”或顶部“暂停”终止当前选中对话的进程树，保留部分回复和 Session，不回滚文件。

Claude CLI 查找顺序：机器设置 `aiTeamManager.claudeExecutable` → VS Code 进程 PATH → 已启用的官方 Claude Code 扩展所带 CLI。默认 `claude` 会自动查找，手动路径优先且失败不切换。支持含空格路径及 Windows `.cmd`，提示词通过 stdin 传入，不拼接 PowerShell 命令，不改变系统执行策略或 PATH。

本机已实测 Claude Code **2.1.322**。采用官方 [非交互流式接口](https://code.claude.com/docs/en/headless) 的 `--print --verbose --output-format stream-json --include-partial-messages`，续聊使用明确的 `--resume <session_id>`。仅展示思考状态，不展示或保存原始推理内容；stderr 仅在内存中用于分类，不原样输出。

Task 03.1 使用 `manual` 权限模式和本任务独立的 HTTP Hooks，要求 CLI ≥ 2.1.295（本机真实测试 2.1.322）。保留 `--print` 非交互流式通信，执行前的 `PreToolUse` 检查文件范围，`PermissionRequest` 将审批结果交回 CLI。显式 ask 规则覆盖本任务开放的六个工具，兜底 `--permission-prompts none` 保证没有审批处理器时拒绝操作；正常审批由 Hooks 处理，不依赖终端提示。Hooks 配置只通过本次进程的 `--settings` 传递，不写入用户设置、项目设置、CC Switch 或登录配置，不使用权限绕过参数。普通 CLI 自身或管理员的拒绝规则仍可阻止操作。

### Claude 任务权限（Task 03.1）

1. 在 Worker 01 点击 **任务权限**，或在右侧“Claude任务权限”点击“设置任务授权范围”；也可使用命令 **AI Team Manager: Configure Claude Task Permissions**。
2. 填写允许修改的精确相对路径，例如 `src/motor_control.c, test/motor.test.c`。不接受通配符或父目录。留空默认只读；范围作为本项目的本地偏好保留，换任务时请主动检查、更新。每次请求开始复制并锁定范围，运行中不能修改。
3. 选择精确命令白名单。`node --test`、`npm test`、`npm run compile/build/lint/typecheck` 会真实执行项目脚本，选择即表示授权；它们不是操作系统沙箱，项目脚本自身的行为不受文件工具路径规则约束。只有信任测试脚本时才加入白名单。未入白名单的这些固定命令进入审批；任意 Shell、复合命令、重定向、命令替换、后台执行、权限绕过和危险命令直接拒绝。
4. 工作区内普通文件可读取；授权范围内的普通源码修改自动批准。工程配置修改、清空文件、删除授权范围内单个普通文件均需审批。文件范围外的修改、敏感路径、工作区外路径、链接和目录删除直接拒绝，不能用一次审批扩大授权范围。目录全文 Grep 暂不开放，需先 Glob 列出文件，再对普通文件逐个读取或搜索。
5. 待审批操作会出现在原生选择框和工作台中，显示工具、操作、目标和内容预览。可选择 **允许一次 / 允许本次任务 / 拒绝**。本次任务授权仅适用于当前请求的同一文件同类操作或同一精确命令，结束、停止、超时或重启即失效，不写入 Claude 会话权限或配置。90秒无人处理按拒绝；原生选择框关闭也按拒绝。
6. 授权记录、拒绝原因和完整日志路径显示在面板中。日志保存在插件全局存储的 `permissions/<项目摘要>.jsonl`，与 request/task ID、Session ID 关联。只保存经过脱敏的操作元数据和决策，不保存工具完整内容、凭据或原始推理。显示最近100条，完整日志在磁盘保留。

HTTP 主机仅监听 `127.0.0.1`，每个任务使用独立随机能力地址；失联、超时、无效响应、日志写入失败时阻止操作。文件检查是工具执行前的权限控制，不是文件系统沙箱，无法防范用户/其他进程在检查与执行间替换文件。项目已安装的 Hooks 和管理员策略仍由原 CLI 管理。Task 06 派发复用这一权限机制，并增加独立 GPT 审批。

## 工作台布局

### 独立代码审查（Task 04）

1. 将所选本地项目的 **Git 仓库根目录**作为工作区打开。插件不会初始化或修改用户仓库，也不自动提交 Claude 的工作。
2. 在 Claude Worker 01 中发送开发任务。插件先保存当前文本快照和原始任务；同一会话的后续指令追加保存，保留最初快照。新任务请新建 Claude 会话。Claude 开发结束后不会自动调用 GPT。
3. 在右侧任务面板底部的“GPT 独立代码审查”点击“执行测试”，明确选择 `npm test` 或 `node --test`。这会真实运行项目脚本，应只对你信任的项目使用。支持 120 秒超时与停止。退出码、可识别的用例数、输出及对应源码摘要单独保存；退出 0 但没有识别到用例时标为 UNVERIFIED，不能当成测试通过。
4. 点击“审查结果”，可补充与本任务有关的 GPT 技术要求。不会自动混入无关 GPT 聊天。缺少执行前快照时要求输入原始任务，比较 Git HEAD，并标为需要人工确认改动来源。
5. GPT 使用现有 CodexService，在隔离的本地证据目录中开启独立只读会话。输入包含真实 Git 生成的差异、前后代码、相关测试和模块上下文、Claude 最终回答，以及实际测试记录。不会使用 Claude 的自述冒充测试证据。
6. 右侧展示审查结论、严重程度统计、六个维度、问题证据、建议与历史。展开详情，点击 **已验证**问题的位置，可在编辑器定位文件行；修改前的已删除代码打开快照文档。报告后文件改变会拒绝旧位置跳转，需重新审查。

审查结论为 PASS、REQUEST_CHANGES、BLOCKED、NEEDS_HUMAN_REVIEW。CRITICAL/HIGH 且代码证据已验证时强制要求修改；缺少必要证据、模型/JSON 失败、代码在审查中变化时不能通过。证据验证只证明代码位置与引用相符，不保证模型判断绝对正确。

六个维度：需求符合性、架构、代码逻辑、安全、性能、测试。每个问题包括编号、严重程度、维度、文件、行号、前/后版本、说明、逐字证据、影响、建议、置信度和证据校验状态。没有代码佐证的候选问题标为 UNVERIFIED，需要人工复核，不能点击伪造位置。

支持按约 22000 字符的代码批次分批审查，最多 100 批；跨批接口与整体架构仍需人工确认，不会仅凭批次全绿自动 PASS。单文件采集上限 512 KiB，项目文本总量 16 MiB，相关测试及架构上下文 64000 字符，单批完整提示 180000 字符；超过限制明确 BLOCKED，不静默省略后通过。默认排除依赖、构建目录、敏感配置与常见二进制资产；无法判断被省略文件是否发生相关变化时报告缺口。

报告与证据写入 `globalStorageUri/reviews`，按项目路径哈希隔离；批次输入、输出和 Session ID 可追溯。Task 04 审查本身只读，修复必须另走下述 Task 05 人工审批流程，不推进下一任务。原始任务、代码和测试可能包含商业信息；常见密钥模式会遮盖，敏感路径不会读取，但不能保证识别所有秘密。历史默认保留且没有自动清理，大量历史可能占用较多本地空间及内存。

### 审查反馈与定向修复（Task 05）

1. 在原项目恢复开发时的 Claude Session，选择一份当前代码快照对应的 Task 04 报告。
2. 在右侧 **GPT → Claude 定向修复** 中按严重程度筛选，勾选一个或多个问题。可忽略、恢复或标记人工处理；缺少当前代码证据的问题只能人工处理。
3. 输入补充要求，点击“生成修复方案”。GPT 只收到选中问题、对应代码/Diff 和已采集的必要上下文，不会自动派发。方案包含根因、精确允许/禁止文件、策略、验收、测试、风险和原始 ID。
4. 查看方案后点击“批准并发送”，或选择“修改修复要求”“暂不处理”“拒绝修复”。修改要求会生成新方案并重新审批。扩大文件范围、删除、公共接口变更和 HIGH 风险需额外原生审批；不能突破原任务显式授权。
5. 批准仅绑定当前方案哈希、代码摘要和原 Session。代码、方案或会话变化会阻止发送。每个修复持久记录发送标记，失败或重启均不自动重发；恢复 Session 失败会明确提示，不偷偷创建新会话。
6. Claude 通过原有 Adapter 续聊，并使用本轮临时文件授权。其他文件修改直接拒绝；常规终端命令不授权，批准删除仍需工具审批。不会更改你的持久权限配置、CC Switch 或模型配置。
7. 执行后点击“编译 / 测试”，明确选择 `npm run compile`、`node --test` 或 `npm test`。这些命令执行当前项目代码，编译不会被当作测试；未运行的检查不会标为通过。随后点击“GPT复审”，每轮只允许一次独立复审。
8. 查看 `FIXED / NOT_FIXED / REGRESSION / UNVERIFIED` 和“查看完整报告”。保留原 review/issue/repair ID、审批、真实工具/授权事件、前后源码、Git Diff、实际检查输出、GPT 用量和验证报告 ID。可从修复历史恢复；完成后不会自动再派发。

`FIXED` 要求独立审查 PASS、当前快照测试通过及无执行/日志缺口。精确代码已经证明的未修复/回归可以保留否定结论，即使其他维度还缺少证据；这不等于整体审查通过。原报告不会因修复而被覆盖，任务推进仍需人工决定。最终回答在报告中最多保留 32000 字符，完整内容仍在原 Claude 会话历史中；事件超过 5000 条会明确记缺口，不自动通过。

Task 05 仅支持本地 Git 根目录、已恢复的原始 Session 和单轮闭环。代码快照用于比较该时间段全部已采集文本变化；其他编辑器/进程同时改动不能独占归因给 Claude。权限 Hooks 保护 Claude 工具调用，不能阻止外部进程或已批准项目脚本的所有副作用，不是操作系统沙箱。

快照记录时间段内的文件变化，若用户或其他进程同时编辑，不能把全部变化独占归因给 Claude。当前审查针对文本内容，不覆盖完整二进制、文件权限元数据、全部动态依赖或运行时行为。独立手动审查不自动派发任务；Task 06 调度流程复用同一审查引擎进行验收。

布局选择支持单窗口、双窗口、三窗口、四窗口和自适应网格。窗口右上角可折叠、最大化、恢复和移除；“＋ 窗口”只增加预留位置，最多 12 个，不会创建额外 Claude 会话。只有 Worker 01 接入 Claude，其余预留窗口输入与停止均禁用。GPT 和 Claude 的消息、草稿、历史与停止操作相互独立。移除窗口不会删除会话，可通过对应导航恢复。

拖动窗口之间的分隔线可改变列宽或行高，导航、任务面板和日志也有拖动分隔线；分隔线可用 Tab 聚焦并通过方向键调整。布局、尺寸和折叠状态保存到 VS Code 的工作区存储，关闭工作台后仍保留，重启可恢复编辑器面板。更多菜单可恢复默认布局。聊天草稿只保留在当前 Webview 内存中，不写入布局配置。

大屏幕显示完整五区域；中屏幕收窄导航并将任务面板改为按需展开；小屏幕通过标签切换当前对话，不同时挤压多个窗口。任务按钮可打开右侧面板。界面使用 VS Code 主题变量，适配深色、浅色及高对比度主题，无 CDN 或单独网页服务器。

顶部“启动”检测当前选中 GPT 或 Claude 窗口的 CLI；“暂停”停止对应进程及其调度任务，并非断点暂停。“继续”返回已有会话输入框，用户发送下一条消息后继续同一会话，不自动恢复已停止的执行。“CC Switch 路由：未检测”表示插件未读取配置，不能据此判断路由服务是否正常。

任务目标、阶段、优先级、进度、预算和审核在没有后端数据时显示未知或未设置。Task 06 的实际审批、审查与修复结果在右侧调度面板展示；下一项任务按钮仍禁用，不自动推进多个任务。GPT Token 和会话数据沿用真实 CLI usage，不估造预算。

执行日志支持来源、级别、搜索、暂停滚动和清空显示。清空仅隐藏已显示条目，不删除插件聊天、CLI 原始记录或宿主中的日志缓冲。日志最多保留本次插件运行的 500 条事件，不把聊天正文复制成日志；未发生的 Claude 和测试事件不会生成假记录。

## 现有 Codex / CC Switch 环境

插件不安装 Codex，不要求 API Key，不读取密钥，不修改供应商、模型名称、CC Switch 配置、路由地址或 Codex 配置文件。继承 VS Code 进程的环境变量，通过 `codex` 命令调用。Routing 模式需保持现有路由服务运行，Direct 模式保留原设置。连接错误只显示分类后的中文信息，原始 stderr 不写入日志。

CLI 查找顺序为：手动指定的 `aiTeamManager.codexExecutable` → 当前 VS Code PATH → 已安装并启用的 OpenAI Codex 扩展自带 CLI。默认值 `codex` 会自动查找；手动指定了其他路径时严格使用该设置，失败也不偷偷切换。扩展路径来自 VS Code 扩展注册表，不固定版本目录，不修改系统 PATH。会话信息可查看实际 CLI 来源及路径。

如果仍找不到 Codex，请确认 Codex 扩展已启用，或设置机器级 `aiTeamManager.codexExecutable` 为现有可执行文件的绝对路径，再重新加载窗口。路径不要填入参数。

## 命令

- Open Dashboard / Diagnose Environment / Refresh Dashboard / Open Settings。
- New GPT Conversation / Stop GPT Response。
- GPT Conversation History / Clear GPT Conversations。
- Select GPT Project / Check Codex Connection。
- Check Claude Connection / New Claude Conversation / Stop Claude Response。
- Claude Conversation History / Select Claude Project。
- Review Claude Code Changes / Run Tests for Code Review / Stop Code Review / Code Review History。
- Open Targeted Repairs：打开工作台，在右侧查看审查问题和修复方案。

以上命令均以 `AI Team Manager:` 开头，可从命令面板调用。“检测 CLI”只检查版本及参数，不发送模型请求。环境诊断仍检查 VS Code、Node、npm、Git、Codex 和 Claude 的版本。独立审查会通过现有 Codex 配置产生真实模型用量。

## 会话与统计

首条消息使用 `exec`，后续消息使用同一 thread ID 的 `exec resume`。恢复失败会明确报错，绝不偷偷新建会话。所有轮次固定只读沙箱及禁止提权审批，用户提示词只从 stdin 输入，不拼入 shell 字符串。

会话及 Token 快照保存在 VS Code 插件 `globalStorageUri/conversations` 中，按规范化的项目绝对路径哈希隔离；`workspaceState` 保存选中项目。重启后恢复历史，未结束请求标为 CANCELLED，不自动重发。聊天不另行上传到存储服务；用户发出的提示词及模型按需读取的项目上下文仍通过原 Codex 配置发送给现有供应商。

“清空记录”仅清除此项目的插件聊天记录，不删除 Codex 原始会话文件。聊天记录属于本地敏感数据，请通过该命令清理。不要在聊天输入中粘贴密钥；插件会遮盖常见密钥模式，但不能保证识别任意格式的秘密。

资源统计来自真实 JSONL usage。当前验证的 CLI 返回累计快照，插件通过差值计算本轮数据；缓存和推理数不重复累加，缺失字段与中断造成的未知数显示 `--`。不同 CLI 的 usage 语义若变化，需重新验证兼容性。CLI 只在 item.completed 提供文本时，界面会在该事件到达后显示回答，不模拟逐字输出。

上面的 `exec`、只读约束及累计差值描述仅适用于 GPT。Claude 使用独立 `globalStorageUri/claude` 命名空间，按项目路径哈希隔离，保留每个对话的真实 Session ID；续聊失败不偷偷新建。写入采用串行原子替换并短暂重试 Windows 文件锁。重启不自动执行任务，中断记录恢复为 CANCELLED。Claude usage 使用该轮 `result.usage`，分别展示普通输入、缓存读取、缓存写入和输出，不从文本推算。没有最终 usage 的中止轮次显示未知。

Claude 输入及模型按需读取的项目内容会通过现有 Claude 配置发送至现有模型服务，本地会话文件也是敏感数据。插件不要求或保存 API Key，不把聊天正文和工具输入复制到执行日志；常见密钥模式会遮盖，但不保证识别任意秘密。清空 Claude 展示不删除持久历史；本阶段没有 Claude 历史批量删除命令。

Markdown 支持标题、列表、引用、强调、行内代码和围栏代码块；代码可复制。原始 HTML 按文本显示，不加载远程图片、脚本或 CDN。

## 设置

- `aiTeamManager.codexExecutable`：默认 `codex`，机器级 CLI 路径，修改后重载窗口。
- `aiTeamManager.requestTimeoutMs`：单轮请求超时，默认 180000，范围 1000–3600000。
- `aiTeamManager.diagnosticTimeoutMs`：版本检查超时，默认 10000，范围 1000–60000。
- `aiTeamManager.claudeExecutable`：默认 `claude`，机器级 CLI 路径，修改后重载窗口。
- `aiTeamManager.claudeRequestTimeoutMs`：Claude 单轮超时，默认 300000，范围 1000–3600000。

## 开发和验证

要求 Node.js 22 或更新版本、npm、VS Code 1.95 或更新版本。

仓库跟踪源码、测试脚本、配置、锁文件、说明文档和VSIX安装包；VSIX必须提交并推送到GitHub。`node_modules/`、`dist/`、`.tools/`、测试运行结果与日志由 `.gitignore` 排除。已完成任务的临时工程、截图、宿主缓存与运行日志可清理，回归测试脚本及验收报告保留；报告中的历史本地证据路径不保证清理后仍存在。克隆后先安装依赖并运行编译，提交前检查暂存区，避免上传凭据和模型运行记录。

```powershell
npm install
npm run typecheck
npm test
npm run test:host
npm run compile
npx @vscode/vsce package
```

按 F5 选择 Run AI Team Manager，可在 Extension Development Host 中运行。`npm run test:host` 使用 Windows PATH 中的 VS Code，在专用 Mock 配置中连续启动两次正常 Development Host，借助本地验收扩展检查真实工作区存储，避免 `--extensionTestsPath` 的内存数据库影响持久化测试。Mock 测试不消耗模型 Token；真实模型验收入口只供手动执行，见 `test/live.cjs`。

浏览器验收与截图：先运行 `npm test` 和 `npm run test:host`，再执行下列 Windows 命令。截图来自宿主实际空会话快照与相同本地 Webview 资产，不包含伪造对话，也不是 VS Code 外框截图。

```powershell
$env:PLAYWRIGHT_BROWSERS_PATH = "$PWD/.tools/ui-browsers"
npx playwright install chromium
node test/workspace-browser.cjs
```

详细结果见 [UI 升级测试报告](UI-UPGRADE-TEST-REPORT.md)，此前的 CLI 验证见 [Task 02 测试报告](TASK02-TEST-REPORT.md)。

Task 03 真实模型测试（会通过本机授权消耗实际模型用量；仅写入独立 `.tools` 测试工程）：

```powershell
npm run test:claude-live
npm run test:claude-host-live
$env:PLAYWRIGHT_BROWSERS_PATH = "$PWD/.tools/ui-browsers"
node test/claude-browser.cjs
```

前者验证真实读写、多轮、新会话、停止和异常退出恢复；后者在正常 VS Code Development Host 中使用生产模块，重启后验证 Session 并通过注册命令停止输出。截图使用真实宿主会话快照和生产 Webview 资产渲染，见 [Task 03 测试报告](TASK03-TEST-REPORT.md)。单元故障测试不调用模型。

Task 04 真实审查验收（会产生实际模型用量，样例为独立测试工程）：

```powershell
npm run test:review-live
node test/review-boundaries-live.cjs
npm run test:review-host-live
$env:PLAYWRIGHT_BROWSERS_PATH = "$PWD/.tools/ui-browsers"
node test/review-browser.cjs
```

五类样例和真实模型结果、宿主定位/重启结果、模块关系及限制见 [Task 04 测试报告](TASK04-TEST-REPORT.md)。样例中的缺陷由验收工程明确构造，不能作为生产代码使用；真实审查结果由 Codex 返回，未使用模拟报告作为验收结论。

Task 05 真实闭环验收（实际调用现有 GPT 和 Claude，模型消耗见报告）：

```powershell
$env:ATM_HOST_VSIX = 'ai-team-manager-0.6.0.vsix'
npm run test:repair-host-live
node test/repair-verification-live.cjs .tools/<本次task05-host目录>
node test/repair-screenshots.cjs .tools/<本次task05-host目录>
```

宿主测试在隔离工程中调用真实模型，并以测试程序显式执行生产审批消息，记录为 `TEST_HARNESS_VIA_PRODUCTION_UI_MESSAGE`，不冒充人工点击。其他状态使用真实代码故障注入和真实 GPT，不声称额外 Claude 执行。详见 [Task 05 测试报告](TASK05-TEST-REPORT.md)。

Task 05 的范围止于用户审批后的单轮修复闭环；多轮自动修复、多 Claude 并行、调度及自动推进属于后续阶段。当前提交规则见上方“开发和验证”。
