import type { DevelopmentPlan } from '../planning/planningTypes';
import type { ScheduledTask } from '../scheduling/taskDependencyScheduler';
import type { ReviewReport, TestEvidence } from '../review/reviewTypes';
import type { WorkerEvent, WorkerRecord } from '../workers/workerTypes';
import type { IntegrationRecord } from '../integration/integrationManager';

export interface ProgressCandidate {
  task_id:string;worker_id:string;digest?:string;captured:boolean;captured_at?:string;tests:TestEvidence[];
}
export interface ProgressStep {id:string;name:string;state:'PENDING'|'RUNNING'|'PASSED'|'BLOCKED'|'FAILED'|'PAUSED'|'CANCELLED';started_at?:string;finished_at?:string;reason?:string}
export interface TaskProgress {
  task_id:string;worker_id?:string;name:string;state:string;phase:string;completed:number;total:number|null;percent:number|null;
  updated_at?:string;steps:ProgressStep[];timeline:ProgressStep[];reason?:string;
}
const names:Record<string,string>={PENDING:'等待执行',READY:'等待执行',WAITING_DEPENDENCY:'等待依赖',WAITING_FILE_LOCK:'等待文件锁',RUNNING:'开发执行中',REPAIRING:'定向修复中',REVIEWING:'独立审查中',WAITING_REVIEW:'审查阻塞',WAITING_APPROVAL:'等待人工确认',WAITING_INTEGRATION:'等待集成',COMPLETED:'已完成',FAILED:'失败',PAUSED:'已暂停',CANCELLED:'已取消',RETRYING:'等待自动重试',TESTING:'测试中',REVIEW_PASSED:'审查通过',WAITING_DELIVERY:'等待用户交付',INTEGRATING:'集成验证中',BLOCKED:'失败或阻塞',UNKNOWN:'阶段未知'};
const validTime=(s?:string)=>s&&Number.isFinite(Date.parse(s))?s:undefined;
const latest=(values:(string|undefined)[])=>values.map(validTime).filter((s):s is string=>!!s).sort((a,b)=>Date.parse(b)-Date.parse(a))[0];
const passed=(r:ReviewReport|undefined,digest?:string)=>!!r&&r.verdict==='PASS'&&!r.stale&&!r.missing_evidence.length&&r.evidence.after.digest===digest&&!r.issues.some(i=>i.severity==='HIGH'||i.severity==='CRITICAL');
const step=(id:string,name:string,ok:boolean,started_at?:string,finished_at?:string):ProgressStep=>({id,name,state:ok?'PASSED':'PENDING',started_at:validTime(started_at),finished_at:ok?validTime(finished_at):undefined});

