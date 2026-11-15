import { redact } from "../services/codexErrors";
export type ClaudeErrorCode =
  | "CLAUDE_NOT_FOUND"
  | "CLAUDE_VERSION_UNSUPPORTED"
  | "AUTHENTICATION_FAILED"
  | "NETWORK_ERROR"
  | "PERMISSION_DENIED"
  | "PROCESS_START_FAILED"
  | "PROCESS_EXIT_NONZERO"
  | "OUTPUT_PARSE_FAILED"
  | "SESSION_RESUME_FAILED"
  | "REQUEST_CANCELLED"
  | "REQUEST_TIMEOUT"
  | "STORAGE_ERROR"
  | "WORKSPACE_UNTRUSTED"
  | "WORKSPACE_CHANGED";
const descriptions: Record<ClaudeErrorCode, string> = {
  CLAUDE_NOT_FOUND:
    "找不到 Claude Code CLI。请检查 VS Code 的 PATH、已启用的 Claude Code 扩展，或设置 claudeExecutable。",
  CLAUDE_VERSION_UNSUPPORTED:
    "Claude CLI 缺少本插件要求的流式或权限参数，未降级执行。",
  AUTHENTICATION_FAILED:
    "Claude 授权无效或已过期，请在原有 Claude CLI 中重新登录。无需在插件填写密钥。",
  NETWORK_ERROR:
    "Claude 网络或模型服务连接失败。请检查现有连接；插件未更改 CC Switch 或模型配置。",
  PERMISSION_DENIED:
    "Claude 操作被权限规则拒绝。请查看任务权限面板的拒绝原因、授权文件范围和审批记录；已有 CLI 或管理员拒绝规则仍然有效。",
  PROCESS_START_FAILED:
    "Claude 进程无法启动，请检查可执行文件、目录权限及工作目录。",
  PROCESS_EXIT_NONZERO:
    "Claude 进程意外退出或任务执行失败。已收到内容与 Session ID 保留。",
  OUTPUT_PARSE_FAILED:
    "Claude 输出解析不完整或包含异常数据；不能确认任务成功。原始输出未写入日志。",
  SESSION_RESUME_FAILED:
    "原 Claude Session 恢复失败，未自动新建或切换会话。请检查历史或主动新建对话。",
  REQUEST_CANCELLED:
    "Claude 已中止，已收到的回复及 Session ID 保留；已发生的文件修改不会自动回滚。",
  REQUEST_TIMEOUT: "Claude 请求超时，已终止对应进程树，保留已收到内容。",
  STORAGE_ERROR: "Claude 本地记录读写失败，未清空原文件。",
  WORKSPACE_UNTRUSTED: "请先信任当前本机工作区再运行 Claude。",
  WORKSPACE_CHANGED:
    "所选 Claude 工作区已经关闭或路径发生变化，请重新选择项目。",
};
export type ClaudeRetryReason = 'HTTP_502' | 'HTTP_503' | 'HTTP_504' | 'HTTP_429' | 'NO_AVAILABLE_CHANNEL' | 'ECONNRESET' | 'ETIMEDOUT' | 'GATEWAY_UNAVAILABLE';
export interface ClaudeRetryHint {
  reason: ClaudeRetryReason;
  routeRejected: boolean;
  retryAfterMs?: number;
}
export class ClaudeError extends Error {
  constructor(public readonly code: ClaudeErrorCode, public readonly retry?: ClaudeRetryHint) {
    super(`[${code}] ${descriptions[code]}`);
  }
}
/** Only parse a delay; never retain response headers, URLs, credentials or raw diagnostics. */
export function retryAfterMs(text: string, now = Date.now()): number | undefined {
  const value = text.match(/\bretry-after\s*["']?\s*:\s*["']?([^\r\n"'}]+)/i)?.[1]?.trim();
  if (!value) return undefined;
  if (/^\d+(?:\.\d+)?$/.test(value)) {
    const ms = Number(value) * 1000;
    return Number.isFinite(ms) ? Math.ceil(ms) : undefined;
  }
  // RFC HTTP-date, not an arbitrary string accepted leniently by Date.parse.
  if (!/^(?:Mon|Tue|Wed|Thu|Fri|Sat|Sun), \d{2} [A-Z][a-z]{2} \d{4} \d{2}:\d{2}:\d{2} GMT$/.test(value)) return undefined;
  const time = Date.parse(value);
  return Number.isFinite(time) ? Math.max(0, time - now) : undefined;
}
/** A CLI-generated API-error envelope is diagnostic data, not assistant output. */
export function isClaudeApiDiagnostic(text: string): boolean {
  return /^API Error:\s*(?:429|502|503|504)\b[^\r\n]*(?:\r?\n\s*)?$/i.test(text.trim());
}
export function classifyClaudeError(text: string, resume = false): ClaudeError {
  // Non-replayable failures take precedence even if the diagnostic also mentions a gateway.
  if (/(?:\b401\b|\b403\b|unauthori[sz]ed|authentication|invalid.api.key|not.logged.in|login.required|token.*expired)/i.test(text))
    return new ClaudeError('AUTHENTICATION_FAILED');
  if (/permission|access.*denied|EACCES|EPERM|not.allowed/i.test(text))
    return new ClaudeError('PERMISSION_DENIED');
  if (/unknown.option|unrecognized.option|invalid.*(?:argument|option)|requires.*verbose/i.test(text))
    return new ClaudeError('CLAUDE_VERSION_UNSUPPORTED');
  if (/no (?:conversation|session) found|session.*(?:not found|does not exist|invalid|expired)|resume.*fail/i.test(text))
    return new ClaudeError('SESSION_RESUME_FAILED');
  if (/\b(?:HTTP(?:\/\d(?:\.\d)?)?|API Error:|status(?: code)?[:=]?)\s*429\b/i.test(text))
    return new ClaudeError('NETWORK_ERROR', {reason:'HTTP_429',routeRejected:false,retryAfterMs:retryAfterMs(text)});
  if (/\bNo available channel for model\b/i.test(text))
    return new ClaudeError('NETWORK_ERROR', {reason:'NO_AVAILABLE_CHANNEL',routeRejected:true,retryAfterMs:retryAfterMs(text)});
  const status = text.match(/\b(?:HTTP(?:\/\d(?:\.\d)?)?|API Error:|status(?: code)?[:=]?)\s*(502|503|504)\b/i)?.[1];
  if (status) return new ClaudeError('NETWORK_ERROR', {reason:('HTTP_'+status) as ClaudeRetryReason,routeRejected:false,retryAfterMs:retryAfterMs(text)});
  if (/\bECONNRESET\b|connection.*reset/i.test(text)) return new ClaudeError('NETWORK_ERROR', {reason:'ECONNRESET',routeRejected:false});
  if (/\bETIMEDOUT\b/i.test(text)) return new ClaudeError('NETWORK_ERROR', {reason:'ETIMEDOUT',routeRejected:false});
  if (/\bECONNREFUSED\b/i.test(text) && /(?:127\.0\.0\.1|\[::1\]|localhost):\d+\b/i.test(text))
    return new ClaudeError('NETWORK_ERROR', {reason:'GATEWAY_UNAVAILABLE',routeRejected:false});
  if (/ECONNREFUSED|ENOTFOUND|fetch failed|connection.*(?:closed|refused)|network|unable to connect|api.error.*500/i.test(text))
    return new ClaudeError('NETWORK_ERROR'); // Insufficient evidence of non-execution: not retryable.
  return new ClaudeError(resume ? 'SESSION_RESUME_FAILED' : 'PROCESS_EXIT_NONZERO');
}
export function safeClaudeText(text: string): string {
  return redact(text)
    .replace(/\bBearer\s+[A-Za-z0-9._~+\/-]{12,}=*/gi, "Bearer [已隐藏]")
    .replace(
      /\b([A-Z_]*(?:TOKEN|PASSWORD|SECRET|API_KEY|PRIVATE_KEY)[A-Z_]*\s*=\s*)(?:"[^"]*"|'[^']*'|[^\s,;]+)/gi,
      "$1[已隐藏]",
    )
    .replace(
      /\beyJ[A-Za-z0-9_-]{10,}\.[A-Za-z0-9_-]+\.[A-Za-z0-9_-]+/g,
      "[已隐藏令牌]",
    );
}
