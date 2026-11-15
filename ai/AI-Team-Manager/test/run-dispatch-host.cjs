// Native VS Code, real GPT MCP calls + real Claude + actual Python tests + real independent review.
const path=require('node:path'),fs=require('node:fs/promises'),{spawn,spawnSync}=require('node:child_process');
(async()=>{
 const root=path.resolve(__dirname,'..'),project=path.join(root,'.tools',`task06-host-fixture-${Date.now()}`);await fs.mkdir(project,{recursive:true});
 await fs.writeFile(path.join(project,'README.md'),'Isolated Task06 addition acceptance fixture. Only test.py may be modified.\n');
 await fs.writeFile(path.join(project,'protocol.c'),'int protocol_version(void) { return 1; }\n');
 for(const args of [['init','--quiet'],['add','README.md','protocol.c'],['-c','user.name=Test Fixture','-c','user.email=fixture@example.invalid','commit','--quiet','-m','fixture baseline']]){const r=spawnSync('git',args,{cwd:project,windowsHide:true});if(r.status!==0)throw Error(r.stderr.toString());}
 const code=await new Promise((resolve,reject)=>{const c=spawn(process.execPath,[path.join(root,'test/run-host.cjs')],{windowsHide:true,stdio:'inherit',env:{...process.env,ATM_HOST_CODEX_REAL:'1',ATM_HOST_LABEL:'task06-host',ATM_HOST_TIMEOUT_MS:'1800000',ATM_HOST_WORKSPACE:project,ATM_HOST_TEST_MODULE:path.join(root,'test/task06-host-live.cjs')}});c.on('error',reject);c.on('close',resolve);});if(code!==0)throw Error('Task06 native acceptance failed');console.log('Task06 fixture:',project);
})().catch(e=>{console.error(e);process.exitCode=1;});
