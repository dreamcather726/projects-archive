import type { Usage } from "../chat/chatTypes";
export const categories = [
  "REQUIREMENTS",
  "ARCHITECTURE",
  "LOGIC",
  "SECURITY",
  "PERFORMANCE",
  "TESTING",
] as const;
export type Category = (typeof categories)[number];
export type Verdict =
  "PASS" | "REQUEST_CHANGES" | "BLOCKED" | "NEEDS_HUMAN_REVIEW";
export type Severity = "CRITICAL" | "HIGH" | "MEDIUM" | "LOW" | "INFO";
export interface SourceFile {
  path: string;
  hash: string;
  text?: string;
  omitted?: string;
  redacted?: boolean;
}
export interface SourceSnapshot {
  root: string;
  time: string;
  head: string;
  digest: string;
  files: SourceFile[];
  gaps: string[];
}
export interface ReviewTask {
  repair_context?: import('../repair/repairTypes').RepairReviewContext;
  task_id: string;
  task_description: string;
  technical_requirements: string;
  follow_up_instructions?: string[];
  allowed_files?: string[];
  forbidden_files?: string[];
  context_files?: string[];
}
export interface ClaudeDelivery {
  conversation_id?: string;
  session_id?: string;
  request_id?: string;
  status: string;
  final_answer: string;
}
export interface TestEvidence {
  id: string;
  command: string;
  started_at: string;
  finished_at: string;
  status:
    "PASSED" | "FAILED" | "CANCELLED" | "TIMEOUT" | "STALE" | "UNVERIFIED";
  test_count?: number;
  exit_code: number | null;
  output: string;
  output_truncated: boolean;
  source_digest: string;
}
export interface TaskCapture {
  id: string;
  task: ReviewTask;
  before: SourceSnapshot;
  delivery?: ClaudeDelivery;
}
export interface FileChange {
  path: string;
  kind: "ADDED" | "MODIFIED" | "DELETED";
  before?: SourceFile;
  after?: SourceFile;
  diff: string;
}
export interface ReviewEvidence {
  scope_proof?: SnapshotScopeProof;
  write_proof?: {task_id:string;worker_id:string;writes:{path:string;count:number;before_hash:string|null;after_hash:string;matches_snapshots:boolean}[]};
  workflow_context?: {
    reviewed_dependencies?: {task_id:string;review_id:string;source_digest:string;acceptance_criteria:string[];files:{path:string;hash:string;interfaces:string[]}[];tests:{command:string;status:string;test_count?:number}[]}[];
    stage:'WORKER_CANDIDATE'|'INTEGRATION_CANDIDATE';
    plan_id:string;task_id?:string;
    tasks:{task_id:string;status:string;events:unknown[]}[];
    workers:{worker_id:string;task_id:string;session_id?:string;status:string}[];
  };
  builds?: TestEvidence[];
  inspection_files?: SourceFile[];
  id: string;
  task: ReviewTask;
  delivery: ClaudeDelivery;
  baseline: "CLAUDE_CAPTURE" | "GIT_HEAD";
  before: SourceSnapshot;
  after: SourceSnapshot;
  changes: FileChange[];
  context: SourceFile[];
  tests: TestEvidence[];
  gaps: string[];
  warnings: string[];
}
export interface SnapshotScopeProof {
  before_digest:string;after_digest:string;binding_valid:boolean;before_files:number;after_files:number;
  complete:boolean;unchanged_files:number;
  changes:{path:string;before_hash:string|null;after_hash:string|null}[];
  outside_allowed:string[];protected_files:{path:string;before_hash:string|null;after_hash:string|null;unchanged:boolean}[];
}
export interface ReviewIssue {
  issue_id: string;
  severity: Severity;
  category: Category;
  file_path: string;
  line_number: number | null;
  basis: "before" | "after";
  problem_description: string;
  evidence: string;
  impact: string;
  suggested_fix: string;
  confidence: number;
  evidence_status: "VERIFIED" | "UNVERIFIED";
}
export interface DimensionResult {
  category: Category;
  status: "PASS" | "ISSUES" | "INSUFFICIENT_EVIDENCE" | "NEEDS_HUMAN_REVIEW";
  summary: string;
}
export interface BatchResult {
  repair_checks?: import('../repair/repairTypes').RepairCheck[];
  batch_id: string;
  session_id?: string;
  verdict: Verdict;
  summary: string;
  dimensions: DimensionResult[];
  issues: ReviewIssue[];
  missing_evidence: string[];
  usage?: Usage;
  error?: string;
}
export interface ReviewReport {
  version: 1;
  review_id: string;
  project_path: string;
  created_at: string;
  finished_at: string;
  task: ReviewTask;
  verdict: Verdict;
  summary: string;
  issues: ReviewIssue[];
  dimensions: DimensionResult[];
  missing_evidence: string[];
  warnings: string[];
  batches: BatchResult[];
  evidence: ReviewEvidence;
  stale: boolean;
}
