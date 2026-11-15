# Task 07.10 — 此前历史成果接入

仅接入用户明确授权的两个历史文件。修改前已分别复制原始字节并计算 SHA-256；随后完整阅读原始代码，未发现硬编码密钥、Token、无关内容或阻止接入的安全问题。历史来源依据为用户对这两个文件的明确声明和授权，未将其宣称为本次从零编写的成果，也未声称核验过不存在的上游来源证明。

| 文件 | 原始字节 SHA-256 | 修改前快照（本地忽略目录） |
|---|---|---|
| `src/team/teamSession.ts` | `2F6A9F9585C79C5B72F140C58042C289EA7EA57ED9E58A0D639EB0D17EB34F7D` | `test-results/task0710-baseline/teamSession.ts` |
| `src/team/teamTools.ts` | `D48E5126772290E72468AA09428E5C47D68DCB3BBE223C6D5929F401FC1FCDEF` | `test-results/task0710-baseline/teamTools.ts` |

## 本次必要增量

`teamSession.ts` 新增 `file(worker,file)`，只查阅已捕获候选文件的差异证据，不接受任意磁盘路径；新增 `instruct(worker,text)`，仅允许对暂停或等待审批的 Worker 保存补充指令，并继续使用原调度器及权限网关。`snapshot()` 的所选 Worker 明细新增 `event_total`，用于真实事件分页。`busy` 纳入集成阶段的忙碌状态。`select()` 改为静默选择 Worker，保持显示刷新，但不增加后端执行修订号、不唤醒 GPT 长轮询。`TeamSessionOptions` 和原有工具接口保持兼容。

原有 `load/open`、`beginTurn/finishTurn`、`execute`、`drive`、暂停/恢复/取消、测试及审查、TokenLedger、原生权限确认、候选集成、用户编辑保护和交付链路保留。Worker 明细仍按原规则保留最近 100 条消息及事件；本次只补充真实总数及显示入口。

`teamTools.ts` 没有修改：工具 schema、`TEAM_INSTRUCTIONS` 及全部原实现保持原始字节一致。它在本次提交中属于“此前历史成果接入”，不计为本次新增功能。

`test/api.ts` 的原有历史修改未覆盖、未暂存、未提交。原始及最终工作区文件 SHA-256 均为 `E46FB9ECA52AC425EC6F0CEB4D200CA12FD6C60D462B1CBD79815BA168BBCC99`。本次专用测试通过新增 `test/task0710-api.ts` 导出生产接口；独立源码构建使用 HEAD 中的 `test/api.ts`，并已验证通过。

## 原始全文与增量

以下原始全文由修改前快照直接读取追加。SHA-256 针对快照原始字节；Markdown 中的源码展示可能受 Git/查看器换行规范化影响，精确字节快照保存在上述本地位置。文件级增量紧随原始全文，便于将历史成果与本次修改分别审核。

### Original: teamSession.ts

