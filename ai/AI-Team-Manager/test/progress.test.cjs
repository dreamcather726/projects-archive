const test=require('node:test'),assert=require('node:assert/strict'),{fixture,accept,integrate,project,time}=require('./progress-fixture.cjs'),api=require('../.tools/progress/api.cjs');
test('one Worker counts only source-bound verified gates; CLI completion alone adds nothing',()=>{
 const f=fixture();assert.equal(project(f).percent,0);f.workers[0].status='COMPLETED';assert.equal(project(f).percent,0);
 accept(f);const p=project(f);assert.equal(p.completed,3);assert.equal(p.total,5);assert.equal(p.percent,60);assert.equal(p.tasks[0].completed,3);assert.equal(p.state,'WAITING_INTEGRATION');assert.equal(p.tasks[0].steps[2].finished_at,time(5));
});
test('multiple Workers keep task identity and progress independent, including reassigned Workers',()=>{
 const f=accept(fixture(2));let p=project(f);assert.deepEqual(p.tasks.map(t=>t.completed),[3,0]);assert.equal(p.total,8);assert.equal(p.percent,37);
 f.workers[0].task_id='new-task';p=project(f);assert.equal(p.tasks[0].worker_id,'worker-0');assert.equal(p.tasks[0].completed,3);assert.equal(p.tasks[1].completed,0);
});
test('retry count, elapsed time, output length and tokens never advance verified progress',()=>{
 const f=fixture();const before=project(f).percent;f.workers[0].status='RETRYING';f.workers[0].updated_at=time(30);f.workers[0].retry={kind:'RETRY_SCHEDULED',retry:3,maxRetries:3};f.output='x'.repeat(100000);f.tokens=999999;
 const p=project(f);assert.equal(p.percent,before);assert.equal(p.tasks[0].state,'RETRYING');assert.equal(p.updated_at,time(30));
});
test('pause resume and cancellation retain evidence and use actual schedule states',()=>{
 const f=accept(fixture());f.tasks[0].status='PAUSED';assert.equal(project(f).state,'PAUSED');assert.equal(project(f).completed,3);
 f.tasks[0].status='REVIEWING';assert.equal(project(f).state,'REVIEW_PASSED');f.tasks[0].status='CANCELLED';assert.equal(project(f).state,'CANCELLED');assert(project(f).percent<100);
});
test('failed tests, zero-test exits and stale test records cannot complete a task or review gate',()=>{
 for(const change of [t=>t.status='FAILED',t=>t.test_count=0,t=>t.source_digest='old',t=>t.exit_code=1]){const f=accept(fixture());change(f.candidates[0].tests[0]);assert.equal(project(f).tasks[0].completed,1);assert(project(f).percent<100);}
});
test('blocked/stale review cannot reach 100%, even if the scheduler status says complete',()=>{
 for(const change of [r=>r.verdict='BLOCKED',r=>r.stale=true,r=>r.missing_evidence=['missing'],r=>r.evidence.after.digest='old']){const f=integrate(accept(fixture()),true);change(f.reviews['task-0'][0]);assert(project(f).percent<100);assert.notEqual(project(f).state,'COMPLETED');}
});
test('READY requires bound integration tests/review and stays distinct from authorized delivery',()=>{
 const f=integrate(accept(fixture()));assert.equal(project(f).percent,80);assert.equal(project(f).state,'WAITING_DELIVERY');assert.equal(project(f).overview.waiting_delivery,1);
 integrate(f,true);assert.equal(project(f).percent,100);assert.equal(project(f).overview.completed,1);f.integration.delivered_at=undefined;assert(project(f).percent<100);
});
test('integration conflicts, missing proof or another plan never count integration or delivery',()=>{
 for(const change of [r=>r.status='CONFLICT',r=>r.plan_id='foreign',r=>r.review.stale=true,r=>r.evidence.tests[0].source_digest='old',r=>r.candidates['task-0'].digest='other']){const f=integrate(accept(fixture()),true);change(f.integration);assert(project(f).percent<100);}
});
test('live authorized test events show TESTING without claiming a passed test',()=>{
 const f=fixture();f.events=[{worker_id:'worker-0',task_id:'task-0',time:time(2),event:{type:'tool',name:'mcp__atm_worker__run_authorized_test',phase:'running'}}];assert.equal(project(f).tasks[0].state,'TESTING');assert.equal(project(f).tasks[0].updated_at,time(2));assert.equal(project(f).overview.testing,1);assert.equal(project(f).completed,0);
});
test('unknown plan or missing schedule data yields indeterminate progress; projection is read-only and restart stable',()=>{
 const f=accept(fixture(2)),before=structuredClone(f);assert.deepEqual(project(f),project(structuredClone(f)));assert.deepEqual(f,before);
 f.plan.validation=undefined;assert.equal(project(f).percent,null);f.plan.validation='VALIDATED_NOT_EXECUTED';f.tasks.pop();assert.equal(project(f).total,null);
});
test('timeline uses recorded timestamps and reasons, never fills missing times with the current clock',()=>{
 const f=fixture();f.tasks[0].status='WAITING_REVIEW';f.tasks[0].reason='缺少接口证明';f.tasks[0].events.push({time:time(5),to:'WAITING_REVIEW',from:'RUNNING',reason:'缺少接口证明'});const p=project(f).tasks[0];
 assert.equal(p.timeline[0].finished_at,time(5));assert.equal(p.timeline[1].finished_at,undefined);assert.equal(p.steps[2].reason,'缺少接口证明');assert.equal(p.steps[2].finished_at,undefined);
});

