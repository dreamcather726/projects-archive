// Exact final VSIX smoke test; real-model orchestration is covered by task0711-host.cjs.
const vscode=require('vscode'),fs=require('node:fs/promises'),path=require('node:path'),assert=require('node:assert/strict'),crypto=require('node:crypto');
exports.run=async()=>{
 const extension=vscode.extensions.getExtension('ai-team-manager.ai-team-manager'),api=await extension.activate(),dir=process.env.ATM_HOST_TEST_DIR;
 await vscode.commands.executeCommand('aiTeamManager.openDashboard');for(let n=0;n<100&&!api.getWorkspace().ready;n++)await new Promise(r=>setTimeout(r,100));assert(api.getWorkspace().ready);assert.equal(extension.packageJSON.version,'0.8.0');
 assert.equal(vscode.workspace.getConfiguration('aiTeamManager').get('teamMaxConcurrent'),3);assert.equal(vscode.workspace.getConfiguration('aiTeamManager').get('teamMaxRepairRounds'),2);assert.equal(api.dispatch.team.ledger.snapshot().cli_requests,0);
 const previous=await fs.readFile(path.join(dir,'stage1.json'),'utf8').then(JSON.parse,()=>undefined),bundle=await fs.readFile(path.join(extension.extensionPath,'dist','extension.js')),actual=crypto.createHash('sha256').update(bundle).digest('hex'),expected=crypto.createHash('sha256').update(await fs.readFile(path.resolve(__dirname,'..','dist','extension.js'))).digest('hex');assert.equal(actual,expected);
 await fs.writeFile(path.join(dir,previous?'stage2.json':'stage1.json'),JSON.stringify({passed:true,version:'0.8.0',exact_final_bundle:true,bundle_sha256:actual,CSP:true,configuration_defaults:true,no_model_requests:true,restart:!!previous},null,2));
};
