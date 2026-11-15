import type { Usage } from '../chat/chatTypes';
import type { ReviewIssue, ReviewReport, SourceSnapshot, ReviewEvidence, TestEvidence } from '../review/reviewTypes';
export type ResolutionStatus = 'FIXED' | 'NOT_FIXED' | 'REGRESSION' | 'UNVERIFIED';
export interface RepairCheck { issue_id: string; status: ResolutionStatus; explanation: string; file_path: string; line_number: number | null; evidence: string; }
export interface RepairPlan {
  repair_id: string; review_id: string; task_id: string; issue_ids: string[];
  title: string; problem_description: string; root_cause: string;
  target_files: string[]; allowed_files: string[]; forbidden_files: string[];
  repair_strategy: string; acceptance_criteria: string[]; test_requirements: string[];
  risk_level: 'LOW' | 'MEDIUM' | 'HIGH'; delete_files: string[]; public_interface_changes: string[];
  missing_context: string[];
}
export type RepairState = 'DRAFT' | 'DEFERRED' | 'REJECTED' | 'SENDING' | 'EXECUTED' | 'FAILED' | 'VERIFYING' | 'FINISHED' | 'INTERRUPTED';
export interface RepairEvent { time: string; kind: string; detail: unknown; }
export interface RepairRecord {
  version: 1; repair_id: string; review_id: string; task_id: string; project_path: string;
  created_at: string; updated_at: string; state: RepairState;
  original_issues: ReviewIssue[]; plan?: RepairPlan; plan_hash?: string; before: SourceSnapshot;
  requirements: string; original_session?: string; original_conversation?: string;
  approvals: { time: string; actor: string; decision: string; plan_hash: string; source_digest: string; risks: string[]; }[];
  events: RepairEvent[]; event_gaps: string[]; received_at?: string; started_at?: string; finished_at?: string;
  sent_once: boolean; request_id?: string; session_id?: string; final_answer?: string; error?: string;
  evidence?: ReviewEvidence; checks: TestEvidence[];
  verification_review_id?: string; verification?: RepairCheck[]; verification_summary?: string;
  gpt_calls: { phase: string; session_id?: string; usage?: Usage; error?: string; }[];
}
export interface IssueDisposition { review_id: string; issue_id: string; disposition: 'OPEN' | 'IGNORED' | 'HUMAN'; time: string; }
export interface RepairReviewContext { repair_id: string; review_id: string; issues: ReviewIssue[]; plan: RepairPlan; }
export type OriginalReport = ReviewReport;
