const vscode = require("vscode"),
  fs = require("node:fs/promises"),
  path = require("node:path"),
  assert = require("node:assert/strict");
exports.run = async () => {
  const extension = vscode.extensions.getExtension(
      "ai-team-manager.ai-team-manager",
    ),
    api = await extension.activate(),
    root = vscode.workspace.workspaceFolders[0].uri.fsPath,
    dir = process.env.ATM_HOST_TEST_DIR;
  const commandNames = await vscode.commands.getCommands(true);
  for (const n of [
    "startCodeReview",
    "runReviewTests",
    "stopCodeReview",
    "codeReviewHistory",
  ])
    assert(commandNames.includes("aiTeamManager." + n));
  await vscode.commands.executeCommand("aiTeamManager.openDashboard");
  for (let n = 0; n < 100 && !api.getWorkspace().ready; n++)
    await new Promise((r) => setTimeout(r, 100));
  assert(api.getWorkspace().ready);
  const stage1 = path.join(dir, "stage1.json"),
    saved = await fs.readFile(stage1, "utf8").then(JSON.parse, () => undefined);
  if (!saved) {
    await api.claude.configurePermissions({allowedFiles:['worker.cjs'],safeCommands:[]});
    const task = require("./fixtures/review/cases.cjs")[1];
    const prompt =
      "这是隔离审查验收工程。请仅使用 Read/Edit/Write，将 worker.cjs 替换为下面这个故意含逻辑错误的验收样例，不能修复该错误，不改其他文件，不运行命令。最终只说明修改了什么。\n" +
      task.after;
    await api.review.withClaudeTask(prompt, () =>
      api.claude.handle({ type: "send", text: prompt }),
    );
    assert.equal(
      api.claude.snapshot().conversation.status,
      "COMPLETED",
      JSON.stringify(api.claude.snapshot()),
    );
    assert.equal(
      await fs.readFile(path.join(root, "worker.cjs"), "utf8"),
      task.after,
    );
    const beforeGPT = JSON.stringify(api.chat.snapshot());
    const beforeSession = api.claude.snapshot().conversation.threadId;
    await api.review.runTests("node --test");
    const report = await api.review.start({
      task: {
        task_id: "HOST-LOGIC",
        task_description: task.task,
        technical_requirements: "按明确原始需求审查验收样例，不自动修改代码。",
      },
    });
    assert(['REQUEST_CHANGES','BLOCKED'].includes(report.verdict),report.verdict);
    if(report.verdict==='BLOCKED')assert(report.missing_evidence.length,'BLOCKED must explain missing evidence');
    const issue = report.issues.find(
      (i) => i.evidence_status === "VERIFIED" && i.file_path === "worker.cjs",
    );
    assert(issue);
    assert.equal(report.evidence.baseline, "CLAUDE_CAPTURE");
    assert.equal(report.evidence.changes.length, 1);
    assert.equal(api.claude.snapshot().conversation.threadId, beforeSession);
    assert.equal(
      JSON.stringify(api.chat.snapshot()),
      beforeGPT,
      "Review must not touch GPT conversation",
    );
    await api.review.handle({
      type: "reviewOpenIssue",
      reviewId: report.review_id,
      issueId: issue.issue_id,
    });
    assert.equal(
      vscode.window.activeTextEditor.document.uri.fsPath,
      path.join(root, "worker.cjs"),
    );
    assert.equal(
      vscode.window.activeTextEditor.selection.start.line,
      issue.line_number - 1,
    );
    await fs.writeFile(
      path.join(dir, "report.json"),
      JSON.stringify(report, null, 2),
    );
    await fs.writeFile(
      path.join(dir, "runtime-snapshot.json"),
      JSON.stringify(api.getWorkspaceSnapshot(), null, 2),
    );
    await fs.writeFile(
      stage1,
      JSON.stringify({
        realClaude: true,
        realGPT: true,
        reviewId: report.review_id,
        verdict: report.verdict,
        line: issue.line_number,
        locationOpened: true,
        session: beforeSession,
        uiReady: true,
      }),
    );
  } else {
    const state = api.review.snapshot();
    assert.equal(state.report.review_id, saved.reviewId);
    assert.equal(state.report.verdict, saved.verdict);
    assert(state.history.some((h) => h.review_id === saved.reviewId));
    assert.equal(api.claude.snapshot().conversation.threadId, saved.session);
    assert.equal(
      await fs.readFile(path.join(root, "worker.cjs"), "utf8"),
      require("./fixtures/review/cases.cjs")[1].after,
    );
    await fs.writeFile(
      path.join(dir, "stage2.json"),
      JSON.stringify({
        reviewHistoryRestored: true,
        claudeSessionUnchanged: true,
        sourceUnchanged: true,
        uiReady: api.getWorkspace().ready,
      }),
    );
  }
};
