// Captures the actual native Extension Host review snapshot with production assets.
const fs = require("node:fs/promises"),
  path = require("node:path"),
  assert = require("node:assert/strict"),
  { pathToFileURL } = require("node:url"),
  { chromium } = require("playwright");
(async () => {
  const root = path.resolve(__dirname, ".."),
    names = (await fs.readdir(path.join(root, ".tools")))
      .filter((n) => /^task04-host-\d+$/.test(n))
      .sort()
      .reverse();
  let snapshot, host;
  for (const n of names) {
    try {
      host = path.join(root, ".tools", n);
      snapshot = JSON.parse(
        await fs.readFile(path.join(host, "runtime-snapshot.json"), "utf8"),
      );
      break;
    } catch {}
  }
  assert(
    snapshot?.review?.report?.issues.some(
      (i) => i.evidence_status === "VERIFIED",
    ),
    "Run the real review host test first",
  );
  const out = path.join(root, ".tools", "task04-screenshots");
  await fs.mkdir(out, { recursive: true });
  const asset = (f) => pathToFileURL(path.join(root, "src/webview", f)).href;
  await fs.writeFile(
    path.join(out, "workspace.html"),
    `<!doctype html><html lang="zh-CN"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><meta http-equiv="Content-Security-Policy" content="default-src 'none'; style-src file:; script-src 'nonce-review';"><link rel="stylesheet" href="${asset("workspace/workspace.css")}"></head><body class="vscode-dark"><main id="workspace"></main>${["markdown.js", "workspace/layoutManager.js", "workspace/chatPanel.js", "workspace/reviewPanel.js", "workspace/workspaceRouter.js", "workspace/gptCommanderPanel.js", "workspace/workspace.js"].map((f) => `<script nonce="review" src="${asset(f)}"></script>`).join("")}</body></html>`,
  );
  const browser = await chromium.launch({ headless: true }),
    page = await browser.newPage({ viewport: { width: 1680, height: 1080 } }),
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
  await page.evaluate(
    (s) =>
      window.dispatchEvent(
        new MessageEvent("message", { data: { type: "snapshot", ...s } }),
      ),
    snapshot,
  );
 await page.locator(".commander-legacy").evaluate(e=>e.open=true);
  await page
    .locator(".code-review-panel details")
    .evaluate((e) => (e.open = true));
  await page.locator(".code-review-panel").scrollIntoViewIfNeeded();
  assert(
    await page
      .locator(".code-review-panel")
      .innerText()
      .then((t) => t.includes("REQUEST_CHANGES") && t.includes("HIGH")),
  );
  const issue = snapshot.review.report.issues.find(
    (i) => i.evidence_status === "VERIFIED",
  );
  await page
    .getByRole("button", { name: `定位 ${issue.issue_id}`, exact: true })
    .click();
  assert(
    (await page.evaluate(() => window.__sent)).some(
      (m) => m.type === "reviewOpenIssue" && m.issueId === issue.issue_id,
    ),
  );
  await page.screenshot({ path: path.join(out, "review-real-report.png") });
  await page.evaluate(() => {
    document.body.className = "vscode-light";
    for (const [k, v] of Object.entries({
      "--vscode-editor-background": "#f7f8fc",
      "--vscode-sideBar-background": "#eef0f7",
      "--vscode-editorWidget-background": "#fff",
      "--vscode-foreground": "#27334d",
      "--vscode-descriptionForeground": "#65728b",
      "--vscode-panel-border": "#d4daea",
      "--vscode-input-background": "#fff",
    }))
      document.documentElement.style.setProperty(k, v);
  });
  await page.screenshot({ path: path.join(out, "review-light.png") });
  await page.setViewportSize({ width: 390, height: 840 });
  await page
    .getByRole("button", { name: "展开 / 折叠任务面板", exact: true })
    .click();
  await page.locator(".code-review-panel").scrollIntoViewIfNeeded();
  assert(await page.locator(".task-inspector").isVisible());
  assert(
    (await page.evaluate(() => document.documentElement.scrollWidth)) <= 390,
  );
  await page.screenshot({ path: path.join(out, "review-mobile.png") });
  assert.equal(errors.length, 0, errors.join("\n"));
  await fs.writeFile(
    path.join(out, "results.json"),
    JSON.stringify(
      {
        host,
        reportId: snapshot.review.report.review_id,
        pageErrors: errors,
        locationMessage: true,
        mobile: true,
      },
      null,
      2,
    ),
  );
  await browser.close();
  console.log("Real review page captures passed:", out);
})().catch((e) => {
  console.error(e);
  process.exitCode = 1;
});
