# Task 06 验证报告

日期：2026-11-13。版本：0.7.0。平台：Windows、VS Code 桌面版。所有模型测试仅使用现有 Codex / Claude 授权与隔离测试工程，未修改 CC Switch 或模型全局配置。

## 实现结果与调用链

GPT 聊天现在通过真实 MCP Tool Calling 派发 Claude，而不是输出一段假定已执行的文本。工作流如下：

```text
GPT ChatController → Codex CLI → 本地 ToolBridge MCP（五个工具）
  → TaskDispatcher：授权范围校验 / 去重 / 持久化 / 单 Worker 状态管理
  → 现有 ClaudeCoordinator.sendDispatched → 原 Claude Controller / CLI / Session
  → Claude 只读计划 → 独立 GPT 结构化任务审批
  → Claude 工具调用 → 原 PermissionHost HTTP Hooks → PermissionGateway
    → 硬性路径/参数规则 → L0允许 / L1独立GPT审批 / L2人工 / L3拒绝
  → 实际事件、前后源码与 Git Diff → 独立执行授权测试
  → 原 Task04 ReviewEngine 六维审查
  → 必要时原 Task05 RepairModel / resolveChecks → 有上限的下一轮
  → 实际输出、测试和审查结果经 MCP 返回主 GPT → GPT 汇报
```

默认 GPT_HYBRID，最多2轮，包含首轮开发，可配置1–3轮。只运行一个 Worker、一项任务，不调度下一项任务。失败、证据不足、取消、重启中断不自动重发，不进行无限重试。

新增模块：

| 文件 | 职责 |
| --- | --- |
| src/dispatch/dispatchTypes.ts | 任务、计划、审批、轮次与日志数据结构 |
| src/dispatch/toolBridge.ts | 真实 Streamable HTTP MCP 服务，五个限定工具，每轮随机凭据 |
| src/dispatch/taskDispatcher.ts | 统一生命周期、授权、去重、Claude计划/执行、测试、审查、修复轮次 |
| src/dispatch/approvalModel.ts | 复用 CodexService 的独立只读审批、严格JSON/范围/有效期校验 |
| src/dispatch/permissionGateway.ts | 在原硬性规则上增加 L0–L3 分级、独立GPT及人工审批 |
| src/dispatch/dispatchCoordinator.ts | 工作区、原生人工审批、命令、主GPT桥接及UI快照 |
| src/webview/workspace/dispatchPanel.js | 复用现有任务面板展示真实调度、权限、会话、用量和记录 |

增量修改原 ChatController / ChatCoordinator、ClaudeCoordinator、CodexService、CLI Launcher、Codex事件解析器、PermissionHost / Manager / Policy、ReviewPrompt / Tests、extension.ts、工作台脚本/样式和 WebviewPanel 资源。GPT 和 Claude 仍使用原通信模块，原 Task05 手动审批入口保留。

新增验证脚本：dispatch.test.cjs、run-dispatch-host.cjs、task06-host-live.cjs、dispatch-permissions-live.cjs、dispatch-loop-live.cjs、dispatch-cancel-live.cjs、dispatch-screenshots.cjs。test/api.ts、run.cjs、workspace.test.cjs 增量接入这些模块。

## 数据与权限

五个真实工具：`dispatch_claude_task`、`get_claude_task_status`、`get_claude_task_output`、`cancel_claude_task`、`get_claude_session`。

派发输入包括 description、allowed_files、forbidden_files、test_commands、acceptance_criteria，可选 expected_commands。持久任务含 task_id、GPT owner_id、原始用户消息、工作区、Claude conversation_id/session_id、审批模式、最大轮数、状态、每轮前后快照/Diff/实际测试/review_id/repair_id/逐问题验证、事件、GPT用量及最终输出。GPT推断出的文件范围要求人工确认；原始用户明确禁止的文件不能由GPT重新授权。

