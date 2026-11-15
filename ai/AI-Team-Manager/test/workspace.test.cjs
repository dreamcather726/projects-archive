const { test } = require("node:test");
const assert = require("node:assert/strict");
const fs = require("node:fs");
const { JSDOM } = require("jsdom");
const {
  validateLayout,
  defaultLayout,
  WorkspaceTelemetry,
} = require("../.tools/task02-test/api.cjs");
function create(initialLayout = defaultLayout()) {
  const dom = new JSDOM('<main id="workspace"></main>', {
      runScripts: "outside-only",
    }),
    w = dom.window,
    sent = [];
  let saved;
  w.acquireVsCodeApi = () => ({
    postMessage: (m) => sent.push(m),
    getState: () => saved,
    setState: (s) => (saved = s),
  });
  for (const name of [
    "markdown.js",
    "workspace/layoutManager.js",
    "workspace/chatPanel.js",
    "workspace/reviewPanel.js",
    'workspace/permissionPanel.js',
    'workspace/repairPanel.js',
    'workspace/dispatchPanel.js',
    "workspace/workspaceRouter.js",
    "workspace/gptCommanderPanel.js", "workspace/workspace.js",
  ])
    w.eval(fs.readFileSync("src/webview/" + name, "utf8"));
  const state = {
    type: "snapshot",
    layout: initialLayout,
    projects: [{ name: "test" }],
    project: "test",
    logs: [
      {
        id: 1,
        time: "12:00:00",
        level: "INFO",
        source: "System",
        message: "初始化",
      },
      {
        id: 2,
        time: "12:01:00",
        level: "ERROR",
        source: "GPT",
        message: "test error",
      },
    ],
    diagnostics: [],
    chat: {
      projectName: "test",
      projectPath: "test",
      connection: "尚未检测",
      busy: false,
      conversation: { id: "a", messages: [], requests: [], status: "READY" },
    },
  };
  const update = () =>
    w.dispatchEvent(new w.MessageEvent("message", { data: state }));
  update();
  const byTitle = (title) =>
    w.document.querySelector(`[aria-label="${title}"]`);
  return { dom, w, sent, state, update, byTitle, getSaved: () => saved };
}
test("layout validation bounds malformed state and forbids duplicate GPT controllers", () => {
  const state = validateLayout({
    mode: "bad",
    split: Infinity,
    navWidth: -8,
    logHeight: 5000,
    panels: [
      { id: "gpt", kind: "claude-placeholder" },
      { id: "gpt", kind: "gpt" },
      { id: "gpt", kind: "gpt" },
      { id: "<script>", kind: "claude-placeholder" },
    ],
    activePanel: "bad",
    maximized: "bad",
  });
  assert.equal(state.panels.length, 1);
  assert.equal(state.panels[0].kind, "gpt");
  assert.equal(state.navWidth, 180);
  assert.equal(state.logHeight, 420);
  assert.equal(state.split, 58);
  assert.equal(state.maximized, null);
});
test('Worker 01 uses an independent Claude channel, clear display retains history, and stop targets the active chat',()=>{
  const {dom,w,sent,state,update,byTitle}=create();
  state.chat.conversation.messages=[{id:'g',role:'assistant',text:'GPT_ONLY',status:'COMPLETED',time:new Date().toISOString()}];
  state.claude={connectedAdapter:true,projectName:'test',projectPath:'test',busy:false,connection:'已连接 · 空闲',sessionCount:1,activities:[{id:'tool',name:'Edit',phase:'completed',detail:'motor.c'}],conversation:{id:'claude-conversation',threadId:'claude-session',status:'COMPLETED',messages:[{id:'c',role:'assistant',text:'CLAUDE_ONLY',status:'COMPLETED',time:new Date().toISOString()}],requests:[]}};update();
  const gpt=w.document.querySelector('.gpt-panel'),claude=w.document.querySelector('[data-panel-id="claude-01"]');assert(!claude.querySelector('textarea').disabled);assert(claude.textContent.includes('CLAUDE_ONLY'));assert(!claude.textContent.includes('GPT_ONLY'));assert(!gpt.textContent.includes('CLAUDE_ONLY'));assert(claude.textContent.includes('Edit · completed'));
  byTitle('清空 Claude 当前展示；保留 Session 与历史').click();assert(!claude.textContent.includes('CLAUDE_ONLY'));assert.equal(state.claude.conversation.messages.length,1);assert(claude.textContent.includes('claude-session'));
  const input=claude.querySelector('textarea');input.value='Claude task';input.dispatchEvent(new w.Event('input'));claude.querySelector('.primary-button').click();assert.equal(sent.at(-1).panelId,'claude-01');assert.equal(sent.at(-1).type,'send');
  state.claude.busy=true;update();claude.dispatchEvent(new w.Event('pointerdown',{bubbles:true}));byTitle('停止当前选中对话的响应（不支持断点暂停）').click();assert.equal(sent.at(-1).panelId,'claude-01');assert.equal(sent.at(-1).type,'stop');dom.window.close();
});
test("all layout modes, dynamic placeholders, maximize/collapse/remove and keyboard resize persist", () => {
  const { dom, w, sent, byTitle, getSaved } = create(),
    select = byTitle("窗口布局");
  for (const [mode, count] of [
    ["single", 1],
    ["dual", 2],
    ["triple", 3],
    ["quad", 4],
    ["auto", 4],
  ]) {
    select.value = mode;
    select.dispatchEvent(new w.Event("change"));
    assert.equal(w.document.querySelector(".chat-grid").dataset.mode, mode);
    assert.equal(
      w.document.querySelectorAll(".chat-panel:not([hidden])").length,
      count,
    );
  }
  byTitle("增加预留窗口；不会启动 Agent").click();
  assert.equal(w.document.querySelectorAll(".chat-panel").length, 5);
  byTitle("最大化 / 恢复对话").click();
  assert.equal(
    w.document.querySelectorAll(".chat-panel:not([hidden])").length,
    1,
  );
  byTitle("最大化 / 恢复对话").click();
  byTitle("折叠对话面板").click();
  assert(
    w.document.querySelector(".gpt-panel").classList.contains("collapsed"),
  );
  w.document
    .querySelector(".nav-resize")
    .dispatchEvent(new w.KeyboardEvent("keydown", { key: "ArrowRight" }));
  assert.equal(getSaved().layout.navWidth, 230);
  byTitle("移除此窗口（保留历史）").click();
  assert.equal(w.document.querySelectorAll(".gpt-panel").length, 0);
  byTitle("GPT Commander").click();
  assert.equal(w.document.querySelectorAll(".gpt-panel").length, 1);
  assert(sent.some((m) => m.type === "layout"));
  dom.window.close();
});
test("placeholder windows never borrow GPT messages or send; streaming keeps draft focus and safe Markdown", () => {
  const { dom, w, sent, state, update } = create(),
    input = w.document.querySelector(".gpt-panel textarea");
  input.focus();
  input.value = "draft";
  input.dispatchEvent(new w.Event("input"));
  update();
  assert.equal(w.document.activeElement, input);
  assert.equal(input.value, "draft");
  input.dispatchEvent(
    new w.KeyboardEvent("keydown", {
      key: "Enter",
      shiftKey: true,
      bubbles: true,
    }),
  );
  assert(!sent.some((m) => m.type === "send"));
  input.dispatchEvent(
    new w.KeyboardEvent("keydown", { key: "Enter", bubbles: true }),
  );
  assert.equal(sent.at(-1).panelId, "gpt");
  state.chat.conversation.messages = [
    {
      id: "m",
      role: "assistant",
      text: "**真实通道测试**\n<script>bad()</script>",
      time: new Date().toISOString(),
      status: "RUNNING",
    },
  ];
  update();
  assert(w.document.querySelector(".gpt-panel strong"));
  assert(!w.document.querySelector(".chat-message script"));
  assert.equal(
    w.document.querySelectorAll(".claude-panel .chat-message").length,
    0,
  );
  assert(w.document.querySelector(".claude-panel textarea").disabled);
  state.chat.error = "CLI 错误";
  state.chat.connection = "不可用";
  update();
  assert(
    w.document
      .querySelector(".gpt-panel .status-pill")
      .textContent.includes("连接异常"),
  );
  assert(
    w.document.querySelector(".chat-error").textContent.includes("CLI 错误"),
  );
  state.chat.projectPath = "other";
  update();
  assert.equal(input.value, "");
  assert(
    !JSON.stringify(w.acquireVsCodeApi().getState() || {}).includes("draft"),
  );
  dom.window.close();
});
test("logs filter/search/clear affect display only; task gate and real unknown states stay explicit", () => {
  const { dom, w, state, byTitle } = create();
  assert.equal(w.document.querySelectorAll(".log-line").length, 2);
  const level = byTitle("日志级别");
  level.value = "ERROR";
  level.dispatchEvent(new w.Event("input"));
  assert.equal(w.document.querySelectorAll(".log-line").length, 1);
  const search = byTitle("搜索日志");
  search.value = "absent";
  search.dispatchEvent(new w.Event("input"));
  assert.equal(w.document.querySelectorAll(".log-line").length, 0);
  search.value = "";
  search.dispatchEvent(new w.Event("input"));
  byTitle("清空日志显示，不删除会话或原始日志").click();
  assert.equal(w.document.querySelectorAll(".log-line").length, 0);
  assert.equal(state.logs.length, 2);
  assert(
    w.document.body.textContent.includes(
      "尚无验收标准 · 未验证",
    ),
  );
  assert(!w.document.querySelector(".commander-progress").hasAttribute("aria-valuenow"));
  assert(w.document.body.textContent.includes("路由：未检测"));
  assert(w.document.body.textContent.includes("Claude：未接入"));
  dom.window.close();
});
test('review UI displays structured issues as text and routes location/history independently',()=>{
  const {dom,w,sent,state,update,byTitle}=create();
  state.review={busy:false,report:{review_id:'R1',verdict:'REQUEST_CHANGES',summary:'Controlled UI rendering fixture',created_at:new Date().toISOString(),issues:[{issue_id:'B1-1',severity:'HIGH',category:'LOGIC',file_path:'worker.cjs',line_number:3,basis:'after',problem_description:'<img src=x onerror=alert(1)>',evidence:'return Math.abs(value);',impact:'wrong negative result',suggested_fix:'return 0',confidence:0.9,evidence_status:'VERIFIED'}],dimensions:[],missing_evidence:[],warnings:[]},history:[{review_id:'R1',verdict:'REQUEST_CHANGES',task:'UI test',created_at:'today'}]};update();
  const panel=w.document.querySelector('.code-review-panel');assert(panel.textContent.includes('HIGH 1'));assert(panel.textContent.includes('return Math.abs(value)'));assert.equal(panel.querySelectorAll('img').length,0);byTitle('定位 B1-1').click();assert.deepEqual(JSON.parse(JSON.stringify(sent.at(-1))),{type:'reviewOpenIssue',reviewId:'R1',issueId:'B1-1'});
  state.review.busy=true;update();assert(byTitle('审查结果').disabled);assert(!byTitle('停止审查').disabled);byTitle('停止审查').click();assert.equal(sent.at(-1).type,'reviewStop');assert.equal(state.chat.conversation.messages.length,0);dom.window.close();
});
test('permission UI shows real scope and pending operation, routes explicit decisions, and escapes content',()=>{
 const {dom,w,sent,state,update,byTitle}=create();
 state.claude={busy:true,permissions:{mode:'manual + task hooks',active:true,scope:{allowedFiles:['motor_control.c'],safeCommands:['node --test']},pending:[{id:'P1',tool:'Edit',operation:'CONFIG_WRITE',target:'package.json',reason:'审批',preview:'<script>alert(1)</script>'}],logs:[{time:'now',decision:'DENY',tool:'Edit',operation:'WRITE',target:'protocol.c',source:'POLICY',reason:'范围外'}]}};update();
 const panel=w.document.querySelector('.permission-panel');assert(panel.textContent.includes('motor_control.c'));assert(panel.textContent.includes('protocol.c'));assert(!panel.querySelector('script'));assert(byTitle('设置Claude任务授权范围').disabled);
 for(const [label,choice]of[['允许一次','ONCE'],['允许本次任务','TASK'],['拒绝','DENY']]){byTitle(`${label} P1`).click();assert.deepEqual(JSON.parse(JSON.stringify(sent.at(-1))),{type:'permissionAnswer',panelId:'claude-01',id:'P1',choice});}
 state.claude.busy=false;state.claude.permissions.pending=[];update();assert(!panel.textContent.includes('待审批'));byTitle('设置Claude任务授权范围').click();assert.equal(sent.at(-1).type,'permissions');dom.window.close();
});

