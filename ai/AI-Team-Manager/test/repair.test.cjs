// Deterministic guard tests; these are not claimed as real GPT/Claude acceptance.
const {test}=require('node:test'),assert=require('node:assert/strict'),fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');
const api=require('../.tools/task02-test/api.cjs'),{create}=require('./review-fixture.cjs');
const sample=require('./fixtures/review/cases.cjs')[1];
async function fixture(){
  const dir=await fs.mkdtemp(path.join(os.tmpdir(),'atm-repair-')),root=path.join(dir,'project');await create(root,sample);
  await fs.writeFile(path.join(root,'worker.cjs'),sample.after);
  const git=new api.ReviewGit(path.join(dir,'temp')),before=await git.snapshot(root),storage=new api.ReviewStorage(path.join(dir,'history'));
  const issue={issue_id:'B1-1',severity:'HIGH',category:'LOGIC',file_path:'worker.cjs',line_number:3,basis:'after',evidence_status:'VERIFIED',evidence:'if (value < 0) return Math.abs(value);',problem_description:'negative values must clamp to 0',impact:'wrong value',suggested_fix:'return 0',confidence:1};
  const report={review_id:'REV',project_path:root,stale:false,task:{task_id:'TASK',task_description:sample.task,technical_requirements:'keep run signature',allowed_files:['worker.cjs'],forbidden_files:['sample.test.cjs']},issues:[issue],evidence:{after:before,changes:[],context:[],delivery:{conversation_id:'CONV',session_id:'SID'}}};
  const raw={title:'Clamp negative input',problem_description:'negative values',root_cause:'absolute value instead of zero',target_files:['worker.cjs'],allowed_files:['worker.cjs'],forbidden_files:[],repair_strategy:'replace Math.abs(value) with 0',acceptance_criteria:['negative input returns zero'],test_requirements:['node --test'],risk_level:'LOW',delete_files:[],public_interface_changes:[],missing_context:[]};
  const state={projectPath:root,busy:false,conversation:{id:'CONV',threadId:'SID',requests:[],messages:[]}};
  let sent=0;
  const port={snapshot:()=>state,stop:async()=>{},sendRepair:async(text,c,s,plan,observe)=>{sent++;assert.equal(c,'CONV');assert.equal(s,'SID');assert.deepEqual(plan.allowed_files,['worker.cjs']);observe({type:'tool',name:'Edit',phase:'completed',detail:'worker.cjs'});await fs.writeFile(path.join(root,'worker.cjs'),sample.after.replace('Math.abs(value)','0'));state.conversation.requests.push({id:'REQ',status:'COMPLETED'});state.conversation.messages.push({role:'assistant',requestId:'REQ',text:'changed'});}};
  const model={plan:async(report,r)=>api.validatePlan(JSON.stringify(raw),report,r),stop:async()=>{}};
  const w=new api.RepairWorkflow(root,storage,git,model,{stop:async()=>{}},new api.ReviewTestRunner(git),port);
  return {dir,root,before,git,storage,issue,report,raw,state,w,getSent:()=>sent};
}
const approved=async(r,risks)=>({approved:true,risks,actor:'UNIT_TEST_APPROVER'});
test('unapproved/deferred/rejected repairs do not dispatch; selected issue IDs and dispositions persist',async()=>{
  const f=await fixture(),r=await f.w.generate(f.report,['B1-1']);assert.equal(f.getSent(),0);assert.equal((await f.git.snapshot(f.root)).digest,f.before.digest);
  assert.deepEqual(r.plan.issue_ids,['B1-1']);assert.equal(r.plan.review_id,'REV');assert.equal(r.plan.task_id,'TASK');
  await f.w.disposition(f.report,['B1-1'],'HUMAN');await f.w.disposition(f.report,['B1-1'],'IGNORED');assert.equal(f.w.dispositions.length,1);
  await f.w.execute(r.repair_id,async()=>({approved:false,risks:[],actor:'TEST_DECLINED'}));assert.equal(f.getSent(),0);
  await f.w.defer(r.repair_id);await f.w.defer(r.repair_id,true);await assert.rejects(f.w.execute(r.repair_id,approved));assert.equal(f.getSent(),0);
  await f.w.load();assert.equal(f.w.get(r.repair_id).state,'REJECTED');assert.equal(f.w.dispositions[0].review_id,'REV');
});
test('one approval dispatches once, captures real file diff and actual tests; duplicate sends fail across reload',async()=>{
  const f=await fixture(),r=await f.w.generate(f.report,['B1-1']);await f.w.execute(r.repair_id,approved);assert.equal(r.state,'EXECUTED');assert.equal(f.getSent(),1);assert(r.evidence.changes[0].diff.includes('+  if (value < 0) return 0;'));
  const check=await f.w.runCheck(r.repair_id,'node --test');assert.equal(check.status,'PASSED');assert(check.test_count>0);
  await f.w.load();await assert.rejects(f.w.execute(r.repair_id,approved));assert.equal(f.getSent(),1);
});
test('stale code, changed session, changed plan and missing separate risk approval fail closed',async()=>{
  for(const mode of ['source','session','plan','risk']){
    const f=await fixture(),r=await f.w.generate(f.report,['B1-1']);
    if(mode==='risk'){r.plan.risk_level='HIGH';r.plan_hash=api.planHash(r.plan);}
    await assert.rejects(f.w.execute(r.repair_id,async()=>{
      if(mode==='source')await fs.appendFile(path.join(f.root,'worker.cjs'),'// changed\n');
      if(mode==='session')f.state.conversation.threadId='OTHER';
      if(mode==='plan')r.plan.repair_strategy='tampered';
      return {approved:true,risks:[],actor:'UNIT_TEST'};
    }));assert.equal(f.getSent(),0,mode);
  }
});
test('interrupted durable dispatch never auto retries, and plan cannot override original file authorization',async()=>{
  const f=await fixture(),r=await f.w.generate(f.report,['B1-1']);
  r.sent_once=true;r.state='SENDING';await f.storage.write(f.root,`repair-${r.repair_id}.json`,r);await f.w.load();assert.equal(f.w.get(r.repair_id).state,'INTERRUPTED');await assert.rejects(f.w.execute(r.repair_id,approved));
  for(const file of ['sample.test.cjs','../escape.c','CON.c','.claude/settings.json'])assert.throws(()=>api.validatePlan(JSON.stringify({...f.raw,allowed_files:[file],target_files:[file]}),f.report,r));
});
test('repair permission policy is transient, denies scripts/unapproved deletion and still asks for approved destructive changes',async()=>{
  const f=await fixture(),scope={allowedFiles:['worker.cjs'],safeCommands:[]};
  const policy=new api.PermissionPolicy(f.root,scope,{deleteFiles:[]});
  assert.equal((await policy.check('Edit',{file_path:'worker.cjs',old_string:'Math.abs(value)',new_string:'0'})).behavior,'allow');
  assert.equal((await policy.check('Edit',{file_path:'sample.test.cjs',new_string:'bad'})).behavior,'deny');
  assert.equal((await policy.check('Bash',{command:'node --test'})).behavior,'deny');
  assert.equal((await policy.check('Bash',{command:'rm worker.cjs'})).behavior,'deny');
  assert.equal((await new api.PermissionPolicy(f.root,scope,{deleteFiles:['worker.cjs']}).check('Bash',{command:'rm worker.cjs'})).behavior,'ask');
});
test('verification preserves original IDs and refuses FIXED without test, code, execution or regression evidence',async()=>{
  const f=await fixture(),r=await f.w.generate(f.report,['B1-1']);
  const check={issue_id:'B1-1',status:'FIXED',explanation:'unit proof',file_path:'worker.cjs',line_number:3,evidence:'if (value < 0) return 0;'};
  const report={stale:false,verdict:'PASS',missing_evidence:[],issues:[],batches:[{repair_checks:[check]}],evidence:{changes:[],tests:[{status:'PASSED'}]}};
  assert.equal(api.resolveChecks(r,report)[0].status,'FIXED');
  assert.equal(api.resolveChecks(r,{...report,evidence:{changes:[],tests:[]}})[0].status,'UNVERIFIED');
  assert.equal(api.resolveChecks(r,{...report,missing_evidence:['test evidence missing']})[0].status,'UNVERIFIED');
  const bad={...report,batches:[{repair_checks:[{...check,status:'REGRESSION'}]}]};assert.equal(api.resolveChecks(r,bad)[0].status,'UNVERIFIED');
  const regression={...report,verdict:'REQUEST_CHANGES',issues:[{evidence_status:'VERIFIED',basis:'after',file_path:'worker.cjs',line_number:4,evidence:'return value;'}],evidence:{...report.evidence,changes:[{path:'worker.cjs',kind:'MODIFIED',before:{text:sample.after}}]},batches:[{repair_checks:[{...check,status:'REGRESSION',evidence:'  if (value < 0) return 0;\n  return value;'}]}]};assert.equal(api.resolveChecks(r,regression)[0].status,'REGRESSION');
  const unfixed={...report,verdict:'BLOCKED',missing_evidence:['unrelated build evidence missing'],batches:[{repair_checks:[{...check,status:'NOT_FIXED'}]}]};assert.equal(api.resolveChecks(r,unfixed)[0].status,'NOT_FIXED');
  const outside={...report,evidence:{...report.evidence,changes:[{path:'sample.test.cjs',kind:'MODIFIED'}]}};assert.equal(api.resolveChecks(r,outside)[0].status,'REGRESSION');
});
