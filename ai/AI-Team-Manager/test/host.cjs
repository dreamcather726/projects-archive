const vscode = require('vscode');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
exports.run = async function () {
  const extension = vscode.extensions.getExtension('ai-team-manager.ai-team-manager');
  assert(extension, 'Extension discovered');
  const api = await extension.activate();
  assert(extension.isActive, 'Extension activated');
  const commands = await vscode.commands.getCommands(true);
  for (const name of ['openDashboard','diagnoseEnvironment','refreshDashboard','openSettings']) {
    assert(commands.includes(`aiTeamManager.${name}`), `${name} registered`);
    await vscode.commands.executeCommand(`aiTeamManager.${name}`);
  }
  for (const view of ['commander','agents','tasks','logs']) {
    await vscode.commands.executeCommand(`aiTeamManager.${view}.focus`);
  }
  const results = api.getDiagnostics();
  assert.equal(results.length, 6);
  assert(results.every(result => typeof result.ok === 'boolean' && result.detail));
  assert(results.find(result => result.name === 'VS Code').ok);
  fs.mkdirSync(path.join(extension.extensionPath,'test-results'),{recursive:true});
  fs.writeFileSync(path.join(extension.extensionPath,'test-results','host.json'),JSON.stringify({activated:true,commands:4,views:4,diagnostics:results},null,2));
};
