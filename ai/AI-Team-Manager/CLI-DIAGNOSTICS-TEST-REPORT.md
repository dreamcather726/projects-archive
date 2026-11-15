# 0.7.4 — CLI 诊断与当前用量拦截修复

日期：2026-11-14。验证通过。

## 两个问题及处理

1. 环境检测原来只向系统 shell 执行裸命令 codex/claude，没有使用聊天服务已有的设置、PATH、已注册扩展解析器。用户本机的两个 CLI 实际都存在于已安装 VS Code 扩展内。现在检测复用相同解析逻辑，仍只执行 --version，不启动模型、不安装 CLI、不更改系统 PATH 或认证配置。显式设置无效时继续报告失败，不悄悄换用其他 CLI。
2. 当前项目有且仅有一条历史 CANCELLED 请求没有返回 usage，导致未知用量门禁。依据用户本轮明确要求修复，先备份账本原始字节并记录 SHA-256，然后使用既有生产确认函数确认这条历史未知用量。没有把未知用量写成 0，没有删除记录，没有提高 500000 预算，没有取消未来新未知用量的确认要求。

账本备份和结果（本地忽略目录）：`test-results/usage-repair-1794639724996/`，包含 `gpt-usage-ledger.original.json`、`baseline.json`、`result.json`。写入前校验目标项目映射、记录 ID、唯一条数、状态、预算及文件未变化；确认后校验全部 entries 和预算保持不变，只有 unknownAcknowledged 从 false 变为 true，blocked=false。备份及用户运行状态不提交 GitHub。

## 代码改动

- `src/services/environmentService.ts`：增加可注入的诊断解析入口，复用 `resolveCodexExecutable` 和 `resolveClaudeExecutable`；使用既有启动器的固定 argv，只传 --version。显示真实来源及脱敏路径，限制输出大小、排除原始 stderr，超时终止对应进程树。
- `src/extension.ts`：环境检测传入当前设置及实时扩展注册路径，与聊天入口一致，不固定扩展版本目录。
- `test/recovery-api.ts`、`test/run-recovery.cjs`、`test/environment.test.cjs`：新增扩展回退、缺失显式路径不降级、带空格 cmd 路径、超时及 stderr 隐藏回归。
- `test/recovery-host.cjs`、`test/run-recovery-host.cjs`：独立宿主使用本机现有 CLI 绝对路径配置，新增实际双 CLI 版本检测；保留未知用量恢复、真实 GPT 回复及重启原线程验证。
- 版本和说明更新为 0.7.4。未修改已有历史 TeamSession/teamTools，未修改或提交用户历史 `test/api.ts`。

## 实际验证

| 检查 | 结果 / 证据 |
|---|---|
| 类型检查、编译、VSIX 打包 | 通过；0.7.4 安装包位于 `test-results/ai-team-manager-0.7.4.vsix` |
| `npm test` | 100/100 通过 |
| `npm run test:task0710` | 7/7 通过 |
| `npm run test:recovery` | 10/10 通过（含 4 项本次诊断测试） |
| 真实扩展回退检测 | `test-results/cli-diagnostics-1794640294712.json`；在解析器的发现 PATH 为空时，通过本机已安装扩展定位并实际运行两个二进制；没有模型请求 |
| 实际 CLI 版本 | codex-cli 0.169.0-alpha.11.1；Claude Code 2.1.323 |
| 最终安装包独立 VS Code 宿主 | `.tools/codex-recovery-host-1794640394362/stage1.json`、`stage2.json`；双 CLI 诊断通过，未知用量恢复后真实 GPT 回复，两次启动保持同一线程且不自动重发 |
| 原生诊断明细 | 同宿主目录 `diagnostics-stage1.json`、`diagnostics-stage2.json` |
| 回归日志 | `.tools/diagnostic-fix-regression.log` |
| 当前项目账本 | 备份后确认唯一历史未知请求；原 entries、未知 usage 和预算保留，门禁已解除 |

单元测试使用明确的模拟夹具验证解析和失败分支；真实本机扩展回退另用实际二进制验证。隔离原生宿主使用 CLI 的绝对路径设置，测试其生产命令和真实版本输出；未知用量的“接受”由同一生产函数的显式测试 UI 回调完成，原生输入框“取消”实际验证。没有把回调测试冒充人工点击，也没有使用模型替身冒充真实 GPT 回复。

当前用户项目的账本修复是依据本轮授权进行的实际状态修改，与隔离宿主测试账本分开。更新扩展后须重新加载窗口，使运行中的旧扩展实例载入新代码及修复后的账本；不会代用户重发历史模型请求。

## 提交边界与限制

只提交本轮代码、测试和说明，不提交账本、备份、模型会话、VSIX、依赖或编译产物。历史 `test/api.ts` 保持原样未提交，SHA-256 为 E46FB9ECA52AC425EC6F0CEB4D200CA12FD6C60D462B1CBD79815BA168BBCC99。

版本检测通过表示找到可执行文件并成功读取版本，不等同于模型认证/网络检查通过。已知预算耗尽或未来出现新的未知用量时，原门禁仍继续工作；本次没有把保护永久关闭。
