const {test} = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs/promises');
const path = require('node:path');
const os = require('node:os');
const {spawn} = require('node:child_process');
const api = require('../.tools/task02-test/api.cjs');
const root = path.resolve(__dirname,'..');
const launcher=(command,args,cwd)=>spawn(process.execPath,[path.join(__dirname,'mock-cli.cjs'),...args],{cwd,stdio:'pipe',windowsHide:true,detached:process.platform!=='win32'});
const service=()=>new api.CodexService('mock',launcher);
const waitFor=async fn=>{for(let i=0;i<100;i++){if(await fn())return;await new Promise(r=>setTimeout(r,25));}throw new Error('wait timeout');};
test('JSONL preserves UTF8 chunks and trailing line, hides reasoning and raw malformed data',()=>{
  const events=[];const parser=new api.CodexEventParser(e=>events.push(e));
  const text=Buffer.from(JSON.stringify({type:'item.updated',item:{id:'x',type:'agent_message',text:'中文'}})+'\n'+JSON.stringify({type:'item.completed',item:{id:'secret',type:'reasoning',text:'PRIVATE_REASONING'}})+'\ninvalid secret\n'+JSON.stringify({type:'thread.started',thread_id:'thread_1'}));
  for(let i=0;i<text.length;i+=2)parser.push(text.subarray(i,i+2));parser.end();
  assert.equal(events.find(e=>e.type==='message').text,'中文');assert(events.some(e=>e.type==='thread'));assert(events.some(e=>e.type==='parseError'));assert(!JSON.stringify(events).includes('secret'));assert(!JSON.stringify(events).includes('PRIVATE_REASONING'));
});
test('oversize lines recover at next newline and unknown events are tolerated',()=>{
  const e=[];const p=new api.CodexEventParser(x=>e.push(x));p.push('x'.repeat(2100000));p.push('\n{"type":"new.event"}\n{"type":"turn.started"}\n');p.end();assert.deepEqual(e.map(x=>x.type),['parseError','unknown','turn']);
});
test('usage does not invent missing fields or double count cached/reasoning tokens',()=>{
  const u=api.parseUsage({input_tokens:100,cached_input_tokens:60,output_tokens:20,reasoning_output_tokens:10});assert.equal(u.ordinaryInput,40);assert.equal(u.total,120);assert.equal(u.cacheWrite,null);assert.equal(api.sumUsage([{usage:u},{}]).total,null);assert.equal(api.sumUsage([{usage:u},{usage:u}]).total,240);
});
test('resume usage subtracts cumulative snapshots; interrupted gaps remain unknown',()=>{
  const a=api.parseUsage({input_tokens:100,cached_input_tokens:40,output_tokens:8});const b=api.parseUsage({input_tokens:220,cached_input_tokens:100,output_tokens:16});
  const delta=api.usageDelta(b,a,false);assert.equal(delta.input,120);assert.equal(delta.output,8);assert.equal(delta.cached,60);assert.equal(delta.total,128);assert.equal(delta.cacheWrite,null);
  assert.equal(api.usageDelta(b,undefined,false).input,null);assert.equal(api.usageDelta(a,b,false).input,null);
});
test('arguments enforce read-only on new and resume; prompt is absent from argv',()=>{
  for(const id of [undefined,'existing-thread']){const args=api.buildArgs(root,id);assert.equal(args[args.indexOf('--sandbox')+1],'read-only');assert.equal(args[args.indexOf('--cd')+1],root);assert.equal(args.at(-1),'-');if(id)assert(args.indexOf('--sandbox')<args.indexOf('resume'));assert(!args.includes('--skip-git-repo-check'));assert(!args.includes('--model'));}
  assert.throws(()=>api.buildArgs(root,'x & echo injected'));
});
test('safe CLI startup, malicious stdin roundtrip, version detection and .cmd shims',async()=>{
  await service().check(root);const events=[];const prompt='你好 & echo INJECTED | $(hello) "test"';await service().run(root,prompt,undefined,e=>events.push(e),5000).done;assert.equal(events.find(e=>e.type==='message').text,prompt);
  if(process.platform==='win32'){
    const dir=await fs.mkdtemp(path.join(os.tmpdir(),'atm-cli-'));const shim=path.join(dir,'codex-test.cmd');await fs.writeFile(shim,`@echo off\r\n"${process.execPath}" "${path.join(__dirname,'mock-cli.cjs')}" %*\r\n`);
    const s=new api.CodexService(shim);await s.check(root);const es=[];await s.run(root,prompt,undefined,e=>es.push(e),5000).done;assert.equal(es.find(e=>e.type==='message').text,prompt);
  }
});
test('error categories redact raw stderr; unavailable route and recovery',async()=>{
  for(const [prompt,code] of [['route','CCSWITCH_ROUTE_UNAVAILABLE'],['auth','AUTHENTICATION_FAILED'],['bad-resume','SESSION_RESUME_FAILED'],['sandbox','CODEX_VERSION_UNSUPPORTED']]){
    await assert.rejects(service().run(root,prompt,prompt==='bad-resume'?'existing':undefined,()=>{},5000).done,e=>e.code===code&&!e.message.includes('secret123'));
  }
  await service().run(root,'recovered',undefined,()=>{},5000).done;
});
test('missing CLI fails closed without a new session',async()=>{await assert.rejects(new api.CodexService('missing-atm-cli-12345').check(root),e=>e.code==='CODEX_NOT_FOUND');});
test('stop kills the process tree and retains partial response',async()=>{
  const dir=await fs.mkdtemp(path.join(os.tmpdir(),'atm-stop-'));const pidfile=path.join(dir,'child.pid');const events=[];
  const request=service().run(root,`slow:${pidfile}`,undefined,e=>events.push(e),5000);const failed=request.done.catch(e=>e);
  await waitFor(()=>fs.access(pidfile).then(()=>true,()=>false));const pid=Number(await fs.readFile(pidfile,'utf8'));await request.stop();assert.equal((await failed).code,'REQUEST_CANCELLED');assert(events.some(e=>e.type==='message'&&e.text==='已收到部分内容'));assert.throws(()=>process.kill(pid,0));
});
test('timeout kills process tree',async()=>{
  const dir=await fs.mkdtemp(path.join(os.tmpdir(),'atm-timeout-'));const pidfile=path.join(dir,'child.pid');await assert.rejects(service().run(root,`slow:${pidfile}`,undefined,()=>{},500).done,e=>e.code==='REQUEST_TIMEOUT');const pid=Number(await fs.readFile(pidfile,'utf8'));assert.throws(()=>process.kill(pid,0));
});
test('controller resumes same thread, restart restores history, workspaces are isolated, clear preserves foreign files',async()=>{
  const dir=await fs.mkdtemp(path.join(os.tmpdir(),'atm-history-'));
  const controller=new api.ChatController(new api.ConversationStorage(dir,root),service(),()=>{},async()=>{},()=>5000);await controller.initialize();await controller.send('CODEX_CONNECT_OK');await controller.send('previous string?');
  let state=controller.snapshot();assert.equal(state.conversation.threadId,'mock-thread-1');assert.equal(state.conversation.messages.at(-1).text,'CODEX_CONNECT_OK');assert.equal(state.conversation.requests.length,2);
  const resumed=new api.ChatController(new api.ConversationStorage(dir,root),service(),()=>{},async()=>{},()=>5000);await resumed.initialize();assert.equal(resumed.snapshot().conversation.messages.length,4);
  assert.equal(await new api.ConversationStorage(dir,root+'-other').load(),undefined);
  await resumed.newConversation();assert.equal(resumed.snapshot().history.length,2);await resumed.select(state.conversation.id);assert.equal(resumed.snapshot().conversation.threadId,'mock-thread-1');
  await fs.writeFile(path.join(dir,'codex-original-session'),'untouched');await resumed.clear();assert.equal(await fs.readFile(path.join(dir,'codex-original-session'),'utf8'),'untouched');assert.equal(resumed.snapshot().history.length,1);
});
test('resume failure retains ID and history, never silently starts a new conversation',async()=>{
  const dir=await fs.mkdtemp(path.join(os.tmpdir(),'atm-resume-'));const c=new api.ChatController(new api.ConversationStorage(dir,root),service(),()=>{},async()=>{},()=>5000);await c.initialize();await c.send('first');await c.send('bad-resume');const s=c.snapshot();assert.equal(s.conversation.threadId,'mock-thread-1');assert.equal(s.history.length,1);assert.match(s.error,/SESSION_RESUME_FAILED/);assert.equal(s.conversation.requests.length,2);
});
test('malformed JSONL produces a safe diagnostic, does not lose final response',async()=>{
  const e=[];await service().run(root,'malformed',undefined,x=>e.push(x),5000).done;assert(e.some(x=>x.type==='parseError'));assert(e.some(x=>x.type==='message'));
});
test('corrupt storage is reported without replacement',async()=>{
  const dir=await fs.mkdtemp(path.join(os.tmpdir(),'atm-corrupt-'));const storage=new api.ConversationStorage(dir,root);await storage.save({version:1,projectPath:root,activeId:'',conversations:[]});const files=await fs.readdir(path.join(dir,'conversations'));const file=path.join(dir,'conversations',files[0]);await fs.writeFile(file,'invalid');await assert.rejects(storage.load(),e=>e.code==='STORAGE_ERROR');assert.equal(await fs.readFile(file,'utf8'),'invalid');
});
test('stop during CLI preflight cancels before any model request',async()=>{
  let processStarted=false;
  const delayed=(command,args,cwd)=>{processStarted=true;return spawn(process.execPath,['-e','setInterval(()=>{},1000)'],{cwd,stdio:'pipe',windowsHide:true,detached:process.platform!=='win32'});};
  const s=new api.CodexService('mock',delayed);const abort=new AbortController();const checking=s.check(root,abort.signal);await waitFor(()=>processStarted);abort.abort();await assert.rejects(checking,e=>e.code==='REQUEST_CANCELLED');
});
test('interrupted persistent requests restore as CANCELLED without resending',async()=>{
  const dir=await fs.mkdtemp(path.join(os.tmpdir(),'atm-interrupted-'));const storage=new api.ConversationStorage(dir,root);await storage.save({version:1,projectPath:root,activeId:'one',conversations:[{id:'one',threadId:'known-thread',projectPath:root,createdAt:'time',updatedAt:'time',status:'RUNNING',messages:[{id:'m',requestId:'r',role:'assistant',text:'partial',time:'time',status:'RUNNING'}],requests:[{id:'r',time:'time',status:'RUNNING'}]}]});
  const c=new api.ChatController(storage,service(),()=>{},async()=>{},()=>5000);await c.initialize();const state=c.snapshot();assert.equal(state.conversation.status,'CANCELLED');assert.equal(state.conversation.threadId,'known-thread');assert.equal(state.conversation.messages[0].text,'partial');assert.equal(state.conversation.requests.length,1);
});
test('Codex discovery prefers a PATH executable over an installed extension',async()=>{
  const dir=await fs.mkdtemp(path.join(os.tmpdir(),'atm-discover-'));const executable=path.join(dir,process.platform==='win32'?'codex.cmd':'codex');await fs.writeFile(executable,'test');await fs.chmod(executable,0o755);
  const resolved=await api.resolveCodexExecutable('codex',{env:{PATH:dir,PATHEXT:'.EXE;.CMD'},extensionDirectory:()=>{throw new Error('Should not inspect extension when PATH is valid');}});
  assert.equal(resolved.command,executable);assert.equal(resolved.source,'path');
});
test('Codex discovery falls back to registered extension without a versioned hardcoded path',async()=>{
  const dir=await fs.mkdtemp(path.join(os.tmpdir(),'atm-extension-'));const relative=path.join('bin','windows-x86_64','codex.exe');
  const make=async name=>{const base=path.join(dir,name);await fs.mkdir(path.dirname(path.join(base,relative)),{recursive:true});await fs.writeFile(path.join(base,relative),'test');await fs.chmod(path.join(base,relative),0o755);return base;};
  let registered=await make('version-one');const options={env:{PATH:''},platform:'win32',arch:'x64',extensionDirectory:()=>registered};
  const first=await api.resolveCodexExecutable('codex',options);assert.equal(first.source,'extension');assert.equal(first.command,path.join(registered,relative));
  registered=await make('version-two');const second=await api.resolveCodexExecutable('codex',options);assert.equal(second.command,path.join(registered,relative));assert.notEqual(first.command,second.command);
});
test('Codex discovery preserves explicit settings even when missing',async()=>{
  const command=path.join(root,'missing-codex.exe');const result=await api.resolveCodexExecutable(command,{extensionDirectory:()=>{throw new Error('Must respect explicit settings');}});assert.equal(result.command,command);assert.equal(result.source,'settings');
});
test('Codex discovery fails clearly if PATH and registered extension are absent',async()=>{
  await assert.rejects(api.resolveCodexExecutable('codex',{env:{PATH:''},extensionDirectory:()=>undefined}),e=>e.code==='CODEX_NOT_FOUND');
});
