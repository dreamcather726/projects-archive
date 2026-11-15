# Task 07.10 验收报告

状态：**COMPLETED**。版本：**0.7.2**。验证日期：2026-11-14。完成范围依据本次新任务书及用户对两个历史文件的有条件授权。实际自动化、真实 GPT/Claude、原生 VS Code 宿主、隔离与交付保护测试均已通过；下述限制仍明确保留。

## 实现结果

沿用现有深色蓝紫工作区、布局和会话入口，完成 GPT 总指挥与 Claude 独立 Worker 工作台。GPT 通过现有规划器、工具桥和 TeamSession 创建真实任务；Worker 列表、状态、会话 ID、任务、测试、日志和文件差异来自生产调度及存储接口。前端只挂载所选 Worker 的工作台，切换不重新启动任务、不创建模型请求、不复制其他 Worker 的消息。

Worker 工作台提供对话、文件、日志、测试、任务五个子视图，支持精确指向 Worker 的暂停、恢复、取消、暂停时补充指令，独立草稿及滚动位置、查询复制、分批显示、暂停自动滚动和可拖动/键盘调整的分栏。文件页只展示已捕获候选证据。未被原后端采集的原始工具参数不伪造补齐。

本地输出格式化提供“格式化 / 原始 / 调试”三种显示。完整 JSON 使用字段卡片和可展开层级，保留全文复制；不完整 JSON 回退安全文本/Markdown。Markdown 支持代码高亮、列表、表格和允许协议的链接；原始 HTML 不执行。工具卡片区分权限决策、执行状态和实际结果，拒绝或失败不会显示成功。显示、切换和格式化均不调用模型，也不引入外部脚本/CDN。

任务中心、审查与集成、项目管理、设置四个未来页面分别由独立组件和路由承载，保留明确占位状态。没有把尚未实现的未来页面标记为业务功能完成。

## 架构及接口改动

| 文件/模块 | 本次改动及保留边界 |
|---|---|
| `src/dispatch/dispatchCoordinator.ts` | 接入 TeamSession 的真实计划、工具、快照及精确身份校验；沿用原权限弹窗、工具执行和用户交付确认；提供所选 Worker、控制、指令、差异、预览及交付消息处理。 |
| `src/extension.ts`、`src/providers/workspacePanel.ts` | 将 `team*` 消息接入协调器，加载新增本地 Webview 脚本；保留既有面板入口和 CSP。 |
| `src/team/teamSession.ts`、`src/team/teamTools.ts` | 明确授权的历史成果接入；原始全文、字节哈希、逐文件增量及保留情况单独记入[历史报告](TASK0710-HISTORY-TEST-REPORT.md)。 |
| `src/workers/claudeWorkerManager.ts` | `select(id,notify=true)`、`persist(notify=true)` 增加可选静默通知；默认调用行为不变，显示选择不再推进执行轮询。 |
| `src/scheduling/teamTaskExecutor.ts` | 新增 `instruct(taskId,text)`，保存补充指令并在原任务恢复时带入；继续使用原执行会话、锁和权限网关。 |
| `src/integration/integrationManager.ts` | 集成审查时从汇总禁改路径中排除已经由计划分配给其他 Worker 的汇总允许路径；各 Worker 原有文件所有权检查不变。 |
| `src/team/teamWorkflow.ts`、`src/review/reviewTypes.ts`、`src/review/reviewPrompt.ts` | 审查证据增加可选 `workflow_context`，提供候选阶段、真实调度状态和 Worker 元数据，避免独立候选审查循环要求未来 READY/交付；原代码及测试审查仍执行。 |
| `src/workspace/workspaceLayout.ts` | 验证并恢复动态 Worker 标签身份，标题从可信元数据生成；保留原布局及占位页。 |
| `src/webview/workspace/workspace.js`、`gptCommanderPanel.js` | 渲染真实团队快照、动态标签、真实 Worker 徽标及所选工作台；关闭标签后同步选择，保留会话草稿和布局。 |
| `workerPanel.js`、`outputFormatter.js`、`futurePages.js` | 新增独立工作台、安全本地格式化器、四个未来路由组件。 |
| `src/webview/markdown.js`、`chatPanel.js`、`workspace.css` | 扩展既有安全文本渲染和三模式入口，补充响应式工作台、JSON/工具卡片及分栏样式。 |
| `package.json`、`package-lock.json`、README、CHANGELOG | 版本 0.7.2、专用测试命令及使用说明；没有修改模型供应商设置或用户正常宿主配置。 |

