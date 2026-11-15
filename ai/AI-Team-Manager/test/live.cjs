// Explicitly invoked acceptance tests only. Never run by npm test or extension activation.
const fs = require('node:fs/promises');
const path = require('node:path');
const {spawnSync} = require('node:child_process');
const {CodexService} = require('../.tools/task02-test/api.cjs');
const root=path.resolve(__dirname,'..');
const dir=path.join(root,'.tools','task02-live');
const git=()=>spawnSync('git',['status','--porcelain'],{cwd:root,encoding:'utf8'}).stdout;
(async()=>{
  await fs.mkdir(dir,{recursive:true});
  const before=git();const service=new CodexService();const version=await service.check(root);
  const previous=JSON.parse(await fs.readFile(path.join(dir,'state.json'),'utf8').catch(()=>'{}'));
  const mode=process.argv[2]||'connect';
  let thread=mode==='connect'?undefined:previous.thread;
  if(mode!=='connect'&&!thread)throw new Error('No saved thread ID; do not silently create a new session');
  const prompt=mode==='connect'?'请只回复 CODEX_CONNECT_OK，不要修改任何文件。':mode==='resume'?'请告诉我上一条消息中的验证字符串。':mode==='analyze'?'请只读分析当前插件目录结构，列出主要源码目录与各自职责，最多 250 字。不要修改任何文件，不执行 Git Commit 或 Push。':'请用中文详细讨论 VS Code 插件聊天系统的设计权衡，尽可能完整地分为二十节展开。只做分析，不修改任何文件。';
  const events=[];let timer;
  const request=service.run(root,prompt,thread,e=>{events.push(e);if(e.type==='thread')thread=e.id;console.log(e.type,e.type==='message'?e.text:e.type==='usage'?JSON.stringify(e.usage):e.type==='thread'?e.id:'');if(mode==='stop'&&e.type==='turn')timer ||= setTimeout(()=>void request.stop(),1200);},120000);
  let error;
  try{await request.done;}catch(e){error={code:e.code,message:e.message};}
  clearTimeout(timer);
  const after=git();
  const result={mode,version,thread,error,gitUnchanged:before===after,events};
  await fs.writeFile(path.join(dir,`${mode}.json`),JSON.stringify(result,null,2));
  if(thread)await fs.writeFile(path.join(dir,'state.json'),JSON.stringify({thread}));
  console.log(JSON.stringify({mode,thread,error,gitUnchanged:before===after}));
  if(error&&!(mode==='stop'&&error.code==='REQUEST_CANCELLED'))process.exitCode=1;
})().catch(e=>{console.error(e.message);process.exitCode=1;});
