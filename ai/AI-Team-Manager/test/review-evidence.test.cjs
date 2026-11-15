const test=require('node:test'),assert=require('node:assert/strict'),fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');
const api=require('../.tools/task0711/api.cjs');
const file=(p,text)=>({path:p,text,hash:api.digest(text)});
const snapshot=files=>({root:'fixture',time:new Date().toISOString(),head:'a'.repeat(40),files,gaps:[],digest:api.digest(JSON.stringify(files.map(f=>[f.path,f.hash])))});
const task=(id='a',deps=[])=>({task_id:id,task_description:'add helper',assigned_worker:id,allowed_files:[id+'.py'],read_only_files:[],forbidden_files:['existing.py'],dependencies:deps,parent_task_id:null,interface_contracts:[],acceptance_criteria:['tests'],test_commands:['python -B '+id+'.py'],base_commit:'a'.repeat(40),status:'PENDING'});
const report=(verdict='BLOCKED')=>({review_id:'review',task:{task_id:'a'},verdict,summary:verdict+' 0',issues:[],missing_evidence:['required local dependency unavailable'],dimensions:[],batches:[],stale:false,evidence:{id:'proof',changes:[],tests:[{id:'t',command:'python -B a.py',status:'PASSED',test_count:3}]}});

test('small Python task ignores unrelated repository tests while keeping direct transitive imports and required test source',()=>{
 const a=file('a.py','from helper import add\n'),helper=file('helper.py','from nested import base\n'),nested=file('nested.py','base = 1\n'),testSource=file('test_a.py','import a\n');
 const after=snapshot([a,helper,nested,testSource,...Array.from({length:30},(_,i)=>file('tests/unrelated'+i+'.py','x'.repeat(4000)))]);
 const selected=api.selectReviewContext(after,[{path:'a.py',after:a}],task(),[{command:'python -B test_a.py'}]);
 assert.deepEqual(selected.context.map(f=>f.path).sort(),['helper.py','nested.py','test_a.py']);assert.deepEqual(selected.gaps,[]);
});
test('necessary context overflow remains blocking, optional background overflow is a warning',()=>{
 const a=file('a.py','from helper import add\n'),huge=file('helper.py','x'.repeat(64001));
 assert(api.selectReviewContext(snapshot([a,huge]),[{path:'a.py',after:a}],task(),[]).gaps.some(g=>g.includes('helper.py')));
 const selected=api.selectReviewContext(snapshot([a,file('README.md','x'.repeat(64001))]),[{path:'a.py',after:{text:''}}],task(),[]);
 assert.equal(selected.gaps.length,0);assert.equal(selected.warnings.length,1);
 const missing=api.selectReviewContext(snapshot([a]),[{path:'a.py',after:a}],{...task(),context_files:['missing.py']},[]);assert(missing.gaps.some(g=>g.includes('missing.py')));
});
test('host proof binds all collected changes and detects protected changes, outside scope and forged digest',()=>{
 const old=snapshot([file('existing.py','user content')]),now=snapshot([...old.files,file('a.py','value = 1\n')]);
 const proof=api.snapshotScopeProof(old,now,task());assert(proof.complete);assert.equal(proof.unchanged_files,1);assert.deepEqual(proof.changes,[{path:'a.py',before_hash:null,after_hash:now.files[1].hash}]);assert(proof.protected_files[0].unchanged);
 const bad=snapshot([file('existing.py','overwritten'),file('a.py','value = 1\n')]);assert(api.snapshotScopeProof(old,bad,task()).outside_allowed.includes('existing.py'));assert(!api.snapshotScopeProof(old,bad,task()).protected_files[0].unchanged);
 assert(!api.snapshotScopeProof(old,{...now,digest:'forged'},task()).complete);
});
test('authenticated write chain proves new file absence and rejects incompatible or foreign audit records',()=>{
 const before=snapshot([]),after=snapshot([file('a.py','two')]),one=api.digest('one'),two=api.digest('two');
 const entry=(old,next,worker='w')=>({task_id:'a',worker_id:worker,entry:{kind:'FILE_WRITE_RESULT',data:{file_path:'a.py',before_hash:old,after_hash:next}}});
 const proof=api.executionWriteProof(before,after,'a','w',[entry(null,one),entry(one,two),entry(null,two,'foreign')]);assert.deepEqual(proof.writes,[{path:'a.py',count:2,before_hash:null,after_hash:two,matches_snapshots:true}]);
 assert(!api.executionWriteProof(before,after,'a','w',[entry(one,two)]).writes[0].matches_snapshots);
});
test('context collector and prompt preserve full scope proof without exposing unrelated test sources',async()=>{
 const dir=await fs.mkdtemp(path.join(os.tmpdir(),'atm-review-fix-')),before=snapshot([file('existing.py','user data')]),after=snapshot([...before.files,file('a.py','value = 1\n'),...Array.from({length:20},(_,i)=>file('tests/unrelated'+i+'.py','x'.repeat(4000)))]);
 // Unrelated files already existed at the baseline; only a.py is a candidate delta.
 before.files.push(...after.files.slice(2));before.digest=api.digest(JSON.stringify(before.files.map(f=>[f.path,f.hash])));
 const e=await new api.ReviewGit(dir).evidence(before,after,task(),{status:'COMPLETED',final_answer:''},[{source_digest:after.digest,status:'PASSED',command:'python -B a.py'}],'CLAUDE_CAPTURE');
 assert.deepEqual(e.gaps,[]);assert.deepEqual(e.changes.map(c=>c.path),['a.py']);assert.equal(e.context.length,0);assert(e.scope_proof.complete);
 const prompt=api.reviewPrompt(e,api.makeBatches(e,120000,true)[0],1);assert(prompt.includes('HOST SCOPE PROOF'));assert(prompt.includes('"before_hash":null'));assert(!prompt.includes('unrelated0.py'));
});
test('BLOCKED holds candidate for explicit re-review, blocks downstream, and never reruns Claude',async()=>{
 let calls=0;const p={tasks:[task(),task('child',['a'])],max_concurrency:2},s=new api.TaskDependencyScheduler(p,{execute:async()=>{calls++;return {worker_id:'w',candidate:{digest:'same'}};},stop:async()=>{}},async()=>{});
 await s.start();await s.wait();await s.reviewed('a',report());assert.equal(s.snapshot()[0].status,'WAITING_REVIEW');assert.equal(s.snapshot()[1].status,'WAITING_DEPENDENCY');assert(s.snapshot()[0].reason.includes('required local dependency'));assert.equal(calls,1);
 await s.resume('a');assert.equal(s.snapshot()[0].status,'REVIEWING');assert.equal(calls,1);
 await s.reviewed('a',{...report('PASS'),missing_evidence:[]});await s.wait();assert.equal(calls,2);
});
test('failed tests remain failed; human review and service errors are distinct actionable states',async()=>{
 const make=async()=>{const s=new api.TaskDependencyScheduler({tasks:[task()],max_concurrency:1},{execute:async()=>({worker_id:'w',candidate:{}}),stop:async()=>{}},async()=>{});await s.start();await s.wait();return s;};
 const failed=report();failed.evidence.tests[0].status='FAILED';const s=await make();await s.reviewed('a',failed);assert.equal(s.snapshot()[0].status,'FAILED');
 const human=await make();await human.reviewed('a',report('NEEDS_HUMAN_REVIEW'));assert.equal(human.snapshot()[0].status,'WAITING_APPROVAL');
 const service=report();service.batches=[{error:'review request timeout',summary:'not completed'}];assert.equal(api.reviewBlock(service).kind,'REVIEW_SERVICE_ERROR');assert.equal(api.reviewBlock(report()).kind,'MISSING_EVIDENCE');
 const summary=api.reviewSummary(report(),'w');assert.equal(summary.review_status,'WAITING_REVIEW');assert(summary.block.reasons.length);assert.equal(summary.tests[0].test_count,3);
});
test('WAITING_REVIEW survives restart without automatic write or request and requires explicit resume',async()=>{
 const p={tasks:[task()],max_concurrency:1},executor={execute:async()=>({worker_id:'w',candidate:{}}),stop:async()=>{}};
 const s=new api.TaskDependencyScheduler(p,executor,async()=>{});await s.start();await s.wait();await s.reviewed('a',report());let calls=0;
 const restored=new api.TaskDependencyScheduler(p,{...executor,execute:async()=>{calls++;throw Error('must not run');}},async()=>{});await restored.restore(s.snapshot());assert.equal(restored.snapshot()[0].status,'PAUSED');await restored.resume('a');assert.equal(restored.snapshot()[0].status,'REVIEWING');assert.equal(calls,0);
});

test('host preflight evidence errors remain actionable without starting downstream or rewriting a candidate',async()=>{
 let writes=0;const p={tasks:[task(),task('child',['a'])],max_concurrency:2},s=new api.TaskDependencyScheduler(p,{execute:async()=>{writes++;return {worker_id:'w',candidate:{}};},stop:async()=>{}},async()=>{});
 await s.start();await s.wait();await s.reviewUnavailable('a','Dependency source differs from reviewed version');assert.equal(s.snapshot()[0].status,'WAITING_REVIEW');assert.equal(s.snapshot()[1].status,'WAITING_DEPENDENCY');assert.equal(writes,1);
});
