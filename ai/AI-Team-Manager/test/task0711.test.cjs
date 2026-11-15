const test=require('node:test'),assert=require('node:assert/strict'),fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');
const api=require('../.tools/task0711/api.cjs');
const task=(id,deps=[])=>({task_id:id,task_name:id,priority:3,parent_task_id:null,task_description:id,assigned_worker:id,allowed_files:[id+'.py'],read_only_files:[],forbidden_files:[],dependencies:deps,interface_contracts:['pure independent interface'],acceptance_criteria:['real tests'],test_commands:['python -B '+id+'.py'],base_commit:'a'.repeat(40),status:'PENDING'});
const proposal=()=>({orchestration_version:2,analysis:{scale:'MEDIUM',technical_domain:'Python',estimated_work:'two independent modules',technical_complexity:'MEDIUM',modules:['a','b'],expected_files:['a.py','b.py'],dependency_reasoning:'independent functions; c consumes their stable interfaces',parallelizable:true,risks:[],human_confirmation:[],worker_rationale:'two independent modules'},evidence_id:'e',task_type:'MULTI_MODULE',goal:'two helpers',current_project_state:'baseline',technical_approach:'isolated pure modules',worker_count:2,parallel_rationale:'independent',tasks:[task('a'),task('b')],shared_file_owners:[],out_of_scope:[],completion_policy:'STOP_AFTER_REQUEST'});
test('structured analysis binds meaningful fields, worker cap, simplicity and priority',()=>{
 const c={evidence_id:'e',base_commit:'a'.repeat(40),original_request:'implement helpers',continuous_authorized:false};assert.equal(api.validateDevelopmentPlan(proposal(),c,2).worker_count,2);
 for(const mutate of [p=>p.analysis.scale='SIMPLE',p=>p.analysis.parallelizable=false,p=>p.analysis.expected_files=['../escape'],p=>p.tasks[0].priority=0,p=>delete p.tasks[0].task_name,p=>delete p.analysis.worker_rationale]){const p=proposal();mutate(p);assert.throws(()=>api.validateDevelopmentPlan(p,c,2));}
 assert.throws(()=>api.validateDevelopmentPlan(proposal(),c,1));
 const schema=api.planningTools.find(t=>t.name==='submit_adaptive_plan').inputSchema;assert(schema.required.includes('analysis'));assert(schema.required.includes('orchestration_version'));
});
test('queued leases wait for real release, cancel, and remain project-isolated',async()=>{
 const dir=await fs.mkdtemp(path.join(os.tmpdir(),'atm0711-lock-')),locks=new api.FileLockManager(dir,'p'),first=await locks.acquire('a','w1',['shared.py']),states=[];
 let acquired=false;const pending=api.acquireQueuedLease(locks,'b','w2',['shared.py'],new AbortController().signal,async waiting=>states.push(waiting)).then(l=>{acquired=true;return l;});
 await new Promise(r=>setTimeout(r,80));assert(!acquired);assert.deepEqual(states,[true]);
 const other=new api.FileLockManager(dir,'other'),isolated=await other.acquire('a','w1',['shared.py']);await other.release(isolated.id);
 await locks.release(first.id);const second=await pending;assert.deepEqual(states,[true,false]);
 const abort=new AbortController(),blocked=api.acquireQueuedLease(locks,'c','w3',['shared.py'],abort.signal);setTimeout(()=>abort.abort(),40);await assert.rejects(blocked,/cancelled/);assert.equal((await locks.list())[0].id,second.id);await locks.release(second.id);
});
test('scheduler reviews first available candidate without waiting for unrelated running worker; cap changes gate the queue',async()=>{
 let finishB;const b=new Promise(r=>finishB=r),ran=[];const p={tasks:[task('a'),task('b'),task('c',['a'])],max_concurrency:2};
 const s=new api.TaskDependencyScheduler(p,{execute:async t=>{ran.push(t.task_id);if(t.task_id==='b')await b;return {worker_id:t.task_id,candidate:{}};},stop:async()=>{}},async()=>{});
 await s.start();await s.waitForReview();assert.equal(s.snapshot().find(r=>r.task.task_id==='a').status,'REVIEWING');assert.equal(s.snapshot().find(r=>r.task.task_id==='b').status,'RUNNING');
 await s.setConcurrency(1);await s.reviewed('a',{task:{task_id:'a'},review_id:'unit-gate',verdict:'PASS',missing_evidence:[],issues:[],stale:false});assert(!ran.includes('c'));finishB();await s.wait();assert(ran.includes('c'));assert(s.snapshot().every(t=>t.status!=='COMPLETED'));await assert.rejects(s.setConcurrency(0));
});
test('supervision records only durable key events and independent review evidence, deduplicates unchanged states',()=>{
 const a={task_id:'a',worker_id:'w',before:{time:'2026-11-14'},audit:[{entry:{decision:'DENY',time:'now'}},{entry:{kind:'FILE_WRITE_RESULT'}}]},r={task:task('a'),events:[{time:'now',from:'READY',to:'RUNNING'}]};
 const journal=api.collectSupervision([], [r],[a],{});assert.equal(journal.length,3);assert(journal.some(e=>e.decision==='HUMAN'));assert.deepEqual(api.collectSupervision(journal,[r],[a],{}),journal);assert(journal.every(e=>e.reason&&e.evidence.length));
 const report={review_id:'r',task:{task_id:'a'},stale:false,verdict:'PASS',issues:[],summary:'actual unit fixture',missing_evidence:[],batches:[],evidence:{id:'e',changes:[{path:'a.py'}],tests:[{id:'t'}]}};assert.equal(api.reviewSummary(report,'w').review_status,'PASS');report.stale=true;assert.equal(api.reviewSummary(report,'w').review_status,'FAILED');
});

