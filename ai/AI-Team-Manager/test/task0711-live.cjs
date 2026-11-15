// Real model acceptance. Fault injection below is explicitly confined to the disposable C/D fixture.
const fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os'),assert=require('node:assert/strict');
const api=require('../.tools/task0711/api.cjs');
const stamp=Date.now(),directory=path.resolve('test-results','task0711-live-'+stamp);
let services;
async function fixture(name){const dir=path.join(directory,name),root=path.join(dir,'project');await fs.mkdir(root,{recursive:true});await fs.writeFile(path.join(root,'README.md'),'Isolated Task 07.11 acceptance fixture. Python standard library. Do not edit README.md.\n');await api.git(root,['init','--quiet']);await api.git(root,['add','README.md']);await api.git(root,['-c','user.name=Acceptance','-c','user.email=acceptance@example.invalid','commit','--quiet','-m','isolated fixture']);return {dir,root};}
async function providers(){const ext=path.join(os.homedir(),'.vscode','extensions'),names=await fs.readdir(ext),latest=prefix=>path.join(ext,names.filter(n=>n.startsWith(prefix)).sort((a,b)=>b.localeCompare(a,undefined,{numeric:true}))[0]);return {codex:()=>new api.CodexService(path.join(latest('openai.chatgpt-'),'bin','windows-x86_64','codex.exe')),claude:()=>new api.ClaudeService('claude',undefined,c=>api.resolveClaudeExecutable(c,{extensionDirectory:()=>latest('anthropic.claude-code-')}))};}
async function adaptive(name,user,{inject=false,scale}={}){
 const {dir,root}=await fixture(name),planner=new api.AdaptiveTaskPlanner(root,new api.ReviewGit(path.join(dir,'evidence')),new api.ReviewStorage(path.join(dir,'plans')),()=>{},3,2,900000);await planner.load();
 let team,chat,injected=false;const calls=[],phases=[],snapshots=[];let last=0;
 const changed=()=>{if(!team)return;const s=team.snapshot(),phase=s.workflow?.tasks.map(t=>t.task.task_id+':'+t.status).join(',');if(phase&&phases.at(-1)!==phase){phases.push(phase);console.log(name+' '+phase);}if(chat&&Date.now()-last>1500){last=Date.now();snapshots.push({type:'snapshot',project:name,chat:{...chat.snapshot(),projectName:name},dispatch:{team:s}});}};
 team=new api.TeamSession({root,directory:path.join(dir,'team'),services,editors:async()=>[],authorize:async()=>true,approve:async()=> 'DENY',changed});await team.load();await team.ledger.configure(5000000,false); // Explicit isolated-fixture budget, never a production budget change.
 chat=new api.ChatController(new api.ConversationStorage(path.join(dir,'chat'),root),services.codex(),changed,async()=>{},()=>1800000,async(project,text,owner,turn)=>{
  await team.beginTurn(turn);const bridge=new api.ToolBridge(async(name,args)=>{
   calls.push({name,time:new Date().toISOString(),args});
   if(name==='get_project_planning_context')return planner.capture(owner,turn,text);
   if(name==='submit_adaptive_plan')return planner.submit(owner,turn,args);
   if(name==='execute_adaptive_plan'){
    const p=planner.snapshot().plans.find(p=>p.plan_id===args.plan_id);assert(p);const result=await team.execute(p,owner,text);
    if(inject){const executor=team.workflow.executor,original=executor.execute.bind(executor);executor.execute=async(...args)=>{
     const result=await original(...args),a=executor.artifacts.get(args[0].task_id);
     if(!injected&&args[0].allowed_files.includes('data.py')){
      const file=path.join(a.workspace.workspace_path,'data.py'),before=await fs.readFile(file,'utf8');
      const broken=before.replace(/^def normalize_title\([^\n]*\)(?:\s*->[^:\n]+)?:[^\n]*\n(?:[ \t]+[^\n]*\n|\n)*/m,'def normalize_title(value):\n    return ""\n\n');
      assert.notEqual(broken,before,'controlled bug function must exist');await fs.writeFile(file,broken);a.after=await executor.collector.snapshot(a.workspace.workspace_path);
      const runner=new api.IsolatedTestRunner(path.join(dir,'injected-tests')),tests=[];
      for(const command of args[0].test_commands){const t=await runner.run(a.workspace.workspace_path,command,args[0].test_commands,a.after,[...args[0].allowed_files,...args[0].read_only_files]);a.tests.push(t);tests.push({id:t.id,command:t.command,started_at:t.started_at,finished_at:t.finished_at,status:t.exit_code===0&&t.test_count>0?'PASSED':'FAILED',test_count:t.test_count,exit_code:t.exit_code,output:t.output,output_truncated:t.truncated,source_digest:t.snapshot_digest});}
      await runner.drain();a.evidence=await executor.collector.evidence(a.before,a.after,a.evidence.task,a.evidence.delivery,tests,'CLAUDE_CAPTURE');injected=true;
      await fs.writeFile(path.join(dir,'controlled-fault.json'),JSON.stringify({test_only:true,reason:'User-requested isolated controlled bug; actual source and actual rerun evidence before independent GPT review',original:before,injected:broken,evidence:a.evidence},null,2));await executor.persist();result.candidate=structuredClone(a);
     }return result;
    };}
    return result;
   }return team.call(name,args,owner);
  },async()=>{},[...api.planningTools,...api.teamTools]);const options=await bridge.start();options.instructions=api.PLANNING_INSTRUCTIONS+'\n'+api.TEAM_INSTRUCTIONS+'\nThis is authorized real acceptance in a disposable project. Execute the requested development via execute_adaptive_plan. Choose the minimum useful worker count from the actual task, never hardcode a count from this test. Use only provided MCP tools. Poll with last revision, until_decision=true and wait_ms=60000 until terminal. READY means await delivery approval. No shell or unrelated tools.';return {options,timeoutMs:1800000,finalize:r=>team.finishTurn(turn,r),close:()=>bridge.close()};
 });await chat.initialize();
 try{
  await chat.send(user);await team.wait();const s=team.snapshot();await fs.writeFile(path.join(dir,'before-delivery.json'),JSON.stringify({team:s,chat:chat.snapshot(),calls,phases,injected},null,2));
  assert.equal(chat.snapshot().conversation.status,'COMPLETED',chat.snapshot().error);assert.equal(s.integration?.status,'READY',JSON.stringify({error:s.error,record:s.workflow?.record,tasks:s.workflow?.tasks}));
  if(scale)assert.equal(s.workflow.plan.analysis.scale,scale);assert(s.workflow.plan.worker_count<=3);assert.equal((await api.git(root,['status','--porcelain'])).trim(),'');
  if(name==='A')assert.equal(s.workflow.plan.worker_count,1);
  if(name==='B'||name==='C-D'){assert(s.workflow.plan.worker_count>=2);assert(phases.some(p=>p.split(':RUNNING').length>=3),'actual simultaneous running tasks');assert(s.workflow.tasks.some(t=>t.task.dependencies.length));assert(s.workflow.tasks.some(t=>t.events.some(e=>e.to==='WAITING_DEPENDENCY')));}
  if(inject){assert(injected);assert(s.workflow.record.repairs?.length);assert(s.workflow.record.repairs.some(r=>r.status==='PASSED'));assert(Object.values(team.workflow.record.reviews).flat().some(r=>r.verdict==='REQUEST_CHANGES'&&r.issues.some(i=>i.evidence_status==='VERIFIED')));}
  const snapshot={type:'snapshot',project:name,chat:{...chat.snapshot(),projectName:name},dispatch:{team:s}};snapshots.push(snapshot);await fs.writeFile(path.join(dir,'runtime-snapshot.json'),JSON.stringify(snapshot));await fs.writeFile(path.join(dir,'runtime-snapshots.json'),JSON.stringify(snapshots));
  const details=[];for(const w of s.workflow.workers){await team.select(w.worker_id);details.push(team.snapshot());}await fs.writeFile(path.join(dir,'worker-details.json'),JSON.stringify(details));
  const result={passed:true,plan:s.workflow.plan,usage:team.ledger.snapshot(),integration:s.integration,review_count:Object.values(s.workflow.record.reviews).flat().length,repairs:s.workflow.record.repairs||[],real_GPT:true,real_Claude:true,fault_injected:injected,phases};await fs.writeFile(path.join(dir,'result.json'),JSON.stringify(result,null,2));console.log(name+' READY');return {dir,root,result};
 }catch(e){await fs.writeFile(path.join(dir,'failure.json'),JSON.stringify({error:e.stack,team:team.snapshot(),chat:chat.snapshot(),calls,phases},null,2));throw e;}finally{await team.dispose();}
}
const common=' Use only Python standard library and exact python -B <script>.py test commands; every authorized script must contain unittest tests with unittest.main(), at least three meaningful normal/boundary/invalid tests. Do not change README.md. Work in isolated candidates, independently review and prepare integration, then wait for user delivery authorization.';
async function main(){await fs.mkdir(directory,{recursive:true});services=await providers();console.log('Evidence '+directory);
 const selected=process.argv[2]||'all',results={};
 if(selected==='all'||selected==='A')results.A=await adaptive('A','Implement one small code change: create normalize.py with normalize_label(text), strip outer whitespace and collapse internal whitespace to single spaces. Non-string input raises TypeError. Only normalize.py may be modified. This is a small single-function task.'+common,{scale:'SIMPLE'});
 if(selected==='all'||selected==='B')results.B=await adaptive('B','Implement a medium two-module measurement helper library. numbers.py provides safe_ratio(a,b), returns float a/b; b=0 raises ValueError. labels.py provides normalize_label(text), collapses whitespace and strips; rejects non-string. check.py imports both and tests them together using their agreed interfaces. Only numbers.py, labels.py, check.py may be modified. The two core modules are independent and benefit from parallel development; their integration check must wait until both are reviewed. Reuse idle workers for the dependent task.'+common,{scale:'MEDIUM'});
 if(selected==='all'||selected==='C-D')results.C=await adaptive('C-D','Build a complete small notes-management website using Python standard library. Only data.py, ui.py, server.py may change. data.py implements normalize_title(value): trim and collapse whitespace, reject non-string and empty/whitespace-only values with ValueError; plus in-memory note creation, listing and deletion. ui.py independently implements render_page(): responsive accessible HTML containing a heading, labelled title input, add button, note list and delete buttons, with JS calling same-origin GET/POST /api/notes and DELETE /api/notes/<id>; use textContent for note text, no dependencies. Agree API contract: each note has integer id and string title; list JSON array; POST JSON {title} returns note and 201; delete 204 or 404; invalid title 400. server.py depends on both modules, integrates HTTP routing and provides create_server(host="127.0.0.1",port=0) -> HTTPServer so acceptance can launch it. Include isolated unittest tests; no real network binding inside the worker sandbox tests (use pure routing or mocked server objects). Full website E2E runs later on localhost outside the worker snapshot. Backend and UI are independent, final server waits for both. Use dynamic useful worker allocation and reuse idle workers. All scripts need unittest.main() entry points for the exact tests.'+common,{inject:true,scale:'LARGE'});
 await fs.writeFile(path.join(directory,'result.json'),JSON.stringify({passed:true,results},null,2));console.log('ACCEPTANCE '+directory);
}
main().catch(e=>{console.error(e.stack);process.exitCode=1;});
