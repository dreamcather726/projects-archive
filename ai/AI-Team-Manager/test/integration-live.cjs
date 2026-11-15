// Reuses persisted, genuine Task07.8 candidates and reviews; only the integration review is new.
const fs=require('node:fs/promises'),path=require('node:path'),assert=require('node:assert/strict');
const {TeamWorkflow,TokenLedger,ReviewStorage,UserEditProtectionManager,IntegrationManager,git}=require('../.tools/task02-test/api.cjs');const services=require('./real-services.cjs');
(async()=>{
 const dir=path.resolve(process.argv[2]||''),saved=JSON.parse(await fs.readFile(path.join(dir,'result.json'),'utf8'));assert(saved.passed&&saved.realGPT&&saved.realClaude);
 const plan=saved.state.plan,ledger=new TokenLedger(plan.project_path,new ReviewStorage(path.join(dir,'usage')));await ledger.load();
 const workflow=new TeamWorkflow(plan,path.join(dir,'team'),services,ledger,async()=> 'DENY');await workflow.load();
 let editors=[];const protection=new UserEditProtectionManager(plan.project_path,async()=>editors,workflow.storage);await protection.load();const integration=new IntegrationManager(workflow,protection);await integration.load();
 try{
  for(const row of workflow.scheduler.snapshot())if(row.status==='PAUSED')await workflow.resume(row.task.task_id);await workflow.wait();assert(workflow.snapshot().ready_for_integration);
  const ready=integration.snapshot()?.status==='READY'?integration.snapshot():await integration.prepare();assert.equal(ready.status,'READY');assert.equal((await git(plan.project_path,['status','--porcelain'])).trim(),'');
  await assert.rejects(integration.deliver(false),/确认/);editors=[{file:'volume.py',dirty:true,version:1,buffer_hash:'UNSAVED_TEST_BUFFER',open:true}];await assert.rejects(integration.deliver(true),/WAITING_INTEGRATION/);assert.equal((await git(plan.project_path,['status','--porcelain'])).trim(),'');
  editors=[];const delivered=await integration.deliver(true);assert.equal(delivered.status,'DELIVERED');assert(workflow.scheduler.snapshot().every(t=>t.status==='COMPLETED'));assert.equal((await git(plan.project_path,['rev-parse','HEAD'])).trim(),plan.tasks[0].base_commit);
  for(const task of plan.tasks)for(const file of task.allowed_files)assert((await fs.readFile(path.join(plan.project_path,file),'utf8')).length>0);
  const memory=await workflow.storage.read(plan.project_path,'project-context.json');assert.equal(memory.completed_tasks.length,plan.tasks.length);await fs.writeFile(path.join(dir,'integration-result.json'),JSON.stringify({passed:true,realIntegrationGPTReview:true,externalEditorConfirmationRequired:true,unsavedBufferBlocked:true,mainBranchNotCommittedOrPushed:true,delivered,memory,usage:ledger.snapshot()},null,2));console.log(dir);
 }finally{await workflow.dispose();}
})().catch(e=>{console.error(e);process.exitCode=1});
