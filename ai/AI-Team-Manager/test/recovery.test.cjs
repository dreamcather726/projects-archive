const {test}=require('node:test'),assert=require('node:assert/strict'),fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os'),{spawn}=require('node:child_process'),{JSDOM}=require('jsdom');
const {TokenLedger,ReviewStorage,configureGPTUsage,validUsageBudget,ChatController,ConversationStorage,CodexService}=require('../.tools/recovery/api.cjs');
const root=path.resolve(__dirname,'..');
const launcher=(_cmd,args,cwd)=>spawn(process.execPath,[path.join(__dirname,'mock-cli.cjs'),...args],{cwd,stdio:'pipe',windowsHide:true,detached:process.platform!=='win32'});
async function fixture(){const dir=await fs.mkdtemp(path.join(os.tmpdir(),'atm-recovery-'));const storage=new ReviewStorage(dir),ledger=new TokenLedger(root,storage);await ledger.load();await ledger.begin('interrupted','PLANNING_AND_COORDINATION');const loaded=new TokenLedger(root,storage);await loaded.load();return {dir,ledger:loaded};}
function controller(dir,ledger,extra){const service=new CodexService('fixture',launcher);return new ChatController(new ConversationStorage(dir,root),service,()=>{},async()=>{},()=>5000,extra||(async(_r,_p,_o,id)=>{await ledger.begin(id,'PLANNING_AND_COORDINATION');return {options:{},close:async()=>{},finalize:r=>ledger.finish(id,r.status,r.usage)};}));}
test('interrupted usage is WAITING_APPROVAL, not process failure; cancel preserves ledger, explicit acknowledgement resumes fresh session',async()=>{
 const {dir,ledger}=await fixture(),chat=controller(dir,ledger);await chat.initialize();await chat.send('CODEX_CONNECT_OK');let s=chat.snapshot();assert.match(s.error,/\[WAITING_APPROVAL\]/);assert(!s.error.includes('PROCESS_START_FAILED'));assert.equal(s.conversation.requests.length,0);assert.equal(s.conversation.messages.length,0);assert(!s.conversation.threadId);
 const before=ledger.snapshot();let confirmations=0;assert.equal(await configureGPTUsage(ledger,{inputBudget:async()=>undefined,confirmUnknown:async()=>{throw Error('must not confirm');}}),false);assert.deepEqual(ledger.snapshot(),before);
 assert.equal(await configureGPTUsage(ledger,{inputBudget:async()=>String(before.budget),confirmUnknown:async()=>{confirmations++;return false;}}),false);assert.deepEqual(ledger.snapshot(),before);
 assert.equal(await configureGPTUsage(ledger,{inputBudget:async()=>String(before.budget),confirmUnknown:async count=>{assert.equal(count,1);confirmations++;return true;}}),true);assert.equal(confirmations,2);assert(!ledger.snapshot().blocked);assert.equal(ledger.snapshot().unknown_requests,1);assert.equal(ledger.snapshot().entries[0].usage,undefined);
 await chat.send('CODEX_CONNECT_OK');s=chat.snapshot();assert.equal(s.conversation.status,'COMPLETED',s.error);assert.equal(s.conversation.requests.length,1);assert.equal(s.conversation.threadId,'mock-thread-1');assert.equal(ledger.snapshot().unknown_requests,1);
 const restored=new TokenLedger(root,new ReviewStorage(dir));await restored.load();assert(!restored.snapshot().blocked);assert.equal(restored.snapshot().unknown_requests,1);
});
test('existing thread survives blocked sends and resumes after explicit acknowledgement',async()=>{
 const {dir,ledger}=await fixture(),storage=new ConversationStorage(dir,root);await storage.save({version:1,projectPath:root,activeId:'saved',conversations:[{id:'saved',projectPath:root,threadId:'mock-thread-1',createdAt:'time',updatedAt:'time',status:'COMPLETED',messages:[],requests:[{id:'older',time:'time',status:'COMPLETED'}]}]});
 const chat=controller(dir,ledger);await chat.initialize();await chat.send('blocked');assert.equal(chat.snapshot().conversation.requests.length,1);assert.equal(chat.snapshot().conversation.threadId,'mock-thread-1');
 await configureGPTUsage(ledger,{inputBudget:async()=> '500000',confirmUnknown:async()=>true});await chat.send('CODEX_CONNECT_OK');assert.equal(chat.snapshot().conversation.threadId,'mock-thread-1');assert.equal(chat.snapshot().conversation.requests.length,2);assert.equal(chat.snapshot().conversation.status,'COMPLETED');
});
test('budget recovery rejects invalid limits and active requests, with no automatic acknowledgement',async()=>{
 const {ledger}=await fixture(),initial=ledger.snapshot();for(const s of ['999','100000001','1000.5','1e6','Infinity','-1000','500000;anything']){assert(!validUsageBudget(s,0));await assert.rejects(configureGPTUsage(ledger,{inputBudget:async()=>s,confirmUnknown:async()=>{throw Error('must not confirm invalid budget');}}));}assert.deepEqual(ledger.snapshot(),initial);assert(!validUsageBudget('1000',1000));assert(validUsageBudget(' 1001 ',1000));
 await configureGPTUsage(ledger,{inputBudget:async()=> '500000',confirmUnknown:async()=>true});await ledger.begin('running','REVIEW');await assert.rejects(configureGPTUsage(ledger,{inputBudget:async()=>{throw Error('must not open while running');},confirmUnknown:async()=>true}),/GPT/);
});
test('bridge preparation failure is distinct, redacted and does not create a sent message',async()=>{
 const {dir,ledger}=await fixture(),chat=controller(dir,ledger,async()=>{throw Error('PRIVATE_CAUSE_DO_NOT_SHOW');});await chat.initialize();await chat.send('draft retained');const s=chat.snapshot();assert.match(s.error,/\[TEAM_PREPARE_FAILED\]/);assert(!JSON.stringify(s).includes('PRIVATE_CAUSE'));assert.equal(s.conversation.requests.length,0);assert.equal(s.conversation.messages.length,0);
});
test('production UI labels blocked usage accurately and exposes project-bound recovery while preserving draft',async()=>{
 const dom=new JSDOM('<main></main>',{runScripts:'outside-only'}),w=dom.window,sent=[];for(const file of ['markdown.js','workspace/chatPanel.js','workspace/gptCommanderPanel.js'])w.eval(await fs.readFile(path.join(root,'src/webview',file),'utf8'));
 const slot={id:'gpt',kind:'gpt',title:'GPT',collapsed:false},actions={draft:()=>{},activate:()=>{},collapse:()=>{},maximize:()=>{},remove:()=>{}},panel=new w.ATMChatPanel(slot,m=>sent.push(m),actions,'unsent draft'),layout={activePanel:'gpt'};w.document.body.append(panel.node);
 const s={projectPath:root,projectName:'fixture',connection:'codex 1.0',conversation:{id:'fresh',messages:[],requests:[]}};panel.update(slot,s,layout);panel.submit();s.error='[WAITING_APPROVAL] Budget requires confirmation';panel.update(slot,s,layout);assert.equal(panel.input.value,'unsent draft');assert.equal(panel.status.textContent,'等待用量确认');assert(!panel.status.textContent.includes('连接异常'));panel.node.querySelector('[aria-label="GPT: 用量与预算"]').click();assert.deepEqual(JSON.parse(JSON.stringify(sent.at(-1))),{type:'teamUsageConfigure',projectPath:root});dom.window.close();
});

test('confirmation is bound to the shown ledger; a concurrent request invalidates it',async()=>{
 const {ledger}=await fixture();await ledger.configure(500000,true);
 await assert.rejects(configureGPTUsage(ledger,{inputBudget:async()=>{await ledger.begin('during-dialog','REVIEW');await ledger.finish('during-dialog','CANCELLED');return '500000';},confirmUnknown:async count=>{assert.equal(count,1);return true;}}));
 assert.equal(ledger.snapshot().unknown_requests,2);assert(ledger.snapshot().blocked,'A confirmation for one unknown request cannot acknowledge a new second request');
});
