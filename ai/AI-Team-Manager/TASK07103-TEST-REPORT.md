# Task 07.10.3：GPT 总指挥主界面与 VSIX

本阶段完成 GPT 对话、当前任务摘要和六类任务详情。沿用原生 JS/CSS、Webview/CSP、原消息桥及原聊天/派发/规划/测试/审查快照，未修改 AI 执行和安全业务层。当前授权结束于 07.10.3，不进入 07.10.4。

## 实现与接口

- 保持正式参考图的深色蓝紫风格、顶部标签和左右双栏。右栏可拖动，新布局默认 420px、范围 250–520px，保留已有尺寸存储。
- 顶部区分 CLI 已检测但模型未验证、连接中、在线响应、在线空闲及异常；本地 CLI 检测不冒充模型连通。
- 保留原多行输入、Enter/Shift+Enter、输入法合成保护、流式 Markdown、请求中断、历史、新会话和项目选择。页面切换保留当前草稿，切换 GPT 会话清除旧草稿。
- 用户/GPT 消息、真实任务通知、人工审批提示、审查摘要和后端开发计划分开展示，不混入 Claude 完整执行对话，不额外调用模型格式化。
- 六类卡片：任务摘要、Worker 分配、任务拆分与依赖、验收与实际测试证据、风险、下一步。按项目路径和 GPT owner_id 关联任务，拒绝其他项目/会话及无关审查报告。
- 实际派发沿用 Claude Worker 01；任务与当前 Claude Session、Conversation 均匹配才允许跳转。跳转只改变路由，不创建、停止或销毁 Worker/Session。
- 自适应规划的 Worker 槽位显示“仅规划/尚未执行”。计划步骤不自动变为完成；缺少优先级、步骤进度或逐项验收结果时显示未提供/未验证，不估算百分比。仅 PASSED 任务、PASS 审查、当前轮实际测试全部 PASSED 且含非编译测试时显示总体 100%，逐项标准仍保持未验证。
- 原审查、权限、修复、调度和统计保留在“业务操作与历史”，继续使用原消息接口。任务详情渲染失败不会阻止聊天或触发重复模型调用。

## 实际文件清单

| 文件 | 变更 |
| --- | --- |
| src/webview/workspace/gptCommanderPanel.js（新增） | 只读数据投影、六类详情、计划/通知及关联校验 |
| src/webview/workspace/chatPanel.js | GPT 信息栏、操作、消息类型及草稿隔离 |
| src/webview/workspace/workspace.js、workspace.css | 组件接入、真实状态、视觉和响应式布局 |
| src/providers/workspacePanel.ts | 仅增加本地组件脚本，保留 nonce/CSP |
| src/workspace/workspaceLayout.ts、src/webview/workspace/layoutManager.js | 详情宽度默认值和范围，复用原布局存储 |
| test/workspace.test.cjs | 增加六组 GPT 数据、证据、隔离和异常测试 |
| test/workspace-browser.cjs | 资源加载、六卡片空状态及布局/生命周期回归 |
| test/gpt-commander-host.cjs（新增） | 安装包真实 GPT 只读对话、重启恢复及只读复核 |
| test/gpt-commander-browser.cjs（新增） | 真实宿主快照的生产 UI 渲染和截图 |
| test/claude-browser.cjs、review-browser.cjs、permissions-browser.cjs、repair-screenshots.cjs、dispatch-screenshots.cjs | 补齐资源列表，相关截图展开原业务区 |
| package.json、package-lock.json | 仅更新发布版本 0.7.1，增加 package:vsix；依赖解析不变 |
| .vscodeignore | 安装包排除 07.10 报告；未修改 .gitignore |
| README.md | 固定每阶段验证、生成新 VSIX、隔离安装、提交和上传流程 |
| ai-team-manager-0.7.1.vsix（新增） | 本阶段安装包 |
| TASK07103-TEST-REPORT.md（新增） | 本报告 |

## 实际测试结果