## 实际验证

所有模型测试都使用已有配置连接真实 GPT 和 Claude CLI；无模型替身冒充真实执行。运行数据、隔离目录、依赖和 VSIX 位于忽略目录，不进入源码提交。

| 检查 | 结果 | 本地证据/入口 |
|---|---|---|
| 类型检查、生产编译 | 通过 | `npm run typecheck`、`npm run compile` |
| 原有自动化回归 | **100/100 通过** | `npm test`，原有测试文件不变 |
| 本次专用自动化 | **7/7 通过** | `npm run test:task0710`；JSON/Markdown 安全、工具真实状态、动态 Worker 隔离与切换、控制身份、分页、四路由、布局验证 |
| 独立源码安装与构建 | 通过 | `test-results/task0710-clean-source`；Git 基线叠加本次文件，排除 `test/api.ts` 历史修改；独立 `npm ci --ignore-scripts --offline` 后类型检查、编译、100+7 测试全部通过；`task0710-clean-verification.json` 验证 HEAD 测试入口一致（规范化换行）。 |
| 三 Worker 真实协作 | 通过 | `test-results/task0710-live-1794632954523/result.json`；真实 GPT 规划、三个独立 Claude Session、定向暂停/补充指令/恢复、单项审查、集成审查及测试项目交付；实际 Python 测试共 **24 项**（6+10+8）。 |
| 精确选择不发请求 | 通过 | 同目录 `selection-result.json`；恢复真实持久会话，120 次选择，provider 调用 **0**，执行修订号、账本和请求数不变；差异仅来自证据，路径穿越拒绝。 |
| 最终 VSIX 原生宿主 | 通过 | `.tools/task0710-package-host-1794634360233/stage1.json`、`stage2.json`；实际安装 0.7.2，走生产 DispatchCoordinator，真实 GPT 启动两个独立 Claude Worker，两个模块各 **3 项**实际测试，通过阶段审查到 READY；没有自动交付。 |
| 关闭重开、进程重启 | 通过 | 上述原生宿主两次启动；Worker Session ID 保持、恢复不自动重发、CSP 正常加载。另三 Worker 实测检查持久化后的原 Session 和无重发。 |
| 原有操作系统隔离 | 通过 | `.tools/task073-os-1794632273886/result.json`；真实 WSL 命名空间、源码只读、网络禁用、秘密及 `.git` 不挂载、锁冲突拒绝。 |
| 原生脏编辑器保护 | 通过 | `.tools/task074-host-1794632999079/stage1.json`、`stage2.json`；实际未保存编辑缓冲区保护；三 Worker 测试另验证外部编辑确认拒绝及脏编辑器阻止交付。 |
| 浏览器生产资产验收 | 通过 | `test-results/task0710-live-1794632954523/browser/result.json`；真实执行快照回放到生产 Webview 资产，三种模式、四路由、分栏拖动及 320/390/640/760/1024/1280/1680 宽度无横向溢出；脚本错误 0、外部请求 0。 |

新增测试入口为 `test/task0710-api.ts`、`run-task0710.cjs`、`task0710.test.cjs`、`task0710-live.cjs`、`task0710-selection.cjs`、`task0710-browser.cjs`、`task0710-host.cjs`、`run-task0710-host.cjs`。真实测试依赖本机可用的 GPT、Claude CLI、VS Code 和既有隔离运行时，不会在缺少凭据时伪造通过。

## 截图证据