test('repair UI keeps review associations, filters/selects issues, escapes text and approves the exact preview hash',()=>{
  const {dom,w,sent,state,update,byTitle}=create();
  const issues=[{issue_id:'I1',severity:'HIGH',file_path:'worker.cjs',line_number:3,basis:'after',evidence_status:'VERIFIED',problem_description:'<img src=x>'},{issue_id:'I2',severity:'LOW',file_path:'worker.cjs',line_number:null,basis:'after',evidence_status:'UNVERIFIED',problem_description:'needs manual evidence'}];
  state.review={report:{review_id:'REV',verdict:'REQUEST_CHANGES',summary:'UI fixture',issues,dimensions:[]}};state.repair={busy:false,dispositions:[]};update();
  const panel=w.document.querySelector('.repair-panel');assert(!panel.querySelector('img'));
  const boxes=panel.querySelectorAll('input[type=checkbox]');boxes[0].click();byTitle('生成修复方案').click();assert.equal(sent.at(-1).reviewId,'REV');assert.deepEqual([...sent.at(-1).issueIds],['I1']);
  boxes[1].click();assert(byTitle('生成修复方案').disabled);byTitle('人工处理').click();assert.equal(sent.at(-1).disposition,'HUMAN');assert.equal(sent.at(-1).issueIds.length,2);
  const filter=byTitle('修复问题严重程度');filter.value='HIGH';filter.dispatchEvent(new w.Event('change'));assert.equal(panel.querySelectorAll('input[type=checkbox]').length,1);
  state.repair.record={repair_id:'REP',review_id:'REV',state:'DRAFT',plan_hash:'IMMUTABLE_HASH',original_issues:issues.slice(0,1),plan:{title:'repair',issue_ids:['I1'],problem_description:'bug',root_cause:'cause',allowed_files:['worker.cjs'],forbidden_files:['test.cjs'],repair_strategy:'return 0',acceptance_criteria:['zero'],test_requirements:['node --test'],risk_level:'LOW',delete_files:[],public_interface_changes:[],missing_context:[]},approvals:[],checks:[]};update();
  assert(!byTitle('批准并发送').disabled);byTitle('批准并发送').click();assert.equal(sent.at(-1).type,'repairApprove');assert.equal(sent.at(-1).planHash,'IMMUTABLE_HASH');assert.equal(sent.at(-1).repairId,'REP');
  state.repair.record.sent_once=true;state.repair.record.state='FINISHED';update();assert(byTitle('批准并发送').disabled);assert(byTitle('GPT复审').disabled);dom.window.close();
});

