// Two real development/review rounds. A labelled fault injector changes only the isolated fixture after first real Claude delivery.
// No GPT result, review issue, Claude output, permission request or test result is mocked.
const fs=require('node:fs/promises'),path=require('node:path'),assert=require('node:assert/strict');
const {CodexService,ClaudeService,ClaudeController,ClaudeStorage,resolveClaudeExecutable,launch,PermissionHost,TaskDispatcher,ReviewGit,ReviewStorage,ReviewTestRunner,ReviewEngine,RepairModel,ApprovalModel}=require('../.tools/task02-test/api.cjs');
(async()=>{
 const dir=path.resolve('.tools',`task06-loop-${Date.now()}`),root=path.join(dir,'project');await require('./repair-fixture.cjs').create(root);
 const protectedNames=['sample.test.cjs','README.md','package.json','input.txt'],original=await Promise.all(protectedNames.map(f=>fs.readFile(path.join(root,f),'utf8')));
 const cli=new ClaudeService('claude',launch,c=>resolveClaudeExecutable(c,{extensionDirectory:()=>undefined}));const controller=new ClaudeController(new ClaudeStorage(path.join(dir,'claude'),root),cli,()=>{},async()=>{},()=>300000);await controller.initialize();
 let executions=0,injected=false;const worker={snapshot:()=>controller.snapshot(),stop:()=>controller.stop(),sendDispatched:async(text,selected,session,files,observe,gateway)=>{
   assert.equal(selected,root);assert.equal(controller.snapshot().conversation.threadId,session);
   await controller.send(text,{observe,preparePermissions:async(taskId,sid)=>{const host=await new PermissionHost(gateway.policy,taskId,async()=>{throw Error('No unexpected manual approval in this low risk fixture')},async e=>observe({type:'permission',...e}),90000,sid,gateway.evaluate).start();return host;}});
   if(controller.snapshot().conversation.status!=='COMPLETED')throw Error(controller.snapshot().error||'Claude did not complete');
   if(gateway.mode!=='PLAN_READ_ONLY'){
     executions++;
     if(executions===1){const delivered=await fs.readFile(path.join(root,'worker.cjs'),'utf8');await fs.writeFile(path.join(dir,'first-real-claude-delivery.cjs'),delivered);await fs.writeFile(path.join(root,'worker.cjs'),require('./repair-fixture.cjs').sample.after);injected=true;await fs.writeFile(path.join(dir,'fault-injection.json'),JSON.stringify({actor:'TEST_HARNESS',phase:'after first real Claude delivery, before actual tests and GPT review',file:'worker.cjs',defect:'negative values incorrectly return Math.abs(value)',claudeOutputWasNotMocked:true},null,2));}
   }
 }};
 const storage=new ReviewStorage(path.join(dir,'reviews')),collector=new ReviewGit(path.join(dir,'temporary')),service=()=>new CodexService();
 const dispatcher=new TaskDispatcher(root,storage,collector,worker,new ApprovalModel(service(),path.join(dir,'gpt')),new ReviewEngine(service(),collector,storage),new ReviewTestRunner(collector),new RepairModel(service(),path.join(dir,'gpt')),{mode:'GPT_HYBRID',maxRounds:2,authorize:async()=>{throw Error('Unexpected scope expansion')},verify:async()=>{}},()=>{});await dispatcher.load();
 const request='只允许修改 worker.cjs。实现 run(value)：有限实数夹到0..100；负数返回0，大于100返回100，合法区间保持原值，NaN和无穷大抛TypeError。不得修改 sample.test.cjs、README.md、package.json 或 input.txt。运行 npm run compile 和 node --test。保持CommonJS接口，无依赖。';
 const started=await dispatcher.call('LOOP_TEST_OWNER','LOOP_TEST_TURN',request,'dispatch_claude_task',{description:request,allowed_files:['worker.cjs'],forbidden_files:protectedNames,test_commands:['npm run compile','node --test'],acceptance_criteria:['all boundaries clamp correctly','nonfinite rejects','compile and all tests pass']});
 let cursor=0;for(;;){const out=await dispatcher.call('LOOP_TEST_OWNER','LOOP_TEST_TURN',request,'get_claude_task_output',{task_id:started.task_id,cursor,wait_ms:25000});cursor=out.next_cursor;console.log(JSON.stringify({state:out.state,events:out.events.length,cursor}));if(out.terminal&&!out.has_more)break;}
 const r=dispatcher.records[0];await fs.writeFile(path.join(dir,'dispatch-record.json'),JSON.stringify(r,null,2));assert(injected);assert.equal(r.state,'PASSED',r.error||JSON.stringify(r.rounds.map(e=>e.review)));
 assert.equal(r.rounds.length,2);assert.equal(executions,2);assert.equal(r.rounds[0].review.verdict,'REQUEST_CHANGES');assert(r.rounds[0].tests.some(t=>t.status==='FAILED'));assert.equal(r.rounds[1].review.verdict,'PASS');assert(r.rounds[1].verification.every(c=>c.status==='FIXED'));assert(r.rounds[1].repair_id);assert(r.rounds[1].tests.every(t=>t.status==='PASSED'));
 assert.deepEqual(await Promise.all(protectedNames.map(f=>fs.readFile(path.join(root,f),'utf8'))),original);const sessions=new Set(r.events.filter(e=>e.kind==='CLAUDE_EVENT'&&e.data.type==='session').map(e=>e.data.id));assert.equal(sessions.size,1);
 await fs.writeFile(path.join(dir,'result.json'),JSON.stringify({passed:true,realClaude:true,realGPT:true,faultInjection:'explicit actual fixture file injection, not attributed to Claude',rounds:r.rounds.map(e=>({round:e.round,repair_id:e.repair_id,review_id:e.review.review_id,verdict:e.review.verdict,verification:e.verification,tests:e.tests.map(t=>({command:t.command,status:t.status,exit_code:t.exit_code,test_count:t.test_count}))})),session:r.session_id,protectedFilesUnchanged:true,gptCalls:r.gpt_calls},null,2));console.log(dir);
})().catch(e=>{console.error(e);process.exitCode=1;});