test('a worker waiting for a file lease cannot be assigned a second task',async()=>{
 let release;const gate=new Promise(r=>release=r),started=[];
 const a=task('a'),b={...task('b'),assigned_worker:'a'};
 const s=new api.TaskDependencyScheduler({tasks:[a,b],max_concurrency:2},{execute:async(t,signal,onWait)=>{started.push(t.task_id);if(t.task_id==='a'){await onWait(true);await gate;await onWait(false);}return {worker_id:t.assigned_worker,candidate:{}};},stop:async()=>{}},async()=>{});
 await s.start();assert.equal(s.snapshot()[0].status,'WAITING_FILE_LOCK');assert.deepEqual(started,['a']);
 release();await s.wait();assert.deepEqual(started,['a']);
 await s.reviewed('a',{task:{task_id:'a'},review_id:'lease-owner',verdict:'PASS',missing_evidence:[],issues:[],stale:false});await s.wait();assert.deepEqual(started,['a','b']);
});
test('same content at authenticated write gateway is checkpointed without another disk write',async()=>{
 const dir=await fs.mkdtemp(path.join(os.tmpdir(),'atm0711-write-')),root=path.join(dir,'p');await fs.mkdir(root);await fs.writeFile(path.join(root,'a.py'),'x = 1\n');
 const locks=new api.FileLockManager(path.join(dir,'locks'),'p'),lease=await locks.acquire('a','w',['a.py']),own=new api.FileOwnershipManager({tasks:[task('a')],shared_file_owners:[]}),logs=[];
 const session=await new api.WorkerExecutionSession(root,'a','w',lease.id,own,locks,new api.IsolatedTestRunner(path.join(dir,'tests')),async()=> 'DENY',async e=>logs.push(e)).start();
 await session.runOptions.onProcess(process.pid);session.bindSession('11111111-1111-4111-8111-111111111111');const c=JSON.parse(session.runOptions.mcpConfig).mcpServers.atm_worker;
 const before=(await fs.stat(path.join(root,'a.py'))).mtimeMs;
 try{const result=await fetch(c.url,{method:'POST',headers:{...c.headers,'Content-Type':'application/json'},body:JSON.stringify({jsonrpc:'2.0',id:1,method:'tools/call',params:{name:'write_task_file',arguments:{file_path:'a.py',content:'x = 1\n'}}})}).then(r=>r.json());assert(!result.error);assert.equal((await fs.stat(path.join(root,'a.py'))).mtimeMs,before);assert(logs.some(e=>e.kind==='FILE_WRITE_SKIPPED'));assert(!logs.some(e=>e.kind==='FILE_WRITE_RESULT'));const failed=await fetch(c.url,{method:'POST',headers:{...c.headers,'Content-Type':'application/json'},body:JSON.stringify({jsonrpc:'2.0',id:2,method:'tools/call',params:{name:'edit_task_file',arguments:{file_path:'a.py',old_string:'missing source',new_string:'unsafe'}}})}).then(r=>r.json());assert(failed.error);assert(logs.some(e=>e.kind==='TOOL_FAILED'&&e.data.tool==='edit_task_file'));assert.equal(await fs.readFile(path.join(root,'a.py'),'utf8'),'x = 1\n');}finally{await session.close();}
});
test('compact team review sends a new file once with real line numbers and retains modified before/after evidence',()=>{
 const added={changes:[{path:'a.py',kind:'ADDED',diff:'+hello',after:{text:'def example():\n    return 1\n'}}]};const content=api.makeBatches(added,60000,true).map(b=>b.content).join('');assert(content.includes('1: def example():'));assert(!content.includes('GIT_DIFF'));assert.equal(content.split('def example').length,2);
 const modified={changes:[{...added.changes[0],kind:'MODIFIED',before:{text:'def example():\n    return 0\n'}}]};const m=api.makeBatches(modified,60000,true).map(b=>b.content).join('');assert(m.includes('BEFORE'));assert(m.includes('AFTER'));assert(m.includes('GIT_DIFF'));
});
test('decision wait coalesces ordinary updates, returns required approval, validates owner and bounds output',async()=>{
 const dir=await fs.mkdtemp(path.join(os.tmpdir(),'atm0711-wait-')),team=new api.TeamSession({root:dir,directory:dir,services:{},editors:async()=>[],authorize:async()=>false,approve:async()=> 'DENY'});team.workflow={plan:{plan_id:'p',owner_id:'owner'}};
 const state={terminal:false,busy:true,workflow:{plan:{plan_id:'p'},tasks:[{task:task('a'),status:'RUNNING'}],record:{reviews:{},supervision:[{id:'event'}]}}};team.snapshot=()=>({...state,revision:team.revision});
 const started=Date.now(),pending=team.call('get_team_task_output',{plan_id:'p',revision:0,wait_ms:500,until_decision:true},'owner');
 setTimeout(()=>{team.revision=1;for(const l of team.listeners)l();},10);setTimeout(()=>{state.workflow.tasks[0].status='WAITING_APPROVAL';team.revision=2;for(const l of team.listeners)l();},40);
 const result=await pending;assert(Date.now()-started>=35);assert.equal(result.workflow.tasks[0].status,'WAITING_APPROVAL');assert(!JSON.stringify(result).includes('audit'));assert(!result.detail);
 await assert.rejects(team.call('get_team_task_output',{plan_id:'p'},'foreign'));await assert.rejects(team.call('get_team_task_output',{plan_id:'p',wait_ms:60001},'owner'));
 const unchanged=await team.call('get_team_task_output',{plan_id:'p',revision:2},'owner');assert.equal(unchanged.unchanged,true);assert.deepEqual(unchanged.workflow.events,[]);
});
test('modified review includes exact old hunk lines without repeating unrelated before source',()=>{
 const before=Array.from({length:100},(_,i)=>'old_value_'+(i+1)+' = '+i).join('\n'),after=before.replace('old_value_50 = 49','new_value_50 = 50'),e={changes:[{path:'a.py',kind:'MODIFIED',before:{text:before},after:{text:after},diff:'@@ -48,5 +48,5 @@\n-old_value_50 = 49\n+new_value_50 = 50\n'}]};
 const text=api.makeBatches(e,60000,true)[0].content,part=text.split('BEFORE changed-hunk context')[1].split('FILE a.py MODIFIED AFTER')[0];assert(part.includes('50: old_value_50 = 49'));assert(!part.includes('1: old_value_1 = 0'));assert(text.includes('50: new_value_50 = 50'));
});
test('dependency proof rejects changed source before any independent model request',async()=>{
 const snapshot=files=>({root:'fixture',head:'a'.repeat(40),time:'fixture',files,gaps:[],digest:api.digest(JSON.stringify(files.map(f=>[f.path,f.hash])))});
 const source=text=>({path:'up.py',hash:api.digest(text),text});
 const upstream=snapshot([source('def original(): pass')]),before=snapshot([source('def altered(): pass')]);
 const after=snapshot([...before.files,{path:'child.py',hash:api.digest('import up'),text:'import up'}]);
 const child={before,after,worker_id:'worker',audit:[]},dependency={after:upstream,evidence:{after:upstream,changes:[{path:'up.py',after:upstream.files[0]}],tests:[]}};
 let modelCalls=0;
 const wf=Object.create(api.TeamWorkflow.prototype);Object.assign(wf,{plan:{plan_id:'p',tasks:[{task_id:'child',dependencies:['up'],allowed_files:['child.py'],forbidden_files:[]},{task_id:'up',acceptance_criteria:['tested']}]},services:{codex:()=>({check:async()=>{modelCalls++;throw Error('must reject before CLI preflight');},run:()=>{modelCalls++;throw Error('must not run');}})},scheduler:{snapshot:()=>[]},executor:{workers:{list:()=>[]},candidate:id=>id==='child'?child:dependency},record:{reviews:{up:[{verdict:'PASS',stale:false,review_id:'r',evidence:{after:upstream}}]}},collector:{},storage:{},ledger:{},changed:()=>{}});
 await assert.rejects(wf.review({task:{task_id:'child'},before,after},'REVIEW','child'),/differs from reviewed version/);
 assert.equal(modelCalls,0);
});
test('configured repair cap is shared across local retries and GPT-directed repairs',async()=>{
 const executor=Object.create(api.TeamTaskExecutor.prototype);let calls=0;Object.assign(executor,{plan:{orchestration_version:2,max_repair_rounds:2},artifacts:new Map([['a',{autonomous_repairs:1,review_repairs:1}]]),persist:async()=>{},run:async()=>{calls++;return {worker_id:'w',candidate:{after:{digest:'current'},evidence:{tests:[{status:'FAILED'}]}}};}});
 const result=await executor.execute({task_id:'a'},new AbortController().signal);assert.equal(calls,1);assert.equal(result.candidate.evidence.tests[0].status,'FAILED');
});
test('English scope clauses stop at sentence boundaries, preserve filename dots, and never override explicit denial',()=>{
 const a=api.userAuthority('Explicitly only normalize.py may be modified; do not change README.md. Run python -B normalize.py.');assert.deepEqual(a.allowed,['normalize.py']);assert.deepEqual(a.forbidden,['README.md']);
 const b=api.userAuthority('Create ratio.py. Do not change ratio.py. Run python -B labels.py.');assert.deepEqual(b.allowed,[]);assert.deepEqual(b.forbidden,['ratio.py']);
 assert.throws(()=>new api.AdaptiveTaskPlanner('',{}, {},()=>{},3,6));assert.throws(()=>new api.AdaptiveTaskPlanner('',{}, {},()=>{},3,2,0));
});
