import type { DevelopmentPlan, PlannedTask, TeamTaskStatus } from '../planning/planningTypes';
import type { ReviewReport } from '../review/reviewTypes';
import { digest } from '../review/reviewGit';
import { reviewBlock } from '../review/reviewBlock';
export interface ScheduledTask { task:PlannedTask; status:TeamTaskStatus; reason?:string; worker_id?:string; candidate?:unknown; review_id?:string; events:{time:string;from:TeamTaskStatus;to:TeamTaskStatus;reason?:string}[] }
export interface TaskExecutor { execute:(task:PlannedTask,signal:AbortSignal,onWait?:(waiting:boolean)=>Promise<void>)=>Promise<{worker_id:string;candidate:unknown}>; stop:(taskId:string)=>Promise<void> }
const terminal=new Set<TeamTaskStatus>(['COMPLETED','FAILED','CANCELLED']);
const transitions:Partial<Record<TeamTaskStatus,TeamTaskStatus[]>>={
 FAILED:['PAUSED'],
 PENDING:['READY','WAITING_DEPENDENCY','PAUSED','CANCELLED'],READY:['RUNNING','WAITING_DEPENDENCY','PAUSED','CANCELLED'],WAITING_DEPENDENCY:['READY','PAUSED','CANCELLED'],
 RUNNING:['REVIEWING','WAITING_FILE_LOCK','WAITING_APPROVAL','FAILED','PAUSED','CANCELLED'],WAITING_FILE_LOCK:['RUNNING','READY','WAITING_DEPENDENCY','PAUSED','CANCELLED'],WAITING_APPROVAL:['READY','REVIEWING','PAUSED','CANCELLED'],
 REVIEWING:['REPAIRING','WAITING_APPROVAL','WAITING_REVIEW','WAITING_INTEGRATION','FAILED','PAUSED','CANCELLED'],WAITING_REVIEW:['REVIEWING','PAUSED','CANCELLED'],REPAIRING:['REVIEWING','FAILED','PAUSED','CANCELLED'],WAITING_INTEGRATION:['COMPLETED','PAUSED','CANCELLED'],PAUSED:['PENDING','REVIEWING','WAITING_INTEGRATION','CANCELLED'],
};
/** Sole writer of Task07 task states. CLI completion is only a candidate, never task completion. */
export class TaskDependencyScheduler {
  private listeners = new Set<()=>void>();
  private wake(){for(const listener of this.listeners)listener();}
  private records=new Map<string,ScheduledTask>();private running=new Map<string,{abort:AbortController;promise:Promise<void>}>();private stopped=false;private saving:Promise<void>=Promise.resolve();private pumping=false;private pumpAgain=false;
  constructor(readonly plan:DevelopmentPlan,private executor:TaskExecutor,private save:(tasks:ScheduledTask[])=>Promise<void>,private changed:()=>void=()=>{}){
    if(!Number.isInteger(plan.max_concurrency)||plan.max_concurrency<1||plan.max_concurrency>8)throw Error('任务并发配置无效。');
    const ids=new Set(plan.tasks.map(t=>t.task_id));if(ids.size!==plan.tasks.length)throw Error('调度任务ID重复。');
    const visited=new Set<string>(),visiting=new Set<string>();const visit=(t:PlannedTask)=>{if(visiting.has(t.task_id))throw Error('循环依赖。');if(visited.has(t.task_id))return;visiting.add(t.task_id);for(const id of t.dependencies){const dep=plan.tasks.find(x=>x.task_id===id);if(!dep)throw Error('依赖任务不存在。');visit(dep);}visiting.delete(t.task_id);visited.add(t.task_id);};
    for(const task of plan.tasks){visit(task);this.records.set(task.task_id,{task:structuredClone(task),status:'PENDING',events:[]});}
  }
  snapshot(){return structuredClone([...this.records.values()]);}
  private persist(){const snapshot=this.snapshot(),run=this.saving.then(()=>this.save(snapshot));this.saving=run;return run.then(()=>{this.changed();this.wake();});}
  private async move(r:ScheduledTask,to:TeamTaskStatus,reason?:string){if(r.status===to)return;if(!transitions[r.status]?.includes(to))throw Error(`非法任务状态转换 ${r.status} → ${to}`);const from=r.status;r.status=to;r.task.status=to;r.reason=reason;r.events.push({time:new Date().toISOString(),from,to,reason});await this.persist();}
  async restore(records:ScheduledTask[]){
    if(records.length!==this.records.size||records.some(r=>!this.records.has(r.task.task_id)||digest(JSON.stringify({...r.task,status:'PENDING'}))!==digest(JSON.stringify({...this.records.get(r.task.task_id)!.task,status:'PENDING'}))))throw Error('持久调度计划与当前计划不一致。');
    for(const r of records){if(!transitions[r.status]&&!terminal.has(r.status))throw Error('调度状态损坏。');this.records.set(r.task.task_id,structuredClone(r));}
    this.stopped=true;for(const r of this.records.values())if(!terminal.has(r.status)&&r.status!=='PAUSED')await this.move(r,'PAUSED','插件重启后等待用户显式恢复；不自动执行旧任务或旧审批。');await this.persist();
  }
  async start(){this.stopped=false;await this.persist();await this.pump();}
  private async pump(){if(this.stopped)return;if(this.pumping){this.pumpAgain=true;return;}this.pumping=true;try{
    for(const r of this.records.values()){
      if(!['PENDING','READY','WAITING_DEPENDENCY','WAITING_FILE_LOCK'].includes(r.status))continue;
      const deps=r.task.dependencies.map(id=>this.records.get(id)!);
      if(deps.some(d=>['FAILED','CANCELLED','PAUSED'].includes(d.status))){await this.move(r,'PAUSED','上游失败、取消或暂停，停止相关下游。');continue;}
      if(deps.some(d=>!['WAITING_INTEGRATION','COMPLETED'].includes(d.status))){if(r.status!=='WAITING_DEPENDENCY')await this.move(r,'WAITING_DEPENDENCY','等待依赖候选通过独立审查。');continue;}
      if(this.running.has(r.task.task_id))continue;if(r.status!=='READY')await this.move(r,'READY');
    }
    for(const r of [...this.records.values()].sort((a,b)=>(a.task.priority??3)-(b.task.priority??3))){
      if(this.running.size>=this.plan.max_concurrency)break;if(r.status!=='READY'||this.running.has(r.task.task_id))continue;
      if([...this.records.values()].some(other=>other!==r&&other.task.assigned_worker===r.task.assigned_worker&&(this.running.has(other.task.task_id)||['REVIEWING','REPAIRING','WAITING_REVIEW','WAITING_APPROVAL'].includes(other.status))))continue;
      const abort=new AbortController();await this.move(r,'RUNNING');
      const promise=this.execute(r,abort);this.running.set(r.task.task_id,{abort,promise});void promise.finally(()=>{this.running.delete(r.task.task_id);this.wake();void this.pump().catch(()=>{this.stopped=true;});}).catch(()=>{});
    }
  }finally{this.pumping=false;this.wake();if(this.pumpAgain){this.pumpAgain=false;void this.pump().catch(()=>{this.stopped=true;});}}}
  private async execute(r:ScheduledTask,abort:AbortController){try{const result=await this.executor.execute(structuredClone(r.task),abort.signal,async waiting=>{if(!abort.signal.aborted)await this.move(r,waiting?'WAITING_FILE_LOCK':'RUNNING',waiting?'等待现有文件写锁释放':'文件写锁已获得');});if(abort.signal.aborted)return;r.worker_id=result.worker_id;r.candidate=result.candidate;await this.move(r,'REVIEWING','真实执行候选已收集，尚未通过GPT独立审查。');}catch(e){if(abort.signal.aborted)return;const reason=(e as Error).message;await this.move(r,reason.includes('WAITING_FILE_LOCK')?'WAITING_FILE_LOCK':reason.includes('WAITING_APPROVAL')?'WAITING_APPROVAL':'FAILED',reason);}}
  async reviewed(taskId:string,report:ReviewReport){
    const r=this.require(taskId);if(r.status!=='REVIEWING'||report.task.task_id!==taskId||report.stale)throw Error('审查与当前任务/候选不一致或已过期。');r.review_id=report.review_id;
    const failedTests=report.evidence?.tests.some(t=>t.status==='FAILED');
    if(report.verdict==='PASS'&&!report.missing_evidence.length&&!failedTests&&!report.issues.some(i=>['HIGH','CRITICAL'].includes(i.severity)))await this.move(r,'WAITING_INTEGRATION','候选审查通过，等待统一集成；不是主项目已完成。');
    else if(report.verdict==='BLOCKED'&&!failedTests){const block=reviewBlock(report)!;await this.move(r,'WAITING_REVIEW',block.kind+'：'+block.reasons.join('；'));}
    else if(report.verdict==='NEEDS_HUMAN_REVIEW'&&!failedTests)await this.move(r,'WAITING_APPROVAL','独立审查需要人工确认；'+(reviewBlock(report)?.reasons.join('；')||''));
    else await this.move(r,'FAILED',failedTests?'当前快照实际测试失败，需定向处理。':`独立审查${report.verdict}，需要定向处理。`);await this.pump();
  }
  async waiting(taskId:string,reason:string){await this.move(this.require(taskId),'WAITING_APPROVAL',reason);}
  async reviewUnavailable(taskId:string,reason:string){await this.move(this.require(taskId),'WAITING_REVIEW',reason);await this.pump();}
  async repair(taskId:string,run:()=>Promise<{worker_id:string;candidate:unknown}|void>){const r=this.require(taskId);if(r.status!=='REVIEWING')throw Error('当前任务不在审查阶段。');await this.move(r,'REPAIRING');try{const result=await run();if(this.require(taskId).status!=='REPAIRING')return;if(result){r.worker_id=result.worker_id;r.candidate=result.candidate;}await this.move(r,'REVIEWING');}catch(e){if(this.require(taskId).status==='REPAIRING')await this.move(r,'FAILED',(e as Error).message);throw e;}}
  async integrated(taskIds:string[],proof:{integration_id:string;review_id:string;tests_passed:boolean}){if(!proof.integration_id||!proof.review_id||!proof.tests_passed)throw Error('缺少真实集成/审查/测试证据。');for(const id of taskIds){const r=this.require(id);if(r.status!=='WAITING_INTEGRATION')throw Error('候选未通过独立审查。');}for(const id of taskIds)await this.move(this.require(id),'COMPLETED',`集成 ${proof.integration_id} 审查 ${proof.review_id}`);}
  private require(id:string){const r=this.records.get(id);if(!r)throw Error('任务不属于当前计划。');return r;}
  async pause(taskId?:string,cancel=false){if(!taskId)this.stopped=true;for(const r of this.records.values()){if(taskId&&r.task.task_id!==taskId||terminal.has(r.status))continue;const active=this.running.get(r.task.task_id);active?.abort.abort();await this.executor.stop(r.task.task_id);await active?.promise;await this.move(r,cancel?'CANCELLED':'PAUSED','用户停止；已确认执行退出，候选和Session保留。');}await this.pump();}
  async resume(taskId:string,reviewStillValid=false){const r=this.require(taskId);if(r.status==='FAILED')await this.move(r,'PAUSED','用户显式请求恢复失败任务；重新验证证据和权限，不重发已完成写入。');if(!['PAUSED','WAITING_APPROVAL','WAITING_REVIEW'].includes(r.status))throw Error('任务当前不能恢复。');if(r.status==='WAITING_REVIEW'&&!r.candidate)throw Error('缺少原候选，不能重新审查或自动重新开发。');await this.move(r,r.candidate&&reviewStillValid?'WAITING_INTEGRATION':r.candidate?'REVIEWING':r.status==='PAUSED'?'PENDING':'READY','显式恢复，重新检查范围和真实运行状态。');this.stopped=false;await this.pump();}
  async setConcurrency(value:number){if(!Number.isInteger(value)||value<1||value>8)throw Error('Concurrency must be 1..8');this.plan.max_concurrency=value;await this.persist();await this.pump();}
  async waitForReview(){while(!this.snapshot().some(t=>t.status==='REVIEWING')&&(this.running.size||this.pumping))await new Promise<void>(resolve=>{const done=()=>{this.listeners.delete(done);resolve();};this.listeners.add(done);});await this.saving;}
  async wait(){while(this.running.size||this.pumping){if(this.running.size)await Promise.allSettled([...this.running.values()].map(r=>r.promise));else await new Promise<void>(resolve=>{const done=()=>{this.listeners.delete(done);resolve();};this.listeners.add(done);});}await this.saving;}
}
