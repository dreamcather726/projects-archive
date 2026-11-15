export type ErrorCode = 'CODEX_NOT_FOUND' | 'CODEX_VERSION_UNSUPPORTED' | 'CCSWITCH_ROUTE_UNAVAILABLE' |
  'AUTHENTICATION_FAILED' | 'NOT_GIT_REPOSITORY' | 'SESSION_RESUME_FAILED' | 'PROCESS_START_FAILED' |
  'PROCESS_EXIT_NONZERO' | 'WAITING_APPROVAL' | 'TEAM_PREPARE_FAILED' | 'JSONL_PARSE_ERROR' | 'REQUEST_TIMEOUT' | 'REQUEST_CANCELLED' | 'STORAGE_ERROR' | 'WORKSPACE_UNTRUSTED' | 'WORKSPACE_CHANGED';
const descriptions: Record<ErrorCode, string> = {
  WORKSPACE_UNTRUSTED: '请先信任当前工作区再运行 Codex；插件没有自动更改信任设置。',
  WORKSPACE_CHANGED: '所选工作区已经关闭或路径已改变，请重新选择项目。',
  CODEX_NOT_FOUND: '找不到 Codex CLI。已检查 VS Code 的 PATH 和已安装的 Codex 扩展；请确认扩展已启用，或在插件设置中指定现有 codex 可执行文件。',
  CODEX_VERSION_UNSUPPORTED: '当前 Codex 版本或沙箱无法满足只读执行要求，请检查 exec/resume 支持及 Windows 沙箱状态。未降级执行权限。',
  CCSWITCH_ROUTE_UNAVAILABLE: '模型连接或路由不可用。如使用 CC Switch Routing，请确认路由服务已运行；Direct 模式请检查原有连接。插件未更改路由配置。',
  AUTHENTICATION_FAILED: 'Codex 身份验证失败。请在原有 Codex / CC Switch 环境中检查登录或授权，无需在插件填写密钥。',
  NOT_GIT_REPOSITORY: '当前工作区不是可用的 Git 仓库，或 Git 拒绝访问。请先在终端确认 git status；插件不会初始化仓库或跳过检查。',
  SESSION_RESUME_FAILED: '原 Codex 会话恢复失败。历史仍保留，未自动创建新会话；请重新选择历史或主动新建会话。',
  WAITING_APPROVAL: 'GPT 请求被用量保护暂停：预算已耗尽或中断请求的用量未知。请打开“用量与预算”，调整预算或明确确认未知用量；历史记录保留，未启动新的模型请求。',
  TEAM_PREPARE_FAILED: 'GPT 团队工具准备失败，尚未启动本次模型请求。请检查团队状态和本地存储；CLI 启动检测已通过。',
  PROCESS_START_FAILED: 'Codex 进程无法启动，请检查可执行文件和工作目录。',
  PROCESS_EXIT_NONZERO: 'Codex 请求执行失败。请在原有 CLI 环境中检查状态，插件不会展示可能包含密钥的原始错误输出。',
  JSONL_PARSE_ERROR: '收到无法解析的 JSONL 数据，已跳过异常行；未记录原始内容。',
  REQUEST_TIMEOUT: '请求超时，已终止对应进程树；已收到的回复和会话 ID 保留。',
  REQUEST_CANCELLED: '请求已取消，已收到的回复和会话 ID 保留。',
  STORAGE_ERROR: '本地会话记录无法读取或保存，请检查插件存储目录权限。原文件未自动清空。'
};
export class CodexError extends Error {
  constructor(public readonly code: ErrorCode, cause?: unknown) { super(`[${code}] ${descriptions[code]}`,{cause}); }
}
export function classifyError(text: string, resume = false): CodexError {
  if (/unexpected argument|unrecognized (argument|option)|invalid value.*read-only|sandbox.*(?:failed|unsupported|unavailable|setup)|failed.*sandbox/i.test(text)) return new CodexError('CODEX_VERSION_UNSUPPORTED');
  if (/401|403|unauthori[sz]ed|authentication|invalid.api.key|not.logged.in/i.test(text)) return new CodexError('AUTHENTICATION_FAILED');
  if (/ECONNREFUSED|connection refused|failed to connect|error sending request|connection.*(?:closed|reset)|route.*unavailable|502|503|504/i.test(text)) return new CodexError('CCSWITCH_ROUTE_UNAVAILABLE');
  if (/not.*(?:git|trusted).*(?:repo|directory)|dubious ownership/i.test(text)) return new CodexError('NOT_GIT_REPOSITORY');
  return new CodexError(resume ? 'SESSION_RESUME_FAILED' : 'PROCESS_EXIT_NONZERO');
}
// This is for activity summaries, not credentials/config inspection. Raw stderr is never persisted.
export function redact(text: string): string {
  return text.replace(/\b(?:sk-[\w-]{12,}|gh[pousr]_[\w]{20,})\b/g, '[已隐藏密钥]')
    .replace(/((?:api[_-]?key|authorization|access[_-]?token|password)\s*[:=]\s*)(?:Bearer\s+)?[^\s,"'}]+/gi, '$1[已隐藏]')
    .replace(/https?:\/\/[^\s<>"']+/gi, value => { try { const u = new URL(value); u.username = ''; u.password = ''; u.search = ''; u.hash = ''; return u.toString(); } catch { return '[链接]'; } });
}
