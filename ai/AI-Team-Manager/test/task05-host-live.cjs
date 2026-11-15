const vscode=require('vscode'),fs=require('node:fs/promises'),path=require('node:path'),assert=require('node:assert/strict');
const wait=ms=>new Promise(r=>setTimeout(r,ms));
exports.run=async()=>{
  const api=await vscode.extensions.getExtension('ai-team-manager.ai-team-manager').activate(),root=vscode.workspace.workspaceFolders[0].uri.fsPath,dir=process.env.ATM_HOST_TEST_DIR;
  const write=(name,data)=>fs.writeFile(path.join(dir,name+'.json'),JSON.stringify(data,null,2));
  await vscode.commands.executeCommand('aiTeamManager.openDashboard');for(let i=0;i<100&&!api.getWorkspace().ready;i++)await wait(100);assert(api.getWorkspace().ready);
  const saved=await fs.readFile(path.join(dir,'stage1.json'),'utf8').then(JSON.parse,()=>undefined);
  if(saved){
    const r=api.repair.snapshot().record,c=api.claude.snapshot();assert.equal(r.repair_id,saved.repairId);assert.equal(r.state,'FINISHED');assert.equal(r.verification[0].status,'FIXED');assert.equal(c.conversation.threadId,saved.session);assert.equal(c.conversation.requests.length,saved.requests);assert.equal(c.permissions.active,false);assert.deepEqual(c.permissions.scope.allowedFiles,['worker.cjs']);
    assert.equal(await fs.readFile(path.join(root,'worker.cjs'),'utf8'),saved.code);
    await api.repair.handle({type:'repairApprove',repairId:r.repair_id,planHash:r.plan_hash});assert(api.repair.snapshot().error);assert.equal(api.claude.snapshot().conversation.requests.length,saved.requests);
    await write('stage2',{repairHistoryRestored:true,approvalHistoryRestored:r.approvals.length>0,sessionRestored:true,noDuplicateSend:true,uiReady:api.getWorkspace().ready});return;
  }
  await write('progress',{phase:'real Claude creates deliberate defective sample'});
  const sample=require('./repair-fixture.cjs').sample;
  await api.claude.configurePermissions({allowedFiles:['worker.cjs'],safeCommands:[]});
  const protectedNames=['sample.test.cjs','README.md','package.json','input.txt'],protectedBefore=await Promise.all(protectedNames.map(f=>fs.readFile(path.join(root,f),'utf8')));
  const prompt='这是隔离验收工程：仅使用Read/Edit/Write，将worker.cjs替换为以下故意有逻辑错误的审查样例。保留故意错误，供后续独立审查发现。不得修改其他文件，不运行命令。\n'+sample.after;
  await api.review.withClaudeTask(prompt,()=>api.claude.handle({type:'send',text:prompt}));assert.equal(api.claude.snapshot().conversation.status,'COMPLETED',JSON.stringify(api.claude.snapshot()));
  assert.equal(await fs.readFile(path.join(root,'worker.cjs'),'utf8'),sample.after);
  const gpt=JSON.stringify(api.chat.snapshot()),session=api.claude.snapshot().conversation.threadId;
  await api.review.runTests('node --test');
  await write('progress',{phase:'real Task04 review'});
  const original=await api.review.start({task:{task_id:'TASK05-NATIVE',task_description:sample.task,technical_requirements:'保留CommonJS run(value)接口，无依赖，只修改worker.cjs，不修改测试、README或配置。',allowed_files:['worker.cjs'],forbidden_files:protectedNames}});
  await write('original-review',original);assert.equal(original.verdict,'REQUEST_CHANGES',JSON.stringify(original));
  const issue=original.issues.find(i=>i.file_path==='worker.cjs'&&i.evidence_status==='VERIFIED'&&i.basis==='after');assert(issue);
  const requests=api.claude.snapshot().conversation.requests.length;
  await write('progress',{phase:'real GPT repair plan'});
  await api.repair.handle({type:'repairGenerate',reviewId:original.review_id,issueIds:[issue.issue_id],requirements:'保持接口、测试和其他文件不变；修复负数夹紧错误。编译命令npm run compile，测试命令node --test，由用户另行选择执行。'});
  let r=api.repair.snapshot().record;assert.equal(r.state,'DRAFT',JSON.stringify(api.repair.snapshot()));assert(r.plan_hash);assert.equal(r.original_session,session);assert.equal(api.claude.snapshot().conversation.requests.length,requests);assert.equal(await fs.readFile(path.join(root,'worker.cjs'),'utf8'),sample.after);
  await write('plan-preview-snapshot',api.getWorkspaceSnapshot());
  await api.repair.handle({type:'repairApprove',repairId:r.repair_id,planHash:'STALE'});assert(api.repair.snapshot().error);assert.equal(api.claude.snapshot().conversation.requests.length,requests);
  await write('progress',{phase:'explicit test approval via production message handler, real Claude repair'});
  // Automated approval action, not a claim that a human clicked. Only low-risk exact-file plan is accepted here.
  assert(!r.plan.delete_files.length && !r.plan.public_interface_changes.length && r.plan.risk_level!=='HIGH');assert.deepEqual(r.plan.allowed_files,['worker.cjs']);
  await api.repair.handle({type:'repairApprove',repairId:r.repair_id,planHash:r.plan_hash});r=api.repair.snapshot().record;
  await write('execution-snapshot',api.getWorkspaceSnapshot());assert.equal(r.state,'EXECUTED',JSON.stringify(r));assert.equal(api.claude.snapshot().conversation.threadId,session);assert.equal(api.claude.snapshot().conversation.requests.length,requests+1);assert(r.events.some(e=>e.detail?.type==='tool'&&e.detail.phase==='completed'));assert(r.events.some(e=>e.detail?.type==='permission'&&e.detail.decision==='ALLOW'));
  assert(r.evidence.changes.some(c=>c.path==='worker.cjs'&&c.diff.includes('-  if (value < 0) return Math.abs(value);')));
  await api.repair.runCheck(r.repair_id,'npm run compile');await api.repair.runCheck(r.repair_id,'node --test');
  await write('progress',{phase:'real GPT independent repair verification'});
  await api.repair.handle({type:'repairVerify',repairId:r.repair_id});r=api.repair.snapshot().record;await write('runtime-snapshot',api.getWorkspaceSnapshot());
  assert.equal(r.state,'FINISHED');assert.equal(r.verification[0].status,'FIXED',JSON.stringify(r));assert(r.checks.every(c=>c.status==='PASSED'));assert(r.checks.find(c=>c.command==='node --test').test_count>0);assert(r.gpt_usage.total>0);
  const reportDir=path.join(path.dirname(api.claude.snapshot().permissions.auditPath),'..','reviews');
  // Export the actual persisted full record and independently generated review, not a reconstructed report.
  const find=async d=>{for(const e of await fs.readdir(d,{withFileTypes:true})){const p=path.join(d,e.name);if(e.isDirectory()){const result=await find(p);if(result)return result;}else if(e.name===`repair-${r.repair_id}.json`)return p;}};
  const recordPath=await find(reportDir);assert(recordPath);const full=JSON.parse(await fs.readFile(recordPath,'utf8'));await write('repair-record',full);await write('verification-review',JSON.parse(await fs.readFile(path.join(path.dirname(recordPath),`report-${r.verification_review_id}.json`),'utf8')));
  assert.equal(JSON.stringify(api.chat.snapshot()),gpt);assert.deepEqual(await Promise.all(protectedNames.map(f=>fs.readFile(path.join(root,f),'utf8'))),protectedBefore);
  await api.repair.handle({type:'repairApprove',repairId:r.repair_id,planHash:r.plan_hash});assert(api.repair.snapshot().error);assert.equal(api.claude.snapshot().conversation.requests.length,requests+1);
  await write('stage1',{realGPT:true,realClaude:true,repairId:r.repair_id,reviewId:original.review_id,verificationReview:r.verification_review_id,verdict:r.verification[0].status,session,requests:requests+1,code:await fs.readFile(path.join(root,'worker.cjs'),'utf8'),unapprovedDidNotSend:true,staleApprovalRejected:true,duplicateRejected:true,protectedFilesUnchanged:true,gptChatUnchanged:true,uiReady:true,approvalActor:'TEST_HARNESS_VIA_PRODUCTION_UI_MESSAGE',gptUsage:r.gpt_usage,checks:r.checks.map(c=>({command:c.command,status:c.status,test_count:c.test_count,exit_code:c.exit_code}))});
};
