// Renders the same local webview assets; no model, remote resource or server is used.
const { chromium } = require("playwright");
const fs = require("node:fs/promises");
const path = require("node:path");
const { pathToFileURL } = require("node:url");
const assert = require("node:assert/strict");
const { defaultLayout } = require("../.tools/task02-test/api.cjs");
const root = path.resolve(__dirname, ".."),
  out = path.join(root, ".tools", "task07103-ui");
(async () => {
  await fs.mkdir(out, { recursive: true });
  const hosts = (await fs.readdir(path.join(root, ".tools")))
    .filter((n) => /^task02-host-\d+$/.test(n))
    .sort()
    .reverse();
  let snapshot;
  for (const host of hosts) {
    try {
      snapshot = JSON.parse(
        await fs.readFile(
          path.join(root, ".tools", host, "workspace-empty.json"),
          "utf8",
        ),
      );
      break;
    } catch {}
  }
  assert(
    snapshot,
    "Run npm run test:host first to capture the actual empty workspace snapshot",
  );
  snapshot.type = "snapshot";
  snapshot.layout = defaultLayout();
  assert.equal(
    snapshot.chat.conversation.messages.length,
    0,
    "Screenshots must not contain Mock chats",
  );
  const asset = (f) => pathToFileURL(path.join(root, "src", "webview", f)).href;
  const html = `<!doctype html><html lang="zh-CN"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1"><meta http-equiv="Content-Security-Policy" content="default-src 'none'; style-src file:; script-src 'nonce-ui-test';"><link rel="stylesheet" href="${asset("workspace/workspace.css")}"></head><body><main id="workspace"></main>${["markdown.js", "workspace/layoutManager.js", "workspace/chatPanel.js", "workspace/reviewPanel.js", "workspace/permissionPanel.js", "workspace/repairPanel.js", "workspace/dispatchPanel.js", "workspace/workspaceRouter.js", "workspace/gptCommanderPanel.js", "workspace/workspace.js"].map((f) => `<script nonce="ui-test" src="${asset(f)}"></script>`).join("")}</body></html>`;
  const htmlPath = path.join(out, "workspace.html");
  await fs.writeFile(htmlPath, html);
  const browser = await chromium.launch({ headless: true }),
    page = await browser.newPage({
      viewport: { width: 1680, height: 1000 },
      deviceScaleFactor: 1,
    });
  const errors = [],
    requests = [];
  page.on("pageerror", (e) => errors.push(e.message));
  page.on("console", (m) => {
    if (m.type() === "error") errors.push(m.text());
  });
  page.on("request", (r) => {
    if (/^https?:/.test(r.url())) requests.push(r.url());
  });
  await page.addInitScript(() => {
    let state;
    window.__sent = [];
    window.acquireVsCodeApi = () => ({
      getState: () => state,
      setState: (s) => {
        state = s;
        window.__state = s;
      },
      postMessage: (m) => window.__sent.push(m),
    });
  });
  await page.goto(pathToFileURL(htmlPath).href);
  async function theme(light = false) {
    await page.evaluate((light) => {
      document.body.className = light ? "vscode-light" : "vscode-dark";
      const values = light
        ? {
            "--vscode-editor-background": "#f7f8fc",
            "--vscode-sideBar-background": "#eef0f7",
            "--vscode-editorWidget-background": "#ffffff",
            "--vscode-foreground": "#27334d",
            "--vscode-descriptionForeground": "#65728b",
            "--vscode-panel-border": "#d4daea",
            "--vscode-input-background": "#fff",
          }
        : {
            "--vscode-editor-background": "#101520",
            "--vscode-sideBar-background": "#141a27",
            "--vscode-editorWidget-background": "#1d2536",
            "--vscode-foreground": "#dce3f1",
            "--vscode-descriptionForeground": "#8995ad",
            "--vscode-panel-border": "#313c52",
            "--vscode-input-background": "#171e2b",
          };
      for (const [key, value] of Object.entries(values))
        document.documentElement.style.setProperty(key, value);
    }, light);
  }
  await theme();
  await page.evaluate(
    (s) => window.dispatchEvent(new MessageEvent("message", { data: s })),
    snapshot,
  );
  await page.locator(".gpt-panel").waitFor();
  assert.equal(await page.locator(".gpt-task-details .commander-card").count(), 6);
  assert.equal(await page.locator(".commander-progress").getAttribute("aria-valuenow"), null);
  const report = {
    emptySnapshot: true,
    viewports: [],
    errors,
    remoteRequests: requests,
  };
  const overflow = async () =>
    page.evaluate(() => ({
      width: innerWidth,
      scroll: document.documentElement.scrollWidth,
      body: document.body.scrollWidth,
      panels: [...document.querySelectorAll(".chat-panel")]
        .filter((e) => getComputedStyle(e).display !== "none")
        .map((e) => ({
          id: e.dataset.panelId,
          width: e.getBoundingClientRect().width,
          x: e.getBoundingClientRect().x,
          right: e.getBoundingClientRect().right,
        })),
    }));
  for (const width of [1680, 1280, 1024, 760, 640, 390, 320]) {
    await page.setViewportSize({ width, height: width < 760 ? 820 : 1000 });
    const metrics = await overflow();
    assert(
      metrics.scroll <= width && metrics.body <= width,
      JSON.stringify(metrics),
    );
    for (const p of metrics.panels)
      assert(p.right <= width + 1 && p.x >= 0, JSON.stringify(metrics));
    report.viewports.push(metrics);
  }
  await page.setViewportSize({ width: 1680, height: 1000 });
  await page.screenshot({ path: path.join(out, "workspace-dark.png") });
  await page.locator(".navigation").screenshot({ path: path.join(out, "navigation.png") });
  await page.locator(".session-bar").screenshot({ path: path.join(out, "session-tabs.png") });
  const checkpoint = await page.evaluate(() => window.__sent.length);
  for (const route of ["task-center", "review-integration", "projects", "settings"]) {
    await page.locator(`[data-route="${route}"].nav-link`).click();
    assert.equal(await page.locator(".route-outlet").getAttribute("data-route"), route);
    assert(await page.locator(".route-outlet").isVisible());
    assert(!(await page.locator(".gpt-panel").isVisible()));
    assert.equal(await page.locator(`[data-route="${route}"].nav-link`).getAttribute("aria-current"), "page");
  }
  await page.screenshot({ path: path.join(out, "page-route.png") });
  await page.getByRole("button", { name: "返回 AI 协同开发", exact: true }).click();
  await page.getByRole("tab", { name: "Claude Worker 01", exact: true }).click();
  await page.getByRole("button", { name: "关闭 Claude Worker 01 标签（后台任务继续）", exact: true }).click();
  await page.locator(".session-reopen summary").click();
  await page.getByRole("button", { name: "重新打开 Claude Worker 01", exact: true }).click();
  await page.getByRole("tab", { name: "GPT 总指挥", exact: true }).click();
  assert(await page.evaluate(checkpoint => window.__sent.slice(checkpoint).every(m => m.type === "layout"), checkpoint));
  report.routesAndTabLifecycle = true;
  await page.getByRole("combobox", { name: "窗口布局" }).selectOption("dual");
  const handle = page.locator(".grid-column-resize"),
    box = await handle.boundingBox();
  const before = await page.locator(".gpt-panel").boundingBox();
  await page.mouse.move(box.x + 3, box.y + 20);
  await page.mouse.down();
  await page.mouse.move(box.x + 90, box.y + 20, { steps: 6 });
  await page.mouse.up();
  const after = await page.locator(".gpt-panel").boundingBox();
  assert(
    after.width > before.width + 50,
    "Pointer resize must change panel width",
  );
  report.pointerResize = true;
  for (const mode of ["single", "dual", "triple", "quad", "auto"]) {
    await page.getByRole("combobox", { name: "窗口布局" }).selectOption(mode);
    assert.equal(
      await page.locator(".chat-grid").getAttribute("data-mode"),
      mode,
    );
  }
  await page.getByRole("combobox", { name: "窗口布局" }).selectOption("quad");
  await page.screenshot({ path: path.join(out, "workspace-four-panels.png") });
  for (const [selector, target, axis, delta] of [
    [".nav-resize", ".navigation", "width", 20],
    [".inspector-resize", ".task-inspector", "width", -20],
    [".log-resize", ".execution-logs", "height", -25],
    [".grid-row-resize", ".gpt-panel", "height", 30],
  ]) {
    const h = await page.locator(selector).boundingBox(),
      before = await page.locator(target).boundingBox();
    assert(h, selector);
    await page.mouse.move(
      h.x + Math.min(3, h.width / 2),
      h.y + Math.min(3, h.height / 2),
    );
    await page.mouse.down();
    await page.mouse.move(
      h.x + Math.min(3, h.width / 2) + (axis === "width" ? delta : 0),
      h.y + Math.min(3, h.height / 2) + (axis === "height" ? delta : 0),
      { steps: 4 },
    );
    await page.mouse.up();
    const after = await page.locator(target).boundingBox();
    assert(
      Math.abs(after[axis] - before[axis]) > 10,
      `${selector} must resize ${target}`,
    );
  }
  report.allSeparators = true;
  assert(
    await page.evaluate(() =>
      window.__sent.some(
        (m) => m.type === "layout" && m.layout.navWidth !== 220,
      ),
    ),
    "Actual drag must persist layout to the host",
  );
  await page
    .getByRole("button", { name: "最大化 / 恢复对话", exact: true })
    .first()
    .click();
  assert.equal(await page.locator(".chat-panel:not([hidden])").count(), 1);
  await page
    .getByRole("button", { name: "最大化 / 恢复对话", exact: true })
    .first()
    .click();
  await page
    .getByRole("button", { name: "折叠对话面板", exact: true })
    .first()
    .click();
  assert(
    await page
      .locator(".gpt-panel")
      .evaluate((e) => e.classList.contains("collapsed")),
  );
  await page
    .getByRole("button", { name: "折叠对话面板", exact: true })
    .first()
    .click();
  await page.getByRole("combobox", { name: "窗口布局" }).selectOption("dual");
  await theme(true);
  await page.screenshot({ path: path.join(out, "workspace-light.png") });
  await theme();
  await page.setViewportSize({ width: 390, height: 820 });
  assert(
    !(await page.locator(".task-inspector").isVisible()),
    "Task inspector must not obscure the primary chat on small screens",
  );
  await page
    .getByRole("tab", { name: "Claude Worker 01", exact: true })
    .click();
  assert(await page.locator('[data-panel-id="claude-01"]').isVisible());
  assert(!(await page.locator(".gpt-panel").isVisible()));
  await page.screenshot({ path: path.join(out, "workspace-mobile.png") });
  await page
    .getByRole("button", { name: "展开 / 折叠任务面板", exact: true })
    .click();
  assert(await page.locator(".task-inspector").isVisible());
  await page.getByRole("button", { name: "折叠任务面板", exact: true }).click();
  assert(!(await page.locator(".task-inspector").isVisible()));
  report.mobileInspector = true;
  assert.equal(errors.length, 0, errors.join("\n"));
  assert.equal(requests.length, 0, "Webview must not load a remote resource");
  report.layouts = true;
  report.maximizeCollapse = true;
  report.mobileTabs = true;
  await fs.writeFile(
    path.join(out, "browser-results.json"),
    JSON.stringify(report, null, 2),
  );
  await browser.close();
  console.log(JSON.stringify(report, null, 2));
})().catch((e) => {
  console.error(e);
  process.exitCode = 1;
});
