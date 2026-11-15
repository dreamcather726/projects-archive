// Screenshot an actual GPT host result with production Webview assets, never fixture chats.
const fs = require("node:fs/promises"), path = require("node:path"), assert = require("node:assert/strict");
const { pathToFileURL } = require("node:url");
process.env.PLAYWRIGHT_BROWSERS_PATH = path.resolve(".tools/ui-browsers");
const { chromium } = require("playwright");
const { defaultLayout } = require("../.tools/task02-test/api.cjs");
(async () => {
  assert(process.argv[2], "Provide the successful real GPT Host output directory");
  const host = path.resolve(process.argv[2]), out = path.resolve(".tools/task07103-ui");
  const evidence = JSON.parse(await fs.readFile(path.join(host, "stage2.json"), "utf8"));
  assert(evidence.realModel && evidence.sameSessionAfterRestart && evidence.packagedVersion === "0.7.1");
  const snapshot = JSON.parse(await fs.readFile(path.join(host, "runtime-snapshot.json"), "utf8"));
  assert(snapshot.chat.conversation.messages.some(m => m.role === "assistant" && m.status === "COMPLETED"));
  assert(!snapshot.dispatch.task, "This is a discussion-only test, not a fake task execution screenshot");
  snapshot.type = "snapshot";
  snapshot.layout = { ...defaultLayout(), logsCollapsed: true, inspectorWidth: 420 };
  await fs.mkdir(out, { recursive: true });
  const asset = f => pathToFileURL(path.resolve("src/webview", f)).href;
  const scripts = ["markdown.js", "workspace/layoutManager.js", "workspace/chatPanel.js", "workspace/reviewPanel.js", "workspace/permissionPanel.js", "workspace/repairPanel.js", "workspace/dispatchPanel.js", "workspace/workspaceRouter.js", "workspace/gptCommanderPanel.js", "workspace/workspace.js"];
  const html = '<!doctype html><html lang="zh-CN"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><meta http-equiv="Content-Security-Policy" content="default-src &apos;none&apos;; style-src file:; script-src &apos;nonce-gpt-ui&apos;;"><link rel="stylesheet" href="' + asset("workspace/workspace.css") + '"></head><body><main id="workspace"></main>' + scripts.map(f => '<script nonce="gpt-ui" src="' + asset(f) + '"></script>').join("") + '</body></html>';
  const htmlPath = path.join(out, "gpt-real.html"); await fs.writeFile(htmlPath, html);
  const browser = await chromium.launch({ headless: true }), page = await browser.newPage({ viewport: { width: 1680, height: 1100 } });
  const errors = [], remote = []; page.on("pageerror", e => errors.push(e.message));
  page.on("console", m => { if (m.type() === "error") errors.push(m.text()); });
  page.on("request", r => { if (/^https?:/.test(r.url())) remote.push(r.url()); });
  await page.addInitScript(() => { window.__sent = []; window.acquireVsCodeApi = () => ({ getState: () => ({}), setState: () => {}, postMessage: m => window.__sent.push(m) }); });
  await page.goto(pathToFileURL(htmlPath).href);
  await page.evaluate(s => window.dispatchEvent(new MessageEvent("message", { data: s })), snapshot);
  assert.equal(await page.locator(".gpt-panel .chat-message").count(), snapshot.chat.conversation.messages.length);
  assert.equal(await page.locator(".gpt-task-details .commander-card").count(), 6);
  assert.equal(await page.locator(".commander-progress").getAttribute("aria-valuenow"), null);
  await page.screenshot({ path: path.join(out, "gpt-real-conversation.png") });
  await page.locator(".task-inspector").screenshot({ path: path.join(out, "gpt-real-details.png") });
  const viewports = [];
  for (const width of [1280, 1024, 760, 640, 390, 320]) {
    await page.setViewportSize({ width, height: 900 });
    assert(await page.evaluate(() => document.documentElement.scrollWidth <= innerWidth)); viewports.push(width);
  }
  await page.setViewportSize({ width: 390, height: 900 });
  await page.screenshot({ path: path.join(out, "gpt-real-mobile.png") });
  assert.equal(errors.length, 0, errors.join("\n")); assert.equal(remote.length, 0);
  await fs.writeFile(path.join(out, "gpt-real-results.json"), JSON.stringify({ realModel: true, sourceHost: path.basename(host), packagedVersion: "0.7.1", messages: snapshot.chat.conversation.messages.length, taskWasNotDispatched: true, sixCards: true, noFabricatedProgress: true, viewports, errors, remote }, null, 2));
  await browser.close(); console.log("Actual GPT conversation screenshots passed: " + out);
})().catch(error => { console.error(error); process.exitCode = 1; });