test("telemetry reports actual request transitions once without copying chat transcripts", () => {
  const entries = [],
    telemetry = new WorkspaceTelemetry((...args) => entries.push(args));
  const chat = {
    projectPath: "p",
    conversation: { requests: [{ id: "r", status: "RUNNING" }] },
    activities: ["只读命令运行中"],
  };
  telemetry.observe(chat);
  telemetry.observe(chat);
  assert.equal(entries.length, 2);
  chat.conversation.requests[0].status = "COMPLETED";
  telemetry.observe(chat);
  assert.equal(entries.length, 3);
  assert(entries.at(-1)[0].includes("COMPLETED"));
});

test('dispatch UI displays real task scope and independently routes stop/config/report without HTML injection',()=>{
 const {dom,w,state,sent,update}=create();
 state.dispatch={mode:'GPT_HYBRID',max_rounds:2,busy:true,task:{task_id:'T',state:'RUNNING',input:{description:'<img src=x onerror=1>',allowed_files:['test.py'],forbidden_files:['protocol.c']},rounds:[{}],events:[{time:'now',kind:'GPT_TOOL_APPROVAL',data:{decision:'APPROVE'}}],session_id:'SESSION'}};update();
 const panel=w.document.querySelector('.dispatch-panel');assert(panel.textContent.includes('test.py'));assert(panel.textContent.includes('protocol.c'));assert(panel.textContent.includes('SESSION'));assert(!panel.querySelector('img'));
 const buttons=[...panel.querySelectorAll('button')];assert(buttons.find(b=>b.textContent==='审批模式 / 轮数').disabled);buttons.find(b=>b.textContent==='停止任务').click();assert.equal(sent.at(-1).type,'dispatchStop');buttons.find(b=>b.textContent==='完整记录').click();assert.equal(sent.at(-1).type,'dispatchReport');dom.window.close();
});

