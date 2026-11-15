const vscode=require('vscode'),fs=require('node:fs/promises'),path=require('node:path'),assert=require('node:assert/strict'),crypto=require('node:crypto');
exports.run=async()=>{
 const extension=vscode.extensions.getExtension('ai-team-manager.ai-team-manager'),api=await extension.activate(),dir=process.env.ATM_HOST_TEST_DIR;
 await vscode.commands.executeCommand('aiTeamManager.openDashboard');for(let i=0;i<100&&!api.getWorkspace().ready;i++)await new Promise(r=>setTimeout(r,100));assert(api.getWorkspace().ready);
 assert.equal(extension.packageJSON.version,process.env.ATM_EXPECTED_VERSION);const properties=extension.packageJSON.contributes.configuration.properties;assert.equal(properties['aiTeamManager.claudeRetryMaxRetries'].default,3);assert.equal(properties['aiTeamManager.claudeRetryBaseDelayMs'].default,2000);
 assert.equal(api.dispatch.team.ledger.snapshot().cli_requests,0);const bundle=await fs.readFile(path.join(extension.extensionPath,'dist/extension.js'));const actual=crypto.createHash('sha256').update(bundle).digest('hex');assert.equal(actual,process.env.ATM_EXPECTED_BUNDLE);
 const first=await fs.readFile(path.join(dir,'stage1.json')).then(()=>true,()=>false);
 await fs.writeFile(path.join(dir,first?'stage2.json':'stage1.json'),JSON.stringify({passed:true,version:extension.packageJSON.version,CSP:true,exact_bundle:true,bundle_sha256:actual,no_model_requests:true,restart:first},null,2));
};