```typescript
import * as path from 'node:path';
import { TeamWorkflow, type TeamServices } from './teamWorkflow';
import { IntegrationManager } from '../integration/integrationManager';
import { UserEditProtectionManager, type EditorFileState } from '../protection/userEditProtectionManager';
import { ReviewStorage } from '../review/reviewStorage';
import { TokenLedger } from '../usage/tokenLedger';
import type { RequestRecord } from '../chat/chatTypes';
import type { DevelopmentPlan } from '../planning/planningTypes';
import type { PermissionChoice, PermissionOperation } from '../permissions/permissionTypes';
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
 private notify=()=>{this.revision++;for(const listener of this.listeners)listener();this.options.changed?.();};
 async load(){await this.ledger.load();const plan=await this.storage.read<DevelopmentPlan>(this.options.root,'active-team-plan.json');if(plan){if(plan.project_path!==this.options.root||plan.validation!=='VALIDATED_NOT_EXECUTED'||!/^[A-Za-z0-9_-]{1,128}$/.test(plan.plan_id))throw Error('持久团队计划无效。');await this.open(plan);}}
 private async open(plan:DevelopmentPlan){
  this.workflow=new TeamWorkflow(plan,path.join(this.options.directory,'plans',plan.plan_id),this.options.services,this.ledger,this.options.approve,this.notify);await this.workflow.load();
  this.protection=new UserEditProtectionManager(this.options.root,this.options.editors,this.storage);await this.protection.load();
  this.integration=new IntegrationManager(this.workflow,this.protection,this.storage,this.notify);await this.integration.load();
 }
 get busy(){return !!this.running||!!this.workflow?.snapshot().busy;}
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
   detail={worker:{...d.worker,events:undefined},chat:{...d.chat,conversation:d.chat.conversation?{...d.chat.conversation,messages:all.slice(start,start+100),requests:d.chat.conversation.requests.slice(-1)}:undefined},message_start:start,message_total:all.length,events,candidate,task:state.plan.tasks.find(t=>t.task_id===d.worker.task_id)};
  }
  const integration=this.integration?.snapshot();const terminal=!!state&&!this.busy&&(!!this.error||!!state.record.error||!!integration&&['READY','BLOCKED','CONFLICT','DELIVERED'].includes(integration.status)||state.tasks.every(t=>!['RUNNING','REPAIRING','REVIEWING','PENDING','READY'].includes(t.status)));
  return {enabled:true,revision:this.revision,error:this.error,busy:this.busy,terminal,selected_worker:this.selected,detail,workflow:state?{...state,usage:{...state.usage,entries:undefined}}:undefined,integration,usage:{...usage,entries:undefined},delivery_requires_user_approval:integration?.status==='READY'};
 }
 async select(worker?:string,messageStart?:number,logStart?:number){
  if(worker){if(!this.workflow)throw Error('没有当前Worker。');for(const value of [messageStart,logStart])if(value!==undefined&&(!Number.isSafeInteger(value)||value<0))throw Error('分页位置无效。');await this.workflow.executor.workers.select(worker);}
  this.selected=worker;this.messageStart=messageStart;this.logStart=logStart;this.notify();
 }
 private assertOwner(planId:string,owner:string){if(!this.workflow||this.workflow.plan.plan_id!==planId||this.workflow.plan.owner_id!==owner)throw Error('团队计划不属于当前会话/项目。');}
 async call(name:string,args:any,owner:string){
  this.assertOwner(args.plan_id,owner);
  if(name==='list_team_workers')return this.workflow!.executor.workers.list();
  if(name==='get_team_task_output'){
   const wait=args.wait_ms??0,revision=args.revision??0;if(!Number.isInteger(wait)||wait<0||wait>25000||!Number.isInteger(revision)||revision<0)throw Error('轮询参数无效。');
   if(wait&&revision>=this.revision&&!this.snapshot().terminal)await new Promise<void>(resolve=>{let timer:ReturnType<typeof setTimeout>;const done=()=>{clearTimeout(timer);this.listeners.delete(done);resolve();};this.listeners.add(done);timer=setTimeout(done,wait);});
   const s=this.snapshot();return {...s,detail:undefined,workflow:s.workflow?{...s.workflow,plan:{plan_id:s.workflow.plan.plan_id,goal:s.workflow.plan.goal,worker_count:s.workflow.plan.worker_count}}:undefined};
  }
  if(name==='control_team_task'){await this.control(args.action,args.task_id);return this.snapshot();}
  throw Error('未知团队工具。');
 }
 async control(action:'pause'|'resume'|'cancel',taskId?:string){
  if(!this.workflow)throw Error('没有当前团队计划。');
  if(action==='resume'){const rows=this.workflow.scheduler.snapshot().filter(t=>(!taskId||t.task.task_id===taskId)&&['PAUSED','WAITING_APPROVAL'].includes(t.status));if(!rows.length)throw Error('没有可恢复的任务。');for(const row of rows)await this.workflow.resume(row.task.task_id);this.drive();}
  else if(action==='pause'||action==='cancel')await this.workflow.pause(taskId,action==='cancel');else throw Error('操作无效。');this.notify();
 }
 async configure(budget:number,acknowledgeUnknown:boolean,concurrency:number){
  if(!Number.isInteger(concurrency)||concurrency<1||concurrency>8)throw Error('并发数量必须为1–8。');await this.ledger.configure(budget,acknowledgeUnknown);
  if(this.workflow){this.workflow.plan.max_concurrency=concurrency;await this.workflow.executor.workers.setConcurrency(concurrency);await this.storage.write(this.options.root,'active-team-plan.json',this.workflow.plan);}this.notify();
 }
 async wait(){await this.running;}
 async dispose(){await this.workflow?.dispose();await this.running;for(const listener of this.listeners)listener();this.listeners.clear();}
}
```

