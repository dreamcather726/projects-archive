import * as path from 'node:path';
import { ClaudeService } from '../claude/claudeService';
import { CodexService } from '../services/codexService';
import { TeamTaskExecutor } from '../scheduling/teamTaskExecutor';
import { TaskDependencyScheduler, type ScheduledTask } from '../scheduling/taskDependencyScheduler';
import { ReviewGit } from '../review/reviewGit';
import { ReviewStorage } from '../review/reviewStorage';
import { ReviewEngine } from '../review/reviewEngine';
import type { ReviewEvidence, ReviewReport } from '../review/reviewTypes';
import type { DevelopmentPlan } from '../planning/planningTypes';
import type { PermissionChoice, PermissionOperation } from '../permissions/permissionTypes';
import { TokenLedger, type GPTPurpose } from '../usage/tokenLedger';
import { collectSupervision, reviewSummary, teamRetryStats, type SupervisionEvent, type RepairTask } from './teamSupervision';
import { randomUUID } from 'node:crypto';
import { snapshotScopeProof, executionWriteProof } from '../review/reviewProof';

import type { ClaudeRetryPolicy } from '../claude/claudeRetry';
import { safeReviewText } from '../review/reviewSafety';
import { buildTaskProgress } from './taskProgress';
import type { IntegrationRecord } from '../integration/integrationManager';
export interface TeamServices { claude:()=>ClaudeService; codex:()=>CodexService; claudeRetryPolicy?:()=>Partial<ClaudeRetryPolicy> }
export interface TeamRunRecord {plan_id:string;started_at:string;finished_at?:string;error?:string;reviews:Record<string,ReviewReport[]>;supervision?:SupervisionEvent[];repairs?:RepairTask[];integration_id?:string;delivered?:boolean}