每个审批请求含 approval_id、task_id、session_id、request_type、tool_name、tool_arguments、working_directory、target_files、command、allowed_files、forbidden_files、task_description、risk_level、evidence。仅接受结构化 decision、risk_level、reason、approved_scope、restrictions、expires_at；自然语言“同意”、越权范围、不完整范围、过期和解析错误均不能授权。任务审批的 REVISE 只允许有限次重新计划，不能修改原始文件授权。

真实工具参数指纹、Session和当前任务绑定；修改参数重新审批。同一工具调用的 PreToolUse→PermissionRequest 可传递一次精确决定，有效期最多60秒，不是通用缓存。GPT决定有效期最多5分钟，执行前再检查文件真实路径。GPT超时、权限服务失联、身份或日志异常 fail-closed。

CLI 检查：Codex `0.167.0-alpha.4.1`；Claude Code `2.1.322`；Python `3.14.6`。Claude继续使用 `-p` 非交互模式和原有 PreToolUse / PermissionRequest HTTP Hooks，不假定终端会弹出审批。没有默认开启危险权限绕过，没有修改全局权限。Codex通过本轮临时 `-c mcp_servers.ai_team_manager...` 注册服务，能力凭据只在该子进程环境中传递。

兼容性实际发现：当前 Codex 在 approval_policy=never 时，MCP 的 `default_tools_approval_mode="auto"` 无法调用；对这五个受插件网关控制的工具配置 `"approve"` 后真实调用成功。这不开放 GPT 自身文件写入或任意 Shell。Windows `.cmd` 对固定提示词中尖括号有重定向解析问题，已移除该提示词的尖括号占位写法，并增加真实 .cmd 子进程回归测试；用户正文仍走 stdin。