### Increment: teamSession.ts

```diff
diff --git a/test-results/task0710-baseline/teamSession.ts b/src/team/teamSession.ts
index 84ef943..c806df0 100644
--- a/test-results/task0710-baseline/teamSession.ts
+++ b/src/team/teamSession.ts
@@ -30,7 +30,7 @@ export class TeamSession {
   this.protection=new UserEditProtectionManager(this.options.root,this.options.editors,this.storage);await this.protection.load();
   this.integration=new IntegrationManager(this.workflow,this.protection,this.storage,this.notify);await this.integration.load();
  }
- get busy(){return !!this.running||!!this.workflow?.snapshot().busy;}
+ get busy(){return !!this.running||!!this.workflow?.snapshot().busy||!!this.integration?.snapshot()?.busy;}
  async beginTurn(id:string){await this.ledger.begin(id,'PLANNING_AND_COORDINATION',this.workflow?.plan.plan_id);}
  async finishTurn(id:string,record:RequestRecord){await this.ledger.finish(id,record.status==='RUNNING'?'FAILED':record.status,record.usage);this.notify();}
  async execute(plan:DevelopmentPlan,owner:string,user:string){
@@ -53,14 +53,30 @@ export class TeamSession {
   let detail:any;
   if(this.selected&&state?.workers.some(w=>w.worker_id===this.selected)){
    const d=this.workflow!.executor.workers.detail(this.selected),all=d.chat.conversation?.messages||[],start=this.messageStart??Math.max(0,all.length-100),events=this.workflow!.executor.workers.events(this.selected,this.logStart??Math.max(0,d.worker.events.length-100),100),candidate=state.candidates.find(c=>c.worker_id===this.selected&&c.task_id===d.worker.task_id);
-   detail={worker:{...d.worker,events:undefined},chat:{...d.chat,conversation:d.chat.conversation?{...d.chat.conversation,messages:all.slice(start,start+100),requests:d.chat.conversation.requests.slice(-1)}:undefined},message_start:start,message_total:all.length,events,candidate,task:state.plan.tasks.find(t=>t.task_id===d.worker.task_id)};
+   detail={worker:{...d.worker,events:undefined},chat:{...d.chat,conversation:d.chat.conversation?{...d.chat.conversation,messages:all.slice(start,start+100),requests:d.chat.conversation.requests.slice(-1)}:undefined},message_start:start,message_total:all.length,event_total:d.worker.events.length,events,candidate,task:state.plan.tasks.find(t=>t.task_id===d.worker.task_id)};
   }
   const integration=this.integration?.snapshot();const terminal=!!state&&!this.busy&&(!!this.error||!!state.record.error||!!integration&&['READY','BLOCKED','CONFLICT','DELIVERED'].includes(integration.status)||state.tasks.every(t=>!['RUNNING','REPAIRING','REVIEWING','PENDING','READY'].includes(t.status)));
   return {enabled:true,revision:this.revision,error:this.error,busy:this.busy,terminal,selected_worker:this.selected,detail,workflow:state?{...state,usage:{...state.usage,entries:undefined}}:undefined,integration,usage:{...usage,entries:undefined},delivery_requires_user_approval:integration?.status==='READY'};
  }
  async select(worker?:string,messageStart?:number,logStart?:number){
-  if(worker){if(!this.workflow)throw Error('没有当前Worker。');for(const value of [messageStart,logStart])if(value!==undefined&&(!Number.isSafeInteger(value)||value<0))throw Error('分页位置无效。');await this.workflow.executor.workers.select(worker);}
-  this.selected=worker;this.messageStart=messageStart;this.logStart=logStart;this.notify();
+  if(worker){if(!this.workflow)throw Error('没有当前Worker。');for(const value of [messageStart,logStart])if(value!==undefined&&(!Number.isSafeInteger(value)||value<0))throw Error('分页位置无效。');await this.workflow.executor.workers.select(worker,false);}
+  this.selected=worker;this.messageStart=messageStart;this.logStart=logStart;this.options.changed?.();
+ }
+ /** Read captured evidence only; a UI path can never trigger a filesystem read. */
+ file(worker:string,file:string){
+  if(!this.workflow)throw Error('没有当前团队计划。');
+  const record=this.workflow.executor.workers.detail(worker).worker;
+  const change=this.workflow.executor.candidate(record.task_id).evidence?.changes.find(c=>c.path===file);
+  if(!change)throw Error('文件不属于当前 Worker 的实际变更证据。');
+  return safeReviewText(change.diff);
+ }
+ /** Supplementary instructions wait for explicit resume through the original scheduler/gateway. */
+ async instruct(worker:string,text:string){
+  if(!this.workflow||typeof text!=='string'||!text.trim()||text.length>12000)throw Error('补充指令无效。');
+  const record=this.workflow.executor.workers.detail(worker).worker;
+  const row=this.workflow.scheduler.snapshot().find(t=>t.task.task_id===record.task_id);
+  if(!row||!['PAUSED','WAITING_APPROVAL'].includes(row.status))throw Error('请先安全暂停当前任务，再添加补充指令；不会向运行中进程插入指令。');
+  await this.workflow.executor.instruct(record.task_id,safeReviewText(text));this.notify();
  }
  private assertOwner(planId:string,owner:string){if(!this.workflow||this.workflow.plan.plan_id!==planId||this.workflow.plan.owner_id!==owner)throw Error('团队计划不属于当前会话/项目。');}
  async call(name:string,args:any,owner:string){
```

