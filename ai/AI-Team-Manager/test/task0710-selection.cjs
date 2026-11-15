// Reopen genuine completed sessions with no usable provider, then verify UI-only selection.
const fs=require('node:fs/promises'),path=require('node:path'),assert=require('node:assert/strict');
const {TeamSession}=require('../.tools/task0710/api.cjs');
(async()=>{
 const dir=path.resolve(process.argv[2]),root=path.join(dir,'project');assert(JSON.parse(await fs.readFile(path.join(dir,'result.json'),'utf8')).passed);
 let providerCalls=0,uiUpdates=0;const service=()=>({run(){providerCalls++;throw Error('Selection cannot call a provider');}});
 const team=new TeamSession({root,directory:path.join(dir,'team'),services:{claude:service,codex:service},editors:async()=>[],authorize:async()=>false,approve:async()=> 'DENY',changed:()=>uiUpdates++});
 try{
  await team.load();const revision=team.snapshot().revision,requests=team.workflow.executor.workers.list().map(w=>team.workflow.executor.workers.detail(w.worker_id).chat.conversation.requests.length),usage=JSON.stringify(team.ledger.snapshot());
  for(let round=0;round<40;round++)for(const worker of team.workflow.executor.workers.list())await team.select(worker.worker_id);
  assert.equal(team.snapshot().revision,revision,'UI-only changes must not wake the GPT execution poll');assert.equal(providerCalls,0);assert.equal(JSON.stringify(team.ledger.snapshot()),usage);assert.deepEqual(team.workflow.executor.workers.list().map(w=>team.workflow.executor.workers.detail(w.worker_id).chat.conversation.requests.length),requests);assert(uiUpdates>=120);
  const current=team.workflow.executor.workers.list()[0];await team.select(current.worker_id);const change=team.snapshot().detail.candidate.files[0];assert(team.file(current.worker_id,change.path).length>0);assert.throws(()=>team.file(current.worker_id,'../../outside.key'));
  const report={passed:true,actualPersistedSessions:true,selections:120,providerCalls,revisionStable:true,ledgerStable:true,requestsStable:true,evidenceOnlyDiff:true};await fs.writeFile(path.join(dir,'selection-result.json'),JSON.stringify(report,null,2));console.log(JSON.stringify(report));
 }finally{await team.dispose();}
})().catch(e=>{console.error(e);process.exitCode=1;});
