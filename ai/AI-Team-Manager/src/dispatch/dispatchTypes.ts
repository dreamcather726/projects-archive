import type { SourceSnapshot, ReviewReport, TestEvidence, ReviewEvidence } from '../review/reviewTypes';
import type { Usage } from '../chat/chatTypes';
export type ApprovalMode = 'MANUAL'|'GPT_HYBRID'|'GPT_SCOPED_AUTO';
export interface DispatchInput {
  description:string; allowed_files:string[]; forbidden_files:string[];
  test_commands:string[]; acceptance_criteria:string[];
  expected_commands?:string[];
}
export interface ExecutionPlan { goal:string; approach:string; files:string[]; steps:string[]; commands:string[]; risks:string[] }
export interface ApprovalRequest {
  approval_id:string;task_id:string;session_id:string;request_type:'PLAN'|'TOOL';tool_name:string;
  tool_arguments:unknown;working_directory:string;target_files:string[];command:string;
  allowed_files:string[];forbidden_files:string[];task_description:string;risk_level:'L0'|'L1'|'L2'|'L3';evidence:unknown;
}
export interface ApprovalDecision {
  decision:'APPROVE'|'DENY'|'NEEDS_HUMAN'|'REVISE';risk_level:'L0'|'L1'|'L2'|'L3';reason:string;
  approved_scope:string[];restrictions:string[];expires_at:string;
}
export type DispatchState='PLANNING'|'APPROVING'|'RUNNING'|'REVIEWING'|'CORRECTING'|'PAUSED'|'PASSED'|'FAILED'|'CANCELLED'|'INTERRUPTED';
export interface DispatchEvent {time:string;kind:string;data:any}
export interface DispatchRound {round:number;plan:ExecutionPlan;before:SourceSnapshot;after?:SourceSnapshot;evidence?:ReviewEvidence;tests:TestEvidence[];review?:ReviewReport;repair_id?:string;verification?:import('../repair/repairTypes').RepairCheck[]}
export interface DispatchRecord {
  version:1;task_id:string;owner_id:string;project_path:string;user_request:string;input:DispatchInput;
  state:DispatchState;created_at:string;updated_at:string;conversation_id?:string;session_id?:string;
  mode:ApprovalMode;max_rounds:number;rounds:DispatchRound[];events:DispatchEvent[];
  gpt_calls:{phase:string;session_id?:string;usage?:Usage}[];error?:string;final_answer?:string;
}
export const terminal=(s:DispatchState)=>['PASSED','FAILED','CANCELLED','INTERRUPTED','PAUSED'].includes(s);