### Original: teamTools.ts

```typescript
import type { BridgeTool } from '../dispatch/toolBridge';
const id={type:'string',minLength:1,maxLength:128};
export const teamTools:BridgeTool[]=[
 {name:'execute_adaptive_plan',description:'Execute a validated fresh plan in isolated Worker workspaces. Requires actual user scope approval when needed. Returns immediately; poll get_team_task_output until terminal.',inputSchema:{type:'object',properties:{plan_id:id},required:['plan_id'],additionalProperties:false}},
 {name:'get_team_task_output',description:'Get actual multi-Worker state, test and independent review summaries. wait_ms up to 25000 long-polls. READY integration still needs user delivery approval; do not claim main code was delivered.',inputSchema:{type:'object',properties:{plan_id:id,revision:{type:'integer',minimum:0},wait_ms:{type:'integer',minimum:0,maximum:25000}},required:['plan_id'],additionalProperties:false}},
 {name:'control_team_task',description:'Pause, resume or cancel a specified plan task, or the whole current plan. Pause stops the current process at a safe boundary and preserves its Session and candidate.',inputSchema:{type:'object',properties:{plan_id:id,task_id:id,action:{type:'string',enum:['pause','resume','cancel']}},required:['plan_id','action'],additionalProperties:false}},
 {name:'list_team_workers',description:'List actual Worker IDs, independent Session IDs, task association and status. Does not create Sessions or call a model.',inputSchema:{type:'object',properties:{plan_id:id},required:['plan_id'],additionalProperties:false}},
];
export const TEAM_INSTRUCTIONS=`已验证的开发计划通过execute_adaptive_plan真实启动隔离Claude团队，不能用旧dispatch_claude_task执行Task07多Worker计划。插件动态调度独立Worker、本地检查普通工具、收集实际测试并使用独立GPT增量审查、定向修复与集成审查。不要自己改代码、审批权限、提交或推送。必须反复调用get_team_task_output，传回revision及wait_ms=25000，直到terminal=true。terminal可能表示等待人工、失败、集成待交付或真正交付；按工具事实报告，不能把WAITING_INTEGRATION或READY当主项目已完成。用户在界面查看实际补丁后授权最终交付。不要轮询多个Worker的完整日志作为上下文；状态/审查摘要足够时不重复发送源码。没有缺陷不重做已通过模块；当前请求完成后停止。`;
```

### Increment: teamTools.ts

No changes: byte-identical to the historical baseline.
