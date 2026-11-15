const vscode=require('vscode'),fs=require('node:fs/promises'),path=require('node:path'),assert=require('node:assert/strict');
const {configureGPTUsage}=require('../.tools/recovery/api.cjs');
exports.run=async()=>{
 const extension=vscode.extensions.getExtension('ai-team-manager.ai-team-manager'),api=await extension.activate(),dir=process.env.ATM_HOST_TEST_DIR;
 await vscode.commands.executeCommand('aiTeamManager.openDashboard');for(let i=0;i<100&&!api.getWorkspace().ready;i++)await new Promise(r=>setTimeout(r,100));assert(api.getWorkspace().ready,'Packaged production Webview under CSP');
 const previous=await fs.readFile(path.join(dir,'stage1.json'),'utf8').then(JSON.parse,()=>undefined);
 const settings=vscode.workspace.getConfiguration('aiTeamManager');
 await settings.update('codexExecutable',process.env.ATM_HOST_CODEX_EXE,vscode.ConfigurationTarget.Global);
 await settings.update('claudeExecutable',process.env.ATM_HOST_CLAUDE_EXE,vscode.ConfigurationTarget.Global);
 await vscode.commands.executeCommand('aiTeamManager.diagnoseEnvironment');const diagnostics=api.getDiagnostics();
 for(const name of ['Codex CLI','Claude Code CLI']){const d=diagnostics.find(d=>d.name===name);assert(d?.ok,JSON.stringify(d));assert.equal(d.source,'settings');}
 await fs.writeFile(path.join(dir,previous?'diagnostics-stage2.json':'diagnostics-stage1.json'),JSON.stringify(diagnostics,null,2));
 const team=api.dispatch.team;assert(team,'Production TeamSession');
 if(!previous){
  await team.ledger.begin('acceptance-interrupted','PLANNING_AND_COORDINATION');await team.ledger.finish('acceptance-interrupted','CANCELLED');
  const before=api.chat.snapshot();await api.chat.handle({type:'send',text:'only blocked preflight'});const blocked=api.chat.snapshot();assert.match(blocked.error,/\[WAITING_APPROVAL\]/);assert.equal(blocked.conversation.requests.length,before.conversation.requests.length);assert.equal(blocked.conversation.messages.length,before.conversation.messages.length);assert.equal(api.dispatch.bridges.size,0,'Rejected preparation must close loopback server');
  // Exercise the real native input cancellation; confirmation acceptance is covered separately with UI callbacks.
  const initial=JSON.stringify(team.ledger.snapshot());const timer=setTimeout(()=>vscode.commands.executeCommand('workbench.action.closeQuickOpen'),1000);
  try{assert.equal(await api.dispatch.handle({type:'teamUsageConfigure',projectPath:blocked.projectPath}),false);}finally{clearTimeout(timer);}
  assert.equal(JSON.stringify(team.ledger.snapshot()),initial,'Cancelling real native input preserves ledger');
  assert(await configureGPTUsage(team.ledger,{inputBudget:async()=> '500000',confirmUnknown:async()=>true}));
  assert.equal(team.ledger.snapshot().unknown_requests,1);assert(!team.ledger.snapshot().blocked);
  await api.chat.handle({type:'send',text:'仅回复 RECOVERY_OK，不使用任何工具。'});const state=api.chat.snapshot();assert.equal(state.conversation.status,'COMPLETED',state.error);assert(state.conversation.messages.some(m=>m.role==='assistant'&&m.text.includes('RECOVERY_OK')));assert(state.conversation.threadId);
  assert.equal(api.dispatch.bridges.size,0);assert.equal(team.ledger.snapshot().unknown_requests,1);
  await fs.writeFile(path.join(dir,'stage1.json'),JSON.stringify({passed:true,version:extension.packageJSON.version,realCodexPreflight:true,bothRealCliDiagnostics:true,correctWaitingApproval:true,noPhantomSentMessage:true,rejectedBridgeClosed:true,realNativeInputCancellation:true,acceptanceConfirmation:'injected explicit UI callbacks on isolated test ledger',unknownUsageRetained:true,realGPTReply:true,thread:state.conversation.threadId,conversation:state.conversation.id,requests:state.conversation.requests.length,cspLoaded:true}));
 }else{
  const before=api.chat.snapshot();assert.equal(before.conversation.threadId,previous.thread);assert.equal(before.conversation.id,previous.conversation);assert.equal(before.conversation.requests.length,previous.requests);assert(!team.ledger.snapshot().blocked);assert.equal(team.ledger.snapshot().unknown_requests,1);
  await api.chat.handle({type:'send',text:'仅回复 RECOVERY_RESUME_OK，不使用任何工具。'});const state=api.chat.snapshot();assert.equal(state.conversation.status,'COMPLETED',state.error);assert.equal(state.conversation.threadId,previous.thread);assert(state.conversation.messages.some(m=>m.role==='assistant'&&m.text.includes('RECOVERY_RESUME_OK')));
  await fs.writeFile(path.join(dir,'stage2.json'),JSON.stringify({passed:true,version:extension.packageJSON.version,restartRetainsAcknowledgement:true,bothRealCliDiagnostics:true,unknownUsageRetained:true,noAutomaticResend:true,sameRealThread:true,realGPTReply:true,requests:state.conversation.requests.length,cspLoaded:true}));
 }
};
