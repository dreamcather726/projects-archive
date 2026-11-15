const vscode=require('vscode'),fs=require('node:fs/promises'),path=require('node:path'),assert=require('node:assert/strict');
const wait=ms=>new Promise(r=>setTimeout(r,ms));
exports.run=async()=>{
 const api=await vscode.extensions.getExtension('ai-team-manager.ai-team-manager').activate(),root=vscode.workspace.workspaceFolders[0].uri.fsPath,dir=process.env.ATM_HOST_TEST_DIR;
 await vscode.commands.executeCommand('aiTeamManager.openDashboard');for(let i=0;i<100&&!api.getWorkspace().ready;i++)await wait(100);assert(api.getWorkspace().ready);
 const saved=await fs.readFile(path.join(dir,'stage1.json'),'utf8').then(JSON.parse,()=>undefined);
 if(saved){const c=api.claude.snapshot();assert.equal(c.conversation.threadId,saved.session);assert(c.permissions.scope.allowedFiles.includes('motor_control.c'));assert(!c.permissions.active);assert.equal(c.permissions.pending.length,0);assert(c.permissions.logs.some(l=>l.source==='USER_DENIED_OR_CANCELLED'));assert.equal(JSON.parse(await fs.readFile(path.join(root,'package.json'),'utf8')).version,'1.0.1');await fs.writeFile(path.join(dir,'stage2.json'),JSON.stringify({sessionRestored:true,scopeRestored:true,auditRestored:true,noResend:true,uiReady:api.getWorkspace().ready}));return;}
 assert((await vscode.commands.getCommands(true)).includes('aiTeamManager.configureClaudePermissions'));
 await api.claude.configurePermissions({allowedFiles:['motor_control.c','package.json'],safeCommands:['node --test']});
 const gpt=JSON.stringify(api.chat.snapshot()),send=text=>api.review.withClaudeTask(text,()=>api.claude.handle({type:'send',text}));
 await send('隔离权限验收：只用Read读取motor_control.c，再用Edit将motor_limit改为负数返回0否则返回speed。然后仅用Bash执行精确命令 node --test。不要修改其他文件，不用其他命令。');
 let c=api.claude.snapshot();assert.equal(c.conversation.status,'COMPLETED',JSON.stringify(c));assert.match(await fs.readFile(path.join(root,'motor_control.c'),'utf8'),/speed\s*<\s*0/);const session=c.conversation.threadId;assert(session);
 async function withApproval(prompt,operation,choice,name){
  const promise=send(prompt);let pending;
  for(let i=0;i<1500;i++){pending=api.claude.snapshot().permissions.pending.find(p=>p.operation===operation);if(pending)break;await wait(100);}
  assert(pending,`Missing real pending ${name}`);assert(api.claude.snapshot().busy);
  const snapshot=api.getWorkspaceSnapshot();await fs.writeFile(path.join(dir,`${name}-pending.json`),JSON.stringify(snapshot,null,2));
  await api.claude.handle({type:'permissionAnswer',id:pending.id,choice});await promise;
  // A replayed UI answer is inert and cannot authorize another task.
  await api.claude.handle({type:'permissionAnswer',id:pending.id,choice:'TASK'});
  assert.equal(api.claude.snapshot().conversation.threadId,session);
 }
 await withApproval('只用Bash执行精确命令 npm --version，等待插件人工审批，不增加其他参数或命令。','TEST_COMMAND','ONCE','allow');
 assert.equal(api.claude.snapshot().conversation.status,'COMPLETED');assert(api.claude.snapshot().permissions.logs.some(l=>l.source==='ONCE'));
 await withApproval('只用Read读取package.json，再用Edit将version 1.0.0改为1.0.1，等待审批。若拒绝立即停止，不重试不绕过。','CONFIG_WRITE','DENY','deny');
 assert.equal(JSON.parse(await fs.readFile(path.join(root,'package.json'),'utf8')).version,'1.0.0');assert.equal(api.claude.snapshot().conversation.status,'FAILED');
 await withApproval('只用Read读取package.json，再用Edit将version 1.0.0改为1.0.1。这次等待新的审批，不能复用先前请求的审批。','CONFIG_WRITE','TASK','task-allow');
 assert.equal(JSON.parse(await fs.readFile(path.join(root,'package.json'),'utf8')).version,'1.0.1');assert.equal(api.claude.snapshot().conversation.status,'COMPLETED');
 const stop=send('只用Read读取package.json，再用Edit将version 1.0.1改为1.0.2，等待本轮审批。');let pending;
 for(let i=0;i<1500;i++){pending=api.claude.snapshot().permissions.pending[0];if(pending)break;await wait(100);}assert(pending,'Task permission grant must expire');await api.claude.handle({type:'stop'});await stop;
 assert.equal(api.claude.snapshot().permissions.pending.length,0);assert.equal(api.claude.snapshot().conversation.status,'CANCELLED');assert.equal(JSON.parse(await fs.readFile(path.join(root,'package.json'),'utf8')).version,'1.0.1');
 assert.equal(JSON.stringify(api.chat.snapshot()),gpt);assert.equal(await fs.readFile(path.join(root,'protocol.c'),'utf8'),'int protocol_version(void) { return 1; }\n');
 await fs.writeFile(path.join(dir,'runtime-snapshot.json'),JSON.stringify(api.getWorkspaceSnapshot(),null,2));
 await fs.writeFile(path.join(dir,'stage1.json'),JSON.stringify({realClaude:true,session,uiReady:true,readAndAllowedWrite:true,safeTests:true,approvalOnce:true,denialPreservedFile:true,taskApproval:true,taskGrantExpired:true,stopWhilePending:true,gptUnchanged:true,approvalActor:'TEST_HARNESS_VIA_PRODUCTION_UI_MESSAGE'}));
};
