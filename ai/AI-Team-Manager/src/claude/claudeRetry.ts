import { ClaudeError, type ClaudeRetryHint } from './claudeErrors';
import type { ClaudeRetryEvent } from './claudeTypes';

export interface ClaudeRetryPolicy { maxRetries: number; baseDelayMs: number }
export const DEFAULT_CLAUDE_RETRY: ClaudeRetryPolicy = {maxRetries:3,baseDelayMs:2000};
export const MAX_RETRY_DELAY_MS = 60000;
export function claudeRetryPolicy(value?: Partial<ClaudeRetryPolicy>): ClaudeRetryPolicy {
  return {
    maxRetries: Number.isInteger(value?.maxRetries) ? Math.max(0,Math.min(5,value!.maxRetries!)) : 3,
    baseDelayMs: Number.isInteger(value?.baseDelayMs) ? Math.max(100,Math.min(10000,value!.baseDelayMs!)) : 2000,
  };
}
/** Retry-After is a minimum. Too long/unknown 429 delays stop rather than violate the server's limit. */
export function claudeRetryDelay(hint: ClaudeRetryHint, retry: number, policy: ClaudeRetryPolicy, random = Math.random): number | undefined {
  if (hint.reason === 'HTTP_429' && hint.retryAfterMs === undefined) return undefined;
  if (hint.retryAfterMs !== undefined && (!Number.isFinite(hint.retryAfterMs) || hint.retryAfterMs < 0 || hint.retryAfterMs > MAX_RETRY_DELAY_MS)) return undefined;
  const backoff = Math.min(MAX_RETRY_DELAY_MS, policy.baseDelayMs * 2 ** (retry - 1));
  const jittered = Math.min(MAX_RETRY_DELAY_MS, Math.ceil(backoff * (1 + Math.max(0,Math.min(1,random())) * 0.25)));
  return Math.max(jittered, hint.retryAfterMs || 0);
}
export function waitForClaudeRetry(ms: number, signal: AbortSignal): Promise<void> {
  return new Promise((resolve,reject) => {
    if (signal.aborted) { reject(new ClaudeError('REQUEST_CANCELLED')); return; }
    const abort = () => { clearTimeout(timer); signal.removeEventListener('abort',abort); reject(new ClaudeError('REQUEST_CANCELLED')); };
    const timer = setTimeout(() => { signal.removeEventListener('abort',abort); resolve(); },ms);
    signal.addEventListener('abort',abort,{once:true});
  });
}
export function claudeRetryReasonLabel(reason: ClaudeRetryHint['reason']): string {
  return {HTTP_502:'HTTP 502：上游网关错误',HTTP_503:'HTTP 503：上游服务暂不可用',HTTP_504:'HTTP 504：上游网关超时',HTTP_429:'HTTP 429：请求限流',NO_AVAILABLE_CHANNEL:'模型通道暂不可用',ECONNRESET:'连接被重置',ETIMEDOUT:'连接超时',GATEWAY_UNAVAILABLE:'临时网关连接失败'}[reason];
}
export function claudeRetryMessage(event: ClaudeRetryEvent): string {
  const count = `${event.retry}/${event.maxRetries}`, reason = claudeRetryReasonLabel(event.reason);
  switch (event.kind) {
    case 'RETRY_SCHEDULED': return `${reason} · 自动重试 ${count} · ${Math.ceil(event.delayMs/1000)} 秒后重试；原 Session、任务及权限保持不变。`;
    case 'RETRY_STARTED': return `${reason} · 正在重试 ${count}；沿用原会话。`;
    case 'RETRY_SUCCEEDED': return 'Claude 已恢复连接，继续原任务。';
    case 'RETRY_EXHAUSTED': return '已达到最大自动重试次数，原 Session、候选文件和任务记录均已保留。';
    case 'RETRY_CANCELLED': return '自动重试已取消；不会启动下一次 CLI 请求。';
    case 'RETRY_BLOCKED_SIDE_EFFECT': return '已停止自动重试：检测到执行活动、文件变化或无法确认无副作用；保留候选，需审查或人工处理。';
    case 'RETRY_DEFERRED': return '未自动重试：服务端 Retry-After 缺失、无效或超过等待上限，请稍后手动恢复。';
  }
}
