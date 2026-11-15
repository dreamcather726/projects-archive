const test=require('node:test'),assert=require('node:assert/strict'),fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');
const {FileLockManager,FileOwnershipManager,WorkerWorkspaceManager,ScopedWorkerPolicy,IsolatedTestRunner,claudeArgs,git}=require('../.tools/task02-test/api.cjs');
const fixture=async()=>{const dir=await fs.mkdtemp(path.join(os.tmpdir(),'atm-isolation-')),root=path.join(dir,'project');await fs.mkdir(root);for(const f of ['motor_control.c','protocol.c'])await fs.writeFile(path.join(root,f),'int value = 1;\n');await git(root,['init','--quiet']);await git(root,['add','motor_control.c','protocol.c']);await git(root,['-c','user.name=Fixture','-c','user.email=fixture@example.invalid','commit','--quiet','-m','fixture']);return {dir,root,base:(await git(root,['rev-parse','HEAD'])).trim()};};
test('atomic logical file locks reject concurrent ownership and directory/case overlap; live writer cannot be released',async()=>{
 const {dir}=await fixture(),a=new FileLockManager(path.join(dir,'locks'),'p'),b=new FileLockManager(path.join(dir,'locks'),'p');
 const outcomes=await Promise.allSettled([a.acquire('a','w1',['src/motor.c']),b.acquire('b','w2',['src/motor.c'])]);assert.equal(outcomes.filter(o=>o.status==='fulfilled').length,1);
 const lease=outcomes.find(o=>o.status==='fulfilled').value;await assert.rejects(b.acquire('c','w3',['src']),/WAITING_FILE_LOCK/);if(process.platform==='win32')await assert.rejects(b.acquire('c','w3',['SRC/MOTOR.C']),/WAITING_FILE_LOCK/);
 await a.attachProcess(lease.id,process.pid);await assert.rejects(a.release(lease.id),/尚未确认退出/);assert.equal((await a.list()).length,1);
 await assert.rejects(a.assertHeld(lease.id,'other','w1','src/motor.c'));assert.equal((await fs.readFile(path.join(dir,'locks',require('node:crypto').createHash('sha256').update('p').digest('hex'),'events.jsonl'),'utf8')).includes('LOCK_RELEASE_REFUSED'),true);
 const own=await b.acquire('q','wq',['other.c']);await b.release(own.id);assert.equal((await b.list()).length,1);
});
test('real Worktree uses an independent branch and raw Git blobs without executing checkout filters',async()=>{
 const {dir,root,base}=await fixture(),manager=new WorkerWorkspaceManager(root,path.join(dir,'workers'));
 const worker=await manager.create(base,'task');await manager.verify(worker);assert.notEqual(worker.workspace_path,root);assert(worker.branch_name.startsWith('atm/task07-'));assert.deepEqual(worker.omitted_files,[]);
 assert.equal(await fs.readFile(path.join(worker.workspace_path,'motor_control.c'),'utf8'),'int value = 1;\n');await fs.writeFile(path.join(worker.workspace_path,'motor_control.c'),'int value = 2;\n');assert.equal(await fs.readFile(path.join(root,'motor_control.c'),'utf8'),'int value = 1;\n');assert.match(await git(worker.workspace_path,['diff','--no-ext-diff','--no-textconv']),/value = 2/);
 await assert.rejects(new WorkerWorkspaceManager(root,path.join(root,'unsafe')).create(base,'task'),/之外/);
 await fs.writeFile(path.join(root,'.gitattributes'),'*.c filter=fixture\n');await git(root,['add','.gitattributes']);await git(root,['-c','user.name=Fixture','-c','user.email=fixture@example.invalid','commit','--quiet','-m','attributes']);await git(root,['config','filter.fixture.smudge','exit 91']);await git(root,['config','filter.fixture.required','true']);
 const raw=await manager.create((await git(root,['rev-parse','HEAD'])).trim(),'raw');assert.equal(await fs.readFile(path.join(raw.workspace_path,'motor_control.c'),'utf8'),'int value = 1;\n');
});
test('scoped gateway checks actual paths, task ownership and lock; no shell, alias or unknown tool escape',async()=>{
 const {dir,root}=await fixture(),locks=new FileLockManager(path.join(dir,'locks'),'p'),lease=await locks.acquire('t','w',['motor_control.c']);
 const ownership=new FileOwnershipManager({tasks:[{task_id:'t',allowed_files:['motor_control.c'],read_only_files:['protocol.c'],forbidden_files:[],test_commands:[]}],shared_file_owners:[]});
 const policy=new ScopedWorkerPolicy(root,ownership,locks,'t','w',lease.id,async()=>{});
 assert.equal((await policy.check('Write',{file_path:'motor_control.c',content:'int n=2;'})).behavior,'allow');assert.equal((await policy.check('Write',{file_path:'protocol.c',content:'bad'})).behavior,'deny');
 for(const tool of ['Bash','PowerShell','NotebookEdit','Agent','WebFetch'])assert.equal((await policy.check(tool,{command:'python -B test.py'})).behavior,'deny');
 assert.equal((await policy.check('Read',{file_path:'../elsewhere.c'})).behavior,'deny');await fs.link(path.join(root,'protocol.c'),path.join(root,'alias.c'));assert.equal((await policy.check('Write',{file_path:'alias.c',content:'bad'})).behavior,'deny');
 await locks.release(lease.id);assert.equal((await policy.check('Write',{file_path:'motor_control.c',content:'int n=2;'})).behavior,'deny');
 const args=claudeArgs(undefined,'{}',{restricted:true});assert(args.includes('--restricted'));assert(!args[args.indexOf('--tools')+1].includes('Bash'));assert(!args.some(a=>a==='--dangerously-skip-permissions'));
 for(const cmd of ['python -B ../test.py','python -B test.py; rm x','node test.js','git reset --hard'])assert.throws(()=>IsolatedTestRunner.argv(cmd));
});
test('a crashed writer retains its durable lease; release needs confirmed process exit, and corrupt records fail closed',async()=>{
 const {dir}=await fixture(),directory=path.join(dir,'locks'),api=path.resolve('.tools/task02-test/api.cjs');
 const {spawn}=require('node:child_process');
 const script=`const {FileLockManager}=require(process.argv[1]);(async()=>{const m=new FileLockManager(process.argv[2],'p');const l=await m.acquire('crash','writer',['motor_control.c']);await m.attachProcess(l.id,process.pid);console.log(JSON.stringify(l));process.exit(23)})()`;
 const child=spawn(process.execPath,['-e',script,api,directory],{windowsHide:true,stdio:['ignore','pipe','pipe']});let out='';child.stdout.on('data',b=>out+=b);const code=await new Promise((resolve,reject)=>{child.on('error',reject);child.on('close',resolve)});assert.equal(code,23);
 const lease=JSON.parse(out),manager=new FileLockManager(directory,'p');assert.equal((await manager.list()).length,1);await assert.rejects(manager.acquire('other','other',['motor_control.c']),/WAITING_FILE_LOCK/);await manager.release(lease.id);assert.equal((await manager.list()).length,0);
 const file=path.join(directory,require('node:crypto').createHash('sha256').update('p').digest('hex'),'leases.json');await fs.writeFile(file,'[{"project_id":"p","process_ids":[-1],"files":[]}]');await assert.rejects(manager.list(),/记录损坏/);assert.match(await fs.readFile(file,'utf8'),/-1/);
});
test('unsupported restricted Claude does not silently launch without the restriction',async()=>{
 const {ClaudeService}=require('../.tools/task02-test/api.cjs');let launches=0;
 const service=new ClaudeService('claude',()=>{launches++;throw Error('must not launch')});const req=service.run(process.cwd(),'no tool',undefined,()=>{},1000,undefined,{restricted:true});await assert.rejects(req.done,e=>e.code==='CLAUDE_VERSION_UNSUPPORTED');assert.equal(launches,0);
});
test('bounded MCP gateway performs real pre-write checks without native tools or hooks',async()=>{
 const {WorkerExecutionSession}=require('../.tools/task02-test/api.cjs'),{dir,root}=await fixture(),locks=new FileLockManager(path.join(dir,'locks'),'p'),lease=await locks.acquire('t','w',['motor_control.c']);
 const ownership=new FileOwnershipManager({tasks:[{task_id:'t',allowed_files:['motor_control.c'],read_only_files:['protocol.c'],forbidden_files:[],test_commands:[]}],shared_file_owners:[]}),logs=[];
 const session=await new WorkerExecutionSession(root,'t','w',lease.id,ownership,locks,new IsolatedTestRunner(path.join(dir,'tests')),async()=> 'DENY',async e=>logs.push(e)).start();
 await session.runOptions.onProcess(process.pid);session.bindSession('11111111-1111-4111-8111-111111111111');
 const config=JSON.parse(session.runOptions.mcpConfig).mcpServers.atm_worker;
 const call=async(name,args)=>{const response=await fetch(config.url,{method:'POST',headers:{...config.headers,'Content-Type':'application/json'},body:JSON.stringify({jsonrpc:'2.0',id:1,method:'tools/call',params:{name,arguments:args}})});return response.json();};
 try {
  const write=await call('write_task_file',{file_path:'motor_control.c',content:'int value = 7;\n'});assert(!write.error,JSON.stringify(write));assert.equal(await fs.readFile(path.join(root,'motor_control.c'),'utf8'),'int value = 7;\n');
  assert((await call('write_task_file',{file_path:'protocol.c',content:'forbidden'})).error);assert.equal(await fs.readFile(path.join(root,'protocol.c'),'utf8'),'int value = 1;\n');
  assert((await call('write_task_file',{file_path:'../escape.c',content:'forbidden'})).error);assert((await call('run_authorized_test',{command:'git reset --hard'})).error);
  assert(logs.some(e=>e.decision==='DENY'));assert(logs.some(e=>e.kind==='FILE_WRITE_RESULT'));assert(logs.filter(e=>e.decision==='ALLOW').every(e=>e.source==='LOCAL_POLICY'));
  const args=claudeArgs(undefined,undefined,session.runOptions);assert.equal(args[args.indexOf('--tools')+1],'');assert(args.includes('--bare'));assert(!args.includes('--restricted'));assert(!args.includes('--dangerously-skip-permissions'));
 }finally{await session.close();}
 await assert.rejects(call('write_task_file',{file_path:'motor_control.c',content:'late'}));assert.equal(await fs.readFile(path.join(root,'motor_control.c'),'utf8'),'int value = 7;\n');
});