以下 12 张图片保存在 `test-results/task0710-live-1794632954523/browser/`：`gpt-commander.png`、`worker-workbench.png`、`multiple-workers.png`、`actual-plan-json.png`、`worker-task-json.png`、`tool-events.png`、`actual-tests.png`、`worker-mobile.png`、`task-center.png`、`review-integration.png`、`projects.png`、`settings.png`。

这些图片是**真实执行快照在生产 Webview 资产中的浏览器回放**，不是原生宿主屏幕截图；原生宿主的实际链路证据为上面的两次启动结果。已人工查看总指挥、Worker 工作台、移动尺寸、JSON 及工具卡片图片。回放最大测得切换/渲染耗时 226ms，仅代表本机此次样本，不能解释为运行性能上界。截图和模型运行原始数据作为本地忽略证据保留，不随源码提交，以免泄漏本机路径或运行数据。

## 真实测试发现与修复

1. 初次三 Worker 运行的单项测试通过，但集成审查将各 Worker 的“禁止改兄弟文件”合并后与集成允许范围矛盾。保留原候选所有权校验，修正集成审查的路径投影后，重新完整执行三 Worker 流程并通过。失败证据保留在 `test-results/task0710-live-1794632459528`。
2. 原生宿主首次任务描述用“仅新增”没有满足现有精确权限语法，实际弹窗未回答导致超时。测试描述改为明确的允许/禁止文件范围；没有绕过用户权限确认。该失败证据位于 `.tools/task0710-package-host-1794633195935`。
3. 下一次原生宿主中两个模块实际测试通过，但单项候选审查循环要求未来集成 READY。补充真实阶段上下文后，保留代码审查要求，重新打包并完整运行两个宿主阶段均通过。失败证据位于 `.tools/task0710-package-host-1794633795989`。
4. 三 Worker 监视期间发现 UI 选择会推进后端修订号，提前唤醒 GPT 长轮询。将显示选择改为静默通知，并使用真实持久会话重复选择验证零调用、账本及执行修订号不变。最终原生宿主测试使用此修复后的版本。

## 用量与适用限制

三 Worker 完整实测的账本记录 GPT 已知 usage 为 **876,866 tokens**（包含缓存统计），GPT CLI 请求共 **5 次**；未知的供应商底层 API 调用数保持 `null`，不伪造为 0。此轮在后续选择修复前执行了大量轮询，不能作为低 Token 消耗样例。低 Token 的直接证据是修复后的 120 次纯显示选择零模型调用。

预算沿用原有请求前检查：默认 500,000 的预算可能在最后一次返回后才被实际用量超过，之后的请求会被阻止；无法预知下一次供应商返回用量，因此不承诺绝对硬上限。未使用隐藏压缩模型调用，也没有把未知用量写成已知值。

隔离与代码执行保持既有运行时支持范围；本机验证的是可用的 WSL2/x86_64/Python 路径，其余不可用能力继续拒绝执行。工具卡片只展示后端实际持有的脱敏信息；未采集参数不会补写。验收条目只有实际证据才能算通过；没有逐条机器映射证据时仍显示待核验，不把整体测试通过推断为每条需求已逐项核验。四个未来页面仍为独立占位组件。

## 提交边界

两个授权历史文件的原始全文、哈希、逐文件增量和保留说明见[历史接入报告](TASK0710-HISTORY-TEST-REPORT.md)。`test/api.ts` 原历史改动保留在工作区，不纳入提交；本次没有接入其他历史未提交成果。

源码提交只包含本次任务改动及两个明确授权的历史文件，不提交凭据、`.env`、模型运行数据、依赖、构建产物或验证 VSIX。验证用 VSIX 为 `test-results/ai-team-manager-0.7.2.vsix`。既有已跟踪发行包不覆盖。

最终在 `master` 分支统一创建唯一提交，目标为现有 `origin/master`（`dreamcather726/AI-Team-Manager`）；提交前检查暂存白名单、`git diff --cached --check` 与 `git diff --cached --stat`。最终回复提供实际提交哈希及推送结果；本报告不预写尚未发生的推送成功。
