export type RequestStatus = 'RUNNING' | 'COMPLETED' | 'FAILED' | 'CANCELLED';
export interface Usage {
  input: number | null;
  cached: number | null;
  cacheWrite: number | null;
  output: number | null;
  reasoning: number | null;
  ordinaryInput: number | null;
  total: number | null;
}
export interface ChatMessage {
  id: string;
  role: 'user' | 'assistant';
  text: string;
  time: string;
  status: RequestStatus;
  requestId: string;
}
export interface RequestRecord {
  claudeRetries?: import('../claude/claudeTypes').ClaudeRetryEvent[];
  toolCalls?:{id:string;summary:string;completed:boolean}[];
  id: string;
  time: string;
  status: RequestStatus;
  usage?: Usage;
  usageSnapshot?: Usage;
  error?: string;
}
export interface Conversation {
  id: string;
  threadId?: string;
  projectPath: string;
  createdAt: string;
  updatedAt: string;
  status: 'READY' | RequestStatus;
  messages: ChatMessage[];
  requests: RequestRecord[];
}
export interface ConversationFile { version: 1; projectPath: string; activeId: string; conversations: Conversation[] }
export type CodexEvent =
  | { type: 'thread'; id: string }
  | { type: 'turn'; state: 'started' | 'completed' | 'failed'; error?: string }
  | { type: 'message'; id: string; text: string; completed: boolean }
  | { type: 'activity'; kind: 'command' | 'file' | 'mcp'; id: string; text: string; output?: string; exitCode?: number; completed: boolean }
  | { type: 'usage'; usage: Usage }
  | { type: 'error'; message: string }
  | { type: 'parseError' }
  | { type: 'unknown' };