test("five registered routes change only presentation and preserve legacy panels and GPT draft", () => {
  const { dom, w, sent, byTitle, getSaved } = create();
  const input = w.document.querySelector(".gpt-panel textarea");
  input.value = "unsent route draft"; input.dispatchEvent(new w.Event("input"));
  const oldPanels = [...w.document.querySelectorAll(".chat-panel")];
  const checkpoint = sent.length;
  const routes = [["task-center", "任务调度中心"], ["review-integration", "代码审查与集成"], ["projects", "项目管理"], ["settings", "系统设置"]];
  for (const [id, title] of routes) {
    byTitle(title).click();
    assert.equal(getSaved().layout.navigation.route, id);
    assert.equal(w.document.querySelector(".navigation [aria-current=page]").dataset.route, id);
    assert.equal(w.document.querySelector(".route-outlet h1").textContent, title);
    assert(w.document.querySelector(".multi-chat-workspace").hidden);
    assert(w.document.querySelector(".task-inspector").hidden);
    assert.equal(w.document.querySelector(".route-outlet").hidden, false);
    assert(w.document.querySelector(".code-review-panel"));
    assert(w.document.querySelector(".repair-panel"));
  }
  byTitle("返回 AI 协同开发").click();
  assert.equal(getSaved().layout.navigation.route, "collaboration");
  assert.equal(w.document.querySelector(".gpt-panel textarea"), input);
  assert.equal(input.value, "unsent route draft");
  assert.deepEqual([...w.document.querySelectorAll(".chat-panel")], oldPanels);
  assert(sent.slice(checkpoint).every(m => m.type === "layout"));
  assert(byTitle("会话历史")); assert(byTitle("现有扩展设置"));
  dom.window.close();
});
test("session tabs use stable IDs; hide/reopen retains DOM and streaming state without execution messages", () => {
  const { dom, w, sent, state, update, byTitle, getSaved } = create();
  state.claude = { connectedAdapter: true, busy: true, conversation: { id: "existing", threadId: "stable-session", messages: [] } }; update();
  const panel = w.document.querySelector('[data-panel-id="claude-01"]');
  const input = panel.querySelector("textarea"); input.value = "keep worker draft"; input.dispatchEvent(new w.Event("input"));
  const checkpoint = sent.length;
  w.document.querySelector('[role=tab][data-session-id="claude-01"]').click();
  assert.equal(getSaved().layout.navigation.route, "collaboration/worker/claude-01");
  byTitle("关闭 Claude Worker 01 标签（后台任务继续）").click();
  assert(getSaved().layout.navigation.hiddenTabs.includes("claude-01"));
  assert.equal(getSaved().layout.navigation.route, "collaboration");
  assert.equal(w.document.querySelector('[data-panel-id="claude-01"]'), panel);
  state.claude.conversation.messages.push({ id: "chunk", role: "assistant", text: "background output", status: "RUNNING" }); update();
  assert(panel.textContent.includes("background output"));
  byTitle("重新打开 Claude Worker 01").click();
  assert.equal(w.document.querySelector('[data-panel-id="claude-01"]'), panel);
  assert.equal(input.value, "keep worker draft");
  assert.equal(state.claude.conversation.threadId, "stable-session");
  assert(sent.slice(checkpoint).every(m => m.type === "layout"));
  assert.equal(getSaved().layout.navigation.hiddenTabs.length, 0);
  dom.window.close();
});
test("route and closed tabs survive rehydration; version-one layouts migrate and invalid routes fail safely", () => {
  const initial = defaultLayout();
  initial.navigation = { route: "projects", hiddenTabs: ["claude-01"] };
  const { dom, w, byTitle, getSaved } = create(initial);
  assert.equal(w.document.querySelector(".workspace-shell").dataset.page, "projects");
  assert(w.document.querySelector('[data-session-id="claude-01"]').parentElement.hidden);
  byTitle("重新打开 Claude Worker 01").click();
  const restored = validateLayout(JSON.parse(JSON.stringify(getSaved().layout)));
  assert.equal(restored.navigation.route, "collaboration/worker/claude-01");
  assert.equal(restored.activePanel, "claude-01");
  assert.equal(validateLayout({ ...initial, navigation: { route: "collaboration/worker/unknown", hiddenTabs: ["x", "gpt", "gpt"] } }).navigation.route, "collaboration");
  const migrated = { ...defaultLayout(), activePanel: "claude-01" }; delete migrated.navigation;
  assert.equal(validateLayout(migrated).navigation.route, "collaboration/worker/claude-01");
  for (const route of ["projects", "collaboration", "collaboration/worker/claude-01", "collaboration/worker/unknown"]) {
    const value = { route, hiddenTabs: ["gpt", "claude-01", "gpt", "unknown"] };
    assert.equal(JSON.stringify(w.ATMWorkspaceRouter.normalize(value, initial.panels)),
      JSON.stringify(validateLayout({ ...initial, navigation: value }).navigation));
  }
  const bad = { ...initial, navigation: { route: "<script>", hiddenTabs: ["x", "gpt", "gpt"] } };
  assert.equal(JSON.stringify(w.ATMWorkspaceRouter.normalize(bad.navigation, bad.panels)), JSON.stringify(validateLayout(bad).navigation));
  dom.window.close();
});
test("dynamic session slots route by ID after reorder; keyboard tab switching emits layout only", () => {
  const initial = defaultLayout();
  initial.panels.push({ id: "worker-stable-b", kind: "claude-placeholder", title: "Worker slot B", collapsed: false });
  const { dom, w, sent, getSaved } = create(initial);
  const checkpoint = sent.length;
  w.document.querySelector('[data-session-id="worker-stable-b"]').click();
  assert.equal(getSaved().layout.navigation.route, "collaboration/worker/worker-stable-b");
  assert.equal(getSaved().layout.activePanel, "worker-stable-b");
  assert(w.document.querySelector('[data-panel-id="worker-stable-b"] textarea').disabled);
  const tab = w.document.querySelector('[data-session-id="worker-stable-b"]'); tab.focus();
  tab.dispatchEvent(new w.KeyboardEvent("keydown", { key: "Home", bubbles: true }));
  assert.equal(getSaved().layout.activePanel, "gpt");
  assert.equal(w.document.activeElement.dataset.sessionId, "gpt");
  assert(sent.slice(checkpoint).every(m => m.type === "layout"));
  const r = new w.ATMWorkspaceRouter();
  assert.equal(r.resolve("collaboration/worker/worker-stable-b", [...initial.panels].reverse()).sessionId, "worker-stable-b");
  assert.throws(() => r.register({ id: "projects", title: "duplicate" }));
  dom.window.close();
});

