// Real providers + production planner, controller, session, isolated tools, review and integration.
// Only the disposable fixture project is written. No precomputed model replies or fake progress.
const fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os'),assert=require('node:assert/strict');
const api=require('../.tools/task0710/api.cjs');
(async()=>{
 const dir=path.resolve('test-results','task0710-live-'+Date.now()),root=path.join(dir,'project');await fs.mkdir(root,{recursive:true});
 await fs.writeFile(path.join(root,'README.md'),'Museum accession helpers. Pure Python standard library. Three independent modules, no network or dependencies. Each module includes unittest tests.\n');
 await api.git(root,['init','--quiet']);await api.git(root,['add','README.md']);await api.git(root,['-c','user.name=Acceptance','-c','user.email=acceptance@example.invalid','commit','--quiet','-m','acceptance fixture']);
 const ext=path.join(os.homedir(),'.vscode','extensions'),installed=(await fs.readdir(ext)).filter(n=>n.startsWith('anthropic.claude-code-')).sort((a,b)=>b.localeCompare(a,undefined,{numeric:true}));
 const services={codex:()=>new api.CodexService('codex'),claude:()=>new api.ClaudeService('claude',undefined,c=>api.resolveClaudeExecutable(c,{extensionDirectory:()=>path.join(ext,installed[0])}))};
 const store=new api.ReviewStorage(path.join(dir,'reviews')),planner=new api.AdaptiveTaskPlanner(root,new api.ReviewGit(path.join(dir,'evidence')),store);await planner.load();
 let team,chat,editors=[],lastWrite=0;const snapshots=[],phases=new Set();
 const changed=()=>{if(!team||!chat)return;const t=team.snapshot();const phase=t.workflow?.tasks.map(t=>t.status).join(',');if(phase&&!phases.has(phase)){phases.add(phase);console.log('Actual tasks: '+phase);}
  if(Date.now()-lastWrite>1000){lastWrite=Date.now();snapshots.push({type:'snapshot',project:'Museum accession acceptance',chat:{...chat.snapshot(),projectName:'Museum accession acceptance'},dispatch:{team:t}});}
 };
 team=new api.TeamSession({root,directory:path.join(dir,'team'),services,editors:async()=>editors,authorize:async()=>true,approve:async()=> 'DENY',changed});await team.load();
 let toolCalls=[],pendingSelect,selectionChecks=0,paused=false,resumed=false;
 chat=new api.ChatController(new api.ConversationStorage(path.join(dir,'chat'),root),services.codex(),changed,async()=>{},()=>1200000,async(project,text,owner,turn)=>{
  await team.beginTurn(turn);
  const bridge=new api.ToolBridge(async(name,args)=>{
   toolCalls.push({name,time:new Date().toISOString()});
   if(name==='get_project_planning_context')return planner.capture(owner,turn,text);
   if(name==='submit_adaptive_plan')return planner.submit(owner,turn,args);
   if(name==='execute_adaptive_plan'){const plan=planner.snapshot().plans.find(p=>p.plan_id===args.plan_id);assert(plan);return team.execute(plan,owner,text);}
   return team.call(name,args,owner);
  },async()=>{},[...api.planningTools,...api.teamTools]);
  const options=await bridge.start();options.instructions=api.PLANNING_INSTRUCTIONS+'\n'+api.TEAM_INSTRUCTIONS+'\n本次是用户授权的真实端到端验收。必须采集上下文、提交三个独立任务的有效计划并调用 execute_adaptive_plan；每个模块独立，不需要访问其他模块。审批由插件执行。不要使用 shell 或其他工具。持续轮询到 terminal，READY 表示尚未交付。';
  return {options,timeoutMs:1200000,finalize:r=>team.finishTurn(turn,r),close:()=>bridge.close()};
 });await chat.initialize();
 const user='请实际开发一个博物馆藏品编号辅助库，新增三个独立 Python 标准库模块：labels.py 提供 normalize_label(text)，返回去除首尾空白并将连续空白折叠成单空格的字符串；checksum.py 提供 accession_checksum(digits)，对仅含 ASCII 数字的字符串返回各位数字之和模 10，空字符串返回 0，非数字输入抛 ValueError；ranges.py 提供 overlaps(a,b,c,d)，返回两个半开整数区间 [a,b) 与 [c,d) 是否有交集，空区间返回 False。明确允许仅修改 labels.py、checksum.py、ranges.py，禁止修改 README.md。每个文件内包含至少三项 unittest 测试和 unittest.main() 入口，分别执行 python -B labels.py、python -B checksum.py、python -B ranges.py。三个模块互不依赖，可创建三个 Worker 并行完成，完成后测试、独立审查和安全集成准备；最终交付仍等待用户确认。';
 const monitor=setInterval(()=>{
  if(pendingSelect||!team.workflow)return;
  pendingSelect=(async()=>{
   const workers=team.workflow.executor.workers.list();
   for(const w of workers){const before=team.workflow.executor.workers.detail(w.worker_id).chat.conversation?.requests.length||0;await team.select(w.worker_id);const after=team.workflow.executor.workers.detail(w.worker_id).chat.conversation?.requests.length||0;assert.equal(after,before);selectionChecks++;}
   // Pause one actual running CLI after its Session exists, then resume the original scheduler task.
   const target=workers[0],row=target&&team.workflow.scheduler.snapshot().find(r=>r.task.task_id===target.task_id);
   if(!paused&&workers.length===3&&target.session_id&&row?.status==='RUNNING'){
    paused=true;const sessions=workers.slice(1).map(w=>w.session_id);await team.control('pause',target.task_id);
    assert.equal(team.workflow.scheduler.snapshot().find(r=>r.task.task_id===target.task_id).status,'PAUSED');
    const others=team.workflow.executor.workers.list().slice(1);assert(others.every(w=>w.status!=='PAUSED'));assert(others.every((w,i)=>!sessions[i]||w.session_id===sessions[i]));
    await team.instruct(target.worker_id,'继续原任务，保留既有实现，仅在原文件范围内完成测试。');await team.control('resume',target.task_id);resumed=true;
   }
  })().catch(e=>{console.error('Monitor assertion: '+e.message);process.exitCode=1;}).finally(()=>{pendingSelect=undefined;});
 },700);
 let result;
 try{
  console.log('Real acceptance output: '+dir);await chat.send(user);await team.wait();clearInterval(monitor);await pendingSelect;
  const c=chat.snapshot(),t=team.snapshot();
  await fs.writeFile(path.join(dir,'before-delivery.json'),JSON.stringify({chat:c,team:t,toolCalls,selectionChecks,paused,resumed},null,2));
  assert.equal(c.conversation.status,'COMPLETED',c.error);assert(toolCalls.some(c=>c.name==='execute_adaptive_plan'));assert(t.workflow.workers.length>=3);assert.equal(t.integration.status,'READY',t.integration.error||JSON.stringify(t.workflow.tasks));
  assert(paused&&resumed,'real targeted pause and same-session resume must be exercised');assert(selectionChecks>3);assert.equal(new Set(t.workflow.workers.map(w=>w.session_id)).size,3);
  assert.equal((await api.git(root,['status','--porcelain'])).trim(),'');
  await assert.rejects(team.integration.deliver(false),/确认/);editors=[{file:'labels.py',dirty:true,version:1,buffer_hash:'UNSAVED_ACCEPTANCE',open:true}];await assert.rejects(team.integration.deliver(true),/WAITING_INTEGRATION/);assert.equal((await api.git(root,['status','--porcelain'])).trim(),'');editors=[];
  await team.integration.deliver(true,false);assert.equal(team.snapshot().integration.status,'DELIVERED');
  const oldSessions=team.workflow.executor.workers.list().map(w=>w.session_id),oldRequests=team.workflow.executor.workers.list().map(w=>team.workflow.executor.workers.detail(w.worker_id).chat.conversation.requests.length);
  const finalSnapshot={type:'snapshot',project:'Museum accession acceptance',chat:{...chat.snapshot(),projectName:'Museum accession acceptance'},dispatch:{team:team.snapshot()}};snapshots.push(finalSnapshot);
  await fs.writeFile(path.join(dir,'runtime-snapshot.json'),JSON.stringify(finalSnapshot));await fs.writeFile(path.join(dir,'runtime-snapshots.json'),JSON.stringify(snapshots));
  const details=[];for(const w of team.workflow.executor.workers.list()){await team.select(w.worker_id);details.push(team.snapshot());}await fs.writeFile(path.join(dir,'worker-details.json'),JSON.stringify(details));
  await team.dispose();team=new api.TeamSession({root,directory:path.join(dir,'team'),services,editors:async()=>[],authorize:async()=>false,approve:async()=> 'DENY'});await team.load();
  assert.deepEqual(team.workflow.executor.workers.list().map(w=>w.session_id),oldSessions);assert.deepEqual(team.workflow.executor.workers.list().map(w=>team.workflow.executor.workers.detail(w.worker_id).chat.conversation.requests.length),oldRequests);
  result={passed:!process.exitCode,realGPT:true,realClaude:true,threeIndependentSessions:true,selectionChecks,targetPauseResume:true,deliveryEditorProtection:true,sameSessionsAfterReload:true,noRequestsOnReload:true,toolCalls,usage:team.ledger.snapshot()};
  assert(!process.exitCode);await fs.writeFile(path.join(dir,'result.json'),JSON.stringify(result,null,2));console.log(JSON.stringify(result));
 }catch(e){await fs.writeFile(path.join(dir,'failure.json'),JSON.stringify({error:e.stack,chat:chat.snapshot(),team:team.snapshot(),toolCalls},null,2));throw e;}
 finally{clearInterval(monitor);await pendingSelect;await team.dispose();}
})().catch(e=>{console.error(e);process.exitCode=1;});
