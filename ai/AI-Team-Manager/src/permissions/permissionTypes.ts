export interface TaskPermissionScope {
  allowedFiles: string[];
  safeCommands: string[];
}
export type PermissionChoice = 'ONCE' | 'TASK' | 'DENY';
export interface PermissionOperation {
  id: string; taskId: string; tool: string; operation: string; target: string;
  reason: string; preview?: string; fingerprint: string;
}
export interface PermissionLog {
  fingerprint?:string;toolUseId?:string;
  scope?: TaskPermissionScope;
  time: string; taskId: string; sessionId?: string; tool: string; operation: string;
  target: string; decision: 'ALLOW' | 'DENY'; source: string; reason: string;
}
export interface PermissionDecision {
  behavior: 'allow' | 'deny' | 'ask'; reason: string; operation: string; target: string; preview?: string;
}
export interface PermissionRun {
  /** False as soon as any authenticated permission/tool operation is observed; unknown implementations block retry. */
  canRetry?(): boolean | Promise<boolean>;
  settings: string;
  close(): Promise<void>;
}