test('failed test and blocked review attempts keep their actual finish timestamps without becoming passed',()=>{
 const f=accept(fixture());f.tasks[0].status='RUNNING';f.candidates[0].tests[0].status='FAILED';let p=project(f).tasks[0];assert.equal(p.steps[1].state,'FAILED');assert.equal(p.steps[1].finished_at,time(3));
 f.candidates[0].tests[0].status='PASSED';f.tasks[0].status='WAITING_REVIEW';f.reviews['task-0'][0].verdict='BLOCKED';p=project(f).tasks[0];assert.equal(p.steps[2].state,'BLOCKED');assert.equal(p.steps[2].finished_at,time(5));assert(p.percent<100);
});
test('selected Worker receives the backend progress projection in the actual TeamSession snapshot',()=>{
 const f=accept(fixture()),team=new api.TeamSession({root:'/fixture',directory:'/fixture',services:{},editors:async()=>[],authorize:async()=>false,approve:async()=> 'DENY'}),p=project(f);
 team.workflow={snapshot:()=>({...f,record:{},workers:[]}),progress:()=>p};assert.equal(team.snapshot().progress.tasks[0].task_id,'task-0');
});

test('executor projects live authenticated test evidence and source checkpoints without logs or CLI completion claims',()=>{
 const e=Object.create(api.TeamTaskExecutor.prototype),result={id:'actual',command:'python -B a.py',snapshot_digest:'source',exit_code:0,test_count:3,started_at:time(2),finished_at:time(3)};
 e.artifacts=new Map([['a',{task_id:'a',worker_id:'w',before:{gaps:[]},after:{digest:'source',gaps:[]},tests:[],audit:[{entry:{kind:'ISOLATED_TEST_RESULT',data:result}}],evidence:{changes:[{path:'a.py'}],after:{digest:'source',time:time(1)},delivery:{status:'CANCELLED',final_answer:'not a completion claim'}}}]]);
 const p=e.progressCandidates()[0];assert(p.captured);assert.equal(p.tests[0].test_count,3);assert.equal(p.tests[0].status,'PASSED');assert.equal(p.tests[0].output,'');assert(!JSON.stringify(p).includes('final_answer'));
 e.artifacts.get('a').after.digest='modified';assert(!e.progressCandidates()[0].captured);assert.equal(e.progressCandidates()[0].tests.length,0);
 const a=e.artifacts.get('a');a.after.digest='source';a.tests=[{...result,id:'latest',finished_at:time(9)}];a.audit[0].entry.data={...result,exit_code:1};assert.equal(e.progressCandidates()[0].tests[0].status,'PASSED');assert.equal(e.progressCandidates()[0].tests[0].id,'latest');
});
