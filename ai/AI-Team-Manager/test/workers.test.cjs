const test=require('node:test'),assert=require('node:assert/strict'),fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');
const {ClaudeWorkerManager,ClaudeService}=require('../.tools/task02-test/api.cjs');
test('Claude stop and timeout settle even when the platform omits the child close event',async()=>{
 const {EventEmitter}=require('node:events'),child=()=>{const c=new EventEmitter();c.stdin={end(){},on(){}};return c;};
 const timeout=new ClaudeService('claude',child).run('.', 'test',undefined,()=>{},5);
 await assert.rejects(Promise.race([timeout.done,new Promise((_,reject)=>setTimeout(()=>reject(Error('hung')),500))]),e=>e.code==='REQUEST_TIMEOUT');
 const stop=new ClaudeService('claude',child).run('.', 'test',undefined,()=>{},10000);const done=assert.rejects(stop.done,e=>e.code==='REQUEST_CANCELLED');await stop.stop();await done;
});
const fixture=async()=>{const dir=await fs.mkdtemp(path.join(os.tmpdir(),'atm-workers-')),project=path.join(dir,'project');await fs.mkdir(project);const cmd=path.join(dir,'mock.cmd');await fs.writeFile(cmd,`@echo off\r\n"${process.execPath}" "${path.resolve('test/mock-claude.cjs')}" %*\r\n`);const options={project_id:'P',directory:path.join(dir,'history'),maxConcurrent:2,service:()=>new ClaudeService(process.platform==='win32'?cmd:process.execPath,process.platform==='win32'?undefined:((c,a,d)=>require('node:child_process').spawn(c,[path.resolve('test/mock-claude.cjs'),...a],{cwd:d,stdio:['pipe','pipe','pipe']}))),verify:async()=>{},timeout:()=>10000};return{dir,project,options};};
test('Worker controllers preserve independent sessions, histories and event ownership across backend reload',async()=>{
 const {project,options}=await fixture(),m=new ClaudeWorkerManager(options);await m.load();const a=await m.create('A',project,'fixture'),b=await m.create('B',project,'fixture');await Promise.all([m.run(a.worker_id,'hello'),m.run(b.worker_id,'hello')]);
 const first=m.detail(a.worker_id);assert.equal(first.worker.status,'COMPLETED');assert(first.worker.session_id);assert(first.worker.events.every(e=>e.worker_id===a.worker_id&&e.task_id==='A'&&e.project_id==='P'));
 await m.select(a.worker_id);assert.equal(m.detail(a.worker_id).worker.unread,0);assert(m.detail(b.worker_id).worker.unread>0);assert.throws(()=>m.detail('foreign'));await m.dispose();const restored=new ClaudeWorkerManager(options);await restored.load();assert.equal(restored.detail(a.worker_id).worker.session_id,first.worker.session_id);assert.equal(restored.detail(a.worker_id).chat.conversation.messages.length,first.chat.conversation.messages.length);await restored.run(a.worker_id,'hello');assert.equal(restored.detail(a.worker_id).worker.session_id,first.worker.session_id);await restored.dispose();
});
test('Worker queue respects concurrency and targeted pause/cancel leaves other runtimes independent',async()=>{
 const {project,options}=await fixture(),m=new ClaudeWorkerManager({...options,maxConcurrent:1});await m.load();
 const a=await m.create('A',project,'fixture'),b=await m.create('B',project,'fixture'),c=await m.create('C',project,'fixture');
 const first=m.run(a.worker_id,'HANG').catch(e=>e),second=m.run(b.worker_id,'hello').catch(e=>e),third=m.run(c.worker_id,'hello');
 for(let i=0;i<100&&!m.detail(a.worker_id).worker.session_id;i++)await new Promise(r=>setTimeout(r,20));assert(m.detail(a.worker_id).worker.session_id);assert.equal(m.detail(b.worker_id).worker.status,'QUEUED');assert.equal(m.detail(c.worker_id).worker.status,'QUEUED');
 await m.stop(b.worker_id);assert(await second instanceof Error);assert.equal(m.detail(b.worker_id).chat.conversation.requests.length,0);
 const session=m.detail(a.worker_id).worker.session_id;await m.stop(a.worker_id,true);assert(await first instanceof Error);assert.equal(m.detail(a.worker_id).worker.status,'PAUSED');await third;assert.equal(m.detail(c.worker_id).worker.status,'COMPLETED');
 await m.resume(a.worker_id,'resumed');assert.equal(m.detail(a.worker_id).worker.session_id,session);assert.equal(m.detail(b.worker_id).worker.status,'CANCELLED');await m.dispose();
});
