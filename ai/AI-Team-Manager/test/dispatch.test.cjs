const test=require('node:test'),assert=require('node:assert/strict'),fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os'),http=require('node:http');
const {ToolBridge,dispatcherTools,validateDispatch,validateExecutionPlan,validateApproval,explicitlyNamedFiles,userAuthority,DispatchPolicy,PermissionGateway,PermissionHost,ReviewGit,ReviewStorage,TaskDispatcher,git}=require('../.tools/task02-test/api.cjs');
const input={description:'add',allowed_files:['test.py'],forbidden_files:['protocol.c'],test_commands:['python -B test.py'],acceptance_criteria:['integer addition and passing boundary tests']};
const temp=()=>fs.mkdtemp(path.join(os.tmpdir(),'atm-dispatch-'));
const record=root=>({task_id:'task',project_path:root,user_request:'create test.py',input,mode:'GPT_HYBRID',rounds:[],state:'RUNNING',gpt_calls:[],events:[]});
const approve=r=>({decision:'APPROVE',risk_level:r.risk_level,reason:'exact authorized operation',approved_scope:[...r.target_files,...(r.command?[r.command]:[])],restrictions:[],expires_at:new Date(Date.now()+60000).toISOString()});

test('Windows cmd shim accepts production dispatcher instructions and MCP config without interpreting redirections',{skip:process.platform!=='win32'},async()=>{
 const {CodexService}=require('../.tools/task02-test/api.cjs'),root=await temp(),command=path.join(root,'mock.cmd');
 await fs.writeFile(command,`@echo off\r\n"${process.execPath}" "${path.resolve('test/mock-cli.cjs')}" %*\r\n`);
 const source=await fs.readFile('src/dispatch/dispatchCoordinator.ts','utf8');
 const instructions=require('../.tools/task02-test/api.cjs').PLANNING_INSTRUCTIONS+'\n'+source.match(/export const DISPATCH_INSTRUCTIONS=`([\s\S]*?)`;/)[1].replace(/\\n/g,'\n');
 const bridge=new ToolBridge(async()=>({})),options=await bridge.start();options.instructions=instructions;
 try{const events=[],service=new CodexService(command);await service.check(root);await service.run(root,'CODEX_CONNECT_OK',undefined,e=>events.push(e),10000,options).done;assert(events.some(e=>e.type==='message'&&e.text==='CODEX_CONNECT_OK'));}finally{await bridge.close();await fs.rm(root,{recursive:true,force:true});}
});
test('Codex timeout settles even if platform child never emits close after termination',async()=>{
 const {EventEmitter}=require('node:events'),{CodexService}=require('../.tools/task02-test/api.cjs');
 const child=new EventEmitter();child.stdin={end(){},on(){}};
 const run=new CodexService('codex',()=>child).run('.', 'test',undefined,()=>{},5);
 await assert.rejects(Promise.race([run.done,new Promise((_,reject)=>setTimeout(()=>reject(Error('hung subprocess promise')),500))]),e=>e.code==='REQUEST_TIMEOUT');
});
test('dispatcher validates exact scope, explicit user files, test argv and Claude plan boundaries',()=>{
 assert.deepEqual(validateDispatch(input),input);assert.deepEqual(explicitlyNamedFiles('创建 test.py，仅允许 src/motor_control.c'),['test.py','src/motor_control.c']);
 assert.deepEqual(userAuthority('创建 test.py，只允许修改 test.py，不得修改 protocol.c 或 README.md。'),{allowed:['test.py'],forbidden:['protocol.c','README.md']});
 for(const file of ['../outside.py','.env','.git/config','x/*.py'])assert.throws(()=>validateDispatch({...input,allowed_files:[file]}));
 for(const c of ['python -B test.py --x','python -B ../test.py','python -B test.py && echo x','git reset --hard'])assert.throws(()=>validateDispatch({...input,test_commands:[c]}));
 const plan={goal:'add',approach:'pure function',files:['test.py'],steps:['implement','test'],commands:['python -B test.py'],risks:[]};assert.deepEqual(validateExecutionPlan(JSON.stringify(plan),input),plan);assert.throws(()=>validateExecutionPlan(JSON.stringify({...plan,files:['protocol.c']}),input));
});
test('structured GPT approvals reject natural language, expired/widened/partial results',()=>{
 const req={target_files:['test.py'],command:'',risk_level:'L1'},d=approve(req);assert.equal(validateApproval(JSON.stringify(d),req).decision,'APPROVE');
 for(const x of ['yes',JSON.stringify({...d,expires_at:'2000-01-01'}),JSON.stringify({...d,approved_scope:['protocol.c']}),JSON.stringify({...d,approved_scope:[]})])assert.throws(()=>validateApproval(x,req));
});
test('gateway reapproves changed exact arguments, never lets GPT override hard denies, and fails closed on timeout',async()=>{
 const root=await temp();await fs.writeFile(path.join(root,'test.py'),'print(1)\n');await fs.writeFile(path.join(root,'protocol.c'),'protected\n');
 const p=new DispatchPolicy(root,['test.py'],['python -B test.py']),logs=[],requests=[];let unavailable=false;
 const gateway=new PermissionGateway(record(root),async r=>{requests.push(r);if(unavailable)throw Error('timeout');return approve(r)},async(k,d)=>logs.push({k,d}));
 const signal=new AbortController().signal;
 assert.equal((await p.check('Write',{file_path:'protocol.c',content:'bad'})).behavior,'deny');assert.equal((await p.check('Bash',{command:'git reset --hard'})).behavior,'deny');assert.equal((await p.check('Bash',{command:'python -B test.py --flag'})).behavior,'deny');
 for(const content of ['print(2)','print(3)']){const args={file_path:'test.py',content};assert.equal((await gateway.evaluate(await p.check('Write',args),'Write',args,'session',signal)).behavior,'allow');}
 assert.equal(requests.length,2);assert.notEqual(requests[0].evidence.fingerprint,requests[1].evidence.fingerprint);assert(requests.every(r=>r.session_id==='session'&&r.target_files[0]==='test.py'));
 unavailable=true;const args={file_path:'test.py',content:'print(4)'};assert.equal((await gateway.evaluate(await p.check('Write',args),'Write',args,'session2',signal)).behavior,'deny');assert(logs.some(e=>e.k==='GPT_APPROVAL_FAILED'));
 const manual=new PermissionGateway({...record(root),mode:'MANUAL'},async()=>{throw Error('must not call')},async()=>{});assert.equal((await manual.evaluate(await p.check('Write',args),'Write',args,'s',signal)).behavior,'ask');
});
test('L2 configuration/dependencies require human even if GPT says APPROVE; L3 model judgment refuses',async()=>{
 const root=await temp(),r=record(root),args={file_path:'package.json',content:'{}'};r.input={...input,allowed_files:['package.json','package-lock.json']};
 const p=new DispatchPolicy(root,r.input.allowed_files,['npm install --ignore-scripts example@1.0.0']);let d=approve({target_files:['package.json'],command:'',risk_level:'L2'});
 const gw=new PermissionGateway(r,async()=>d,async()=>{}),signal=new AbortController().signal;
 assert.equal((await gw.evaluate(await p.check('Write',args),'Write',args,'s',signal)).behavior,'ask');
 assert.equal((await p.check('Bash',{command:'npm install --ignore-scripts example@1.0.0'})).behavior,'ask');
 d={...d,risk_level:'L3'};assert.equal((await gw.evaluate(await p.check('Write',args),'Write',args,'s',signal)).behavior,'deny');
});
test('native MCP HTTP server authenticates and exposes only five real dispatcher tools',async()=>{
 const calls=[],bridge=new ToolBridge(async(n,a)=>{calls.push([n,a]);return{actual:true}});const options=await bridge.start(),url=JSON.parse(options.config[0].split('=')[1]);
 const invoke=async(body,auth=true)=>{const u=new URL(url);return new Promise((resolve,reject)=>{const req=http.request(u,{method:'POST',headers:{'Content-Type':'application/json',...(auth?{Authorization:'Bearer '+options.env.ATM_DISPATCH_CAPABILITY}:{})}},res=>{let out='';res.on('data',b=>out+=b);res.on('end',()=>resolve({status:res.statusCode,body:out?JSON.parse(out):undefined}));});req.on('error',reject);req.end(JSON.stringify(body));});};
 try{assert.equal((await invoke({jsonrpc:'2.0',id:1,method:'tools/list'},false)).status,403);const list=await invoke({jsonrpc:'2.0',id:2,method:'tools/list'});assert.equal(list.body.result.tools.length,5);assert.equal(dispatcherTools.length,5);const result=await invoke({jsonrpc:'2.0',id:3,method:'tools/call',params:{name:'get_claude_session',arguments:{}}});assert.equal(JSON.parse(result.body.result.content[0].text).actual,true);assert.equal(calls.length,1);assert(!JSON.stringify(options.config).includes(options.env.ATM_DISPATCH_CAPABILITY));}finally{await bridge.close();}
});
test('persisted interrupted dispatch never auto-resends and task queries are bound to GPT conversation',async()=>{
 const root=await temp(),storage=new ReviewStorage(path.join(root,'history'));const r={...record(root),version:1,owner_id:'owner',project_path:root,created_at:new Date().toISOString(),updated_at:new Date().toISOString(),max_rounds:2};
 await storage.write(root,'dispatch-history.json',[r]);let sends=0;const d=new TaskDispatcher(root,storage,{}, {snapshot:()=>({}),sendDispatched:async()=>sends++,stop:async()=>{}},{},{},{},{},{mode:'GPT_HYBRID',maxRounds:2,authorize:async()=>false,verify:async()=>{}});
 await d.load();assert.equal(d.records[0].state,'INTERRUPTED');assert.equal(sends,0);await assert.rejects(()=>d.call('other','turn','test','get_claude_task_status',{task_id:'task'}));assert.equal((await d.call('owner','turn','test','get_claude_task_status',{task_id:'task'})).state,'INTERRUPTED');
});

