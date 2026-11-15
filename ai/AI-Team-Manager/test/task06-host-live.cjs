const vscode=require('vscode'),fs=require('node:fs/promises'),path=require('node:path'),assert=require('node:assert/strict');
const wait=ms=>new Promise(r=>setTimeout(r,ms));
exports.run=async()=>{
 const api=await vscode.extensions.getExtension('ai-team-manager.ai-team-manager').activate(),root=vscode.workspace.workspaceFolders[0].uri.fsPath,dir=process.env.ATM_HOST_TEST_DIR;
 const write=(n,v)=>fs.writeFile(path.join(dir,n+'.json'),JSON.stringify(v,null,2));
 await vscode.commands.executeCommand('aiTeamManager.openDashboard');for(let i=0;i<100&&!api.getWorkspace().ready;i++)await wait(100);assert(api.getWorkspace().ready);
 const saved=await fs.readFile(path.join(dir,'stage1.json'),'utf8').then(JSON.parse,()=>undefined);
 if(saved){const s=api.getWorkspaceSnapshot();assert.equal(s.dispatch.task.task_id,saved.task_id);assert.equal(s.dispatch.task.state,'PASSED');assert.equal(s.claude.conversation.threadId,saved.session);assert.equal(s.claude.conversation.requests.length,saved.requests);assert.equal(s.chat.conversation.threadId,saved.gptSession);await write('stage2',{historyRestored:true,sessionRestored:true,noAutomaticResend:true,uiReady:true});return;}
 const protectedBefore=await fs.readFile(path.join(root,'protocol.c'),'utf8');let last='';
 const monitor=setInterval(()=>{const s=api.getWorkspaceSnapshot();const phase=s.dispatch.task?.state||'GPT_TOOL_CALL';if(phase!==last){last=phase;void write('progress',{phase,time:new Date().toISOString(),error:s.dispatch.task?.error});}void write('runtime-snapshot',s);},1000);
 try{
  // Same production controller as the GPT webview send handler. No direct call to Claude or Dispatcher here.
  await api.chat.handle({type:'send',text:'让 Claude 在当前项目创建 test.py，实现一个加法函数，并运行测试。只允许修改 test.py，不得修改 protocol.c 或 README.md。请实际调用派发工具，并等真实执行、测试和GPT审查结束后汇报。'});
  const s=api.getWorkspaceSnapshot();await write('runtime-snapshot',s);assert.equal(s.chat.conversation.status,'COMPLETED',JSON.stringify(s.chat));
  assert(s.dispatch.task,'GPT must really call dispatch_claude_task');assert.equal(s.dispatch.task.state,'PASSED',JSON.stringify(s.dispatch));
  const r=api.dispatch.dispatcher.records[0];await write('dispatch-record',r);
  assert(r.events.some(e=>e.kind==='GPT_PLAN_APPROVAL'&&e.data.decision.decision==='APPROVE'));
  assert(r.events.some(e=>e.kind==='GPT_TOOL_APPROVAL'&&e.data.result.decision==='APPROVE'));
  assert(r.events.some(e=>e.kind==='CLAUDE_EVENT'&&e.data.type==='permission'&&e.data.decision==='ALLOW'));
  assert(r.rounds[0].evidence.changes.some(c=>c.path==='test.py'&&c.kind==='ADDED'));assert(r.rounds[0].tests.some(t=>t.status==='PASSED'&&t.test_count>0));assert.equal(r.rounds.at(-1).review.verdict,'PASS');
  const calls=s.chat.conversation.requests.at(-1).toolCalls||[];assert(calls.some(t=>t.summary.includes('dispatch_claude_task')));assert(calls.some(t=>t.summary.includes('get_claude_task_output')));assert(s.chat.conversation.messages.some(m=>m.role==='assistant'&&/test.py/.test(m.text)));
  assert.equal(await fs.readFile(path.join(root,'protocol.c'),'utf8'),protectedBefore);assert(!s.claude.permissions.active);
  await write('stage1',{realGPTToolCalling:true,realClaude:true,task_id:r.task_id,state:r.state,session:r.session_id,gptSession:s.chat.conversation.threadId,requests:s.claude.conversation.requests.length,tests:r.rounds.at(-1).tests,protectedFileUnchanged:true,gptReturnedActualResult:true,gptCalls:r.gpt_calls,uiReady:true});
 }finally{clearInterval(monitor);}
};
