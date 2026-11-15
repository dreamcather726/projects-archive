import * as path from 'node:path';
import { TeamWorkflow, type TeamServices } from './teamWorkflow';
import { IntegrationManager } from '../integration/integrationManager';
import { UserEditProtectionManager, type EditorFileState } from '../protection/userEditProtectionManager';
import { ReviewStorage } from '../review/reviewStorage';
import { TokenLedger } from '../usage/tokenLedger';
import type { RequestRecord } from '../chat/chatTypes';
import type { DevelopmentPlan } from '../planning/planningTypes';
import type { PermissionChoice, PermissionOperation } from '../permissions/permissionTypes';
import { digest } from '../review/reviewGit';
import { safeReviewText } from '../review/reviewSafety';

export interface TeamSessionOptions {
 root:string;directory:string;services:TeamServices;
 editors:()=>Promise<EditorFileState[]>;
 authorize:(plan:DevelopmentPlan)=>Promise<boolean>;
 approve:(op:PermissionOperation,signal:AbortSignal)=>Promise<PermissionChoice>;
 changed?:()=>void;
}
/** Shared production/test orchestration entry. UI selection never owns execution lifetime. */
export class TeamSession {
 readonly storage:ReviewStorage;readonly ledger:TokenLedger;
 workflow?:TeamWorkflow;integration?:IntegrationManager;protection?:UserEditProtectionManager;
 private selected?:string;private messageStart?:number;private logStart?:number;
 private revision=0;private listeners=new Set<()=>void>();private running?:Promise<void>;private error?:string;private serial:Promise<unknown>=Promise.resolve();
 constructor(readonly options:TeamSessionOptions){this.storage=new ReviewStorage(options.directory);this.ledger=new TokenLedger(options.root,this.storage);}
 private eventKey='';
 private notify=()=>{const state=this.workflow?.snapshot(),integration=this.integration?.snapshot(),usage=this.ledger.snapshot();const key=digest(JSON.stringify({tasks:state?.tasks.map(t=>[t.task.task_id,t.status,t.reason,t.review_id]),supervision:state?.record.supervision?.filter(e=>!['RETRY_SCHEDULED','RETRY_STARTED','RETRY_SUCCEEDED'].includes(e.kind)).map(e=>e.id),workers:state?.workers.map(w=>[w.worker_id,w.task_id,w.status==='RETRYING'?'RUNNING':w.status,w.session_id]),integration:integration?[integration.status,integration.review?.review_id]:undefined,usage:[usage.cli_requests,usage.running,usage.known_total,usage.unknown_requests,usage.blocked],error:this.error}));if(key!==this.eventKey){this.eventKey=key;this.revision++;for(const listener of this.listeners)listener();}this.options.changed?.();};
 async load(){await this.ledger.load();const plan=await this.storage.read<DevelopmentPlan>(this.options.root,'active-team-plan.json');if(plan){if(plan.project_path!==this.options.root||plan.validation!=='VALIDATED_NOT_EXECUTED'||!/^[A-Za-z0-9_-]{1,128}$/.test(plan.plan_id))throw Error('持久团队计划无效。');await this.open(plan);}}
 private async open(plan:DevelopmentPlan){
  this.workflow=new TeamWorkflow(plan,path.join(this.options.directory,'plans',plan.plan_id),this.options.services,this.ledger,this.options.approve,this.notify);await this.workflow.load();
  this.protection=new UserEditProtectionManager(this.options.root,this.options.editors,this.storage);await this.protection.load();
  this.integration=new IntegrationManager(this.workflow,this.protection,this.storage,this.notify);await this.integration.load();
 }
 get busy(){return !!this.running||!!this.workflow?.snapshot().busy||!!this.integration?.snapshot()?.busy;}
 async beginTurn(id:string){await this.ledger.begin(id,'PLANNING_AND_COORDINATION',this.workflow?.plan.plan_id);}
 async finishTurn(id:string,record:RequestRecord){await this.ledger.finish(id,record.status==='RUNNING'?'FAILED':record.status,record.usage);this.notify();}
 async execute(plan:DevelopmentPlan,owner:string,user:string){
  const operation=this.serial.then(async()=>{
   if(plan.project_path!==this.options.root||plan.owner_id!==owner||plan.original_request!==user||plan.validation!=='VALIDATED_NOT_EXECUTED')throw Error('计划不属于当前真实用户需求/会话/项目。');
   if(/只规划|仅规划|只讨论|仅讨论|不要执行|不要开发|plan only|do not execute/i.test(user))throw Error('用户仅授权规划或讨论，禁止启动开发。');
   if(this.workflow?.plan.plan_id===plan.plan_id)return this.snapshot();
   if(this.busy)throw Error('已有团队计划运行中，请先明确暂停或结束。');
   if(plan.user_approval_required&&!await this.options.authorize(plan))throw Error('用户未授权当前具体任务与文件范围。');
   await this.workflow?.dispose();await this.open(plan);await this.storage.write(this.options.root,'active-team-plan.json',plan);this.drive(true);return this.snapshot();
  });this.serial=operation.catch(()=>{});return operation;
 }
 private drive(start=false){
  if(this.running||!this.workflow)return;this.error=undefined;const workflow=this.workflow;
  const run=(async()=>{if(start)await workflow.start();await workflow.wait();if(workflow.snapshot().ready_for_integration&&this.integration?.snapshot()?.status!=='READY')await this.integration!.prepare();})().catch(e=>{this.error=safeReviewText((e as Error).message);this.notify();});
  this.running=run;void run.finally(()=>{this.running=undefined;this.notify();});this.notify();
 }
 snapshot(){
  const state=this.workflow?.snapshot(),usage=this.ledger.snapshot();
  let detail:any;
  if(this.selected&&state?.workers.some(w=>w.worker_id===this.selected)){
   const d=this.workflow!.executor.workers.detail(this.selected),all=d.chat.conversation?.messages||[],start=this.messageStart??Math.max(0,all.length-100),events=this.workflow!.executor.workers.events(this.selected,this.logStart??Math.max(0,d.worker.events.length-100),100),candidate=state.candidates.find(c=>c.worker_id===this.selected&&c.task_id===d.worker.task_id);
   detail={worker:{...d.worker,events:undefined},chat:{...d.chat,conversation:d.chat.conversation?{...d.chat.conversation,messages:all.slice(start,start+100),requests:d.chat.conversation.requests.slice(-1)}:undefined},message_start:start,message_total:all.length,event_total:d.worker.events.length,events,candidate,task:state.plan.tasks.find(t=>t.task_id===d.worker.task_id),reviews:state.record.reviews[d.worker.task_id],repairs:state.record.repairs?.filter(r=>r.task_id===d.worker.task_id)};
  }
  const integration=this.integration?.snapshot();const terminal=!!state&&!this.busy&&(!!this.error||!!state.record.error||!!integration&&['READY','BLOCKED','CONFLICT','DELIVERED'].includes(integration.status)||state.tasks.every(t=>!['RUNNING','REPAIRING','REVIEWING','PENDING','READY'].includes(t.status)));
  const progress=this.workflow?.progress(this.integration?.progressSource());
  return {enabled:true,revision:this.revision,error:this.error,busy:this.busy,terminal,selected_worker:this.selected,detail,progress,workflow:state?{...state,usage:{...state.usage,entries:undefined}}:undefined,integration,usage:{...usage,entries:undefined},delivery_requires_user_approval:integration?.status==='READY'};
 }
 async select(worker?:string,messageStart?:number,logStart?:number){
  if(worker){if(!this.workflow)throw Error('没有当前Worker。');for(const value of [messageStart,logStart])if(value!==undefined&&(!Number.isSafeInteger(value)||value<0))throw Error('分页位置无效。');await this.workflow.executor.workers.select(worker,false);}
  this.selected=worker;this.messageStart=messageStart;this.logStart=logStart;this.options.changed?.();
 }
 /** Read captured evidence only; a UI path can never trigger a filesystem read. */
 file(worker:string,file:string){
  if(!this.workflow)throw Error('没有当前团队计划。');
  const record=this.workflow.executor.workers.detail(worker).worker;
  const change=this.workflow.executor.candidate(record.task_id).evidence?.changes.find(c=>c.path===file);
  if(!change)throw Error('文件不属于当前 Worker 的实际变更证据。');
  return safeReviewText(change.diff);
 }
 /** Supplementary instructions wait for explicit resume through the original scheduler/gateway. */
 async instruct(worker:string,text:string){
  if(!this.workflow||typeof text!=='string'||!text.trim()||text.length>12000)throw Error('补充指令无效。');
  const record=this.workflow.executor.workers.detail(worker).worker;
  const row=this.workflow.scheduler.snapshot().find(t=>t.task.task_id===record.task_id);
  if(!row||!['PAUSED','WAITING_APPROVAL'].includes(row.status))throw Error('请先安全暂停当前任务，再添加补充指令；不会向运行中进程插入指令。');
  await this.workflow.executor.instruct(record.task_id,safeReviewText(text));this.notify();
 }
 private assertOwner(planId:string,owner:string){if(!this.workflow||this.workflow.plan.plan_id!==planId||this.workflow.plan.owner_id!==owner)throw Error('团队计划不属于当前会话/项目。');}
 async call(name:string,args:any,owner:string){
  this.assertOwner(args.plan_id,owner);
  if(name==='list_team_workers')return this.workflow!.executor.workers.list();
  if(name==='get_team_task_output'){
   const wait=args.wait_ms??0,revision=args.revision??0;if(!Number.isInteger(wait)||wait<0||wait>60000||!Number.isInteger(revision)||revision<0||args.until_decision!==undefined&&typeof args.until_decision!=='boolean')throw Error('轮询参数无效。');
   const deadline=Date.now()+wait;let observed=revision;
   while(wait&&Date.now()<deadline){const current=this.snapshot();if(current.terminal)break;const actionable=current.workflow?.tasks.some(t=>['FAILED','WAITING_APPROVAL','WAITING_REVIEW','PAUSED'].includes(t.status));if(this.revision>observed&&(!args.until_decision||actionable))break;observed=this.revision;await new Promise<void>(resolve=>{let timer:ReturnType<typeof setTimeout>;const done=()=>{clearTimeout(timer);this.listeners.delete(done);resolve();};this.listeners.add(done);timer=setTimeout(done,Math.max(1,deadline-Date.now()));});}
   const s=this.snapshot();return {revision:s.revision,unchanged:revision===s.revision,terminal:s.terminal,busy:s.busy,error:s.error,usage:s.usage,integration:s.integration?{status:s.integration.status,error:s.integration.error}:undefined,workflow:s.workflow?{plan:{plan_id:s.workflow.plan.plan_id,goal:s.workflow.plan.goal,worker_count:s.workflow.plan.worker_count,analysis:s.workflow.plan.analysis},tasks:s.workflow.tasks.map(t=>({task_id:t.task.task_id,assigned_worker:t.task.assigned_worker,dependencies:t.task.dependencies,status:t.status,reason:t.reason,review_id:t.review_id})),reviews:Object.fromEntries(Object.entries(s.workflow.record.reviews).map(([id,reports])=>[id,reports.map(r=>({review_id:r.review_id,review_status:r.review_status,summary:r.summary,block:r.block,tests:r.tests}))])),events:revision===s.revision?[]:s.workflow.record.supervision?.slice(-12),workers:s.workflow.workers?.map(w=>({worker_id:w.worker_id,task_id:w.task_id,status:w.status,retry:w.retry})),retry_stats:s.workflow.retry_stats,repairs:s.workflow.record.repairs?.map(r=>({repair_id:r.repair_id,task_id:r.task_id,status:r.status,round:r.round})),error:s.workflow.record.error}:undefined};
  }
  if(name==='control_team_task'){await this.control(args.action,args.task_id);return this.snapshot();}
  throw Error('未知团队工具。');
 }
 async control(action:'pause'|'resume'|'cancel',taskId?:string){
  if(!this.workflow)throw Error('没有当前团队计划。');
  if(action==='resume'){const rows=this.workflow.scheduler.snapshot().filter(t=>(!taskId||t.task.task_id===taskId)&&['PAUSED','WAITING_APPROVAL','WAITING_REVIEW','FAILED'].includes(t.status));if(!rows.length)throw Error('没有可恢复的任务。');for(const row of rows)await this.workflow.resume(row.task.task_id);this.drive();}
  else if(action==='pause'||action==='cancel')await this.workflow.pause(taskId,action==='cancel');else throw Error('操作无效。');this.notify();
 }
 async configure(budget:number,acknowledgeUnknown:boolean,concurrency:number){
  if(!Number.isInteger(concurrency)||concurrency<1||concurrency>8)throw Error('并发数量必须为1–8。');await this.ledger.configure(budget,acknowledgeUnknown);
  if(this.workflow){await this.workflow.executor.workers.setConcurrency(concurrency);await this.workflow.scheduler.setConcurrency(concurrency);await this.storage.write(this.options.root,'active-team-plan.json',this.workflow.plan);}this.notify();
 }
 async wait(){await this.running;}
 async dispose(){await this.workflow?.dispose();await this.running;for(const listener of this.listeners)listener();this.listeners.clear();}
}