参考官方协议：[Codex MCP](https://learn.chatgpt.com/docs/extend/mcp)、[Codex 配置](https://learn.chatgpt.com/docs/config-file/config-reference)、[Claude Code Hooks](https://code.claude.com/docs/en/hooks)。本地实际版本与真实执行结果是本次兼容性结论的依据。

## 真实原生宿主验收

证据目录：`.tools/task06-host-1794508836375/`。通过 VS Code Extension Host 的 `api.chat.handle`，与GPT窗口发送共用同一入口；没有从测试脚本直接调用 Dispatcher 或手工复制到 Claude。

用户要求在当前项目创建 test.py，实现加法并运行测试，只允许 test.py，不得修改 protocol.c / README.md。实际结果：

- task_id：`f2e58699-9ac4-474e-a101-ed2877900a31`。
- GPT 原生 MCP 调用被实际解析并保存，最终结果返回原 GPT 会话。
- Claude Session：`2a0e6811-8270-42fc-ab8c-d2cc6e042979`，计划修改一次后批准，在同一 Session 开发。
- 实际新建 test.py；`python -B test.py`：exit 0，3项 unittest 通过，包含正数、负数和零。
- 独立六维审查 PASS，review_id：`dc7bbcaf-d81b-4862-ae33-0c7bf79bd0d5`；受保护文件未改变。
- 关闭并重新启动原生宿主，调度历史和 GPT / Claude Session 恢复，无自动重发。

`stage1.json`、`stage2.json`、`dispatch-record.json`、`runtime-snapshot.json` 保留完整实际依据。早期失败运行另留在 .tools 中，不计作通过。

## 真实执行前权限 A–H

证据目录：`.tools/task06-permissions-1794509045442/`，`result.json` passed=true；Claude Session `30893ccd-2e11-4f9d-a065-7382e46f7202`。使用真实 Claude CLI、真实 GPT 决策和生产 HTTP Hooks。

| 案例 | 实际结果及证据边界 |
| --- | --- |
| A 读取 motor_control.c | Claude真实Read，允许 |
| B 修改授权 motor_control.c | Claude真实Edit，独立GPT批准后执行 |
| C 修改 protocol.c | 先Read再Edit，执行前拒绝，文件摘要未改变；首次未先Read的失败尝试也保留 |
| D 授权编译 | Claude真实Bash `npm run compile`，批准并执行。此工程脚本为Node语法检查，不是C编译器验收 |
| E git reset --hard | Claude自行拒绝，没有发起Bash。另通过生产HTTP Hook注入该请求，确认硬性拒绝；后者不是Claude产生的工具事件 |
| F 安装依赖 | 真实安装请求经GPT风险评估进入人工路径；测试驱动选择拒绝，未安装依赖。另一个真实配置Edit被测试驱动允许一次后继续 |
| G GPT超时 | 真实Codex子进程1000ms超时故障注入，真实Claude修改未获许可，目标文件未变。不是伪造GPT回复 |
| H 改变参数 | 同文件两次不同Write内容使用不同指纹，分别真实GPT审批，旧批准不可复用 |

人工分支 actor=TEST_HARNESS，用于可重复验证“允许/拒绝”的实际执行效果，不冒充用户在UI点击。原生人工审批交互沿用已有 Task03.1 机制。protocol.c / main.c 始终未被违规修改。权限验收没有通过依赖安装产生网络副作用。

## 真实两轮自动修复

证据目录：`.tools/task06-loop-1794546906073/`，`result.json` passed=true。task_id `1bd9d1df-78d6-47cd-82ad-44d3a4d1a004`；两轮共用 Claude Session `b579105d-6be2-45ab-9a64-92a4694328ad`。

为了确定性覆盖失败→修复路径，测试驱动在**真实Claude第一轮交付后**，只在隔离工程注入一个真实负数处理缺陷，记录原交付与 fault-injection.json；该缺陷不归因于Claude。后续测试、GPT发现问题、方案生成、Claude修复及复审均真实运行，没有模拟审查结果。

| 轮次 | 实际编译 / 测试 | 独立 GPT 审查 |
| --- | --- | --- |
| 1 | npm run compile exit0；node --test exit1，`5 !== 0` | REQUEST_CHANGES，问题 B1-1，review `0f8e248c-665b-4384-8f2d-d1187c4ffc38` |
| 2 | npm run compile exit0；node --test exit0，1项边界测试通过 | PASS；B1-1 FIXED，review `9b303f4f-f2c4-446d-859a-c4c3370b7319` |

关联原 Task05 repair_id `e226d3ff-0995-4d27-b7ee-10705e39a107`。真实修复核心Diff：

```diff
-  if (value < 0) return Math.abs(value);
+  if (value < 0) return 0;
```

测试文件、README、package.json、input.txt 摘要保持不变。最终状态 PASSED，实际执行两轮。此前一次因编译证据未传入审查提示而 PAUSED，已修正为每次传入实际编译记录，再完整重跑通过；未将缺证据的暂停视为通过。

真实取消记录同目录 `cancel-result.json`：真实 Claude RUNNING 后经 cancel_claude_task 停止，最终 CANCELLED、workerBusy=false、Session保留、文件未改变；相同owner/turn/input重复派发去重成功。

## 自动检查、截图与安装

- `npm run typecheck`、`npm run compile` 通过。
- `npm test`：最终69项，覆盖原GPT/Claude/审查/单轮修复/UI，加上真实HTTP身份/Session/TTL/参数变更、权限失败关闭、MCP、Windows .cmd 等回归；这些测试中的模型替身不计作真实模型验收。
- `npx @vscode/vsce package` 生成 `ai-team-manager-0.7.0.vsix`。构建产物和VSIX只在本地交付，不提交Git。
- 原生宿主安装包激活、编辑器工作台、命令、四个侧栏视图、CSP、模拟GPT续聊、布局和重启持久化全部通过：`.tools/task06-vsix-1794550063848/stage1.json`、`stage2.json`。真实模型派发的证据是上面的独立原生宿主验收，安装冒烟测试不重复消耗模型。
- 最终包132816字节，SHA256：`E5AA773F55703A5AFE6FD3BF644C3E1D582F15D90D91ABAAFF1453CAF47D454F`。当前用户VS Code也已安装，并通过扩展列表确认 `ai-team-manager.ai-team-manager@0.7.0`；已有窗口须重新加载后启用新版本。
- 环境诊断六项均成功：VS Code1.146.0、Node26.13.1、npm11.20.0、Git2.56.0.windows.1、上述实际Codex和Claude版本。安装冒烟聊天使用测试CLI替身，版本诊断是另行执行的真实环境检测。
- Webview截图无pageerror；原生宿主日志另有VS Code内置AgentHost市场策略、API提案和Node弃用警告，退出阶段有内部dispose的Canceled堆栈。恢复侧栏的早期阶段出现missing-CSP警告，最终三个Webview均已加载且hasCsp=true；不能声称整机控制台完全无错误。这些日志保留供后续排查，本次未观察到插件激活失败或最终页面加载失败。
- `screenshots/results.json`：生产Webview资源 + 原生宿主真实快照，深色1900px、浅色1900px、窄屏620px；无页面异常、无横向溢出。截图是这些真实状态的Webview渲染截图，不包含操作系统/VS Code窗口边框。已实际查看三张图。
- [深色截图](.tools/task06-host-1794508836375/screenshots/result-dark.png)、[浅色截图](.tools/task06-host-1794508836375/screenshots/result-light.png)、[窄屏截图](.tools/task06-host-1794508836375/screenshots/result-narrow.png)。

## 实际GPT用量

数值来自 CLI usage，输入含缓存输入；不再次叠加缓存或推理Token，不换算价格。仅统计以下验收记录，不冒充整个开发期间总成本。

| 验收 | 独立GPT调用 | 已知 input / cached / output / total | 缺少usage |
| --- | --- | --- | --- |
| 原生派发 | 7 | 134832 / 87936 / 1543 / 136375 | 0 |
| 两轮修复 | 11 | 214861 / 124160 / 2853 / 217714 | 0 |
| 权限A–H（含该记录保留的失败重试） | 13 | 199750 / 129536 / 1648 / 201398 | 2，未知而非0 |

原生派发的主 GPT 对话另计：input958614、cached926848、output2327、total960941。主对话轮询成本较高；已合并无意义心跳等待、输出按游标分页、同工具两个Hook阶段单次授权传递以减少后续开销，但没有承诺零开销。Claude用量继续由原有会话统计记录。

## 当前边界

1. 文件权限是真实执行前控制，不是操作系统沙箱。显式授权的 npm / Python 项目脚本仍能执行项目代码，不能把命令白名单解释成对脚本全部副作用的隔离。真实路径、符号链接、硬链接及敏感路径检查不能消除外部进程的检查/执行竞态。
2. 只开放有限且精确的测试/编译命令；任意Shell/PowerShell、组合命令、重定向、后台、未知参数拒绝。Task06不授权物理删除。依赖安装只支持精确版本、禁安装脚本的有限npm语法并逐次人工审批。
3. GPT是语义审批与代码审查，不能保证发现所有逻辑/安全问题；越权与硬性拒绝由插件规则执行，GPT不能覆盖。审批失败、无法判断及缺必要证据停止，不按“模型没说有问题”通过。
4. 一个Worker、一项任务、最多3轮；达到上限、Session恢复失败、网络/授权失败不会自动新建Session、扩大范围或无限重试。停止不回滚已经发生的修改。重启只恢复记录，不恢复旧批准。
5. 框架保留跨平台实现，但本轮真实CLI和原生宿主仅验证Windows。仅支持受信任的本地Git根目录。大型/被排除/超过采集上限的代码可能BLOCKED，不能伪称完整验收。
6. 完整日志、源码快照和会话可能很大，仅本地保留，无自动清理；不随源码上传。与其他进程同时编辑时，Diff不能独占归因给Claude。

Task06核心能力已真实验证：主GPT主动调度、独立任务/工具审批、实际执行结果返回、有限多轮修复与独立复审。本次未开发多Claude、其他模型、Worktree或后续多任务调度。
