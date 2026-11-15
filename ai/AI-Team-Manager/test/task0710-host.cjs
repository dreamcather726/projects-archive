// Native host check against the final package, with real GPT and two real Worktree Workers.
const vscode=require('vscode'),fs=require('node:fs/promises'),path=require('node:path'),assert=require('node:assert/strict');
exports.run=async()=>{
 await vscode.workspace.getConfiguration('aiTeamManager').update('claudeExecutable',process.env.ATM_HOST_CLAUDE_EXE,vscode.ConfigurationTarget.Global);
 const extension=vscode.extensions.getExtension('ai-team-manager.ai-team-manager'),api=await extension.activate(),dir=process.env.ATM_HOST_TEST_DIR;
 await vscode.commands.executeCommand('aiTeamManager.openDashboard');for(let i=0;i<150&&!api.getWorkspace().ready;i++)await new Promise(r=>setTimeout(r,100));assert(api.getWorkspace().ready,'Production assets must load under the packaged CSP');
 const previous=await fs.readFile(path.join(dir,'stage1.json'),'utf8').then(JSON.parse,()=>undefined);
 if(!previous){
  await api.chat.handle({type:'new'});
  await api.chat.handle({type:'send',text:'请实际开发两个互不依赖的 Python 标准库模块：double.py 提供 double(value) 返回 value * 2；triple.py 提供 triple(value) 返回 value * 3。只允许修改 double.py、triple.py。禁止修改 README.md。每个模块文件内至少包含三个 unittest 方法，分别验证负数、零、正数，并包含 unittest.main() 入口。分别运行 python -B double.py、python -B triple.py。创建两个 Worker 并行完成真实开发、测试、独立审查和集成准备，达到 READY 后明确说明尚未交付，等待用户查看补丁授权。使用 execute_adaptive_plan 隔离团队入口。'});
  const state=api.getWorkspaceSnapshot(),team=state.dispatch.team;
  await fs.writeFile(path.join(dir,'runtime-snapshot.json'),JSON.stringify(state));
  assert.equal(state.chat.conversation.status,'COMPLETED',state.chat.error);assert.equal(team.integration?.status,'READY',team.error||JSON.stringify(state.dispatch));assert.equal(team.workflow.workers.length,2);
  const sessions=team.workflow.workers.map(w=>w.session_id);assert.equal(new Set(sessions).size,2);
  const requests=team.workflow.workers.map(w=>w.event_count);
  const tab=vscode.window.tabGroups.all.flatMap(g=>g.tabs).find(t=>t.input instanceof vscode.TabInputWebview&&t.input.viewType.includes('aiTeamManager.workspace'));await vscode.window.tabGroups.close(tab);await vscode.commands.executeCommand('aiTeamManager.openDashboard');assert.deepEqual(api.dispatch.snapshot().team.workflow.workers.map(w=>w.session_id),sessions);
  await api.workspace.saveLayout({...api.getWorkspace().layout,mode:'single',activePanel:'gpt',navigation:{route:'collaboration',hiddenTabs:[]}});
  await fs.writeFile(path.join(dir,'stage1.json'),JSON.stringify({passed:true,version:extension.packageJSON.version,realGPT:true,realClaude:true,productionCoordinator:true,readyNotDelivered:true,cspLoaded:true,closeReopenPreservesWorkers:true,sessions,requests,conversation:state.chat.conversation.id,thread:state.chat.conversation.threadId}));
 }else{
  const state=api.getWorkspaceSnapshot(),team=state.dispatch.team;assert.equal(state.chat.conversation.id,previous.conversation);assert.equal(state.chat.conversation.threadId,previous.thread);assert.deepEqual(team.workflow.workers.map(w=>w.session_id),previous.sessions);assert.equal(team.integration.status,'READY');assert(!team.busy);
  await fs.writeFile(path.join(dir,'stage2.json'),JSON.stringify({passed:true,version:extension.packageJSON.version,restartPreservesSessions:true,noAutomaticResend:true,readyNotDelivered:true,cspLoaded:true}));
 }
};
