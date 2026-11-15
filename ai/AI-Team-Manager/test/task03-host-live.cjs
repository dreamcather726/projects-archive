// Uses the production coordinator, VS Code APIs, actual CLI and existing account.
const vscode = require("vscode"),
  fs = require("node:fs/promises"),
  path = require("node:path"),
  assert = require("node:assert/strict");
exports.run = async () => {
  const extension = vscode.extensions.getExtension(
      "ai-team-manager.ai-team-manager",
    ),
    api = await extension.activate(),
    dir = process.env.ATM_HOST_TEST_DIR,
    project = vscode.workspace.workspaceFolders[0].uri.fsPath;
  assert(
    project
      .toLowerCase()
      .startsWith(
        path
          .join(extension.extensionPath.split(".tools")[0], ".tools")
          .toLowerCase(),
      ) || project.includes("task03-live-"),
    "Only an isolated fixture may be used",
  );
  const names = [
      "checkClaudeConnection",
      "newClaudeConversation",
      "stopClaudeResponse",
      "claudeHistory",
      "selectClaudeProject",
    ],
    commands = await vscode.commands.getCommands(true);
  for (const name of names) assert(commands.includes("aiTeamManager." + name));
  await vscode.commands.executeCommand("aiTeamManager.openDashboard");
  for (let i = 0; i < 100 && !api.getWorkspace().ready; i++)
    await new Promise((r) => setTimeout(r, 100));
  assert(api.getWorkspace().ready);
  await vscode.commands.executeCommand("aiTeamManager.checkClaudeConnection");
  assert(!api.claude.snapshot().error, api.claude.snapshot().error);
  const saved = JSON.parse(
    await fs
      .readFile(path.join(dir, "stage1.json"), "utf8")
      .catch(() => "null"),
  );
  if (!saved) {
    await api.claude.configurePermissions({allowedFiles:['motor_control.c'],safeCommands:[]});
    await vscode.commands.executeCommand("aiTeamManager.newClaudeConversation");
    await api.claude.handle({
      type: "send",
      text: "这是隔离验收。只允许修改 motor_control.c；禁止修改其他文件或运行命令。请 Read 读取 README.md 和 motor_control.c，在 motor_control.c 顶部加一行注释 // CLAUDE_HOST_OK，不改变函数行为。最后输出 README 中的 Session marker、CLAUDE_HOST_OK 和操作结果。",
    });
    const state = api.claude.snapshot();
    assert.equal(state.conversation.status, "COMPLETED", state.error);
    assert.equal(state.error, undefined);
    assert(
      (
        await fs.readFile(path.join(project, "motor_control.c"), "utf8")
      ).includes("// CLAUDE_HOST_OK"),
    );
    await fs.writeFile(
      path.join(dir, "runtime-snapshot.json"),
      JSON.stringify(api.getWorkspaceSnapshot(), null, 2),
    );
    await fs.writeFile(
      path.join(dir, "stage1.json"),
      JSON.stringify({
        activated: true,
        claudeReal: true,
        session: state.conversation.threadId,
        id: state.conversation.id,
        messages: state.conversation.messages.length,
        uiReady: api.getWorkspace().ready,
      }),
    );
  } else {
    const restored = api.claude.snapshot();
    assert.equal(restored.conversation.threadId, saved.session);
    assert.equal(restored.conversation.id, saved.id);
    await api.claude.handle({
      type: "send",
      text: "不要使用工具或修改文件。根据上一轮上下文回复 README 的 Session marker，末尾加上 CLAUDE_HOST_RESUME_OK。",
    });
    const resumed = api.claude.snapshot();
    assert.equal(resumed.conversation.status, "COMPLETED", resumed.error);
    assert.equal(resumed.error, undefined);
    assert.equal(resumed.conversation.threadId, saved.session);
    assert(
      resumed.conversation.messages
        .at(-1)
        .text.includes("CLAUDE_HOST_RESUME_OK"),
    );
    await fs.writeFile(
      path.join(dir, "resumed-snapshot.json"),
      JSON.stringify(api.getWorkspaceSnapshot(), null, 2),
    );
    const pending = api.claude.handle({
      type: "send",
      text: "不要使用工具或修改文件。逐行写出 1 到 2000，每行用一句话说明该数字的奇偶性，不要省略。",
    });
    let started = false;
    let busySeen = false;
    for (let i = 0; i < 1200; i++) {
      const s = api.claude.snapshot(),
        request = s.conversation.requests.at(-1);
      busySeen ||= !!s.busy;
      if (
        s.busy &&
        request?.status === "RUNNING" &&
        s.conversation.messages.some(
          (m) => m.requestId === request.id && m.role === "assistant" && m.text,
        )
      ) {
        started = true;
        await fs.writeFile(
          path.join(dir, "streaming-snapshot.json"),
          JSON.stringify(api.getWorkspaceSnapshot(), null, 2),
        );
        break;
      }
      if (busySeen && !s.busy) break;
      await new Promise((r) => setTimeout(r, 100));
    }
    assert(started, JSON.stringify(api.claude.snapshot()));
    await vscode.commands.executeCommand("aiTeamManager.stopClaudeResponse");
    await pending;
    assert.equal(api.claude.snapshot().conversation.status, "CANCELLED");
    assert.equal(api.claude.snapshot().conversation.threadId, saved.session);
    await fs.writeFile(
      path.join(dir, "stage2.json"),
      JSON.stringify({
        restartRestored: true,
        resumedSameSession: true,
        realStreamStoppedFromCommand: true,
        uiReady: api.getWorkspace().ready,
      }),
    );
  }
};
