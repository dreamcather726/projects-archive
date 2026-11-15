const test=require('node:test'),assert=require('node:assert/strict'),fs=require('node:fs'),{JSDOM}=require('jsdom');
const {fixture,accept,integrate,project}=require('./progress-fixture.cjs');
function view(){const dom=new JSDOM('<main></main>',{runScripts:'outside-only'});const w=dom.window;for(const p of ['markdown.js','workspace/layoutManager.js','workspace/chatPanel.js','workspace/outputFormatter.js'])w.eval(fs.readFileSync('src/webview/'+p,'utf8'));return {dom,w};}
test('progress accessibility distinguishes unknown, waiting delivery and completed without estimating progress in UI',()=>{
 const {dom,w}=view();try{const v=w.ATMOutputFormatter.progressView('验收');v.update();assert.equal(v.node.querySelector('[role=progressbar]').hasAttribute('aria-valuenow'),false);assert(v.node.textContent.includes('未知'));
 const f=integrate(accept(fixture()));v.update(project(f));assert.equal(v.node.querySelector('[role=progressbar]').getAttribute('aria-valuenow'),'80');assert(v.node.textContent.includes('等待用户交付'));integrate(f,true);v.update(project(f));assert.equal(v.node.querySelector('[role=progressbar]').getAttribute('aria-valuenow'),'100');assert(v.node.textContent.includes('已完成'));}finally{dom.window.close();}
});
test('timeline renders recorded errors as text and keeps absent timestamps unknown',()=>{
 const {dom,w}=view();try{const v=w.ATMOutputFormatter.progressTimeline({steps:[{id:'s',name:'review',state:'BLOCKED',reason:'<img src=x onerror=alert(1)>'}]});assert.equal(v.open,false);assert(v.textContent.includes('未记录'));assert.equal(v.querySelectorAll('img').length,0);assert(v.textContent.includes('<img'));}finally{dom.window.close();}
});
