const test=require('node:test'),assert=require('node:assert/strict'),fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');
const api=require('../.tools/retry/api.cjs');
const helper=path.resolve('test/mock-claude.cjs');
const noChannel='API Error: 503 No available channel for model test-model';
const retryEvents=c=>c.active.requests.at(-1).claudeRetries||[];
async function fixture(failures=[],options={}){
 const dir=await fs.mkdtemp(path.join(os.tmpdir(),'atm-r1-')),root=path.join(dir,'project');await fs.mkdir(root);await fs.writeFile(path.join(root,'source.py'),'initial source');
 const launches=[],events=[];
 const service=new api.ClaudeService('mock',(command,args,cwd,env)=>{
  const check=args.includes('--version')||args.includes('--help');
  const fault=check?undefined:failures[launches.length];
  if(!check)launches.push({args:[...args],cwd,env});
  return api.launch(process.execPath,[helper,...args],cwd,{...env,ATM_MOCK_RETRY:fault?JSON.stringify(fault):''});
 });
 const store=new api.ClaudeStorage(path.join(dir,'history'),root);
 const c=new api.ClaudeController(store,service,options.changed||(()=>{}),async()=>{},()=>options.timeout||10000,options.prepare,()=>({maxRetries:options.maxRetries??3,baseDelayMs:options.baseDelayMs??100}));
 await c.initialize();return {dir,root,c,service,store,launches,events,send:opts=>c.send('UNCHANGED ORIGINAL PROMPT',{observe:e=>{events.push(e);options.observe?.(e,c);},...opts})};
}
test('退避默认 2/4/8 秒、有界正向抖动，遵守 Retry-After 且配置有界',()=>{
 const p=api.claudeRetryPolicy();assert.deepEqual(p,{maxRetries:3,baseDelayMs:2000});
 const hint=api.classifyClaudeError('HTTP 503').retry;
 assert.deepEqual([1,2,3].map(i=>api.claudeRetryDelay(hint,i,p,()=>0)),[2000,4000,8000]);
 assert.equal(api.claudeRetryDelay(hint,3,p,()=>1),10000);
 assert.equal(api.claudeRetryDelay({...hint,reason:'HTTP_429',retryAfterMs:12000},1,p,()=>0),12000);
 for(const retryAfterMs of [undefined,60001,Infinity,-1])assert.equal(api.claudeRetryDelay({reason:'HTTP_429',routeRejected:false,retryAfterMs},1,p),undefined);
 assert.equal(api.retryAfterMs('Retry-After: 4'),4000);assert.equal(api.retryAfterMs('Retry-After: Sun, 15 Nov 2026 00:00:04 GMT',Date.parse('2026-11-15T00:00:00Z')),4000);
 assert.equal(api.retryAfterMs('Retry-After: not-a-date'),undefined);
 assert.deepEqual(api.claudeRetryPolicy({maxRetries:99,baseDelayMs:1}),{maxRetries:5,baseDelayMs:100});
});
test('只将明确临时错误列入重试候选；认证/权限/恢复/版本错误优先',()=>{
 for(const text of ['HTTP 502','API Error: 503','HTTP/1.1 504','ECONNRESET','ETIMEDOUT','ECONNREFUSED 127.0.0.1:15721',noChannel])assert(api.classifyClaudeError(text).retry,text);
 for(const text of ['ECONNREFUSED','fetch failed','network unavailable','HTTP 500','401 authentication failed '+noChannel,'permission denied '+noChannel,'No conversation found with session ID. '+noChannel,'unknown option '+noChannel])assert.equal(api.classifyClaudeError(text).retry,undefined,text);
});
for(const failures of [1,3])test(`连续 ${failures} 次 503 后成功：原会话/用户消息/请求记录均不重复`,async()=>{
 const f=await fixture(Array.from({length:failures},()=>({diagnostic:true,usage:'missing'})));await f.send();
 assert.equal(f.c.active.status,'COMPLETED',JSON.stringify(f.c.snapshot()));assert.equal(f.launches.length,failures+1);
 assert.equal(f.c.active.requests.length,1);assert.equal(f.c.active.messages.filter(m=>m.role==='user').length,1);assert(f.c.active.messages.filter(m=>m.role==='assistant').every(m=>!m.text.includes('API Error')));
 const sid=f.c.active.threadId;for(const l of f.launches.slice(1)){assert.equal(l.args[l.args.indexOf('--resume')+1],sid);assert.equal(l.cwd,f.root);assert(!l.args.includes('--model'));}
 assert.deepEqual(retryEvents(f.c).filter(e=>e.kind==='RETRY_SCHEDULED').map(e=>e.retry),Array.from({length:failures},(_,i)=>i+1));assert.equal(retryEvents(f.c).at(-1).kind,'RETRY_SUCCEEDED');
 const persisted=await f.store.load();assert.equal(persisted.conversations[0].requests.length,1);assert.equal(persisted.conversations[0].threadId,sid);
});
test('四次均失败后安全终止，保留原 Session 与一次请求记录',async()=>{
 const f=await fixture(Array.from({length:4},()=>({})));await f.send();assert.equal(f.launches.length,4);assert.equal(f.c.active.status,'FAILED');assert(f.c.active.threadId);assert.equal(f.c.active.requests.length,1);assert.equal(retryEvents(f.c).at(-1).kind,'RETRY_EXHAUSTED');assert(f.c.snapshot().statusEvent.includes('已达到最大'));
});
test('首次尚无 Session 时只在同一会话记录内重试；maxRetries=0 禁用',async()=>{
 const f=await fixture([{noSession:true}]);await f.send();assert.equal(f.launches.length,2);assert(!f.launches[1].args.includes('--resume'));assert.equal(f.c.active.requests.length,1);assert.equal(f.c.active.status,'COMPLETED');
 const off=await fixture([{}],{maxRetries:0});await off.send();assert.equal(off.launches.length,1);assert.deepEqual(retryEvents(off.c),[]);
});
test('取消退避等待后不会启动下一进程；中途重启不会重放',async()=>{
 let stopped;const stop=new Promise(r=>stopped=r);
 const f=await fixture([{}],{baseDelayMs:1000,observe:(e,c)=>{if(e.type==='retry'&&e.kind==='RETRY_SCHEDULED'){assert(c.isBusy);assert.equal(c.active.status,'RUNNING');void c.stop().then(stopped);}}});
 await f.send();await stop;assert.equal(f.launches.length,1);assert.equal(f.c.active.status,'CANCELLED');assert.equal(retryEvents(f.c).at(-1).kind,'RETRY_CANCELLED');
 const saved=await f.store.load();saved.conversations[0].status='RUNNING';saved.conversations[0].requests[0].status='RUNNING';await f.store.save(saved);
 let calls=0;const restored=new api.ClaudeController(f.store,{run(){calls++;throw Error('no replay');}},()=>{},async()=>{},()=>1000);await restored.initialize();assert.equal(calls,0);assert.equal(restored.active.status,'CANCELLED');assert.equal(restored.active.threadId,f.c.active.threadId);
});
for(const [name,fault,expected] of [
 ['认证失败',{error:'401 authentication failed'},'AUTHENTICATION_FAILED'],
 ['权限拒绝',{denied:true},'PERMISSION_DENIED'],
 ['Session 恢复失败',{error:'No conversation found with session ID.'},'SESSION_RESUME_FAILED'],
 ['版本不兼容',{error:'unknown option --bare'},'CLAUDE_VERSION_UNSUPPORTED'],
 ['解析失败',{parse:true},'OUTPUT_PARSE_FAILED'],
 ['会话身份改变',{wrongSession:true},'OUTPUT_PARSE_FAILED'],
 ['普通超时',{hang:true},'REQUEST_TIMEOUT'],
])test(`${name}不重试`,async()=>{const f=await fixture([fault],{timeout:fault.hang?250:10000});await f.send();assert.equal(f.launches.length,1);assert(f.c.snapshot().error.includes(expected),f.c.snapshot().error);});
for(const [name,fault] of [
 ['非空正文',{text:'已开始执行'}],['普通正文中的错误字样',{text:noChannel}],['工具调用',{tool:true}],['文件写入',{write:'source.py'}],['正数 Token',{usage:'positive'}],['中途用量后最终零用量',{partialUsage:true}],['思考执行',{thinking:true}],['子任务活动',{child:true}],['普通 503 缺少用量',{error:'HTTP 503',usage:'missing'}],['正数模型分项用量',{modelUsage:true}],['正数计费记录',{cost:true}],['残缺用量',{usage:'partial'}],['无效用量',{usage:'invalid'}],
])test(`${name}禁止重放并保留已有结果`,async()=>{const f=await fixture([fault]);await f.send();assert.equal(f.launches.length,1);assert.equal(retryEvents(f.c).at(-1).kind,'RETRY_BLOCKED_SIDE_EFFECT');if(fault.write)assert.equal(await fs.readFile(path.join(f.root,fault.write),'utf8'),'changed by fault fixture');});
test('等待期间外部改动会在第二次快照校验被拦截',async()=>{
 let f;f=await fixture([{}],{observe:e=>{if(e.type==='retry'&&e.kind==='RETRY_SCHEDULED')require('node:fs').writeFileSync(path.join(f.root,'new.py'),'external edit');}});await f.send();assert.equal(f.launches.length,1);assert.equal(retryEvents(f.c).at(-1).kind,'RETRY_BLOCKED_SIDE_EFFECT');
});
test('普通权限入口已被调用（即使 stdout 没有工具事件）禁止重试',async()=>{
 const f=await fixture([{permission:true}]);let host;
 await f.send({preparePermissions:async(id,sid)=>host=await new api.PermissionHost(new api.PermissionPolicy(f.root,{allowedFiles:[],safeCommands:[]}),id,async()=> 'DENY',async()=>{},1000,sid).start(),observe:e=>f.events.push(e)});
 assert.equal(f.launches.length,1);assert.equal(retryEvents(f.c).at(-1).kind,'RETRY_BLOCKED_SIDE_EFFECT');assert.equal(host.canRetry(),false);
});
test('未知权限实现或待审批状态不能推断安全',async()=>{
 for(const prepare of [async()=>({settings:'{}',close:async()=>{}}),async()=>({settings:'{}',canRetry:()=>false,close:async()=>{}})]){const f=await fixture([{}],{prepare});await f.send();assert.equal(f.launches.length,1);assert.equal(retryEvents(f.c).at(-1).kind,'RETRY_BLOCKED_SIDE_EFFECT');}
});
test('429 遵守秒值 Retry-After，缺失或超上限不自动重试',async()=>{
 const f=await fixture([{error:'HTTP 429\nRetry-After: 0.25'}]);await f.send();assert.equal(f.launches.length,2);assert(retryEvents(f.c).find(e=>e.kind==='RETRY_SCHEDULED').delayMs>=250);
 for(const error of ['HTTP 429','HTTP 429\nRetry-After: 120']){const g=await fixture([{error}]);await g.send();assert.equal(g.launches.length,1);assert.equal(retryEvents(g.c).at(-1).kind,'RETRY_DEFERRED');}
});
test('日志不保留上游密钥、认证信息和原始诊断参数',async()=>{
 const f=await fixture([{error:noChannel+' password=SECRET_SHOULD_NOT_APPEAR Authorization: Bearer ABCDEFGHIJKLMNOPQRST'}]);await f.send();const saved=JSON.stringify([f.c.snapshot(),f.events,await f.store.load()]);assert(!saved.includes('SECRET_SHOULD_NOT_APPEAR'));assert(!saved.includes('ABCDEFGHIJKLMNOPQRST'));
});
const planned=(id)=>({task_id:id,assigned_worker:id,allowed_files:[id+'.py'],read_only_files:[],forbidden_files:['forbidden.py'],test_commands:['python -B '+id+'.py'],dependencies:[],interface_contracts:[],acceptance_criteria:['test'],task_description:id,status:'PENDING',parent_task_id:null,base_commit:'a'.repeat(40)});
test('两个 Worker 并发重试复用各自原租约和 MCP，不扩大权限或重复任务',async()=>{
 const dir=await fs.mkdtemp(path.join(os.tmpdir(),'atm-r1-workers-')),locks=new api.FileLockManager(path.join(dir,'locks'),'p'),tasks=[planned('a'),planned('b')],ownership=new api.FileOwnershipManager({tasks,shared_file_owners:[]}),sessions=[],leases=[],calls=new Map();
 const manager=new api.ClaudeWorkerManager({project_id:'p',directory:path.join(dir,'history'),maxConcurrent:2,verify:async()=>{},retryPolicy:()=>({maxRetries:3,baseDelayMs:100}),service:()=>new api.ClaudeService('mock',(cmd,args,cwd,env)=>{const check=args.includes('--version')||args.includes('--help'),n=calls.get(cwd)||0;if(!check)calls.set(cwd,n+1);return api.launch(process.execPath,[helper,...args],cwd,{...env,ATM_MOCK_RETRY:!check&&n===0?JSON.stringify({}):''});})});
 await manager.load();
 try{
 const workers=[];
 for(const t of tasks){const root=path.join(dir,t.task_id);await fs.mkdir(root);const worker=await manager.create(t.task_id,root,'fixture',t.allowed_files);workers.push(worker);const lease=await locks.acquire(t.task_id,worker.worker_id,t.allowed_files);leases.push(lease);sessions.push(await new api.WorkerExecutionSession(root,t.task_id,worker.worker_id,lease.id,ownership,locks,new api.IsolatedTestRunner(path.join(dir,'tests')),async()=> 'DENY',async()=>{}).start());}
 const configs=sessions.map(s=>s.runOptions.mcpConfig),seen=[];
 const results=await Promise.allSettled(workers.map((w,i)=>manager.run(w.worker_id,'same prompt',{preparePermissions:async()=>sessions[i],observe:e=>{if(e.type==='session')sessions[i].bindSession(e.id);if(e.type==='retry'&&e.kind==='RETRY_SCHEDULED'){seen.push(w.worker_id);assert.equal(manager.detail(w.worker_id).worker.status,'RETRYING');}}})));
 assert(results.every(r=>r.status==='fulfilled'),JSON.stringify({results:results.map(r=>r.status==='rejected'?r.reason.message:r.status),registration:sessions.map(s=>s.registrationError?.message),counts:[...calls.values()],retries:manager.retryEvents().map(e=>({kind:e.event.kind,task:e.task_id}))}));
 assert.equal(seen.length,2);for(const [i,w] of workers.entries()){assert.equal(calls.get(w.workspace_path),2);const d=manager.detail(w.worker_id);assert.equal(d.worker.status,'COMPLETED');assert.equal(d.chat.conversation.requests.length,1);assert.equal(d.chat.conversation.messages.filter(m=>m.role==='user').length,1);assert.equal(sessions[i].runOptions.mcpConfig,configs[i]);}
 const held=await locks.list();assert.equal(held.length,2);assert(held.every(l=>l.process_ids.length===2));assert.deepEqual(held.map(l=>l.id).sort(),leases.map(l=>l.id).sort());
 await assert.rejects(locks.acquire('foreign','other',['a.py']),/WAITING_FILE_LOCK/);
 const events=manager.retryEvents();assert(events.some(e=>e.event.kind==='RETRY_SUCCEEDED'));const journal=api.collectSupervision([],[],[],{},events);assert(journal.some(e=>e.kind==='RETRY_SCHEDULED'));assert.equal(api.collectSupervision(journal,[],[],{},events).length,journal.length);
 }finally{await manager.dispose();for(const s of sessions)await s.close();for(const l of leases)await locks.release(l.id);}
});
test('受限 MCP 已调用后不能再次启动 CLI，原租约保持不变',async()=>{
 const f=await fixture([{gateway:true}]),task=planned('a'),locks=new api.FileLockManager(path.join(f.dir,'locks'),'p'),lease=await locks.acquire('a','worker',['a.py']),ownership=new api.FileOwnershipManager({tasks:[task],shared_file_owners:[]});let session;
 try{await f.send({preparePermissions:async()=>session=await new api.WorkerExecutionSession(f.root,'a','worker',lease.id,ownership,locks,new api.IsolatedTestRunner(path.join(f.dir,'tests')),async()=> 'DENY',async()=>{}).start(),observe:e=>{if(e.type==='session')session.bindSession(e.id);}});assert.equal(f.launches.length,1);assert.equal(retryEvents(f.c).at(-1).kind,'RETRY_BLOCKED_SIDE_EFFECT');assert.equal((await locks.list())[0].id,lease.id);}finally{await session?.close();await locks.release(lease.id);}
});
test('重试中 Worker 的暂停立即取消等待；重新载入不会自动执行',async()=>{
 const f=await fixture([]),calls=[];let manager,worker;
 manager=new api.ClaudeWorkerManager({project_id:'p',directory:path.join(f.dir,'workers'),verify:async()=>{},retryPolicy:()=>({baseDelayMs:1000}),service:()=>new api.ClaudeService('mock',(cmd,args,cwd)=>{if(!args.includes('--version')&&!args.includes('--help'))calls.push(args);return api.launch(process.execPath,[helper,...args],cwd,{ATM_MOCK_RETRY:JSON.stringify({})});})});await manager.load();worker=await manager.create('a',f.root,'fixture',['a.py']);
 await assert.rejects(manager.run(worker.worker_id,'x',{observe:e=>{if(e.type==='retry'&&e.kind==='RETRY_SCHEDULED')void manager.stop(worker.worker_id,true);}}));assert.equal(calls.length,1);assert.equal(manager.detail(worker.worker_id).worker.status,'PAUSED');const sid=manager.detail(worker.worker_id).worker.session_id;await manager.dispose();
 const reload=new api.ClaudeWorkerManager({...manager.options,service:()=>({run(){throw Error('must not run');}})});
 await reload.load();assert.equal(reload.detail(worker.worker_id).worker.session_id,sid);assert.equal(calls.length,1);await reload.dispose();
});
test('重连后真实 MCP 写入仍受原文件权限和同一租约约束',async()=>{
 for(const file of ['a.py','forbidden.py']){
  const f=await fixture([{}, {gateway:true,gatewayWrite:file,success:true}]),task=planned('a'),locks=new api.FileLockManager(path.join(f.dir,'locks'),'p'),lease=await locks.acquire('a','worker',['a.py']),ownership=new api.FileOwnershipManager({tasks:[task],shared_file_owners:[]});let session;const audit=[];
  try{await f.send({preparePermissions:async()=>session=await new api.WorkerExecutionSession(f.root,'a','worker',lease.id,ownership,locks,new api.IsolatedTestRunner(path.join(f.dir,'tests')),async()=> 'DENY',async e=>audit.push(e)).start(),observe:e=>{if(e.type==='session')session.bindSession(e.id);}});
   assert.equal(f.launches.length,2);const arg=(l,k)=>l.args[l.args.indexOf(k)+1];assert.equal(arg(f.launches[0],'--mcp-config'),arg(f.launches[1],'--mcp-config'));assert.equal(arg(f.launches[0],'--allowedTools'),arg(f.launches[1],'--allowedTools'));assert.equal(arg(f.launches[1],'--tools'),'');assert.equal(arg(f.launches[1],'--permission-mode'),'manual');assert(!f.launches[1].args.some(a=>a.includes('skip-permission')));
   if(file==='a.py'){assert.equal(f.c.active.status,'COMPLETED');assert.equal(await fs.readFile(path.join(f.root,file),'utf8'),'written after reconnect');assert.equal(audit.filter(e=>e.kind==='FILE_WRITE_RESULT').length,1);}else{assert.equal(f.c.active.status,'FAILED');await assert.rejects(fs.stat(path.join(f.root,file)));assert(audit.some(e=>e.decision==='DENY'));}
   const held=await locks.list();assert.equal(held[0].id,lease.id);assert.equal(held[0].process_ids.length,2);
  }finally{await session?.close();await locks.release(lease.id);}
 }
});
test('权限主机在未完成的真实 HTTP 审批期间禁止重试，关闭会取消审批',async()=>{
 const f=await fixture();let entered;const pending=new Promise(r=>entered=r);
 const host=await new api.PermissionHost(new api.PermissionPolicy(f.root,{allowedFiles:[],safeCommands:[]}),'task',async(op,signal)=>{entered();return new Promise(r=>signal.addEventListener('abort',()=>r('DENY'),{once:true}));},async()=>{},1000,'session').start();
 const url=JSON.parse(host.settings).hooks.PreToolUse[0].hooks[0].url;
 const call=fetch(url,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({hook_event_name:'PreToolUse',session_id:'session',cwd:f.root,tool_name:'Bash',tool_input:{command:'npm --version'}})}).then(r=>r.json());await pending;assert.equal(host.canRetry(),false);const close=host.close();const answer=await call;await close;assert.equal(answer.hookSpecificOutput.permissionDecision,'deny');
});
test('普通重试只刷新界面不推进 GPT 决策修订；工具摘要仍可查看当前重试',async()=>{
 const dir=await fs.mkdtemp(path.join(os.tmpdir(),'atm-r1-notify-'));let renders=0;
 const team=new api.TeamSession({root:dir,directory:dir,services:{},editors:async()=>[],authorize:async()=>false,approve:async()=> 'DENY',changed:()=>renders++});
 const row={task:planned('a'),status:'RUNNING'},worker={worker_id:'w',task_id:'a',session_id:'s',status:'RUNNING'},state={plan:{plan_id:'p',owner_id:'owner',goal:'fixture'},tasks:[row],workers:[worker],record:{reviews:{},supervision:[]}};
 team.workflow={plan:state.plan,snapshot:()=>state};team.notify();const revision=team.revision;
 worker.status='RETRYING';worker.retry={kind:'RETRY_SCHEDULED',retry:1,maxRetries:3,reason:'HTTP_503',delayMs:2000};state.record.supervision.push({id:'retry',kind:'RETRY_SCHEDULED'});team.notify();assert.equal(team.revision,revision);assert.equal(renders,2);
 team.snapshot=()=>({revision:team.revision,terminal:false,busy:true,workflow:state});const result=await team.call('get_team_task_output',{plan_id:'p',revision},'owner');assert.equal(result.workers,undefined);assert.equal(result.workflow.workers[0].retry.retry,1);assert.equal(result.workflow.tasks[0].status,'RUNNING');assert.equal(result.unchanged,true);
 state.record.supervision.push({id:'exhausted',kind:'RETRY_EXHAUSTED'});worker.status='FAILED';team.notify();assert(team.revision>revision);
});
test('所有重试参数、已取消等待、文件快照都能保守失败',async()=>{
 const f=await fixture();const abort=new AbortController();abort.abort();await assert.rejects(api.waitForClaudeRetry(2000,abort.signal),e=>e.code==='REQUEST_CANCELLED');await assert.rejects(api.retryWorkspaceFingerprint(f.root,abort.signal));
 const config=require('../package.json').contributes.configuration.properties;assert.equal(config['aiTeamManager.claudeRetryMaxRetries'].default,3);assert.equal(config['aiTeamManager.claudeRetryMaxRetries'].maximum,5);assert.equal(config['aiTeamManager.claudeRetryBaseDelayMs'].default,2000);
});
test('文件锁登记遇到尚未写完的存活锁主记录时等待，不误判权限错误或抢占',async()=>{
 const f=await fixture(),locks=new api.FileLockManager(path.join(f.dir,'locks'),'p'),mutex=path.join(locks.watchDirectory(),'mutex');await fs.mkdir(mutex,{recursive:true});await fs.writeFile(path.join(mutex,'owner.json'),'{');
 let acquired=false;const pending=locks.acquire('a','w',['a.py']).then(l=>{acquired=true;return l;});
 await new Promise(r=>setTimeout(r,60));assert.equal(acquired,false);await fs.writeFile(path.join(mutex,'owner.json'),JSON.stringify({pid:process.pid,nonce:'fixture'}));await fs.unlink(path.join(mutex,'owner.json'));await fs.rmdir(mutex);
 const lease=await pending;assert.equal(lease.task_id,'a');await locks.release(lease.id);
});
test('中文重试状态、错误类型与原会话提示在工作台可见',()=>{
 const {JSDOM}=require('jsdom'),read=require('node:fs').readFileSync,dom=new JSDOM('<main></main>',{runScripts:'outside-only'}),w=dom.window;
 for(const file of ['markdown.js','workspace/layoutManager.js','workspace/chatPanel.js','workspace/outputFormatter.js','workspace/workerPanel.js'])w.eval(read('src/webview/'+file,'utf8'));
 const sent=[],panel=new w.ATMWorkerPanel(m=>sent.push(m),()=>{}),id='11111111-1111-4111-8111-111111111111',retry={type:'retry',kind:'RETRY_SCHEDULED',reason:'NO_AVAILABLE_CHANNEL',retry:2,maxRetries:3,delayMs:4000,nextRetryAt:new Date(Date.now()+4000).toISOString(),sessionId:'original'};
 const worker={worker_id:id,task_id:'a',project_id:'p',session_id:'original',status:'RETRYING',allowed_files:['a.py'],retry},row={task:planned('a'),status:'RUNNING',events:[]};
 panel.update({workflow:{plan:{project_id:'p',plan_id:'plan',project_path:'/project'},workers:[worker],tasks:[row]},selected_worker:id,detail:{worker,task:row.task,chat:{conversation:{messages:[]}},events:{events:[]},message_start:0,message_total:0,event_total:0}},id);
 assert(panel.node.textContent.includes('自动重试中'));assert(panel.node.textContent.includes('模型通道暂不可用'));assert(panel.node.textContent.includes('第 2 / 3 次'));assert(panel.node.textContent.includes('原会话已保留'));assert(!panel.node.textContent.includes('NO_AVAILABLE_CHANNEL'));panel.node.querySelector('[aria-label="暂停当前 Worker"]').click();assert.equal(sent[0].action,'pause');dom.window.close();
});
