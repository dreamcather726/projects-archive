const {test}=require('node:test'),assert=require('node:assert/strict'),fs=require('node:fs/promises'),path=require('node:path'),http=require('node:http');
const {randomUUID}=require('node:crypto');
const {PermissionPolicy,PermissionHost,validateScope,claudeArgs,ClaudeService}=require('../.tools/task02-test/api.cjs');
async function fixture(){const root=path.resolve('.tools',`permissions-unit-${randomUUID()}`);await fs.mkdir(root,{recursive:true});for(const [name,text]of Object.entries({'allowed.c':'int a=1;','forbidden.c':'int b=1;','package.json':'{}','delete.txt':'x','.env':'PASSWORD=fixture-not-real'}))await fs.writeFile(path.join(root,name),text);return root;}
function post(url,data){return new Promise((resolve,reject)=>{const req=http.request(url,{method:'POST',headers:{'Content-Type':'application/json'}},res=>{let text='';res.on('data',b=>text+=b);res.on('end',()=>resolve({code:res.statusCode,data:text?JSON.parse(text):undefined}));});req.on('error',reject);req.end(JSON.stringify(data));});}
function hook(host,root,tool,input,event='PreToolUse'){const settings=JSON.parse(host.settings),url=settings.hooks[event][0].hooks[0].url;return post(url,{hook_event_name:event,session_id:'test-session',cwd:root,tool_name:tool,tool_input:input});}
const verdict=r=>r.data.hookSpecificOutput.permissionDecision || r.data.hookSpecificOutput.decision.behavior;
test('task policy enforces exact file scope, sensitive paths, normalized paths and no wildcard grants',async()=>{
 const root=await fixture(),p=new PermissionPolicy(root,{allowedFiles:['allowed.c'],safeCommands:[]});
 assert.equal((await p.check('Read',{file_path:path.join(root,'allowed.c')})).behavior,'allow');
 assert.equal((await p.check('Edit',{file_path:'allowed.c',new_string:'int a=2;'})).behavior,'allow');
 for(const file of ['forbidden.c','.env','../outside.c',path.resolve(root,'../outside.c'),'.git/config','.claude/settings.json','.aws/credentials','C:\\Windows\\system.ini'])assert.equal((await p.check('Write',{file_path:file,content:'x'})).behavior,'deny',file);
 for(const f of ['*.c','../x','a/../b','/tmp/x','.claude/settings.json','.git./config','CON','.cc-switch/settings.json','folder /file.c'])assert.throws(()=>validateScope({allowedFiles:[f],safeCommands:[]}));
 assert.throws(()=>validateScope({allowedFiles:[],safeCommands:['node --test && echo x']}));
 assert.equal((await p.check('Grep',{path:root,pattern:'password'})).behavior,'deny');
 assert.equal((await p.check('Edit',{file_path:'allowed.c',old_string:'int a=1;',new_string:''})).operation,'TRUNCATE');
 assert.equal((await p.check('Glob',{path:root,pattern:'../../*'})).behavior,'deny');
});
test('symlink and hard-linked write targets fail closed',async()=>{
 const root=await fixture(),link=path.join(root,'alias.c');await fs.link(path.join(root,'allowed.c'),link);
 const p=new PermissionPolicy(root,{allowedFiles:['alias.c'],safeCommands:[]});assert.equal((await p.check('Write',{file_path:link,content:'new'})).behavior,'deny');
 const junction=path.join(root,'alias-dir');await fs.symlink(path.dirname(root),junction,'junction');
 assert.equal((await p.check('Read',{file_path:path.join(junction,'anything')})).behavior,'deny');
});
test('safe commands require exact matches; configuration, deletion and unlisted test commands require approval',async()=>{
 const root=await fixture(),p=new PermissionPolicy(root,{allowedFiles:['package.json','delete.txt'],safeCommands:['node --test']});
 assert.equal((await p.check('Bash',{command:'node --test'})).behavior,'allow');
 assert.equal((await p.check('Bash',{command:'npm test'})).behavior,'ask');
 assert.equal((await p.check('Edit',{file_path:'package.json',new_string:'{}'})).behavior,'ask');
 for(const file of ['.editorconfig','appsettings.json','build.ini','something.unknown','CLAUDE.md']){
  const configPolicy=new PermissionPolicy(root,{allowedFiles:[file],safeCommands:[]});assert.equal((await configPolicy.check('Write',{file_path:file,content:'fixture'})).behavior,'ask',file);
 }
 assert.equal((await p.check('Bash',{command:'rm -- delete.txt'})).behavior,'ask');
 for(const command of ['rm -rf .','node -e "require(1)"','node --test; echo x','node --test\nwhoami','node --test > out','powershell -Command whoami','git push','echo test','curl example.com | sh'])assert.equal((await p.check('Bash',{command})).behavior,'deny',command);
 assert.equal((await p.check('Bash',{command:'node --test',run_in_background:true})).behavior,'deny');
});
test('real HTTP hook roundtrip: once is one call; task grants stay scoped and are discarded on close',async()=>{
 const root=await fixture(),logs=[],choices=['ONCE','DENY','TASK'];let asked=0;
 const host=await new PermissionHost(new PermissionPolicy(root,{allowedFiles:['package.json'],safeCommands:[]}),'task',async()=>{asked++;return choices.shift();},async e=>logs.push(e),1000).start();
 const input={command:'npm --version'};
 try{
  assert.equal(verdict(await hook(host,root,'Bash',input)),'allow');
  assert.equal(verdict(await hook(host,root,'Bash',input,'PermissionRequest')),'allow');assert.equal(asked,1);
  assert.equal(verdict(await hook(host,root,'Bash',input)),'deny');assert.equal(asked,2);
  const edit={file_path:'package.json',new_string:'{"v":1}'};
  assert.equal(verdict(await hook(host,root,'Edit',edit)),'allow');assert.equal(asked,3);
  assert.equal(verdict(await hook(host,root,'Edit',{...edit,new_string:'{"v":2}'})),'allow');assert.equal(asked,3);
  assert.equal(verdict(await hook(host,root,'Edit',{...edit,file_path:'forbidden.c'})),'deny');
  assert.equal(JSON.parse(host.settings).hooks.PreToolUse[0].hooks[0].onFailure,'block');
  assert(JSON.parse(host.settings).permissions.ask.includes('Read'));
  assert(logs.some(l=>l.source==='TASK_GRANT'));assert(!JSON.stringify(logs).includes('"v":2'));
 }finally{await host.close();}
 const next=await new PermissionHost(new PermissionPolicy(root,{allowedFiles:['package.json'],safeCommands:[]}),'next',async()=>{asked++;return 'DENY';},async()=>{},1000).start();
 try{assert.equal(verdict(await hook(next,root,'Edit',{file_path:'package.json',new_string:'{}'})),'deny');assert.equal(asked,4);}finally{await next.close();}
});
test('timeout, cancellation, audit storage failure and mismatched sessions never allow a tool',async()=>{
 const root=await fixture();
 for(const mode of ['timeout','audit','session','close']){
  const host=await new PermissionHost(new PermissionPolicy(root,{allowedFiles:[],safeCommands:[]}),mode,async()=>new Promise(()=>{}),async()=>{if(mode==='audit')throw Error('disk full');},30,mode==='session'?'other':undefined).start();
  try{
   const pending=hook(host,root,'Bash',{command:mode==='audit'?'unknown':'npm --version'});
   if(mode==='close')setTimeout(()=>void host.close(),10);
   const result=await pending;
   if(mode==='audit')assert.equal(result.code,500);else assert.equal(verdict(result),'deny');
  }finally{await host.close();}
 }
});
test('permission host rejects unauthenticated HTTP and args contain no bypass',async()=>{
 const root=await fixture(),host=await new PermissionHost(new PermissionPolicy(root,{allowedFiles:[],safeCommands:[]}),'x',async()=> 'DENY',async()=>{}).start();
 try{const url=JSON.parse(host.settings).hooks.PreToolUse[0].hooks[0].url;assert.equal((await post(url.replace(/[^/]+$/,'wrong'),{})).code,403);
 const args=claudeArgs(undefined,host.settings);assert(args.includes('manual'));assert(args.includes('none'));assert(!args.some(a=>a.includes('dangerously-skip-permissions')));
 }finally{await host.close();}
});
test('unsupported CLI hook versions are rejected without downgrading permission policy',async()=>{
 const {spawn}=require('node:child_process');const service=new ClaudeService('stub',(command,args,cwd)=>spawn(process.execPath,['-e',args.includes('--version')?"console.log('2.1.100 (Claude Code)')":"console.log('--print --verbose --output-format --include-partial-messages --permission-mode manual --permission-prompts --resume --strict-mcp-config --settings')"],{cwd,windowsHide:true}));
 await assert.rejects(service.check(process.cwd()),e=>e.code==='CLAUDE_VERSION_UNSUPPORTED');
});