test('same actual Hook call uses a single bounded approval; changed parameters, Session or expired approval cannot reuse it',async()=>{
 const root=await temp();await fs.writeFile(path.join(root,'test.py'),'print(1)\n');let judged=0;
 const host=await new PermissionHost(new DispatchPolicy(root,['test.py'],[]),'T',async()=>{throw Error('unexpected manual')},async()=>{},1000,'SESSION',async d=>{judged++;return d;}).start();
 const url=JSON.parse(host.settings).hooks.PreToolUse[0].hooks[0].url;
 const request=async(event,content='print(2)',session='SESSION',id='CALL')=>{const r=await fetch(url,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({hook_event_name:event,session_id:session,cwd:root,tool_name:'Write',tool_use_id:id,tool_input:{file_path:'test.py',content}})});return r.json();};
 try{
  assert.equal((await request('PreToolUse')).hookSpecificOutput.permissionDecision,'allow');assert.equal(judged,1);
  assert.equal((await request('PermissionRequest')).hookSpecificOutput.decision.behavior,'allow');assert.equal(judged,1);
  await request('PermissionRequest','print(3)');assert.equal(judged,2);
  assert.equal((await request('PreToolUse','print(2)','OTHER')).hookSpecificOutput.permissionDecision,'deny');assert.equal(judged,2);
  await request('PreToolUse');for(const v of host.preDecisions.values())v.expires=0;await request('PermissionRequest');assert.equal(judged,4);
 }finally{await host.close();}
});
