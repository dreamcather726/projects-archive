# AI Team Manager 0.3.0 · UI 升级验收

日期：2026-11-12。范围仅为现有插件的工作台布局，不新增模型接入或自动调度。

## 交付

- 源码：`src/providers/workspacePanel.ts`、`src/workspace/`、`src/webview/workspace/`，增量修改入口、清单和侧边栏。
- VSIX：`ai-team-manager-0.3.0.vsix`，16 个文件，51.79 KB。
- VSIX SHA256：`33781C96A90EBA6AFFB252292EA633E7C46AA8BC473CFB8A73BD6BFB7E221066`。
- 已安装到当前用户的 VS Code，命令行确认 `ai-team-manager.ai-team-manager@0.3.0`。现有窗口需重新加载后使用新版。
- 本地截图：`.tools/ui-upgrade/workspace-dark.png`、`workspace-light.png`、`workspace-four-panels.png`、`workspace-mobile.png`。

截图由 Chromium 渲染生产 Webview 的相同 HTML/CSS/JS，使用 Extension Host 真实初始空会话快照，不是 VS Code 外框截图，不包含 Mock 聊天、在线 Claude 或测试任务进度。源码和报告提交 Git；VSIX、截图、浏览器与宿主运行数据留作本地交付，不纳入提交。

## 验证结果

| 项目 | 结果与证据 |
| --- | --- |
| npm install | 成功；新增 Playwright 仅用于开发验收；审计 0 个漏洞 |
| TypeScript / compile | `npm run typecheck`、`npm run compile` 和打包前检查均通过 |
| 回归测试 | `npm test`：26/26 通过，覆盖 CLI 解析、统计、取消、超时、多轮恢复、项目隔离、安全 Markdown、界面草稿和新布局交互 |
| 编辑器工作台 | 正常 Extension Development Host 断言 `TabInputWebview` 位于编辑器区域，脚本 ready；重复 Open Dashboard 不创建重复标签 |
| 现有入口与命令 | Activity Bar 与四个旧视图正常；10 个命令均注册，Open Dashboard、Refresh、Settings、Diagnose、CLI 检测、新会话和停止实际执行 |
| 布局 | 单、双、三、四、自适应网格切换通过；动态增删预留窗、折叠、最大化与恢复通过 |
| 尺寸调整 | 浏览器真实指针拖动验证对话列宽、行高、导航、右侧和底部尺寸；键盘调整及宿主保存消息验证通过 |
| 持久化 | 关闭再开保留布局；两次正常宿主进程启动后恢复三窗口、导航 238px、折叠日志；GPT 继续原 thread |
| 消息隔离 | Claude 及预留窗禁用输入和停止，不渲染 GPT 内容；Host 路由拒绝非 GPT panelId；新增窗口不创建会话 |
| 任务面板 | 真实数据缺失时显示未知 / 未设置；验收提示可见，下一任务按钮禁用；小屏按需展开 |
| 日志 | 来源、级别、搜索、暂停滚动、清空显示通过；清空不改变宿主缓冲和聊天；相同日志快照避免重复渲染 |
| 响应式 | 1680、1280、1024、760、640、390、320px 均未发生页面横向溢出；小屏标签切换通过 |
| 主题与资源 | 深色、浅色截图已检查；高对比度使用原生边框变量；浏览器页面脚本 / 控制台错误 0，远程资源请求 0 |
| VSIX | `npx @vscode/vsce package` 成功；安装到全新隔离配置后再进行两次宿主启动，激活、资源加载、会话和布局恢复均通过 |

源码宿主证据：`.tools/task02-host-1794477490707/`。最终安装包宿主证据：`.tools/task02-host-1794478509599/`。浏览器结果：`.tools/ui-upgrade/browser-results.json`。这些路径是本次机器上的本地测试记录。

本机 VS Code 的 `--extensionTestsPath` 模式没有持久化工作区数据库，因此验收运行器改为专用配置中的正常 Development Host，并用仅存在于 `.tools` 的本地验收扩展运行断言。生产插件不包含测试启动钩子，不更改用户设置。

## 环境版本检查

通过现有环境诊断命令，仅运行 `--version`：

| 软件 | 实际结果 |
| --- | --- |
| VS Code | 1.146.0 |
| Node.js | v26.13.1 |
| npm | 11.20.0 |
| Git | 2.56.0.windows.1 |
| Codex CLI | 0.167.0-alpha.4.1 |
| Claude Code CLI | 2.1.322 |

六项均成功。Claude CLI 已安装，但当前工程没有 Claude Adapter；本次不补建 Adapter，因此不能宣称 Claude 多轮、流式输出和停止已通过验收。Worker 01 保持未接入。GPT 回归使用本地 Mock CLI，不发起真实模型请求。

## 已知限制与日志观察

VS Code 宿主日志存在启动 / 恢复 Webview 时的 missing-CSP 警告；检查所有生成页面均包含 CSP 与 nonce，Webview ready 和浏览器 CSP 运行检查通过，仍将该宿主警告保留记录。退出宿主时还记录 VS Code 内部 `Canceled`，以及内置 Agent Host 的未登录 / SDK 提示和 `url.parse` 弃用警告。未把这些情况描述为整个 VS Code 控制台零错误；本插件工作台的浏览器运行没有脚本错误。

没有接入任务执行、预算、验收引擎、CC Switch 路由探测或 Claude 会话。暂停实际取消当前响应，继续需用户发送下一条消息；不提供断点续跑或自动进入下一任务。额外窗口只是布局预留，最多 12 个。

未修改 CC Switch、API Key、模型配置、系统 PATH 或 `.gitignore`。保留原有未提交构建文件和依赖变化，提交时仅列出本次 UI 源码、清单、文档与测试，执行暂存区差异检查后推送既有 `origin/master`，最终提交短哈希与 Push 结果在交付回复中提供。
