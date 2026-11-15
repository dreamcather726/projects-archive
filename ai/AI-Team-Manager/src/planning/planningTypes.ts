import type { Usage } from '../chat/chatTypes';

export interface TaskAnalysis {
  scale: 'SIMPLE' | 'MEDIUM' | 'LARGE' | 'EXTRA_LARGE';
  technical_domain: string;
  estimated_work: string;
  technical_complexity: 'LOW' | 'MEDIUM' | 'HIGH';
  modules: string[];
  expected_files: string[];
  dependency_reasoning: string;
  parallelizable: boolean;
  risks: string[];
  human_confirmation: string[];
  worker_rationale: string;
}

export type DevelopmentKind = 'SMALL_CHANGE' | 'MODULE_TASK' | 'MULTI_MODULE' | 'PROJECT_PHASE' | 'FULL_PROJECT';
export type TeamTaskStatus = 'PENDING' | 'READY' | 'RUNNING' | 'WAITING_DEPENDENCY' | 'WAITING_FILE_LOCK' |
  'WAITING_APPROVAL' | 'WAITING_REVIEW' | 'REVIEWING' | 'REPAIRING' | 'WAITING_INTEGRATION' | 'COMPLETED' | 'FAILED' | 'PAUSED' | 'CANCELLED';
export interface PlannedTask {
  task_name?: string;
  priority?: number;
  task_id: string;
  parent_task_id: string | null;
  task_description: string;
  assigned_worker: string;
  allowed_files: string[];
  read_only_files: string[];
  forbidden_files: string[];
  dependencies: string[];
  interface_contracts: string[];
  acceptance_criteria: string[];
  test_commands: string[];
  base_commit: string;
  status: TeamTaskStatus;
}
export interface PlanProposal {
  orchestration_version?: 2;
  analysis?: TaskAnalysis;
  evidence_id: string;
  task_type: DevelopmentKind;
  goal: string;
  current_project_state: string;
  technical_approach: string;
  worker_count: number;
  parallel_rationale: string;
  tasks: PlannedTask[];
  shared_file_owners: { file: string; task_id: string }[];
  out_of_scope: string[];
  completion_policy: 'STOP_AFTER_REQUEST' | 'CONTINUE_APPROVED_PLAN';
}
export interface PlanningContext {
  orchestration_limits?: { max_workers: number; max_repair_rounds: number };
  evidence_id: string;
  project_id: string;
  project_path: string;
  owner_id: string;
  turn_id: string;
  original_request: string;
  captured_at: string;
  base_commit: string;
  source_digest: string;
  tracked_files: string[];
  file_hashes: { path: string; hash: string }[];
  dirty_files: string[];
  contexts: { path: string; text: string }[];
  gaps: string[];
  continuous_authorized: boolean;
  previous_context?:unknown;
}
export interface DevelopmentPlan extends PlanProposal {
  worker_timeout_ms?: number;
  max_repair_rounds?: number;
  version: 1;
  plan_id: string;
  project_id: string;
  project_path: string;
  owner_id: string;
  original_request: string;
  source_digest: string;
  created_at: string;
  validation: 'VALIDATED_NOT_EXECUTED';
  user_approval_required: boolean;
  max_concurrency: number;
  gpt_usage?: Usage;
}
