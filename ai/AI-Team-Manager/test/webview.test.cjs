const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const {JSDOM}=require('jsdom');
test('Webview chat supports safe Markdown, copy, Enter/Shift+Enter and streaming without losing draft focus',()=>{
  const dom=new JSDOM('<body data-section="commander"><main id="root"></main></body>',{runScripts:'outside-only'});
  const {window:w}=dom;const sent=[];let saved;
  w.acquireVsCodeApi=()=>({postMessage:m=>sent.push(m),getState:()=>saved,setState:s=>saved=s});
  w.eval(fs.readFileSync('src/webview/markdown.js','utf8'));w.eval(fs.readFileSync('src/webview/dashboard.js','utf8'));
  const state={project:'test',diagnostics:[],running:false,chat:{projectPath:'project',projectName:'test',connection:'mock',busy:false,conversation:{status:'READY',messages:[],requests:[]}}};
  const update=()=>w.dispatchEvent(new w.MessageEvent('message',{data:state}));update();
  const input=w.document.querySelector('textarea');input.focus();input.value='draft';input.dispatchEvent(new w.Event('input'));update();assert.equal(w.document.activeElement,input);assert.equal(input.value,'draft');
  input.dispatchEvent(new w.KeyboardEvent('keydown',{key:'Enter',shiftKey:true,bubbles:true,cancelable:true}));assert(!sent.some(m=>m.type==='send'));
  input.dispatchEvent(new w.KeyboardEvent('keydown',{key:'Enter',bubbles:true,cancelable:true}));assert.equal(sent.at(-1).text,'draft');
  state.chat.conversation.messages=[{role:'assistant',time:new Date().toISOString(),status:'COMPLETED',text:'# 标题\n**加粗**\n<script>window.PWNED=true</script>\n```js\nconst x = 1;\n```'}];update();
  assert(w.document.querySelector('.message-body h1'));assert(w.document.querySelector('.message-body strong'));assert(!w.document.querySelector('.message-body script'));assert(!w.PWNED);assert(w.document.body.textContent.includes('<script>'));
  w.document.querySelector('.code-header button').click();assert.equal(sent.at(-1).type,'copy');assert.equal(sent.at(-1).text,'const x = 1;');assert(w.document.body.textContent.includes('--'));dom.window.close();
});