| 验证 | 结果和边界 |
| --- | --- |
| 类型检查/编译 | 打包 prepublish 的 typecheck、compile 均通过 |
| 自动回归 | npm.cmd test：100/100 通过，原 94 项加本阶段 6 组；覆盖会话、GPT/Claude 通道、调度、权限、锁、编辑保护、依赖、审查、修复和集成等自动测试。UI 场景夹具只用于测试，不冒充运行状态 |
| 空状态浏览器 | 七档 320–1680 宽度无横向溢出；五种原布局、全部分隔线、最大化/折叠、小屏、路由和标签验证通过；渲染异常 0、远程资源请求 0 |
| 真实对话浏览器 | 读取真实 GPT 宿主快照，消息数一致、六卡片存在、无假进度、响应式通过；渲染异常 0、远程资源请求 0 |
| 最终 VSIX 隔离安装及稳定宿主回归 | 安装成功，生产资源/CSP、面板复用、尺寸/路由/隐藏标签恢复及重启续聊均通过；此项使用 Mock CLI，与真实模型结果分开 |
| 首次真实 GPT 安装包验证 | 两次宿主启动、两轮只读讨论及同一 Session 续聊通过，没有派发 Worker |
| 最终 VSIX 真实 GPT 复测 | 首轮成功，重启续聊曾 FAILED，分类 CCSWITCH_ROUTE_UNAVAILABLE。保留失败记录，原 Session 重试成功；最终只读复核确认 3 个请求中保留 1 个失败记录，同一 Session、无 Worker 派发。不将首次失败标成通过 |
| 测试脚本修正 | 重试实际成功后，原断言因快照随调用变化而计数错误；改为调用前保存计数。之后通过只读宿主复核已完成请求，没有再次调用模型 |
| 生命周期/文件保护 | 导航及 Worker 跳转只发 layout，不发 send/new/stop；原隐藏重开保留 DOM/草稿/Session 测试通过；保护清单 9 个文件与开工 SHA-256 一致 |
| 旧专用截图工具 | 更新加载列表并语法检查；历史真实证据已清理，未重新运行这些截图工具，不标为运行通过 |

真实烟测仅讨论，不写入工程、不启动 Claude；未进行真实多 Worker 并行写入或全量 07.1–07.9 真实模型验收。格式化和数据场景的自动测试不等于所有真实模型场景验收。

## 安装包

- 文件：ai-team-manager-0.7.1.vsix；版本 0.7.1；150599 字节；ZIP 条目 22。
- SHA-256：9F5D3C1B3F28ED8E6FA929E5F63E9CD678261815F281B1991836AA401DEA02D1
- 包内 15 个生产资源/配置/编译文件与工作区字节哈希相同，包含 GPT 新组件。不含测试、.tools、.git、node_modules、受保护 Team TS 源码或凭据文件。
- VSCE 的 MarkdownProcessor 自动把 README 相对报告链接改为仓库 URL，属于正常处理，不要求 README 字节一致。
- 最终包已在隔离 VS Code 配置安装验证，保留旧 0.6.0/0.7.0 包；新 VSIX 与源码一并提交上传。

## 本地证据与截图

- .tools/task07103/unit-tests.txt、package-audit.json：最终单测和包审计。
- .tools/task07103-package-host-1794600854418/：最终包稳定宿主回归。
- .tools/task07103-host-1794596138500/：最终包真实 GPT；failed-route-attempt.json 保留路由失败；retry-count-assertion.json 保留测试断言失败；stage2.json、runtime-snapshot.json 记录最终只读复核。
- .tools/task07103-ui/gpt-real-conversation.png、gpt-real-details.png、gpt-real-mobile.png：真实 GPT 对话、六类详情及小屏。
- .tools/task07103-ui/workspace-dark.png、navigation.png、session-tabs.png、browser-results.json、gpt-real-results.json：空状态及布局证据。

截图是浏览器真实运行生产 Webview 并读取真实宿主快照的页面截图，不是 VS Code 整窗截图。对话为实际 GPT 回复；仅讨论未派发任务，因此右栏保持真实空状态。运行记录、专用配置和截图放在现有 .tools 忽略目录，不上传会话数据。

## Git 与后续限制

起点 master / origin/master：4ed6006。原有 test/api.ts 修改、未跟踪 teamSession.ts 和 teamTools.ts 不修改、不暂存；DispatchCoordinator、TeamWorkflow、IntegrationManager、ClaudeWorkerManager、FileLockManager、Permission Gateway 未改动。仅提交本清单文件和 VSIX，暂存差异检查后正常推送，不强制推送。提交哈希与实际 Push 结果在交付消息中报告，避免自引用。

观察到真实模型路由曾临时不可用，未改动认证、CC Switch 或模型配置。优先级、逐步骤状态、逐项验收以及真正多 Worker 的接口仍须后续接入，UI 明确显示缺口。本阶段不提前实现 07.10.4 Claude 完整工作台、07.10.6 四页组件或 07.10.8 通用格式化。
