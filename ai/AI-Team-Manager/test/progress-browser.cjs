// Synthetic test transport with the actual backend progress projection and production UI assets.
const fs=require('node:fs/promises'),path=require('node:path'),assert=require('node:assert/strict'),{chromium}=require('playwright');
const ui=require('./r1-ui-fixture.cjs'),backend=require('./progress-fixture.cjs');
function scenario(kind='partial'){
 const s=ui.fixture('RUNNING'),f=backend.fixture(2);f.plan={...f.plan,...s.dispatch.team.workflow.plan,validation:'VALIDATED_NOT_EXECUTED',tasks:f.plan.tasks};backend.accept(f,0);
 f.tasks.forEach((row,i)=>row.task.task_name=s.dispatch.team.workflow.tasks[i].task.task_name);
 f.workers=f.workers.map((w,i)=>({...w,worker_id:ui.ids[i]}));f.candidates[0].worker_id=ui.ids[0];
 if(kind==='ready'||kind==='delivered'){backend.accept(f,1);backend.integrate(f,kind==='delivered');}
 if(kind==='blocked'){f.tasks[0].status='WAITING_REVIEW';f.tasks[0].reason='缺少接口证明';f.reviews['task-0'][0].verdict='BLOCKED';}
 if(kind==='paused')f.tasks[0].status='PAUSED';if(kind==='cancelled')f.tasks[0].status='CANCELLED';
 if(kind==='retry'){f.workers[1].status='RETRYING';f.workers[1].retry=ui.retry();}
 if(kind==='unknown')delete f.plan.validation;
 s.dispatch.team.progress=backend.project(f);f.tasks.forEach((row,i)=>{s.dispatch.team.workflow.tasks[i].status=row.status;s.dispatch.team.workflow.workers[i].status=f.workers[i].status;s.dispatch.team.workflow.workers[i].retry=f.workers[i].retry;});
 s.dispatch.team.workflow.plan.goal=f.plan.goal;return s;
}
(async()=>{
 const out=path.resolve('test-results/ui-progress');await fs.mkdir(out,{recursive:true});ui.verifyProductionAssets();
 const browser=await chromium.launch({headless:true,...(process.platform==='win32'?{channel:process.env.ATM_BROWSER_CHANNEL||'msedge'}:{})});const errors=[],messages=[],screenshots=[],sizes=[];let snapshot,page;
 try{
 async function publish(){await page.evaluate(s=>dispatchEvent(new MessageEvent('message',{data:s})),snapshot);}
 async function boot(){page=await browser.newPage();page.on('pageerror',e=>errors.push(e.message));await page.exposeFunction('hostMessage',async m=>{messages.push(m);if(m.type==='teamSelect'){ui.selectDetail(snapshot,m.workerId,m);await publish();}});
  await page.setContent('<main id="workspace"></main><div id="fixture-label">Progress UI test · synthetic transport · production backend projection</div>');await page.evaluate(()=>{let state={};window.acquireVsCodeApi=()=>({getState:()=>state,setState:s=>state=s,postMessage:m=>window.hostMessage(m)});});
  for(const p of ['src/webview/dashboard.css','src/webview/workspace/workspace.css'])await page.addStyleTag({path:p});await page.addStyleTag({content:'#fixture-label{position:fixed;bottom:0;left:4px;font:10px sans-serif;background:#10151f;color:#aaa;z-index:999;pointer-events:none}'});for(const p of ui.scripts)await page.addScriptTag({path:'src/webview/'+p});await publish();
 }
 const by=name=>page.getByLabel(name,{exact:true});const shot=async name=>{await page.waitForTimeout(350);await page.screenshot({path:path.join(out,name),fullPage:true});screenshots.push(name);};
 snapshot=scenario();ui.selectDetail(snapshot,ui.ids[0]);await boot();
 for(const [width,height] of [[1920,1080],[1600,900],[1366,768],[1280,720],[760,900]]){
  await page.setViewportSize({width,height});await by('AI 协同开发').click();await page.locator('[role=tab][data-session-id="gpt"]').click();const global=by('GPT 整体任务验证进度');assert.equal(await global.getAttribute('aria-valuenow'),'37');
  if(width===1920)await shot('gpt-progress.png');
  await page.locator('[role=tab][data-session-id="'+ui.ids[0]+'"]').click();const worker=by('当前 Worker验证进度');assert.equal(await worker.getAttribute('aria-valuenow'),'60');assert(await by('展开或收起 Worker 详情').isVisible());
  if(width===1920)await shot('worker-progress.png');if(width===760)await shot('narrow-progress.png');
  await page.locator('[role=tab][data-session-id="'+ui.ids[1]+'"]').click();assert.equal(await worker.getAttribute('aria-valuenow'),'0');
  await by('任务调度中心').click();assert.equal(await by('任务中心整体任务验证进度').getAttribute('aria-valuenow'),'37');assert((await by('任务进度总览').innerText()).includes('审查通过 1'));
  await by('查看任务 task-0').click();if(!await page.locator('.task-drawer .progress-timeline').evaluate(n=>n.open))await page.locator('.task-drawer .progress-timeline > summary').click();assert((await by('任务详情').innerText()).includes('当前版本测试'));await page.keyboard.press('Escape');
  assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth));sizes.push({width,height,passed:true});if(width===1920)await shot('task-center-progress.png');
 }
 await page.setViewportSize({width:1600,height:900});await by('AI 协同开发').click();await page.locator('[role=tab][data-session-id="gpt"]').click();
 for(const kind of ['ready','delivered','blocked','paused','cancelled','retry','unknown']){snapshot=scenario(kind);await publish();const bar=by('GPT 整体任务验证进度'),expected=snapshot.dispatch.team.progress.percent;assert.equal(await bar.getAttribute('aria-valuenow'),expected===null?null:String(expected));if(kind!=='delivered')assert.notEqual(await bar.getAttribute('aria-valuenow'),'100');if(kind==='ready')assert((await by('GPT 整体任务').innerText()).includes('等待用户交付'));if(kind==='blocked')await shot('blocked-progress.png');}
 snapshot=scenario('retry');ui.selectDetail(snapshot,ui.ids[1]);await publish();await page.locator('[role=tab][data-session-id="'+ui.ids[1]+'"]').click();const before=await by('当前 Worker验证进度').getAttribute('aria-valuenow');await page.waitForTimeout(1200);assert.equal(await by('当前 Worker验证进度').getAttribute('aria-valuenow'),before);await shot('retry-progress.png');
 const count=messages.length;await by('关闭 Claude '+ui.ids[1].slice(0,8)+' 标签（后台任务继续）').click();await page.locator('.session-reopen > summary').click();await by('重新打开 Claude '+ui.ids[1].slice(0,8)).click();assert.equal(await by('当前 Worker验证进度').getAttribute('aria-valuenow'),before);assert(messages.slice(count).every(m=>['layout','teamSelect'].includes(m.type)));
 await page.close();await boot();await page.locator('[role=tab][data-session-id="'+ui.ids[1]+'"]').click();assert.equal(await by('当前 Worker验证进度').getAttribute('aria-valuenow'),before);
 await page.emulateMedia({reducedMotion:'reduce'});assert.equal(await page.locator('.worker-main .verification-track > span').evaluate(n=>getComputedStyle(n).transitionDuration),'0s');
 assert.equal(errors.length,0,errors.join('\n'));await fs.writeFile(path.join(out,'result.json'),JSON.stringify({passed:true,synthetic_transport:true,production_backend_projection:true,production_UI:true,no_model_requests:true,sizes,worker_isolation:true,reopen_restores:true,route_switch_no_execution:true,unknown:true,tests_review_delivery_distinct:true,retry_no_increase:true,reduced_motion:true,errors,screenshots},null,2));console.log('Progress browser PASS '+out);
 }catch(e){if(page)await page.screenshot({path:path.join(out,'failure.png'),fullPage:true}).catch(()=>{});await fs.writeFile(path.join(out,'failure.json'),JSON.stringify({error:e.stack,errors},null,2));throw e;}finally{await browser.close();}
})().catch(e=>{console.error(e.stack);process.exitCode=1;});