/** Drives actual isolated candidates through independent GPT review; never writes the user workspace. */
export class TeamWorkflow {
  readonly executor:TeamTaskExecutor;
  readonly scheduler:TaskDependencyScheduler;
  readonly storage:ReviewStorage;
  readonly collector:ReviewGit;
  private record:TeamRunRecord;
  private driving?:Promise<void>;
  private reviewer?:ReviewEngine;
  private reviewing?:string;
  private repairAbort?:AbortController;
  private disposed=false;
  constructor(readonly plan:DevelopmentPlan,readonly directory:string,private services:TeamServices,readonly ledger:TokenLedger,
    approve:(op:PermissionOperation,signal:AbortSignal)=>Promise<PermissionChoice>,private changed:()=>void=()=>{}){
    this.storage=new ReviewStorage(directory);this.collector=new ReviewGit(path.join(directory,'evidence'));
    this.record={plan_id:plan.plan_id,started_at:new Date().toISOString(),reviews:{}};
    this.executor=new TeamTaskExecutor(plan,path.join(directory,'execution'),services.claude,approve,changed,services.claudeRetryPolicy);
    this.scheduler=new TaskDependencyScheduler(plan,this.executor,t=>this.storage.write(plan.project_path,'schedule.json',t),changed);
  }
  async load(){
    const record=await this.storage.read<TeamRunRecord>(this.plan.project_path,'team-run.json');
    if(record){if(record.plan_id!==this.plan.plan_id||!record.reviews)throw Error('团队运行记录与当前计划不一致。');this.record=record;}
    await this.executor.load();
    const tasks=await this.storage.read<ScheduledTask[]>(this.plan.project_path,'schedule.json');if(tasks)await this.scheduler.restore(tasks);
  }
  private async persist(){this.record.supervision=collectSupervision(this.record.supervision||[],this.scheduler.snapshot(),this.executor.list(),this.record.reviews,this.executor.workers.retryEvents());await this.storage.write(this.plan.project_path,'team-run.json',this.record);this.changed();}
  reviewHistory(taskId:string){return structuredClone(this.record.reviews[taskId]||[]);}
  progress(integration?:Readonly<IntegrationRecord>){return buildTaskProgress(this.plan,this.scheduler.snapshot(),this.executor.progressCandidates(),this.executor.workers.list(),this.record.reviews,this.executor.workers.progressEvents(),integration,this.record.error);}
  snapshot(){const tasks=this.scheduler.snapshot().map(t=>({...t,candidate:undefined,events:t.events.slice(-50)}));const record={...this.record,supervision:collectSupervision(this.record.supervision||[],this.scheduler.snapshot(),this.executor.list(),this.record.reviews,this.executor.workers.retryEvents()),reviews:Object.fromEntries(Object.entries(this.record.reviews).map(([id,reports])=>[id,reports.map(r=>reviewSummary(r,this.executor.list().find(a=>a.task_id===id)?.worker_id))]))};return {plan:this.plan,tasks,workers:this.executor.workers.list(),retry_stats:teamRetryStats(tasks,this.executor.workers.list()),record,busy:!!this.driving,reviewing:this.reviewing,usage:this.ledger.snapshot(this.plan.plan_id),candidates:this.executor.list().map(a=>({task_id:a.task_id,worker_id:a.worker_id,workspace:a.workspace,files:a.evidence?.changes.map(c=>({path:c.path,kind:c.kind})),tests:a.evidence?.tests.map(t=>({...t,output:t.output.slice(-6000)})),autonomous_repairs:a.autonomous_repairs||0,review_repairs:a.review_repairs||0,local_approvals:a.local_approvals,error:a.error})),ready_for_integration:tasks.length>0&&tasks.every(t=>t.status==='WAITING_INTEGRATION')};}
  async start(){
    if(this.disposed)throw Error('团队运行已关闭。');
    if(this.driving)return;
    if((await this.collector.snapshot(this.plan.project_path)).digest!==this.plan.source_digest)throw Error('主项目已经变化，需重新采集并规划；不会覆盖用户修改。');
    await this.persist();await this.scheduler.start();this.launch();
  }
  private launch(){if(this.driving||this.disposed)return;const run=this.drive().catch(async e=>{this.record.error=(e as Error).message;await this.persist();});this.driving=run;void run.finally(()=>{this.driving=undefined;this.changed();});}
  private async drive(){
    for(;;){
      await this.scheduler.waitForReview();if(this.disposed)return;
      const row=this.scheduler.snapshot().find(t=>t.status==='REVIEWING');if(!row)break;
      this.reviewing=row.task.task_id;this.changed();
      const candidate=this.executor.candidate(row.task.task_id);if(!candidate.evidence)throw Error('候选缺少实际代码与测试证据。');
      if(this.ledger.snapshot().blocked){await this.scheduler.waiting(row.task.task_id,'GPT预算或未知用量需要用户处理，不能跳过审查。');continue;}
      const history=this.record.reviews[row.task.task_id]||[],purpose=history.length?'REREVIEW':'REVIEW';
      const previous=history.at(-1);
      if(previous?.verdict==='REQUEST_CHANGES'&&previous.evidence.after.digest===candidate.after?.digest){for(const repair of this.record.repairs||[])if(repair.task_id===row.task.task_id&&repair.status==='RUNNING'){repair.status='FAILED';repair.after_digest=candidate.after?.digest;}await this.persist();await this.scheduler.reviewed(row.task.task_id,previous);continue;}
      const reviewTask={...candidate.evidence.task,context_files:row.task.read_only_files};
      const evidence=previous?await this.collector.evidence(previous.verdict==='REQUEST_CHANGES'?previous.evidence.after:candidate.before,candidate.evidence.after,reviewTask,candidate.evidence.delivery,candidate.evidence.tests,'CLAUDE_CAPTURE'):candidate.evidence;
      if(previous?.issues.length)evidence.task={...evidence.task,follow_up_instructions:['Verify all previous review findings against this change: '+JSON.stringify(previous.issues)]};
      let report:ReviewReport;
      try{report=await this.review(evidence,purpose,row.task.task_id);}
      catch(error){await this.scheduler.reviewUnavailable(row.task.task_id,'审查准备受阻：'+safeReviewText((error as Error).message).slice(0,1000));await this.persist();continue;}
      if(this.scheduler.snapshot().find(t=>t.task.task_id===row.task.task_id)?.status!=='REVIEWING')continue;
      const latest=this.executor.candidate(row.task.task_id);
      if(report.evidence.after.digest!==latest.after?.digest||report.stale)throw Error('审查未绑定当前Worker候选，停止交付。');
      history.push(report);this.record.reviews[row.task.task_id]=history;await this.persist();
      if(this.ledger.snapshot().blocked&&report.verdict!=='PASS'){await this.scheduler.waiting(row.task.task_id,'GPT用量或预算门禁已触发。');continue;}
      for(const repair of this.record.repairs||[])if(repair.task_id===row.task.task_id&&(repair.status==='RUNNING'||repair.after_digest===candidate.after?.digest)){repair.status=report.verdict==='PASS'?'PASSED':'FAILED';repair.after_digest=candidate.after?.digest;repair.test_ids=candidate.evidence.tests.map(t=>t.id);repair.verification_reviews=[...(repair.verification_reviews||[]),{review_id:report.review_id,verdict:report.verdict}];}
      await this.persist();
      if(['REQUEST_CHANGES','BLOCKED'].includes(report.verdict)&&(candidate.review_repairs||0)+(candidate.autonomous_repairs||0)<(this.plan.max_repair_rounds??2)&&report.issues.some(i=>i.evidence_status==='VERIFIED')){
        const issues=report.issues.filter(i=>i.evidence_status==='VERIFIED');
        const repairs:RepairTask[]=issues.map(i=>({repair_id:randomUUID(),task_id:row.task.task_id,worker_id:candidate.worker_id,issue_id:i.issue_id,round:(candidate.review_repairs||0)+1,allowed_files:[i.file_path],description:i.problem_description,acceptance:i.suggested_fix,review_id:report.review_id,before_digest:candidate.after!.digest,status:this.executor.mayRepair(row.task.task_id,i.file_path)?'RUNNING':'WAITING_APPROVAL'}));
        this.record.repairs=[...(this.record.repairs||[]),...repairs];await this.persist();
        if(repairs.some(r=>r.status==='WAITING_APPROVAL')){await this.scheduler.waiting(row.task.task_id,'Repair exceeds original file ownership; explicit new authorization required');continue;}
        this.repairAbort=new AbortController();
        try{await this.scheduler.repair(row.task.task_id,()=>this.executor.repair(row.task,this.repairAbort!.signal,JSON.stringify({review_id:report.review_id,issues:report.issues,missing_evidence:report.missing_evidence})));}
        catch(e){for(const repair of repairs)if(repair.status==='RUNNING')repair.status='FAILED';await this.persist();throw e;}
        this.repairAbort=undefined;continue;
      }
      await this.scheduler.reviewed(row.task.task_id,report);
    }
    this.reviewing=undefined;this.record.finished_at=new Date().toISOString();await this.persist();
  }
  async review(evidence:ReviewEvidence,purpose:GPTPurpose,taskId?:string){
    if(this.reviewer?.busy)throw Error('已有团队独立审查运行中。');
    this.reviewer=new ReviewEngine(this.services.codex(),this.collector,this.storage,this.changed,240000,{
      begin:id=>this.ledger.begin(id,purpose,this.plan.plan_id,taskId),
      finish:(id,status,usage)=>this.ledger.finish(id,status,usage),
    });
    const workflow_context:NonNullable<ReviewEvidence['workflow_context']>={stage:purpose==='INTEGRATION_REVIEW'?'INTEGRATION_CANDIDATE':'WORKER_CANDIDATE',plan_id:this.plan.plan_id,task_id:taskId,
      tasks:this.scheduler.snapshot().map(t=>({task_id:t.task.task_id,status:t.status,events:t.events.slice(-10)})),
      workers:this.executor.workers.list().map(w=>({worker_id:w.worker_id,task_id:w.task_id,session_id:w.session_id,status:w.status}))};
    if(taskId){
      const candidate=this.executor.candidate(taskId),task=this.plan.tasks.find(t=>t.task_id===taskId)!;
      if(!candidate.after||candidate.after.digest!==evidence.after.digest)throw Error('Review candidate version mismatch');
      evidence={...evidence,scope_proof:snapshotScopeProof(candidate.before,candidate.after,{...evidence.task,allowed_files:task.allowed_files,forbidden_files:task.forbidden_files}),write_proof:executionWriteProof(candidate.before,candidate.after,taskId,candidate.worker_id,candidate.audit)};
      if(!evidence.scope_proof!.complete||evidence.scope_proof!.outside_allowed.length||evidence.scope_proof!.protected_files.some(f=>!f.unchanged))throw Error('候选变更范围证据不完整或包含越界修改，停止审查。');
      workflow_context.reviewed_dependencies=[];
      for(const id of this.plan.tasks.find(t=>t.task_id===taskId)?.dependencies||[]){
        const dependency=this.executor.candidate(id),review=this.reviewHistory(id).at(-1),task=this.plan.tasks.find(t=>t.task_id===id)!;
        if(!dependency.evidence||!review||review.verdict!=='PASS'||review.stale||review.evidence.after.digest!==dependency.after?.digest)throw Error('Dependency review proof invalid');
        const files=dependency.evidence.changes.filter(c=>c.after&&evidence.after.files.some(f=>f.path===c.path)).map(c=>{const actual=evidence.after.files.find(f=>f.path===c.path)!;if(actual.hash!==c.after!.hash)throw Error('Dependency source differs from reviewed version');return {path:c.path,hash:actual.hash,interfaces:(actual.text||'').split('\n').filter(line=>/^(?:async\s+)?(?:def |class |export |interface |type )/.test(line)).slice(0,50)};});
        workflow_context.reviewed_dependencies.push({task_id:id,review_id:review.review_id,source_digest:dependency.after!.digest,acceptance_criteria:task.acceptance_criteria,files,tests:dependency.evidence.tests.map(t=>({command:t.command,status:t.status,test_count:t.test_count}))});
      }
    }
    return this.reviewer.run({...evidence,workflow_context});
  }
  async pause(taskId?:string,cancel=false){
    if(!taskId||taskId===this.reviewing){this.repairAbort?.abort();await this.reviewer?.stop();}
    await this.scheduler.pause(taskId,cancel);await this.persist();
  }
  async resume(taskId:string){if(this.disposed)throw Error('团队运行已关闭。');this.record.error=undefined;const original=this.plan.tasks.find(t=>t.task_id===taskId);if(!original)throw Error('Recovery task outside plan');await this.executor.verifyRecovery(original);const row=this.scheduler.snapshot().find(t=>t.task.task_id===taskId);let reviewed=false;if(row?.status==='PAUSED'&&row.events.at(-1)?.from==='WAITING_INTEGRATION'){const a=this.executor.candidate(taskId),r=this.reviewHistory(taskId).at(-1);reviewed=!!a.after&&r?.verdict==='PASS'&&!r.stale&&r.evidence.after.digest===a.after.digest&&(await this.collector.snapshot(a.workspace.workspace_path)).digest===a.after.digest;if(!reviewed)throw Error('候选在暂停后已改变，旧审查不能复用。');}await this.scheduler.resume(taskId,reviewed);this.launch();}
  async wait(){await this.driving;}
  async markIntegrated(id:string,reviewId:string){await this.scheduler.integrated(this.plan.tasks.map(t=>t.task_id),{integration_id:id,review_id:reviewId,tests_passed:true});this.record.integration_id=id;this.record.delivered=true;await this.persist();}
  async dispose(){this.disposed=true;this.repairAbort?.abort();await this.reviewer?.stop();await this.scheduler.pause();await this.executor.dispose();await this.driving;}
}
