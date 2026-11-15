// Dedicated VS Code profile; GPT uses a Mock, while an optional test module can use real Claude.
// Does not change the user's VS Code settings.
const fs = require('node:fs/promises');
const path = require('node:path');
const {spawn,execFileSync} = require('node:child_process');
const root=path.resolve(__dirname,'..');
const wait=ms=>new Promise(resolve=>setTimeout(resolve,ms));
(async()=>{
  if(process.platform!=='win32')throw new Error('This local host runner targets Windows; use your VS Code extension test runner on other platforms.');
  const command=execFileSync('where.exe',['code.cmd'],{encoding:'utf8'}).trim().split(/\r?\n/)[0];
  const executable=process.argv[2]||path.join(path.dirname(command),'..','Code.exe');
  const testModule=process.env.ATM_HOST_TEST_MODULE||path.join(root,'test','task02-host.cjs'),workspace=process.env.ATM_HOST_WORKSPACE||root;
  const dir=path.join(root,'.tools',`${process.env.ATM_HOST_LABEL || (process.env.ATM_HOST_CODEX_REAL?'task04-host':process.env.ATM_HOST_TEST_MODULE?'task03-host':'task02-host')}-${Date.now()}`);
  await fs.mkdir(path.join(dir,'profile','User'),{recursive:true});
  await fs.writeFile(path.join(dir,'mock.cmd'),`@echo off\r\n"${process.execPath}" "${path.join(root,'test','mock-cli.cjs')}" %*\r\n`);
  await fs.writeFile(path.join(dir,'profile','User','settings.json'),JSON.stringify({'aiTeamManager.codexExecutable':process.env.ATM_HOST_CODEX_REAL?'codex':path.join(dir,'mock.cmd'),'workbench.startupEditor':'none','extensions.autoUpdate':false}));
  // --extensionTestsPath uses in-memory VS Code storage. A local probe extension in a
  // normal Development Host lets this test verify actual workspaceState across restarts.
  const probe=path.join(dir,'probe');await fs.mkdir(probe,{recursive:true});
  await fs.writeFile(path.join(probe,'package.json'),JSON.stringify({name:'atm-local-host-probe',publisher:'test',version:'1.0.0',engines:{vscode:'^1.95.0'},main:'index.cjs',activationEvents:['onStartupFinished'],extensionDependencies:['ai-team-manager.ai-team-manager']}));
  await fs.writeFile(path.join(probe,'index.cjs'),`const vscode=require('vscode'),fs=require('node:fs/promises'),path=require('node:path');exports.activate=()=>{setTimeout(async()=>{try{await require(${JSON.stringify(path.resolve(testModule))}).run();await fs.writeFile(path.join(process.env.ATM_HOST_TEST_DIR,'probe-complete.json'),JSON.stringify({ok:true}));}catch(error){console.error(error);await fs.writeFile(path.join(process.env.ATM_HOST_TEST_DIR,'probe-complete.json'),JSON.stringify({ok:false,error:String(error),stack:error.stack}));}finally{setTimeout(()=>vscode.commands.executeCommand('workbench.action.quit'),500);}},1000);};`);
  const env={...process.env,ATM_HOST_TEST_DIR:dir};delete env.ELECTRON_RUN_AS_NODE;
  const vsix=process.env.ATM_HOST_VSIX;
  if(vsix){const installed=require('cross-spawn').sync(command,[`--user-data-dir=${path.join(dir,'profile')}`,`--extensions-dir=${path.join(dir,'extensions')}`,'--install-extension',path.resolve(vsix)],{env,encoding:'utf8',windowsHide:true});if(installed.status!==0)throw new Error(installed.stderr||installed.stdout||'VSIX installation failed');console.log(installed.stdout.trim());}
  for(const stage of [1,2]){
    await fs.rm(path.join(dir,'probe-complete.json'),{force:true});
    await new Promise((resolve,reject)=>{
      const child=spawn(executable,[`--user-data-dir=${path.join(dir,'profile')}`,`--extensions-dir=${path.join(dir,'extensions')}`,...(vsix?[]:[`--extensionDevelopmentPath=${root}`]),`--extensionDevelopmentPath=${probe}`,'--skip-welcome','--skip-release-notes','--disable-workspace-trust',workspace],{env,windowsHide:true,stdio:['ignore','pipe','pipe']});
      let output='';child.stdout.on('data',b=>output+=b);child.stderr.on('data',b=>output+=b);
      const timer=setTimeout(()=>{child.kill();reject(new Error('Host test timeout; inspect dedicated test profile.'));},Number(process.env.ATM_HOST_TIMEOUT_MS)||(process.env.ATM_HOST_TEST_MODULE?240000:60000));
      child.on('error',reject);child.on('close',async()=>{clearTimeout(timer);await fs.writeFile(path.join(dir,`stage${stage}.log`),output);resolve();});
    });
    for(let i=0;i<100;i++){if(await fs.access(path.join(dir,`stage${stage}.json`)).then(()=>true,()=>false))break;await wait(100);}
    const completion=JSON.parse(await fs.readFile(path.join(dir,'probe-complete.json'),'utf8'));if(!completion.ok)throw new Error(JSON.stringify(completion));
    console.log(await fs.readFile(path.join(dir,`stage${stage}.json`),'utf8'));
  }
  console.log('Host tests passed:',dir);
})().catch(error=>{console.error(error.message);process.exitCode=1;});
