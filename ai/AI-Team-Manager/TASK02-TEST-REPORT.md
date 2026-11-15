# Task 02 测试与交付记录

日期：2026-11-12。基线：`41e6b14`，分支 `master`，工作区开始时无未提交改动。复用 `https://github.com/dreamcather726/AI-Team-Manager`，上游 `origin/master`。

## 0.2.1 修复验证

用户实际界面暴露 CODEX_NOT_FOUND：0.2.0 仅通过宿主 PATH 启动 Codex，而普通 VS Code 进程没有继承 Codex 会话注入的临时 PATH。现已增加已安装 `openai.chatgpt` 扩展的 CLI 发现；手动路径优先，其次 PATH，最后查询扩展注册表，不固定版本目录，不改系统 PATH 或 CC Switch。

新增四项路径回归测试，合计 21 项全部通过；类型检查、编译及两次真实宿主启动、聊天和重启恢复均通过。还显式清空 resolver 的 PATH，成功定位本机已安装扩展的 codex.exe，实际 `--version` / exec、resume 帮助检查通过，版本为 0.167.0-alpha.4.1。本次验证未再调用真实模型。会话信息新增 CLI 来源及实际路径。

当前修复安装包为 `ai-team-manager-0.2.1.vsix`。以下内容保留 0.2.0 的原始验收记录。

## 实现范围

在原 Dashboard 的 GPT 区域增加聊天，未重写 Claude 团队、任务管理和日志区域。新增安全进程启动、JSONL 解析、会话控制、按项目隔离的本地存储及 Token 快照计算。

所有新建及恢复请求都显式使用 `--sandbox read-only`；提示词通过 stdin 传递。继承当前进程环境，不设置模型、供应商、Key 或路由地址。只设置本次请求的只读权限、禁止提权审批及总指挥角色，不写 CC Switch 或 Codex 配置文件。

## 实际 CLI 兼容性

已执行 `codex --version`、`codex exec --help`、`codex exec resume --help`。本机版本 `0.167.0-alpha.4.1`；恢复参数顺序经实际请求验证：

```text
codex exec --json --sandbox read-only --cd <workspace> -c approval_policy="never" -c developer_instructions=<固定角色> resume <thread_id> -
```

`approval_policy="never"` 在只读沙箱之外拒绝提权，不扩大沙箱权限。未使用忽略 Git 检查、全权限或危险绕过参数。

## 自动测试

`npm run typecheck`、`npm run compile`、`npm test`、`npm audit` 通过。17 项自动测试全部通过，依赖审计 0 漏洞：

- UTF-8 分块、残行、超长行和异常 JSONL；忽略内部推理与未知事件。
- Token 缺失字段、缓存不重复相加、累计快照差值、异常下降时未知值。
- 新建 / 恢复参数顺序，只读沙箱、stdin 传参；Windows `.cmd` 启动及特殊字符输入。
- CLI 缺失、路由不可用、身份失败、沙箱失败、恢复失败；原始 stderr 不进入界面或存储。
- 停止与超时终止父子进程树，保留部分回复。
- 同一 thread 续接、存储重载、工作区隔离、历史选择、新建会话及清空插件记录。
- 存储损坏不覆盖；中断请求标为 CANCELLED，不自动重发；预检可取消。
- 本地 Markdown 渲染、代码复制、HTML 注入防护、输入焦点与草稿保持、Enter / Shift+Enter。

## 真实模型验收

真实请求只执行任务要求的首次连接、同会话续接、项目分析和停止测试，均通过现有 `codex` 环境，无独立 API 调用：

| 测试 | 结果 |
| --- | --- |
| 首次连接 | 回复 `CODEX_CONNECT_OK`，收到 thread.started 并保存 ID |
| 连续对话 | 使用原 thread ID，正确回忆验证字符串，没有新建会话 |
| 项目分析 | 回答实际 src/chat、providers、services、process、parsers、storage、webview 和 test 结构 |
| 停止 | 在原会话请求中停止，返回 REQUEST_CANCELLED |
| 只读检查 | 四次真实请求前后 Git status 一致，没有产生 Commit 或 Push |

本机 CLI 的 usage 是累计值。首轮 input 17478、output 8；续接快照 input 34980、output 16，因此续接本轮差值 input 17502、output 8。第三轮累计 input 92734、output 413。插件保存原始快照，并用相邻完整快照计算本轮统计；中断导致基线缺失时本轮显示未知，不伪造零。缓存读取、写入和推理 Token 单列，不再加到总数中。

## 真实 Extension Development Host

在独立测试用户目录中使用 Mock CLI，不额外消耗真实模型 Token。已验证：

- 插件激活、十个命令注册、四个视图可打开。
- GPT 新建及两轮聊天成功。
- Commander / Tasks / Logs 三个 Webview 的本地脚本 ready 握手与 CSP 标签。
- 关闭宿主后再次启动，历史、当前会话 ID 和选中项目恢复，并能续接同一个会话。

专用 Mock 宿主测试使用 `--disable-workspace-trust` 排除测试目录的信任提示；用户真实配置未改变，生产插件仍拒绝未信任工作区。测试入口为 `test/task02-host.cjs`。

## 限制与日志观察

- 路由中断及恢复通过 Mock 验证；没有停止或更改用户正在使用的 CC Switch 路由服务，也未读取其密钥。真实四次请求验证了当前路由/直连环境可用。
- Windows 界面截图工具仍报 `helper_unknown_error: setup refresh had errors`。未完成人工深浅主题视觉验收；已通过 DOM 交互及真实 Webview ready 验证。
- 宿主日志存在 VS Code 自身 mutex / 内置扩展警告、初始空 Webview 的 missing-CSP 警告及测试关闭时的 Canceled 日志；最终页面均验证含 CSP 且脚本加载成功。不能声称整个 VS Code 控制台完全无警告。
- 原基线已跟踪 node_modules、dist、旧 VSIX 和运行记录。本次保留基线，仅显式暂存 Task 02 源码、配置、锁文件、文档和测试入口；不提交新增运行记录、依赖目录或构建产物，不修改 .gitignore。

## 安装包与复现

安装包：`ai-team-manager-0.2.0.vsix`。执行 `npx @vscode/vsce package` 生成，包含 bundle、本地 Webview 资源和依赖许可证，不包含聊天记录、测试配置或 node_modules。已用 VS Code CLI 在独立扩展目录安装成功，列表确认 `ai-team-manager.ai-team-manager@0.2.0`；没有改变用户正常使用的扩展目录。

常规验证：`npm install` → `npm run typecheck` → `npm test` → `npm run test:host` → `npm run compile` → `npx @vscode/vsce package`。`test:host` 已完整执行通过，可自动生成隔离的 Mock 配置并连续启动两次宿主。

`test/live.cjs` 仅手动执行真实验收，不会由 npm test 或插件激活自动运行。其本地记录保存在 `.tools/task02-live`，不纳入本次提交。

提交说明：`[feat] 接入 Codex CLI GPT 对话`。最终提交哈希及 Push 状态以交付回复为准。

未接入 Claude、未创建 Agent、未实现调度或 Task 03。
