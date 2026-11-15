import { digest } from '../review/reviewGit';
import type { ScheduledTask } from '../scheduling/taskDependencyScheduler';
import type { TaskCandidate } from '../scheduling/teamTaskExecutor';
import type { ReviewReport } from '../review/reviewTypes';
import { reviewBlock } from '../review/reviewBlock';
import { claudeRetryMessage } from '../claude/claudeRetry';
import type { WorkerEvent, WorkerRecord } from '../workers/workerTypes';

export type SupervisionDecision = 'CONTINUE'|'WAIT_DEPENDENCY'|'REQUIRE_INFO'|'REPAIR'|'PAUSE'|'TERMINATE'|'HUMAN';
export interface SupervisionEvent { id:string; time:string; task_id:string; worker_id?:string; kind:string; decision:SupervisionDecision; reason:string; evidence:string[]; model_review_id?:string; retry?:{reason:string;attempt:number;max_retries:number;delay_ms:number;side_effect?:boolean} }
export interface RepairTask { repair_id:string; task_id:string; worker_id:string; issue_id:string; round:number; allowed_files:string[]; description:string; acceptance:string; review_id:string; before_digest:string; after_digest?:string; status:'PENDING'|'RUNNING'|'PASSED'|'FAILED'|'WAITING_APPROVAL'; test_ids?:string[]; verification_reviews?:{review_id:string;verdict:string}[] }

/** Durable key events only. Streaming text never triggers a new model request. */
export function collectSupervision(previous:SupervisionEvent[], tasks:ScheduledTask[], candidates:TaskCandidate[], reviews:Record<string,ReviewReport[]>,workerEvents:WorkerEvent[]=[]):SupervisionEvent[]{
  const events=[...previous],ids=new Set(events.map(e=>e.id));
  const add=(key:string,event:Omit<SupervisionEvent,'id'>)=>{const id=digest(key);if(!ids.has(id)){ids.add(id);events.push({id,...event});}};
  for(const t of tasks)for(const [index,e] of t.events.entries()){
    const decision:SupervisionDecision=e.to==='WAITING_DEPENDENCY'||e.to==='WAITING_FILE_LOCK'?'WAIT_DEPENDENCY':e.to==='WAITING_APPROVAL'?'HUMAN':e.to==='WAITING_REVIEW'?'REQUIRE_INFO':e.to==='PAUSED'?'PAUSE':e.to==='CANCELLED'?'TERMINATE':e.to==='FAILED'?'REQUIRE_INFO':e.to==='REPAIRING'?'REPAIR':'CONTINUE';
    add(`task:${t.task.task_id}:${index}`,{time:e.time,task_id:t.task.task_id,worker_id:t.worker_id,kind:e.to,decision,reason:e.reason||`Scheduler transition ${e.from} → ${e.to}`,evidence:[`schedule:${t.task.task_id}:${index}`]});
  }
  for(const a of candidates)for(const [index,raw] of a.audit.entries()){
    const entry=(raw as any)?.entry,kind=entry?.kind||(entry?.decision==='DENY'?'PERMISSION_DENIED':undefined);
    if(!kind||!/FILE_WRITE|TEST_RESULT|DENIED|FAIL|ERROR/.test(kind)&&entry?.decision!=='DENY')continue;
    const failedTest=kind==='ISOLATED_TEST_RESULT'&&entry.data?.exit_code!==0;
    add(`audit:${a.task_id}:${index}`,{time:entry.time||a.after?.time||a.before.time,task_id:a.task_id,worker_id:a.worker_id,kind,decision:/DENIED/.test(kind)||entry.decision==='DENY'?'HUMAN':failedTest?'REPAIR':/FAIL|ERROR/.test(kind)?'REQUIRE_INFO':'CONTINUE',reason:failedTest?'Actual isolated test failed; bounded scoped repair and independent review required':'Observed authenticated execution evidence; code/test outcomes are independently reviewed',evidence:[`candidate:${a.task_id}:audit:${index}`]});
  }
  for(const [task,reports] of Object.entries(reviews))for(const r of reports)add(`review:${r.review_id}`,{time:r.finished_at,task_id:task,worker_id:candidates.find(a=>a.task_id===task)?.worker_id,kind:'GPT_REVIEW',decision:r.verdict==='PASS'?'CONTINUE':r.verdict==='REQUEST_CHANGES'?'REPAIR':r.verdict==='NEEDS_HUMAN_REVIEW'?'HUMAN':'REQUIRE_INFO',reason:r.summary,evidence:[r.evidence.id,...r.evidence.tests.map(t=>t.id)],model_review_id:r.review_id});
  for(const raw of workerEvents){
    const e=raw.event;if(e.type!=='retry')continue;
    add(`retry:${raw.worker_id}:${e.requestId}:${raw.sequence}`,{time:e.time,task_id:raw.task_id,worker_id:raw.worker_id,kind:e.kind,decision:e.kind==='RETRY_EXHAUSTED'||e.kind==='RETRY_BLOCKED_SIDE_EFFECT'||e.kind==='RETRY_DEFERRED'?'REQUIRE_INFO':e.kind==='RETRY_CANCELLED'?'PAUSE':'CONTINUE',reason:claudeRetryMessage(e),evidence:[`worker:${raw.worker_id}:event:${raw.sequence}`],retry:{reason:e.reason,attempt:e.retry,max_retries:e.maxRetries,delay_ms:e.delayMs,side_effect:e.sideEffect}});
  }
  return events;
}

