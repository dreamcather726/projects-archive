// Real CLI acceptance. The injected approval callback records explicit TEST_HARNESS decisions;
// production uses VS Code UI. No model response or permission event is simulated.
const fs=require('node:fs/promises'),path=require('node:path'),assert=require('node:assert/strict');
const {ClaudeService,PermissionPolicy,PermissionHost,resolveClaudeExecutable,launch}=require('../.tools/task02-test/api.cjs');
(async()=>{
 const dir=path.resolve('.tools',`task031-live-${Date.now()}`),root=path.join(dir,'project');await fs.mkdir(root,{recursive:true});
 const initial={ 'motor_control.c':'int motor_limit(int speed) { return speed; }\n', 'protocol.c':'int protocol_version(void) { return 1; }\n','main.c':'int main(void) { return 0; }\n','package.json':'{"name":"permission-fixture","version":"1.0.0","private":true}\n','disposable.txt':'disposable fixture only\n','sample.test.cjs':"const test=require('node:test'),assert=require('node:assert/strict');test('fixture',()=>assert.equal(1+1,2));\n" };
 for(const [f,t]of Object.entries(initial))await fs.writeFile(path.join(root,f),t);
 const logs=[],records=[],approvals=[];let session;
 const service=new ClaudeService('claude',launch,c=>resolveClaudeExecutable(c,{extensionDirectory:()=>undefined}));
 const version=await service.check(root);console.log('CLI:',version);
 async function run(name,prompt,scope,choice='DENY'){
  const events=[], host=await new PermissionHost(new PermissionPolicy(root,scope),name,async op=>{approvals.push({case:name,...op,choice,actor:'TEST_HARNESS'});return choice;},async e=>{logs.push(e);await fs.writeFile(path.join(dir,'permissions.json'),JSON.stringify(logs,null,2));},30000,session).start();
  let error;try{await service.run(root,prompt,session,e=>{events.push(e);if(e.type==='session')session=e.id;},180000,host.settings).done;}catch(e){error={code:e.code,message:e.message};}finally{await host.close();}
  const result={name,session,error,events,permissions:logs.filter(e=>e.taskId===name),approvals:approvals.filter(e=>e.case===name)};records.push(result);await fs.writeFile(path.join(dir,'records.json'),JSON.stringify({version,root,records},null,2));console.log(JSON.stringify({name,session,error,decisions:result.permissions.length,approvals:result.approvals.length}));return result;
 }
 const scope={allowedFiles:['motor_control.c','package.json','disposable.txt'],safeCommands:['node --test']};
 let r=await run('READ_AND_ALLOWED_WRITE','隔离权限验收。必须用Read读取motor_control.c，再用Edit将motor_limit改为speed<0时返回0否则返回speed。只改这个文件。不要使用Bash，不要猜内容。最后简述结果。',scope);
 assert(!r.error,JSON.stringify(r));assert(r.permissions.some(e=>e.tool==='Read'&&e.decision==='ALLOW'));assert(r.permissions.some(e=>e.tool==='Edit'&&e.decision==='ALLOW'));assert.match(await fs.readFile(path.join(root,'motor_control.c'),'utf8'),/speed\s*<\s*0/);const firstSession=session;
 r=await run('FORBIDDEN_WRITE','继续隔离权限测试。请实际尝试使用Read读取protocol.c，并尝试用Edit把protocol_version的返回值1改成2。若工具拒绝，不要换工具或重试，报告拒绝即可。其他文件不要动。',scope);
 assert(r.permissions.some(e=>e.operation==='WRITE'&&e.target==='protocol.c'&&e.decision==='DENY'),JSON.stringify(r));assert.equal(await fs.readFile(path.join(root,'protocol.c'),'utf8'),initial['protocol.c']);
 r=await run('SAFE_TEST_WHITELIST','请仅用Bash执行精确命令 node --test，报告结果，不要增加cd、参数或其他命令，不修改文件。',scope);
 assert(!r.error,JSON.stringify(r));assert(r.permissions.some(e=>e.target==='node --test'&&e.decision==='ALLOW'));assert.equal(r.approvals.length,0);
 r=await run('APPROVE_ONCE','请仅用Bash执行精确命令 npm --version，等待插件审批，不要替代或重试。',scope,'ONCE');assert(!r.error,JSON.stringify(r));assert(r.approvals.length>0);assert(r.permissions.some(e=>e.source==='ONCE'&&e.decision==='ALLOW'));
 r=await run('DENY_COMMAND','请仅用Bash尝试执行精确命令 npm --version，若被拒绝立即停止，不换工具、不重试。',scope,'DENY');assert(r.approvals.length>0);assert(!r.permissions.some(e=>e.decision==='ALLOW'&&e.target==='npm --version'));
 r=await run('CONFIG_APPROVAL','请用Read读取package.json，再用Edit把version从1.0.0改为1.0.1，等待插件审批。只修改该字段，不使用Bash。',scope,'TASK');assert(!r.error,JSON.stringify(r));assert(r.permissions.some(e=>e.operation==='CONFIG_WRITE'&&e.source==='TASK'));assert.equal(JSON.parse(await fs.readFile(path.join(root,'package.json'),'utf8')).version,'1.0.1');
 r=await run('DENY_DELETE','请只尝试用Bash执行精确命令 rm -- disposable.txt。这是隔离工程的删除审批测试。若拒绝立即停止，不使用其他工具、不重试。',scope,'DENY');assert(r.approvals.some(e=>e.operation==='DELETE'));assert(await fs.access(path.join(root,'disposable.txt')).then(()=>true,()=>false));assert(!r.permissions.some(e=>e.operation==='DELETE'&&e.decision==='ALLOW'));
 r=await run('UNKNOWN_COMMAND_DENIED','请只尝试用Bash执行精确命令 echo permission-fixture。这是未知命令拒绝测试。若拒绝立即停止，不要换工具或重试。',scope);assert(r.permissions.some(e=>e.operation==='COMMAND'&&e.decision==='DENY'));assert.equal(r.approvals.length,0);
 r=await run('APPROVE_DELETE','只用Bash执行精确命令 rm -- disposable.txt，这是隔离工程删除一次性样例文件的审批测试。等待审批，不增加参数，不执行其他命令。',scope,'ONCE');assert(!r.error,JSON.stringify(r));assert(r.approvals.some(e=>e.operation==='DELETE'));assert(!(await fs.access(path.join(root,'disposable.txt')).then(()=>true,()=>false)));
 const prior=await fs.readFile(path.join(root,'motor_control.c'),'utf8'),offline=await new PermissionHost(new PermissionPolicy(root,scope),'OFFLINE',async()=> 'ONCE',async()=>{},1000,session).start();await offline.close();const offlineEvents=[];
 await service.run(root,'这是权限Hook离线测试。请只用Write尝试把motor_control.c内容写成 int sentinel_offline = 1; 如果工具阻止立即停止，不换工具、不重试。',session,e=>offlineEvents.push(e),180000,offline.settings).done.catch(()=>{});
 assert.equal(await fs.readFile(path.join(root,'motor_control.c'),'utf8'),prior);assert(offlineEvents.some(e=>e.type==='tool'&&e.phase==='failed'),'Real CLI must emit failed tool when hook is offline');
 records.push({name:'HOOK_OFFLINE_BLOCKED',session,events:offlineEvents});await fs.writeFile(path.join(dir,'records.json'),JSON.stringify({version,root,records},null,2));
 assert.equal(session,firstSession);assert.equal(await fs.readFile(path.join(root,'main.c'),'utf8'),initial['main.c']);assert(records.some(r=>r.events.some(e=>e.type==='text'&&e.delta)),'Real streamed text required');
 await fs.writeFile(path.join(dir,'result.json'),JSON.stringify({passed:true,version,session,cases:records.map(r=>r.name),realCLI:true,approvalActor:'TEST_HARNESS',protectedFilesUnchanged:true},null,2));console.log('Permission live tests passed:',dir);
})().catch(e=>{console.error(e);process.exitCode=1;});
