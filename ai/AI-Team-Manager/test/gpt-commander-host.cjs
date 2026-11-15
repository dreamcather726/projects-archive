// Real, read-only GPT smoke test for the packaged UI. Never dispatches a Claude task.
const vscode = require("vscode");
const fs = require("node:fs/promises");
const path = require("node:path");
const assert = require("node:assert/strict");
exports.run = async () => {
  const extension = vscode.extensions.getExtension("ai-team-manager.ai-team-manager");
  assert(extension); assert.equal(extension.packageJSON.version, "0.7.1");
  const api = await extension.activate(), dir = process.env.ATM_HOST_TEST_DIR;
  await vscode.commands.executeCommand("aiTeamManager.openDashboard");
  for (let i = 0; i < 100 && !api.getWorkspace().ready; i++) await new Promise(r => setTimeout(r, 100));
  assert(api.getWorkspace().ready, "Packaged Webview scripts must load under CSP");
  const saved = JSON.parse(await fs.readFile(path.join(dir, "stage1.json"), "utf8").catch(() => "null"));
  if (!saved) {
    await api.workspace.saveLayout({ ...api.getWorkspace().layout, mode: "single", inspectorWidth: 420, logsCollapsed: true, activePanel: "gpt", navigation: { route: "collaboration", hiddenTabs: [] } });
    await api.chat.handle({ type: "new" });
    await fs.writeFile(path.join(dir, "workspace-empty.json"), JSON.stringify(api.getWorkspaceSnapshot()));
    await api.chat.handle({ type: "send", text: "仅讨论，不派发 Claude，不调用任何工具，不读写文件、不运行命令。请为 Python 加法函数和 unittest 单元测试给出三步简短计划，并明确尚未执行。开头写 GPT_COMMANDER_UI_OK。" });
    const state = api.chat.snapshot();
    assert.equal(state.conversation.status, "COMPLETED", state.error || JSON.stringify(state));
    assert(state.conversation.threadId);
    assert(state.conversation.messages.at(-1).text.includes("GPT_COMMANDER_UI_OK"));
    assert(!api.dispatch.snapshot().task, "Discussion must not dispatch a Worker");
    const id = state.conversation.id, thread = state.conversation.threadId;
    const tab = vscode.window.tabGroups.all.flatMap(g => g.tabs).find(t => t.input instanceof vscode.TabInputWebview && t.input.viewType.includes("aiTeamManager.workspace"));
    await vscode.window.tabGroups.close(tab);
    await vscode.commands.executeCommand("aiTeamManager.openDashboard");
    assert.equal(api.chat.snapshot().conversation.threadId, thread);
    await fs.writeFile(path.join(dir, "stage1.json"), JSON.stringify({ realModel: true, packagedVersion: extension.packageJSON.version, id, thread, requests: state.conversation.requests.length, cspLoaded: true }));
  } else {
    const before = api.chat.snapshot();
    const beforeRequests = before.conversation.requests.length;
    const priorFailedRequests = before.conversation.requests.filter(r => r.status === "FAILED").length;
    assert.equal(before.conversation.id, saved.id); assert.equal(before.conversation.threadId, saved.thread);
    assert.equal(api.getWorkspace().layout.navigation.route, "collaboration");
    if (!process.env.ATM_HOST_VERIFY_ONLY)
      await api.chat.handle({ type: "send", text: "继续仅讨论，不调用工具也不派发 Worker：把刚才的计划压缩成一句话，并说明尚未执行。" });
    const state = api.chat.snapshot();
    assert.equal(state.conversation.status, "COMPLETED", state.error);
    assert.equal(state.conversation.threadId, saved.thread);
    assert.equal(state.conversation.requests.length, beforeRequests + (process.env.ATM_HOST_VERIFY_ONLY ? 0 : 1));
    assert(state.conversation.requests.length > saved.requests);
    assert(!api.dispatch.snapshot().task);
    await fs.writeFile(path.join(dir, "stage2.json"), JSON.stringify({ realModel: true, packagedVersion: extension.packageJSON.version, sameSessionAfterRestart: true, requests: state.conversation.requests.length, priorFailedRequests, verificationOnly: !!process.env.ATM_HOST_VERIFY_ONLY, noWorkerDispatch: true }));
  }
  await fs.writeFile(path.join(dir, "runtime-snapshot.json"), JSON.stringify(api.getWorkspaceSnapshot()));
};