/** Current backend state counts; the UI must not turn historical events into progress. */
export function teamRetryStats(tasks:ScheduledTask[],workers:Pick<WorkerRecord,'task_id'|'status'|'retry'>[]){
  const current=workers.filter(w=>tasks.some(t=>t.task.task_id===w.task_id));
  return {
    running:current.filter(w=>w.status==='RUNNING').length,
    retrying:current.filter(w=>w.status==='RETRYING').length,
    recovered:current.filter(w=>w.retry?.kind==='RETRY_SUCCEEDED').length,
    exhausted:current.filter(w=>w.retry?.kind==='RETRY_EXHAUSTED').length,
    needs_attention:tasks.filter(t=>['WAITING_APPROVAL','WAITING_REVIEW','FAILED','PAUSED'].includes(t.status)).length,
    delivered:tasks.filter(t=>t.status==='COMPLETED').length,
    total_tasks:tasks.length,
  };
}

export function reviewSummary(report:ReviewReport,worker_id?:string){
  const severity=['CRITICAL','HIGH','MEDIUM','LOW','INFO'].find(s=>report.issues.some(i=>i.severity===s))||'INFO';
  const tests=report.evidence.tests.map(t=>({command:t.command,status:t.status,test_count:t.test_count,source_digest:t.source_digest}));
  return {review_id:report.review_id,task_id:report.task.task_id,worker_id,reviewed_files:report.evidence.changes.map(c=>c.path),findings:report.issues,severity,evidence:[report.evidence.id,...report.evidence.tests.map(t=>t.id)],tests,suggested_fix:report.issues.map(i=>i.suggested_fix),review_status:report.stale?'FAILED':report.verdict==='PASS'?(report.issues.length?'CONDITIONAL_PASS':'PASS'):report.verdict==='REQUEST_CHANGES'?'REPAIR_REQUIRED':report.verdict==='NEEDS_HUMAN_REVIEW'?'WAITING_HUMAN':'WAITING_REVIEW',block:reviewBlock(report),verdict:report.verdict,summary:report.summary,dimensions:report.dimensions,warnings:report.warnings,issues:report.issues,missing_evidence:report.missing_evidence,batches:report.batches.map(b=>({session_id:b.session_id,summary:b.summary,error:b.error,missing_evidence:b.missing_evidence,usage:b.usage}))};
}
