import type { Usage, Conversation } from "../chat/chatTypes";
import type { ClaudeRetryReason } from './claudeErrors';
export interface ClaudeRetryEvent {
  type: 'retry';
  kind: 'RETRY_SCHEDULED'|'RETRY_STARTED'|'RETRY_SUCCEEDED'|'RETRY_EXHAUSTED'|'RETRY_CANCELLED'|'RETRY_BLOCKED_SIDE_EFFECT'|'RETRY_DEFERRED';
  requestId: string; time: string; retry: number; maxRetries: number;
  reason: ClaudeRetryReason; delayMs: number; nextRetryAt?: string; sessionId?: string;
  sideEffect?: boolean;
}
export type ClaudeEvent =
  | ClaudeRetryEvent
  | { type: 'execution'; kind: 'THINKING'|'UNKNOWN'|'CHILD_ACTIVITY' }
  | { type: 'usage'; usage: Usage }
  | { type: "session"; id: string }
  | {
      type: "text";
      id: string;
      text: string;
      completed: boolean;
      delta?: boolean;
    }
  | { type: "status"; message: string }
  | {
      type: "tool";
      id: string;
      name: string;
      phase: "running" | "completed" | "failed";
      detail?: string;
    }
  | {
      type: "result";
      ok: boolean;
      usage?: Usage;
      error?: string;
      denied: boolean;
    }
  | { type: "parseError" };
export interface ClaudeActivity {
  id: string;
  name: string;
  phase: "running" | "completed" | "failed";
  detail?: string;
}
export interface ClaudeSnapshot {
  projectPath: string;
  projectName?: string;
  connection: string;
  busy: boolean;
  error?: string;
  cli?: { command: string; source: string; version?: string };
  conversation?: Conversation;
  usage?: Usage;
  activities: ClaudeActivity[];
  statusEvent?: string;
  retry?: ClaudeRetryEvent;
  sessionCount: number;
  history: { id: string; title: string; status: string; updatedAt: string }[];
}
