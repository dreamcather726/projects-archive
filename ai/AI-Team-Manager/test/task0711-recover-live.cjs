// Continue retained real C/D candidates after fixing the review evidence pipeline.
// No completed implementation is regenerated; every subsequent review/repair is a real model run.
const fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os'),assert=require('node:assert/strict');
const api=require('../.tools/task0711/api.cjs');
(async()=>{
 const dir=path.resolve(process.argv[2]),root=path.join(dir,'project'),ext=path.join(os.homedir(),'.vscode','extensions'),names=await fs.readdir(ext),latest=prefix=>path.join(ext,names.filter(n=>n.startsWith(prefix)).sort((a,b)=>b.localeCompare(a,undefined,{numeric:true}))[0]);
 const services={codex:()=>new api.CodexService(path.join(latest('openai.chatgpt-'),'bin','windows-x86_64','codex.exe')),claude:()=>new api.ClaudeService('claude',undefined,c=>api.resolveClaudeExecutable(c,{extensionDirectory:()=>latest('anthropic.claude-code-')}))};
 let team,phase='';const changed=()=>{const next=team?.snapshot().workflow?.tasks.map(t=>t.task.task_id+':'+t.status).join(',');if(next&&next!==phase){phase=next;console.log(phase);}};
 team=new api.TeamSession({root,directory:path.join(dir,'team'),services,editors:async()=>[],authorize:async()=>true,approve:async()=> 'DENY',changed});await team.load();
 try{
  const before=team.snapshot();await fs.writeFile(path.join(dir,'before-recovery.json'),JSON.stringify(before,null,2));
  assert.equal(team.ledger.snapshot().unknown_requests,0);await team.ledger.configure(5000000,false); // Explicit disposable-test budget; no production setting is changed.
  team.workflow.plan.worker_timeout_ms=900000;await team.storage.write(root,'active-team-plan.json',team.workflow.plan);await team.control('resume');await team.wait();const s=team.snapshot();await fs.writeFile(path.join(dir,'recovered-state.json'),JSON.stringify(s,null,2));assert.equal(s.integration?.status,'READY',JSON.stringify({error:s.error,record:s.workflow.record,tasks:s.workflow.tasks}));
  const original=JSON.parse(await fs.readFile(path.join(dir,'before-delivery.json'),'utf8'));
  if(original.injected)assert(s.workflow.record.repairs.some(r=>r.status==='PASSED'));
  assert.equal(s.workflow.plan.analysis.scale,original.team.workflow.plan.analysis.scale);assert(s.workflow.plan.worker_count>=2&&s.workflow.plan.worker_count<=3);assert.equal((await api.git(root,['status','--porcelain'])).trim(),'');
  const old=JSON.parse(await fs.readFile(path.join(dir,'runtime-snapshot.json'),'utf8').catch(()=>fs.readFile(path.join(dir,'before-delivery.json'),'utf8').then(t=>{const r=JSON.parse(t);return JSON.stringify({type:'snapshot',project:'C-D',chat:r.chat,dispatch:{team:r.team}});})));old.dispatch.team=s;await fs.writeFile(path.join(dir,'runtime-snapshot.json'),JSON.stringify(old));
  const details=[];for(const w of s.workflow.workers){await team.select(w.worker_id);details.push(team.snapshot());}await fs.writeFile(path.join(dir,'worker-details.json'),JSON.stringify(details));
  await fs.writeFile(path.join(dir,'result.json'),JSON.stringify({passed:true,explicit_checkpoint_recovery:true,fault_injected:original.injected,real_GPT:true,real_Claude:true,plan:s.workflow.plan,integration:s.integration,usage:team.ledger.snapshot(),repairs:s.workflow.record.repairs||[],team:s},null,2));console.log('RECOVERED READY '+dir);
 }finally{await team.dispose();}
})().catch(e=>{console.error(e.stack);process.exitCode=1;});
