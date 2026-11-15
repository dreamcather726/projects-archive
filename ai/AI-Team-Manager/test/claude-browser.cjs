// Screenshot genuine Extension Host snapshots with the production webview assets.
// This is a page capture, not a capture of the Windows VS Code window frame.
const { chromium } = require("playwright"),
  fs = require("node:fs/promises"),
  path = require("node:path"),
  assert = require("node:assert/strict"),
  { pathToFileURL } = require("node:url");
const root = path.resolve(__dirname, ".."),
  out = path.join(root, ".tools", "task03-screenshots");
(async () => {
  const hosts = (await fs.readdir(path.join(root, ".tools")))
    .filter((n) => /^task03-host-\d+$/.test(n))
    .sort()
    .reverse();
  let dir, snapshot;
  for (const host of hosts) {
    try {
      dir = path.join(root, ".tools", host);
      snapshot = JSON.parse(
        await fs.readFile(path.join(dir, "runtime-snapshot.json"), "utf8"),
      );
      break;
    } catch {}
  }
  assert(
    snapshot?.claude?.conversation?.threadId,
    "Run the real Claude host acceptance first",
  );
  assert(
    snapshot.claude.conversation.messages.some(
      (m) => m.role === "assistant" && m.text.includes("CLAUDE_HOST_OK"),
    ),
  );
  await fs.mkdir(out, { recursive: true });
  const asset = (f) => pathToFileURL(path.join(root, "src", "webview", f)).href;
  const html = `<!doctype html><html lang="zh-CN"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1"><meta http-equiv="Content-Security-Policy" content="default-src 'none'; style-src file:; script-src 'nonce-capture';"><link rel="stylesheet" href="${asset("workspace/workspace.css")}"></head><body class="vscode-dark"><main id="workspace"></main>${["markdown.js", "workspace/layoutManager.js", "workspace/chatPanel.js", "workspace/workspaceRouter.js", "workspace/gptCommanderPanel.js", "workspace/workspace.js"].map((f) => `<script nonce="capture" src="${asset(f)}"></script>`).join("")}</body></html>`;
  await fs.writeFile(path.join(out, "workspace.html"), html);
  const browser = await chromium.launch({ headless: true }),
    page = await browser.newPage({ viewport: { width: 1680, height: 1050 } }),
    errors = [];
  page.on("pageerror", (e) => errors.push(e.message));
  page.on("console", (m) => {
    if (m.type() === "error") errors.push(m.text());
  });
  await page.addInitScript(() => {
    let state;
    window.__sent = [];
    window.acquireVsCodeApi = () => ({
      getState: () => state,
      setState: (s) => (state = s),
      postMessage: (m) => window.__sent.push(m),
    });
  });
  await page.goto(pathToFileURL(path.join(out, "workspace.html")).href);
  const show = async (data) => {
    // Each capture starts from a fresh page; older snapshots must not inherit newer DOM messages.
    await page.reload();
    await page.evaluate(
      (s) =>
        window.dispatchEvent(
          new MessageEvent("message", { data: { type: "snapshot", ...s } }),
        ),
      data,
    );
    await page.evaluate(() => new Promise(resolve => requestAnimationFrame(() => requestAnimationFrame(resolve))));
    await page
      .locator('[data-panel-id="claude-01"] .session-details')
      .last()
      .evaluate((e) => (e.open = true));
    await page.getByRole('tab', {name:'Claude Worker 01',exact:true}).click();
    assert.equal(await page.locator('[data-panel-id="claude-01"] .chat-message').count(), data.claude.conversation.messages.length);
  };
  await show(snapshot);
  await page
    .getByRole("tab", { name: "Claude Worker 01", exact: true })
    .click();
  assert(
    await page
      .locator('[data-panel-id="claude-01"] .chat-messages')
      .innerText()
      .then((t) => t.includes("CLAUDE_HOST_OK")),
  );
  assert(
    !(await page
      .locator(".gpt-panel")
      .innerText()
      .then((t) => t.includes("CLAUDE_HOST_OK"))),
  );
  await page.screenshot({
    path: path.join(out, "claude-real-conversation.png"),
  });
  const streaming = JSON.parse(
    await fs.readFile(path.join(dir, "streaming-snapshot.json"), "utf8"),
  );
  await show(streaming);
  assert(
    await page
      .locator(
        '[data-panel-id="claude-01"] .compose-tools button[aria-label="停止当前响应"]',
      )
      .isEnabled(),
  );
  await page.screenshot({ path: path.join(out, "claude-real-streaming.png") });
  await show(snapshot);
  await page.evaluate(() => {
    document.body.className = "vscode-light";
    const variables = {
      "--vscode-editor-background": "#f7f8fc",
      "--vscode-sideBar-background": "#eef0f7",
      "--vscode-editorWidget-background": "#ffffff",
      "--vscode-foreground": "#27334d",
      "--vscode-descriptionForeground": "#65728b",
      "--vscode-panel-border": "#d4daea",
      "--vscode-input-background": "#fff",
    };
    for (const [name, value] of Object.entries(variables)) document.documentElement.style.setProperty(name, value);
  });
  await page.screenshot({ path: path.join(out, "claude-light.png") });
  assert((await page.evaluate(() => document.documentElement.scrollWidth)) <= 1680);
  await page.evaluate(() => {document.body.className = "vscode-dark";document.documentElement.removeAttribute("style");});
  await show(snapshot);
  await page
    .getByRole("button", {
      name: "清空 Claude 当前展示；保留 Session 与历史",
      exact: true,
    })
    .click();
  assert(
    !(await page
      .locator('[data-panel-id="claude-01"] .chat-messages')
      .innerText()
      .then((t) => t.includes("CLAUDE_HOST_OK"))),
  );
  assert(snapshot.claude.conversation.messages.length > 0);
  await page.setViewportSize({ width: 390, height: 840 });
  assert(!(await page.locator(".task-inspector").isVisible()));
  assert(
    (await page.evaluate(() => document.documentElement.scrollWidth)) <= 390,
  );
  await page.screenshot({ path: path.join(out, "claude-mobile.png") });
  assert.equal(errors.length, 0, errors.join("\n"));
  await fs.writeFile(
    path.join(out, "results.json"),
    JSON.stringify(
      {
        actualSnapshot: dir,
        session: snapshot.claude.conversation.threadId,
        pageErrors: errors,
        isolatedChannels: true,
        clearDisplayPreservesRecords: true,
        mobile: true,
      },
      null,
      2,
    ),
  );
  await browser.close();
  console.log("Claude actual-session page screenshots:", out);
})().catch((e) => {
  console.error(e);
  process.exitCode = 1;
});
