const {test}=require('node:test'),assert=require('node:assert/strict'),fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os'),{spawn}=require('node:child_process');
const {diagnose,resolveCodexExecutable,resolveClaudeExecutable}=require('../.tools/recovery/api.cjs');
const root=path.resolve(__dirname,'..');
const mock=(_cmd,args,cwd)=>spawn(process.execPath,[path.join(__dirname,'mock-cli.cjs'),...args],{cwd,stdio:'pipe',windowsHide:true,detached:process.platform!=='win32'});
test('diagnostics use both chat resolvers with empty PATH and registered extension binaries',async()=>{
 const dir=await fs.mkdtemp(path.join(os.tmpdir(),'atm-diagnostic-')),codex=path.join(dir,'codex-provider'),claude=path.join(dir,'claude-provider');
 for(const file of [path.join(codex,'bin','windows-x86_64','codex.exe'),path.join(claude,'resources','native-binary','claude.exe')]){await fs.mkdir(path.dirname(file),{recursive:true});await fs.writeFile(file,'fixture executable');await fs.chmod(file,0o755);}
 const seen=[];const results=await diagnose('fixture',5000,{codex:()=>resolveCodexExecutable('codex',{env:{PATH:''},platform:'win32',arch:'x64',extensionDirectory:()=>codex}),claude:()=>resolveClaudeExecutable('claude',{env:{PATH:''},platform:'win32',extensionDirectory:()=>claude}),launcher:(command,args,cwd)=>{seen.push({command,args,cwd});return mock(command,args,cwd);}});
 assert.equal(results.length,6);for(const name of ['Codex CLI','Claude Code CLI']){const d=results.find(r=>r.name===name);assert(d.ok,d.detail);assert.equal(d.source,'extension');assert(path.isAbsolute(d.command));}
 assert(seen.every(c=>c.args.length===1&&c.args[0]==='--version'));assert(seen.every(c=>c.cwd!==root));assert(seen.some(c=>c.command.includes('codex.exe')));assert(seen.some(c=>c.command.includes('claude.exe')));
});
test('missing explicit CLI path fails without fallback; raw failed stderr never enters diagnostics',async()=>{
 const absent=path.join(os.tmpdir(),'missing-atm-'+Date.now()+'.exe');const results=await diagnose('fixture',5000,{codex:()=>resolveCodexExecutable(absent,{extensionDirectory:()=>{throw Error('Explicit path must remain authoritative');}}),claude:async()=>({command:'fixture-claude',source:'settings'}),launcher:(command,args,cwd)=>command===absent?require('cross-spawn')(command,args,{cwd,windowsHide:true}):command==='fixture-claude'?spawn(process.execPath,['-e','process.stderr.write("PRIVATE_STDERR_DO_NOT_SHOW");process.exit(2)'],{cwd,stdio:'pipe',windowsHide:true}):mock(command,args,cwd)});
 const codex=results.find(r=>r.name==='Codex CLI'),claude=results.find(r=>r.name==='Claude Code CLI');assert(!codex.ok);assert.equal(codex.source,'settings');assert.equal(codex.command,absent);assert(!claude.ok);assert(!JSON.stringify(results).includes('PRIVATE_STDERR'));assert(claude.detail.includes('2'));
});
test('diagnostic timeout terminates the actual hung subprocess',async()=>{
 let pid;const result=await diagnose('fixture',500,{codex:async()=>({command:'hung-codex',source:'settings'}),claude:async()=>({command:'fixture-claude',source:'settings'}),launcher:(command,args,cwd)=>{if(command!=='hung-codex')return mock(command,args,cwd);const child=spawn(process.execPath,['-e','setInterval(()=>{},1000)'],{cwd,stdio:'pipe',windowsHide:true,detached:process.platform!=='win32'});pid=child.pid;return child;}});
 assert(!result.find(r=>r.name==='Codex CLI').ok);assert(pid);assert.throws(()=>process.kill(pid,0));
});
test('Windows configured cmd path with spaces executes safely without bare codex/claude PATH entries',async()=>{
 if(process.platform!=='win32')return;
 const dir=await fs.mkdtemp(path.join(os.tmpdir(),'atm diagnostic spaces ')),shim=path.join(dir,'version test.cmd');await fs.writeFile(shim,'@echo off\r\n"'+process.execPath+'" "'+path.join(__dirname,'mock-cli.cjs')+'" %*\r\n');
 const result=await diagnose('fixture',5000,{codex:async()=>({command:shim,source:'settings'}),claude:async()=>({command:shim,source:'settings'})});for(const name of ['Codex CLI','Claude Code CLI']){const d=result.find(r=>r.name===name);assert(d.ok,d.detail);assert.equal(d.command,shim);assert.equal(d.source,'settings');}
});
