// Real GPT and Claude; deliberately separate from npm test. Runs twice to verify durable history.
const path=require('node:path'),{spawn}=require('node:child_process');
(async()=>{
  const root=path.resolve(__dirname,'..'),project=path.join(root,'.tools',`task05-host-fixture-${Date.now()}`);
  await require('./repair-fixture.cjs').create(project);
  const code=await new Promise((resolve,reject)=>{
    const child=spawn(process.execPath,[path.join(root,'test','run-host.cjs')],{windowsHide:true,stdio:'inherit',env:{...process.env,ATM_HOST_CODEX_REAL:'1',ATM_HOST_LABEL:'task05-host',ATM_HOST_TIMEOUT_MS:'900000',ATM_HOST_WORKSPACE:project,ATM_HOST_TEST_MODULE:path.join(root,'test','task05-host-live.cjs')}});
    child.on('error',reject);child.on('close',resolve);
  });if(code!==0)throw Error('Task05 native acceptance failed');console.log('Repair host fixture:',project);
})().catch(e=>{console.error(e);process.exitCode=1;});
