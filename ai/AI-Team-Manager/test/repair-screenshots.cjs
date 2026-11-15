// Renders production Webview assets with real native-host snapshots. These are not OS-frame screenshots.
const fs=require('node:fs/promises'),path=require('node:path'),assert=require('node:assert/strict');
process.env.PLAYWRIGHT_BROWSERS_PATH=path.resolve('.tools/ui-browsers');
const {chromium}=require('playwright'),{defaultLayout}=require('../.tools/task02-test/api.cjs');
(async()=>{
  const dir=path.resolve(process.argv[2]),output=path.join(dir,'screenshots');await fs.mkdir(output,{recursive:true});
  const browser=await chromium.launch({headless:true}),page=await browser.newPage({viewport:{width:1900,height:1200},deviceScaleFactor:1});const errors=[];
  page.on('pageerror',e=>errors.push(String(e)));
  const results=[];
  for(const [name,file,theme,width]of [['plan-dark','plan-preview-snapshot.json','dark',1900],['result-dark','runtime-snapshot.json','dark',1900],['result-light','runtime-snapshot.json','light',1900],['result-narrow','runtime-snapshot.json','light',620]]){
    await page.setViewportSize({width,height:1200});
    await page.setContent('<!doctype html><html><head><meta charset="utf-8"></head><body><main id="workspace"></main></body></html>');
    await page.addStyleTag({content:await fs.readFile('src/webview/workspace/workspace.css','utf8')});
    await page.evaluate(light=>{
      const colors=light?{'editor-background':'#ffffff','sideBar-background':'#f3f3f3','editorWidget-background':'#f8f8f8','foreground':'#24292f','descriptionForeground':'#535c68','panel-border':'#c7cdd4','input-background':'#ffffff','input-foreground':'#24292f','focusBorder':'#007acc','textCodeBlock-background':'#eef1f5'}:{'editor-background':'#10151f','sideBar-background':'#161c29','editorWidget-background':'#1b2232','foreground':'#d8e0f0','descriptionForeground':'#95a1b8','panel-border':'#344057','input-background':'#10151f','input-foreground':'#d8e0f0','focusBorder':'#4daafc','textCodeBlock-background':'#10151f'};
      for(const [key,value]of Object.entries(colors))document.documentElement.style.setProperty('--vscode-'+key,value);
      window.acquireVsCodeApi=()=>({postMessage:()=>{},getState:()=>({}),setState:()=>{}});
    },theme==='light');
    for(const script of ['markdown.js','workspace/layoutManager.js','workspace/chatPanel.js','workspace/reviewPanel.js','workspace/permissionPanel.js','workspace/repairPanel.js',"workspace/workspaceRouter.js", "workspace/gptCommanderPanel.js", "workspace/workspace.js"])await page.addScriptTag({content:await fs.readFile('src/webview/'+script,'utf8')});
    const data=JSON.parse(await fs.readFile(path.join(dir,file),'utf8'));data.type='snapshot';data.layout={...defaultLayout(),inspectorWidth:420};
    await page.evaluate(data=>window.dispatchEvent(new MessageEvent('message',{data})),data);
    await page.locator(".commander-legacy").evaluate(e=>e.open=true);
    if(width<1000)await page.getByLabel('展开 / 折叠任务面板',{exact:true}).click();
    await page.locator('.repair-panel').evaluate(el=>{const parent=el.closest('.task-inspector');parent.scrollTop+=el.getBoundingClientRect().top-parent.getBoundingClientRect().top-12;});
    assert(await page.locator('.repair-panel').isVisible());
    const overflow=await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth);assert(!overflow,name+' horizontal overflow');
    await page.screenshot({path:path.join(output,name+'.png')});results.push({name,theme,width,horizontalOverflow:overflow,source:file});
  }
  assert.deepEqual(errors,[]);await fs.writeFile(path.join(output,'results.json'),JSON.stringify({actualNativeSnapshots:true,productionAssets:true,osFrame:false,errors,results},null,2));await browser.close();console.log(output);
})().catch(e=>{console.error(e);process.exitCode=1;});