/** Read-only projection. Percent is the fraction of verified gates, never execution/time/token estimates. */
export function buildTaskProgress(plan:DevelopmentPlan,tasks:ScheduledTask[],candidates:ProgressCandidate[],workers:Omit<WorkerRecord,'events'>[],reviews:Record<string,ReviewReport[]>,events:WorkerEvent[]=[],integration?:Readonly<IntegrationRecord>,error?:string){
  const reliable=plan.validation==='VALIDATED_NOT_EXECUTED'&&!!plan.plan_id&&plan.tasks.length>0&&new Set(tasks.map(t=>t.task.task_id)).size===plan.tasks.length&&plan.tasks.every(t=>tasks.some(r=>r.task.task_id===t.task_id));
  const prelim=tasks.map(row=>{
    const c=candidates.find(c=>c.task_id===row.task.task_id),w=workers.find(w=>w.task_id===row.task.task_id&&(!c||w.worker_id===c.worker_id));
    const tests=c?.tests.filter(t=>t.source_digest===c.digest)||[];
    const testsOK=!!c?.captured&&row.task.test_commands.length>0&&row.task.test_commands.every(command=>{const t=tests.filter(t=>t.command===command).at(-1);return t?.status==='PASSED'&&(t.test_count||0)>0&&t.exit_code===0;})&&tests.every(t=>t.status==='PASSED');
    const review=reviews[row.task.task_id]?.at(-1),reviewOK=testsOK&&passed(review,c?.digest);
    return {row,c,w,tests,testsOK,review,reviewOK};
  });
  const allReviewed=reliable&&prelim.every(p=>p.reviewOK);
  const integrationOK=allReviewed&&!!integration&&integration.plan_id===plan.plan_id&&['READY','DELIVERED'].includes(integration.status)&&!!integration.evidence&&passed(integration.review,integration.evidence.after.digest)&&integration.evidence.tests.length>0&&integration.evidence.tests.every(t=>t.status==='PASSED'&&(t.test_count||0)>0&&t.exit_code===0&&t.source_digest===integration.evidence!.after.digest)&&prelim.every(p=>integration.candidates[p.row.task.task_id]?.digest===p.c?.digest&&integration.candidates[p.row.task.task_id]?.review_id===p.review?.review_id);
  const delivered=integrationOK&&integration?.status==='DELIVERED'&&!!validTime(integration.delivered_at)&&tasks.every(t=>t.status==='COMPLETED');
  const result:TaskProgress[]=prelim.map(({row,c,w,tests,testsOK,review,reviewOK})=>{
    const history=row.events,workerEvents=events.filter(e=>e.task_id===row.task.task_id&&e.worker_id===w?.worker_id);
    const lastTool=workerEvents.filter(e=>e.event.type==='tool').at(-1);
    const tool=lastTool?.event.type==='tool'?lastTool.event:undefined;
    const testing=['RUNNING','REPAIRING'].includes(row.status)&&tool?.phase==='running'&&tool.name.endsWith('run_authorized_test')&&(!tests.length||Date.parse(lastTool!.time)>Math.max(...tests.map(t=>Date.parse(t.finished_at))));
    let state:string=row.status;
    if(['RUNNING','REPAIRING'].includes(state)&&w?.status==='RETRYING')state='RETRYING';
    else if(testing)state='TESTING';
    else if(['RUNNING','REPAIRING','REVIEWING'].includes(state)&&tests.some(t=>t.status==='FAILED'||t.status==='TIMEOUT'))state='FAILED';
    else if(state==='REVIEWING'&&reviewOK)state='REVIEW_PASSED';
    else if(state==='COMPLETED'&&!delivered)state='WAITING_DELIVERY';
    if(integrationOK&&['WAITING_INTEGRATION','COMPLETED'].includes(row.status))state=delivered?'COMPLETED':'WAITING_DELIVERY';
    const reason=['FAILED','WAITING_REVIEW','WAITING_APPROVAL','PAUSED','CANCELLED'].includes(state)?row.reason||review?.missing_evidence.join('；'):undefined;
    const steps=[step('candidate','候选采集',!!c?.captured,history.find(e=>e.to==='RUNNING')?.time,c?.captured_at),step('tests','当前版本测试',testsOK,tests[0]?.started_at,latest(tests.map(t=>t.finished_at))),step('review','独立审查',reviewOK,review?.created_at,review?.finished_at),step('integration','集成验证',integrationOK,integration?.review?.created_at,integration?.review?.finished_at),step('delivery','用户交付',delivered,undefined,integration?.delivered_at)];
    const current=state==='TESTING'?1:state==='REVIEWING'||state==='WAITING_REVIEW'||state==='WAITING_APPROVAL'?2:integration?.status==='PREPARING'&&reviewOK?3:state==='WAITING_DELIVERY'?4:!c?.captured?0:!testsOK?1:!reviewOK?2:3;
    if(steps[current].state!=='PASSED')steps[current]={...steps[current],state:['FAILED','WAITING_REVIEW','WAITING_APPROVAL'].includes(state)?state==='FAILED'?'FAILED':'BLOCKED':state==='PAUSED'?'PAUSED':state==='CANCELLED'?'CANCELLED':['RUNNING','REPAIRING','TESTING','REVIEWING','RETRYING'].includes(state)||integration?.status==='PREPARING'?'RUNNING':'PENDING',reason};
    if(current===1&&tests.some(t=>t.status==='FAILED'||t.status==='TIMEOUT'))steps[1].finished_at=latest(tests.map(t=>t.finished_at));
    if(current===2&&review?.evidence.after.digest===c?.digest&&!review?.stale&&review?.verdict!=='PASS')steps[2].finished_at=validTime(review?.finished_at);
    const completed=steps.filter(s=>s.state==='PASSED').length;
    return {task_id:row.task.task_id,worker_id:c?.worker_id||w?.worker_id,name:row.task.task_name||row.task.task_description,state,phase:names[state]||names.UNKNOWN,completed,total:reliable?5:null,percent:reliable?Math.floor(completed/5*100):null,updated_at:latest([w?.updated_at,lastTool?.time,history.at(-1)?.time,c?.captured_at,review?.finished_at,...tests.map(t=>t.finished_at),integration?.review?.finished_at,integration?.delivered_at]),reason,steps,
      timeline:history.map((e,i)=>({id:'event-'+i,name:names[e.to]||e.to,state:e.to==='FAILED'?'FAILED':e.to==='CANCELLED'?'CANCELLED':e.to==='PAUSED'?'PAUSED':e.to==='WAITING_REVIEW'||e.to==='WAITING_APPROVAL'?'BLOCKED':history[i+1]?'PASSED':'RUNNING',started_at:validTime(e.time),finished_at:validTime(history[i+1]?.time),reason:e.reason}))};
  });
  const completed=result.reduce((n,p)=>n+p.steps.slice(0,3).filter(s=>s.state==='PASSED').length,0)+(integrationOK?1:0)+(delivered?1:0),total=reliable?tasks.length*3+2:null;
  let state:string;
  // Resolve precedence explicitly: blocked/paused work must never look delivered.
  if(delivered)state='COMPLETED';else if(error||integration?.status==='BLOCKED'||integration?.status==='CONFLICT')state='BLOCKED';else if(result.some(t=>t.state==='FAILED'||t.state==='WAITING_REVIEW'))state='BLOCKED';else if(result.some(t=>t.state==='WAITING_APPROVAL'))state='WAITING_APPROVAL';else if(result.length&&result.every(t=>t.state==='CANCELLED'))state='CANCELLED';else if(result.some(t=>t.state==='PAUSED')&&!result.some(t=>['RUNNING','RETRYING','REVIEWING','REPAIRING','TESTING'].includes(t.state)))state='PAUSED';else if(integrationOK)state='WAITING_DELIVERY';else if(integration?.status==='PREPARING')state='INTEGRATING';else state=result.find(t=>['TESTING','RETRYING','REVIEWING','RUNNING','REPAIRING'].includes(t.state))?.state||result.find(t=>t.state==='PAUSED')?.state||result.find(t=>t.state==='WAITING_INTEGRATION'||t.state==='REVIEW_PASSED')?.state||'PENDING';
  const overview={waiting:0,running:0,testing:0,waiting_review:0,review_passed:prelim.filter(p=>p.reviewOK).length,waiting_integration:0,waiting_delivery:0,completed:0,blocked:0};
  for(const t of result){if(['FAILED','WAITING_REVIEW','WAITING_APPROVAL','PAUSED','CANCELLED'].includes(t.state))overview.blocked++;else if(t.state==='TESTING')overview.testing++;else if(t.state==='REVIEWING')overview.waiting_review++;else if(t.state==='WAITING_INTEGRATION'||t.state==='REVIEW_PASSED')overview.waiting_integration++;else if(t.state==='WAITING_DELIVERY')overview.waiting_delivery++;else if(t.state==='COMPLETED')overview.completed++;else if(['RUNNING','REPAIRING','RETRYING'].includes(t.state))overview.running++;else overview.waiting++;}
  return {plan_id:plan.plan_id,name:plan.goal,state,phase:names[state]||names.UNKNOWN,completed,total,percent:total===null?null:Math.floor(completed/total*100),remaining:total===null?null:total-completed,tasks:result,overview,updated_at:latest(result.map(t=>t.updated_at)),basis:'验证阶段计数：每项任务的候选、测试、审查，加上整体集成和用户交付；不是开发工作量估算。'};
}
