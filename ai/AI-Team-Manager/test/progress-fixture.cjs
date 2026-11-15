// Explicitly synthetic backend scenarios; the UI receives the production projection.
const {buildTaskProgress}=require('../.tools/progress/api.cjs');
const time=n=>'2026-11-15T10:00:'+String(n).padStart(2,'0')+'.000Z';
function fixture(count=1){
 const tasks=Array.from({length:count},(_,i)=>({task:{task_id:'task-'+i,task_name:'模块 '+(i+1),task_description:'隔离开发与验证',test_commands:['python -B task'+i+'.py'],dependencies:[],assigned_worker:'worker-'+i,status:'PENDING'},status:'RUNNING',worker_id:'worker-'+i,events:[{time:time(0),from:'READY',to:'RUNNING'}]}));
 return {plan:{plan_id:'progress-plan',project_id:'p',owner_id:'ui-owner',project_path:'/ui-fixture/project',goal:'可验证任务进度',validation:'VALIDATED_NOT_EXECUTED',tasks:tasks.map(t=>t.task)},tasks,candidates:[],workers:tasks.map((t,i)=>({worker_id:'worker-'+i,task_id:t.task.task_id,project_id:'p',status:'RUNNING',updated_at:time(0)})),reviews:{},events:[],integration:undefined};
}
function accept(f,i=0){const row=f.tasks[i],digest='digest-'+i,tests=[{id:'test-'+i,command:row.task.test_commands[0],status:'PASSED',test_count:3,exit_code:0,source_digest:digest,started_at:time(2),finished_at:time(3)}];
 f.candidates.push({task_id:row.task.task_id,worker_id:f.workers[i].worker_id,digest,captured:true,captured_at:time(1),tests});f.workers[i].status='COMPLETED';
 const r={review_id:'review-'+i,task:{task_id:row.task.task_id},created_at:time(4),finished_at:time(5),verdict:'PASS',stale:false,missing_evidence:[],issues:[],evidence:{after:{digest}}};f.reviews[row.task.task_id]=[r];row.status='WAITING_INTEGRATION';row.events.push({time:time(4),from:'RUNNING',to:'REVIEWING'},{time:time(5),from:'REVIEWING',to:'WAITING_INTEGRATION'});return f;
}
function integrate(f,delivered=false){f.integration={plan_id:f.plan.plan_id,integration_id:'integration',status:delivered?'DELIVERED':'READY',candidates:Object.fromEntries(f.candidates.map(c=>[c.task_id,{digest:c.digest,review_id:f.reviews[c.task_id][0].review_id}])),review:{review_id:'integration-review',verdict:'PASS',missing_evidence:[],issues:[],stale:false,created_at:time(6),finished_at:time(7),evidence:{after:{digest:'merged'}}},evidence:{after:{digest:'merged'},tests:[{source_digest:'merged',status:'PASSED',exit_code:0,test_count:3}]},...(delivered?{delivered_at:time(8)}:{})};if(delivered)f.tasks.forEach(t=>t.status='COMPLETED');return f;}
const project=f=>buildTaskProgress(f.plan,f.tasks,f.candidates,f.workers,f.reviews,f.events,f.integration,f.error);
module.exports={fixture,accept,integrate,project,time};