function commanderTask(state, overrides = {}) {
  return {
    task_id: "task-fixture", owner_id: state.chat.conversation.id, project_path: state.chat.projectPath,
    state: "RUNNING", session_id: "worker-session", conversation_id: "worker-conversation",
    input: { description: "UI fixture: add tests", allowed_files: ["test.py"], forbidden_files: ["package.json"], acceptance_criteria: ["Four test cases pass"] },
    rounds: [{ plan: { goal: "Fixture goal", approach: "Use unittest", steps: ["Read", "Implement", "Test"], risks: [] }, tests: [] }], events: [], ...overrides,
  };
}
test("GPT details show six truthful empty states and never treat validated planning slots as running Workers", () => {
  const { dom, w, state, update } = create();
  assert.equal(w.document.querySelectorAll(".gpt-task-details .commander-card").length, 6);
  assert(!w.document.querySelector(".commander-progress").hasAttribute("aria-valuenow"));
  state.dispatch = { planning: { note: "Safety gate remains closed", plans: [{ plan_id: "plan-only", project_path: state.chat.projectPath, owner_id: state.chat.conversation.id, goal: "UI planning fixture", worker_count: 2, tasks: [{ task_id: "A", assigned_worker: "worker-a", task_description: "Module A", dependencies: [] }, { task_id: "B", assigned_worker: "worker-b", task_description: "Module B", dependencies: ["A"] }] }] } }; update();
  assert(w.document.querySelector(".commander-workers").textContent.includes("未启动执行"));
  assert.equal(w.document.querySelectorAll(".commander-worker").length, 0);
  assert(w.document.querySelector(".commander-steps").textContent.includes("依赖：A"));
  assert(w.document.querySelector(".commander-steps").textContent.includes("仅规划 · 尚未执行"));
  assert(!w.document.querySelector(".commander-progress").hasAttribute("aria-valuenow"));
  assert(w.document.querySelector(".commander-risks").textContent.includes("Safety gate remains closed"));
  dom.window.close();
});
test("GPT task summary, scope, approvals and Worker link use the existing real snapshot IDs without model messages", () => {
  const { dom, w, state, sent, update, byTitle, getSaved } = create();
  state.claude = { connectedAdapter: true, projectPath: state.chat.projectPath, conversation: { id: "worker-conversation", threadId: "worker-session", messages: [{ id: "private", role: "assistant", text: "CLAUDE_EXECUTION_ONLY" }] } };
  state.dispatch = { task: commanderTask(state), pending: { task_id: "task-fixture", reason: "Exact file scope approval" } }; update();
  const text = w.document.querySelector(".gpt-task-details").textContent;
  assert(text.includes("task-fixture")); assert(text.includes("执行中")); assert(text.includes("后端未提供"));
  assert(w.document.querySelector(".phase-list p").textContent.includes("RUNNING"));
  assert(text.includes("Exact file scope approval"));
  assert(w.document.querySelector(".gpt-panel .commander-notices").textContent.includes("禁止文件：package.json"));
  assert(!w.document.querySelector(".gpt-panel").textContent.includes("CLAUDE_EXECUTION_ONLY"));
  const checkpoint = sent.length;
  byTitle("查看当前任务的 Claude Worker 01").click();
  assert.equal(getSaved().layout.navigation.route, "collaboration/worker/claude-01");
  assert(sent.slice(checkpoint).every(m => m.type === "layout"));
  assert.equal(state.claude.conversation.threadId, "worker-session");
  byTitle("GPT Commander").click();
  byTitle("查看当前 GPT 派发任务记录").click(); assert.equal(sent.at(-1).type, "dispatchReport");
  state.claude.conversation.threadId = "new-session"; update();
  assert(byTitle("查看当前任务的 Claude Worker 01").disabled);
  dom.window.close();
});
test("GPT never infers per-step or per-criterion success from a review or a plan; acceptance needs current test evidence", () => {
  const { dom, w, state, update } = create();
  const task = commanderTask(state, { state: "PASSED" });
  task.rounds[0].review = { verdict: "PASS", summary: "Review fixture" };
  state.dispatch = { task }; update();
  assert(!w.document.querySelector(".commander-progress").hasAttribute("aria-valuenow"));
  task.rounds[0].tests.push({ command: "python -B test.py", status: "FAILED" }); update();
  assert(!w.document.querySelector(".commander-progress").hasAttribute("aria-valuenow"));
  assert(w.document.querySelector(".commander-risks").textContent.includes("FAILED"));
  task.rounds[0].tests[0].status = "PASSED"; update();
  assert.equal(w.document.querySelector(".commander-progress").getAttribute("aria-valuenow"), "100");
  assert.equal(w.document.querySelector(".criterion").dataset.result, "UNVERIFIED");
  assert(w.document.querySelector(".commander-steps").textContent.includes("步骤结果未验证"));
  task.rounds[0].review.verdict = "NEEDS_HUMAN_REVIEW"; update();
  assert(!w.document.querySelector(".commander-progress").hasAttribute("aria-valuenow"));
  assert(w.document.querySelector(".commander-acceptance").textContent.includes("需要人工确认"));
  dom.window.close();
});
test("GPT rejects other projects, other GPT owners and unrelated review reports instead of mixing task contexts", () => {
  const { dom, w, state, update } = create();
  const task = commanderTask(state);
  state.dispatch = { task }; update();
  assert(w.document.querySelector(".commander-summary").textContent.includes("task-fixture"));
  task.owner_id = "other-gpt"; update(); assert(!w.document.querySelector(".gpt-task-details").textContent.includes("task-fixture"));
  task.owner_id = state.chat.conversation.id; task.project_path = "other-project"; update();
  assert(!w.document.querySelector(".gpt-panel .commander-notices").textContent.includes("task-fixture"));
  task.project_path = state.chat.projectPath;
  state.review = { report: { task: { task_id: "unrelated" }, project_path: task.project_path, verdict: "PASS", summary: "DO_NOT_ATTACH" } }; update();
  assert(!w.document.querySelector(".gpt-panel").textContent.includes("DO_NOT_ATTACH"));
  state.chat.conversation.id = "new-gpt-session"; update();
  assert(w.document.querySelector(".commander-summary").textContent.includes("尚未建立任务"));
  dom.window.close();
});
test("GPT connection status distinguishes local CLI detection, connecting, actual model activity, idle and errors", () => {
  const { dom, w, state, update, byTitle, sent } = create();
  const read = () => w.document.querySelector(".gpt-panel .status-pill");
  state.chat.connection = "codex-cli fixture"; update(); assert.equal(read().dataset.connection, "idle"); assert(read().textContent.includes("模型未验证"));
  state.chat.busy = true; update(); assert.equal(read().dataset.connection, "connecting");
  assert(byTitle("GPT: 历史会话").disabled);
  state.chat.connection = "已连接 · 正在响应"; update(); assert.equal(read().dataset.connection, "online");
  byTitle("停止当前响应").click(); assert.equal(sent.at(-1).type, "stop"); assert.equal(sent.at(-1).panelId, "gpt");
  state.chat.busy = false; state.chat.connection = "已连接 · 空闲"; update(); assert(read().textContent.includes("空闲"));
  for (const [label, type] of [["历史会话", "history"], ["新会话", "new"], ["选择项目", "project"], ["检测 CLI", "check"]]) {
    byTitle("GPT: " + label).click(); assert.equal(sent.at(-1).type, type); assert.equal(sent.at(-1).panelId, "gpt");
  }
  state.chat.error = "UI error fixture"; update(); assert.equal(read().dataset.connection, "error");
  dom.window.close();
});
test("GPT typed notices escape output, and detail rendering failure cannot block chat streaming or lose drafts", () => {
  const { dom, w, state, sent, update } = create();
  state.dispatch = { task: commanderTask(state) };
  state.dispatch.task.rounds[0].plan.goal = "<img src=x onerror=1>";
  state.dispatch.task.events.push({ kind: "GPT_TOOL_APPROVAL", data: { result: { decision: "DENY", reason: "Denied fixture" } } }); update();
  assert.equal(w.document.querySelectorAll(".commander-notices img").length, 0);
  assert(w.document.querySelector(".commander-risks").textContent.includes("Denied fixture"));
  const input = w.document.querySelector(".gpt-panel textarea"); input.value = "retained draft"; input.focus();
  input.dispatchEvent(new w.Event("input"));
  input.dispatchEvent(new w.KeyboardEvent("keydown", { key: "Enter", isComposing: true, bubbles: true }));
  assert(!sent.some(m => m.type === "send"));
  const original = w.ATMGPTCommanderPanel.prototype.update;
  w.ATMGPTCommanderPanel.prototype.update = () => { throw new Error("Injected detail renderer failure"); };
  state.chat.conversation.messages.push({ id: "stream", role: "assistant", text: "Partial streamed response", time: new Date().toISOString(), status: "RUNNING" }); update();
  assert(w.document.querySelector(".gpt-panel").textContent.includes("Partial streamed response"));
  assert.equal(w.document.activeElement, input); assert.equal(input.value, "retained draft");
  assert(w.document.querySelector(".gpt-task-details [role=alert]").textContent.includes("暂时无法显示"));
  w.ATMGPTCommanderPanel.prototype.update = original; update();
  state.chat.conversation.id = "second-gpt-session"; update(); assert.equal(input.value, "");
  dom.window.close();
});
