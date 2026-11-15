# 项目总结

AI Team Manager 是基于 VS Code 的多智能体开发工作台，使用现有 Codex CLI 和 Claude Code CLI，将需求讨论、任务规划、开发执行、测试、独立审查与候选交付连接起来。

## 核心能力

- **GPT 总指挥**：多轮对话、任务规模评估、计划审批、依赖分析和团队监督。
- **Claude 团队**：隔离工作区、并发调度、会话复用、暂停恢复及逐任务执行记录；保留旧单 Worker 入口。
- **质量闭环**：收集候选差异，在当前快照上运行测试，进行独立审查，并在限定范围和轮数内返工。
- **权限与保护**：精确文件授权、路径校验、文件锁和有限 MCP 工具；对敏感或不确定操作实施门禁。
- **可追溯工作台**：展示对话、日志、差异、测试、审查、用量和历史；进度依据验证证据计算，候选变化时允许回退。
- **恢复与交付**：中断后保存会话和候选，恢复前重新校验；READY 表示候选验证通过，实际交付需要明确授权。

## 技术架构

TypeScript 编写 VS Code 扩展后端，Webview 使用 JavaScript/CSS；esbuild 打包，VSCE 生成 VSIX。通过 CLI 子进程和本地 MCP 连接模型能力，沿用已有 CLI 登录。

| 目录 | 主要职责 |
| --- | --- |
| `src/chat`、`src/services`、`src/claude` | 对话、Codex/Claude 服务接入 |
| `src/planning`、`src/team`、`src/scheduling`、`src/workers` | 规划、团队工作流、依赖调度与 Worker 管理 |
| `src/permissions`、`src/protection`、`src/isolation` | 权限、文件保护和隔离执行 |
| `src/review`、`src/repair`、`src/integration` | 独立审查、修复与集成 |
| `src/webview`、`src/providers`、`src/workspace` | 工作台界面及 VS Code 入口 |
| `src/storage`、`src/usage` | 历史存储和用量记录 |
| `test` | 单元、故障注入、浏览器、宿主与真实模型验收脚本 |

## 典型流程

用户提出需求 → GPT 规划并获得所需授权 → 调度 Claude Worker → 收集候选与测试证据 → 独立审查及有界返工 → 集成验证 → 用户授权交付。

## 开发入口

环境要求：Node.js 22+、npm、VS Code 1.95+。使用真实模型还需本机可用且已登录的 Codex/Claude CLI。

```powershell
npm ci
npm run typecheck
npm run compile
npm test
npm run package:vsix
```

专项回归包括 `test:retry`、`test:review-evidence`、`test:progress`、`test:progress-ui` 和 `test:task0711`。真实模型验收与普通回归分开执行，具体入口及已有验收结果见 [README](README.md) 和对应阶段报告。

## 当前基线与边界

本总结以已提交的 0.8.3 基线及仓库说明为依据，覆盖自适应调度、审查证据修复、保守自动重试和真实任务进度。工作区已有后续开发改动，不作为本次文档提交的发布内容。

隔离执行存在平台和运行时限制，详见 README；权限 Hook 不能代替操作系统沙箱。未知 Token 用量保持未知，预算是请求前门禁。历史报告记录对应阶段的验收，不代表每次更新均已重新完成全部真实模型验收；LARGE 真实模型验收在现有 README 中仍标为待继续。
