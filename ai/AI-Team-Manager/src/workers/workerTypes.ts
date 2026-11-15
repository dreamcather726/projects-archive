import type { ClaudeEvent, ClaudeSnapshot, ClaudeRetryEvent } from '../claude/claudeTypes';

export type WorkerStatus = 'IDLE' | 'QUEUED' | 'RUNNING' | 'RETRYING' | 'PAUSING' | 'PAUSED' | 'COMPLETED' | 'FAILED' | 'CANCELLED' | 'INTERRUPTED';
export interface WorkerRecord {
  worker_id: string;
  session_id?: string;
  project_id: string;
  task_id: string;
  workspace_path: string;
  branch_name: string;
  allowed_files: string[];
  status: WorkerStatus;
  retry?: ClaudeRetryEvent;
  execution_result?: { final_answer: string; error?: string; finished_at: string; request_id?: string };
  created_at: string;
  updated_at: string;
  unread: number;
  events: WorkerEvent[];
}
export interface WorkerEvent {
  sequence: number;
  time: string;
  project_id: string;
  task_id: string;
  worker_id: string;
  session_id?: string;
  event: ClaudeEvent | { type: 'lifecycle'; status: WorkerStatus; reason?: string };
}
export interface WorkerDetail { worker: WorkerRecord; chat: ClaudeSnapshot }
