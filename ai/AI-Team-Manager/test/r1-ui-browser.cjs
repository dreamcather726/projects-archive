// Synthetic scenarios in the actual production Webview. No model requests or user checkpoints.
const fs=require('node:fs/promises'),path=require('node:path'),assert=require('node:assert/strict'),{chromium}=require('playwright');
const {ids,scripts,fixture,selectDetail,verifyProductionAssets}=require('./r1-ui-fixture.cjs');
(async()=>{
 verifyProductionAssets();const out=path.resolve('test-results/r1-01-ui');await fs.mkdir(out,{recursive:true});
 const browser=await chromium.launch({headless:true,...(process.platform==='win32'?{channel:process.env.ATM_BROWSER_CHANNEL||'msedge'}:{})}),results=[],screenshots=[],errors=[],messages=[];
 let snapshot;
 try{
  const page=await browser.newPage();page.on('pageerror',e=>errors.push(e.message));
  await page.exposeFunction('hostMessage',async m=>{messages.push(m);if(m.type==='teamSelect'){selectDetail(snapshot,m.workerId,m);await publish();}});
  await page.setContent('<main id="workspace"></main><div id="fixture-label">UI test fixture · production Webview · no model requests</div>');
  await page.evaluate(()=>{let saved;window.acquireVsCodeApi=()=>({getState:()=>saved,setState:s=>saved=s,postMessage:m=>window.hostMessage(m)});});
  for(const css of ['src/webview/dashboard.css','src/webview/workspace/workspace.css'])await page.addStyleTag({path:css});
  await page.addStyleTag({content:'#fixture-label{position:fixed;bottom:0;left:4px;font:10px sans-serif;color:#aaa;z-index:1000;background:#161b27;padding:2px 4px;pointer-events:none}'});
  for(const script of scripts)await page.addScriptTag({path:'src/webview/'+script});
  async function publish(){await page.evaluate(s=>dispatchEvent(new MessageEvent('message',{data:s})),snapshot);}
  const by=name=>page.getByLabel(name,{exact:true});
  async function route(name){await by(name).click();}
  async function noOverflow(name){assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),name+' document overflows');const overlap=await page.evaluate(()=>{const nodes=['.navigation','.workspace-main'].map(s=>document.querySelector(s)).filter(Boolean).map(n=>n.getBoundingClientRect());return nodes.length===2&&nodes[0].right>nodes[1].left+1;});assert(!overlap,name+' navigation overlaps main');}
  async function shot(name){await page.screenshot({path:path.join(out,name),fullPage:true});screenshots.push(name);}
  for(const [width,height] of [[1920,1080],[1600,900],[1366,768],[1280,720],[760,900]]){
   await page.setViewportSize({width,height});snapshot=fixture();selectDetail(snapshot,ids[0]);await publish();
   await route('AI 协同开发');await page.getByRole('tab',{name:'GPT 总指挥',exact:true}).click();await noOverflow('GPT '+width);
   assert(await page.locator('.gpt-panel').isVisible());assert(await page.locator('.navigation').isVisible());
   const toggle=by('展开 / 折叠任务面板');
   await toggle.click();const folded=await toggle.getAttribute('aria-pressed');await toggle.click();assert.notEqual(await toggle.getAttribute('aria-pressed'),folded);
   if(width===1920)await shot('gpt-main.png');
   await page.locator('[role="tab"][data-session-id="'+ids[0]+'"]').click();await page.locator('.worker-retry .retry-status').waitFor();await noOverflow('Worker '+width);
   assert.equal(await page.locator('.worker-subtabs [role="tab"]').count(),5);assert(await by('暂停当前 Worker').isEnabled());assert(await by('恢复当前 Worker').isDisabled());
   const before=messages.length;await by('暂停当前 Worker').click();assert(messages.slice(before).some(m=>m.type==='teamControl'&&m.action==='pause'));assert.equal(snapshot.dispatch.team.workflow.tasks[0].status,'RUNNING');
   if(width===1920)await shot('claude-worker-retry.png');if(width===760)await shot('narrow-worker.png');
   await route('任务调度中心');await noOverflow('Task center '+width);assert.equal(await page.locator('.task-table tbody tr').count(),2);
   assert.equal(await page.locator('.task-dependencies').getAttribute('open'),null);await page.locator('.task-dependencies > summary').click();assert(await page.locator('.dependency-node').first().isVisible());await page.locator('.task-dependencies > summary').click();
   assert.equal(await page.locator('.task-event-log').getAttribute('open'),null);await page.locator('.task-event-log > summary').click();assert.equal(await page.locator('.task-event').count(),2);await page.locator('.task-event-log > summary').click();
   await by('查看任务 task-0').click();await by('任务详情').waitFor();assert(await by('关闭任务详情').isVisible());await page.keyboard.press('Escape');assert(await by('任务详情').isHidden());
   if(width===1920)await shot('task-center.png');if(width===760)await shot('narrow-task-center.png');results.push({width,height,GPT:true,worker:true,task_center:true,no_document_overflow:true,drawer:true,folds:true});
  }
  await page.setViewportSize({width:1600,height:900});await route('AI 协同开发');
  const statuses=[];
  for(const kind of ['RUNNING','RETRY_SCHEDULED','RETRY_STARTED','RETRY_SUCCEEDED','RETRY_EXHAUSTED','RETRY_CANCELLED','RETRY_BLOCKED_SIDE_EFFECT']){
   snapshot=fixture(kind);selectDetail(snapshot,ids[0]);await publish();await page.locator('[role="tab"][data-session-id="'+ids[0]+'"]').click();
   const bar=page.locator('.worker-retry .retry-status');
   if(kind==='RUNNING'){assert.equal(await bar.count(),0);assert((await page.locator('.worker-retry').innerText()).includes('执行中'));}
   else{assert.equal(await bar.getAttribute('data-retry-kind'),kind);assert.equal(await bar.getAttribute('open'),null);assert((await bar.innerText()).includes('Claude'));await bar.locator('summary').click();assert(await bar.getByRole('button',{name:'复制重试诊断信息'}).isVisible());if(kind==='RETRY_SCHEDULED')await shot('retry-details.png');}
   statuses.push(kind);
  }
  snapshot=fixture();snapshot.dispatch.team.workflow.workers[0].retry.nextRetryAt=new Date(Date.now()+4000).toISOString();selectDetail(snapshot,ids[0]);await publish();
  const countdown=page.locator('.worker-retry .retry-countdown');const first=await countdown.innerText();await page.waitForTimeout(1300);assert.notEqual(await countdown.innerText(),first);assert.equal(snapshot.dispatch.team.workflow.workers[0].status,'RETRYING');
  const before=messages.length;await page.locator('[role="tab"][data-session-id="'+ids[1]+'"]').click();await page.getByText('ONLY_WORKER_1',{exact:false}).waitFor();assert(!(await page.locator('.worker-body').innerText()).includes('ONLY_WORKER_0'));
  await by('关闭 Claude '+ids[1].slice(0,8)+' 标签（后台任务继续）').click();await page.locator('.session-reopen > summary').click();await by('重新打开 Claude '+ids[1].slice(0,8)).click();assert(messages.slice(before).every(m=>['layout','teamSelect'].includes(m.type)));
  assert.equal(errors.length,0,errors.join('\n'));await fs.writeFile(path.join(out,'result.json'),JSON.stringify({passed:true,synthetic_UI_fixture:true,production_resources:true,no_model_requests:true,viewports:results,statuses,worker_isolation:true,close_reopen_no_cancel:true,countdown_local_only:true,page_errors:errors,screenshots},null,2));console.log('R1 browser PASS '+out);
 }catch(e){await fs.writeFile(path.join(out,'failure.json'),JSON.stringify({error:e.stack,errors},null,2));throw e;}finally{await browser.close();}
})().catch(e=>{console.error(e.stack);process.exitCode=1;});
