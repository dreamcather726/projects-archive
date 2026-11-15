(() => {
  const vscode = acquireVsCodeApi(),
    { el, button } = window.ATMElements;
  const saved = vscode.getState() || {},
    root = document.getElementById("workspace");
  let layout,
    snapshot,
    initialized = false,
    lastLogRender = "",
    clearedThrough = saved.clearedThrough || 0,
    paused = !!saved.paused;
  const panels = new Map(),
    drafts = {}; // Drafts stay in this webview, never in layout or another panel.
  const post = (message) => vscode.postMessage(message);
  const persist = () =>
    vscode.setState({ layout: layout.state, clearedThrough, paused });
  const shell = el("div", undefined, "workspace-shell");
  root.append(shell);
  const nav = el("nav", undefined, "navigation");
  nav.setAttribute("aria-label", "工作台导航");
  const brand = el("div", undefined, "nav-brand");
  brand.append(
    el("span", "◈", "brand-mark"),
    el("strong", "AI Team Manager"),
    el("small", "研发协作工作台"),
  );
  nav.append(brand);
  const router = new window.ATMWorkspaceRouter();
  const navButtons = new Map();
  const navLinks = el("div", undefined, "nav-links");
  for (const page of router.pages.values()) {
    const item = button("", page.title, () => navigate(page.id), "nav-link");
    item.dataset.route = page.id;
    item.append(el("span", page.icon, "nav-icon"), el("span", page.title));
    navButtons.set(page.id, item);
    navLinks.append(item);
  }
  nav.append(navLinks);
  const shortcuts = el("details", undefined, "legacy-shortcuts");
  shortcuts.open = true;
  shortcuts.append(el("summary", "已有功能"));
  for (const [title, label, action] of [
    ["GPT Commander", "◎ GPT 总指挥", () => focus("gpt")],
    ["Claude Agents", "✳ Claude Worker 01", () => focus("claude-01")],
    ["Tasks", "☷ 当前业务面板", () => {
      focus("gpt"); layout.state.inspectorCollapsed = false;
      shell.classList.add("inspector-open"); layout.persist(); inspector.focus();
    }],
    ["Logs", "⌘ 执行日志", () => {
      focus("gpt"); layout.state.logsCollapsed = false; layout.persist(); search.focus();
    }],
    ["会话历史", "◷ 会话历史", () => post({ type: "history", panelId: "gpt" })],
    ["现有扩展设置", "⚙ 现有扩展设置", () => post({ type: "settings" })],
  ]) shortcuts.append(button(label, title, action, "legacy-link"));
  nav.append(shortcuts);
  const projectList = el("section", undefined, "project-list");
  projectList.append(el("h3", "当前项目"));
  const projectItems = el("div");
  projectList.append(
    projectItems,
    button(
      "＋ 选择项目",
      "选择真实工作区项目",
      () => post({ type: "project", panelId: "gpt" }),
      "text-button",
    ),
  );
  nav.append(projectList);
  const phase = el("section", undefined, "phase-list");
  phase.append(el("h3", "任务阶段"), el("p", "尚未建立任务", "muted-text"));
  for (const label of ["已完成", "正在执行", "待执行"]) {
    const line = el("div", undefined, "count-line");
    line.append(el("span", label), el("span", "—"));
    phase.append(line);
  }
  nav.append(
    phase,
    el("p", "真实会话 · GPT受控派发\n任务与工具执行前审批。", "nav-footnote"),
  );
  const navHandle = el("div", undefined, "resize-handle nav-resize");
  navHandle.title = "拖动调整导航宽度";
  const header = el("header", undefined, "topbar");
  const identity = el("div", undefined, "top-identity");
  identity.append(
    el("span", "◈", "brand-mark"),
    el("strong", "双 AI 协同开发平台"),
  );
  const project = button(
    "未选择项目",
    "选择项目",
    () => post({ type: "project", panelId: "gpt" }),
    "project-button",
  );
  const statuses = el("div", undefined, "connection-statuses"),
    route = el("span", "路由：未检测", "status-chip"),
    gptStatus = el("span", "GPT：未检测", "status-chip"),
    claudeStatus = el("span", "Claude：未接入", "status-chip");
  route.title = "本插件不读取或修改 CC Switch 配置，无法确认其路由状态";
  statuses.append(route, gptStatus, claudeStatus);
  const controls = el("div", undefined, "top-controls");
  const start = button(
    "▷ 启动",
    "检测 Codex CLI 并打开 GPT 会话；不自动发送消息",
    () => {
      const id = activeChatId();
      focus(id);
      post({ type: "check", panelId: id });
    },
    "primary-button",
  );
  const pause = button(
    "Ⅱ 暂停",
    "停止当前选中对话的响应（不支持断点暂停）",
    () => post({ type: "stop", panelId: activeChatId() }),
    "secondary-button",
  );
  const resume = button(
    "▷ 继续",
    "回到当前选中的会话，输入下一条消息继续",
    () => {
      const id = activeChatId();
      focus(id);
      panels.get(id)?.input.focus();
    },
    "secondary-button",
  );
  const more = el("details", undefined, "more-menu");
  more.append(el("summary", "•••"));
  const menu = el("div", undefined, "menu-popover");
  [
    ["检测环境", () => post({ type: "diagnose" })],
    ["GPT 历史", () => post({ type: "history", panelId: "gpt" })],
    ["新 GPT 会话", () => post({ type: "new", panelId: "gpt" })],
    ["恢复默认布局", () => layout.reset()],
    ["设置", () => post({ type: "settings" })],
  ].forEach(([title, fn]) =>
    menu.append(
      button(
        title,
        title,
        () => {
          more.open = false;
          fn();
        },
        "menu-item",
      ),
    ),
  );
  more.append(menu);
  controls.append(start, pause, resume, more);
  header.append(identity, project, statuses, controls);
  const center = el("section", undefined, "multi-chat-workspace");
  center.setAttribute("aria-label", "多 AI 对话工作区");
  const toolbar = el("div", undefined, "workspace-toolbar");
  const heading = el("div");
  const pageTitle = el("strong", "AI 协同开发");
  const pageStatus = el("span", " /  GPT 总指挥", "muted-text");
  heading.append(pageTitle, pageStatus);
  const layoutControls = el("div", undefined, "layout-controls");
  const select = el("select");
  select.setAttribute("aria-label", "窗口布局");
  for (const [value, title] of [
    ["single", "单窗口"],
    ["dual", "双窗口并排"],
    ["triple", "三窗口"],
    ["quad", "四窗口"],
    ["auto", "自适应网格"],
  ]) {
    const option = el("option", title);
    option.value = value;
    select.append(option);
  }
  select.addEventListener("change", () => layout.setMode(select.value));
  const add = button(
    "＋ 窗口",
    "增加预留窗口；不会启动 Agent",
    () => layout.add(),
    "secondary-button",
  );
  const inspectorToggle = button("▤", "展开 / 折叠任务面板", () => {
    if(window.matchMedia?.('(max-width:1200px)').matches){
      const open=!shell.classList.contains('inspector-open');layout.state.inspectorCollapsed=!open;shell.classList.toggle('inspector-open',open);
    }else layout.state.inspectorCollapsed=!layout.state.inspectorCollapsed;
    layout.persist();
  });
  layoutControls.append(select, add, inspectorToggle);
  toolbar.append(heading, layoutControls);
  const tabs = el("div", undefined, "panel-tabs");
  tabs.setAttribute("role", "tablist");
  tabs.setAttribute("aria-label", "对话窗口");
  const grid = el("div", undefined, "chat-grid");
  const columnHandle = el("div", undefined, "resize-handle grid-column-resize"),
    rowHandle = el("div", undefined, "resize-handle grid-row-resize");
  columnHandle.title = "调整对话窗口宽度";
  rowHandle.title = "调整对话窗口高度";
  const sessionBar = el("section", undefined, "session-bar");
  sessionBar.setAttribute("aria-label", "顶部会话标签");
  const reopen = el("details", undefined, "session-reopen more-menu");
  const reopenMenu = el("div", undefined, "menu-popover");
  reopen.append(el("summary", "+ 会话"), reopenMenu);
  sessionBar.append(tabs, reopen);
  center.append(toolbar, grid, columnHandle, rowHandle);
  const outlet = el("section", undefined, "route-outlet");
  outlet.tabIndex = -1;
  const outletTitle = el("h1");
  outlet.append(el("span", "页面框架 · Task 07.10.2", "route-kicker"), outletTitle,
    el("p", "路由已就绪，业务内容将在后续阶段接入。", "muted-text"),
    button("返回 AI 协同开发", "返回 AI 协同开发", () => navigate("collaboration"), "primary-button"));
  const futurePages=window.ATMFuturePages?.create(()=>navigate('collaboration'),post,id=>focus(id));
  if(futurePages){outlet.replaceChildren();for(const page of futurePages.values())outlet.append(page.node);}
  tabs.addEventListener("keydown", event => {
    const items = [...tabs.querySelectorAll('[role="tab"]')];
    const index = items.indexOf(document.activeElement);
    if (index < 0 || !["ArrowLeft", "ArrowRight", "Home", "End"].includes(event.key)) return;
    event.preventDefault();
    const next = event.key === "Home" ? 0 : event.key === "End" ? items.length - 1 :
      (index + (event.key === "ArrowRight" ? 1 : -1) + items.length) % items.length;
    items[next].click(); tabs.querySelectorAll('[role="tab"]')[next]?.focus();
  });
  const inspector = el("aside", undefined, "task-inspector");
  inspector.tabIndex = -1;
  inspector.setAttribute("aria-label", "当前任务信息");
  const inspectorHeader = el("header", undefined, "section-heading");
  inspectorHeader.append(
    el("span", "▱", "accent"),
    el("h2", "任务详情"),
    button("›", "折叠任务面板", () => {
      layout.state.inspectorCollapsed = true;
      layout.persist();
    }),
  );
  const commander = new window.ATMGPTCommanderPanel(post, id => focus(id));
  const legacyDetails = el("details", undefined, "commander-legacy");
  legacyDetails.append(el("summary", "业务操作与历史"));
  const agentList = el("section", undefined, "agent-list");
  agentList.append(el("h3", "会话状态"));
  const gptAgent = el("div", undefined, "agent-row"),
    claudeAgent = el("div", undefined, "agent-row");
  agentList.append(gptAgent, claudeAgent);
  const stats = el("div", undefined, "stat-cards"),
    tokenCard = el("div", undefined, "stat-card"),
    claudeCard = el("div", undefined, "stat-card");
  stats.append(tokenCard, claudeCard);
  const reviewPanel = window.ATMReviewPanel ? new window.ATMReviewPanel(post) : undefined;
  const permissionPanel = window.ATMPermissionPanel ? new window.ATMPermissionPanel(post) : undefined;
  const repairPanel = window.ATMRepairPanel ? new window.ATMRepairPanel(post) : undefined;
  const dispatchPanel = window.ATMDispatchPanel ? new window.ATMDispatchPanel(post) : undefined;
  inspector.append(inspectorHeader, commander.node, legacyDetails);
  legacyDetails.append(agentList, stats);
  if (dispatchPanel) legacyDetails.append(dispatchPanel.node);
  if (permissionPanel) legacyDetails.append(permissionPanel.node);
  if (reviewPanel) legacyDetails.append(reviewPanel.node);
  if (repairPanel) legacyDetails.append(repairPanel.node);
  const inspectorHandle = el(
    "div",
    undefined,
    "resize-handle inspector-resize",
  );
  inspectorHandle.title = "拖动调整任务面板宽度";
  const logs = el("section", undefined, "execution-logs");
  logs.setAttribute("aria-label", "执行日志");
  const logHandle = el("div", undefined, "resize-handle log-resize");
  logHandle.title = "拖动调整日志高度";
  const logHeader = el("header", undefined, "log-header");
  const logToggle = button("⌄", "展开 / 收起日志", () => {
    layout.state.logsCollapsed = !layout.state.logsCollapsed;
    layout.persist();
  });
  logHeader.append(
    logToggle,
    el("h2", "执行日志"),
    el("span", "只清空显示，保留真实记录", "log-note"),
  );
  const filters = el("div", undefined, "log-filters");
  const level = el("select"),
    source = el("select");
  level.setAttribute("aria-label", "日志级别");
  source.setAttribute("aria-label", "日志来源");
  for (const value of ["全部级别", "ERROR", "WARN", "INFO", "DEBUG"])
    level.append(el("option", value));
  for (const value of ["全部来源", "GPT", "Claude", "System", "Tool", "Test"])
    source.append(el("option", value));
  const search = el("input");
  search.type = "search";
  search.placeholder = "搜索日志…";
  search.setAttribute("aria-label", "搜索日志");
  const scrollToggle = button(paused ? "▷" : "Ⅱ", "暂停 / 继续自动滚动", () => {
    paused = !paused;
    scrollToggle.textContent = paused ? "▷" : "Ⅱ";
    scrollToggle.setAttribute("aria-pressed", String(paused));
    persist();
    renderLogs();
  });
  scrollToggle.setAttribute("aria-pressed", String(paused));
  const clear = button("⌫", "清空日志显示，不删除会话或原始日志", () => {
    clearedThrough = Math.max(
      clearedThrough,
      ...(snapshot.logs || []).map((log) => log.id),
    );
    persist();
    renderLogs();
  });
  filters.append(level, source, search, scrollToggle, clear);
  logHeader.append(filters);
  const logBody = el("div", undefined, "log-body");
  logBody.setAttribute("role", "log");
  logBody.setAttribute("aria-live", "off");
  logs.append(logHandle, logHeader, logBody);
  [level, source, search].forEach((input) =>
    input.addEventListener("input", renderLogs),
  );
  shell.append(
    nav,
    navHandle,
    header,
    sessionBar,
    outlet,
    center,
    inspectorHandle,
    inspector,
    logs,
  );
  const workerPanel=window.ATMWorkerPanel?new window.ATMWorkerPanel(post,id=>focus(id)):undefined;
  if(workerPanel)grid.append(workerPanel.node);
  function currentTeam(){const team=snapshot?.dispatch?.team;return team?.workflow?.plan.project_path===snapshot?.chat?.projectPath&&team.workflow.plan.owner_id===snapshot?.chat?.conversation?.id?team:undefined;}
  function selectTeamWorker(id){
    const team=currentTeam();if(!team)return;
    const worker=team.workflow.workers.find(w=>w.worker_id===id);
    const key=team.workflow.plan.plan_id+':'+id;
    if(worker&&team.selected_worker!==id&&requestedWorker!==key){requestedWorker=key;post({type:'teamSelect',projectPath:team.workflow.plan.project_path,planId:team.workflow.plan.plan_id,workerId:id,...workerPanel.selection(id)});}
    else if(!worker&&team.selected_worker){requestedWorker=undefined;post({type:'teamSelect',projectPath:team.workflow.plan.project_path,planId:team.workflow.plan.plan_id});}
  }
  let requestedWorker;
  function focus(id) {
    workerPanel?.save();selectTeamWorker(id);
    layout.state.navigation.route = window.ATMWorkspaceRouter.forSession(id);
    layout.state.navigation.hiddenTabs = layout.state.navigation.hiddenTabs.filter(value => value !== id);
    if (!layout.state.panels.some((p) => p.id === id) && id === "claude-01") {
      layout.state.panels.push({
        id,
        kind: "claude-placeholder",
        title: "Claude Worker 01",
        collapsed: false,
      });
    }
    layout.focus(id);
  }
  function navigate(route) {
    if (route === "collaboration") return focus("gpt");
    layout.state.navigation.route = router.resolve(route, layout.state.panels).page.id;
    layout.persist();
  }
  function closeTab(id) {
    const state = layout.state;
    const remaining = state.panels.filter(p => p.id !== id && !state.navigation.hiddenTabs.includes(p.id));
    // Always retain one visible session; hiding a tab never removes its panel or execution state.
    if (!remaining.length) return;
    state.navigation.hiddenTabs.push(id);
    if (state.activePanel === id) {
      state.activePanel = remaining[0].id;
      if (router.resolve(state.navigation.route, state.panels).sessionId)
        state.navigation.route = window.ATMWorkspaceRouter.forSession(state.activePanel);
      state.maximized = null;
      selectTeamWorker(state.activePanel);
    }
    layout.persist();
  }
  let lastActivePanel;
  function syncLayout() {
    if (!snapshot) return;
    const state = layout.state;
    if (lastActivePanel && lastActivePanel !== state.activePanel && state.navigation.route.startsWith("collaboration"))
      state.navigation.route = window.ATMWorkspaceRouter.forSession(state.activePanel);
    state.navigation = window.ATMWorkspaceRouter.normalize(state.navigation, state.panels, state.activePanel);
    const current = router.resolve(state.navigation.route, state.panels);
    if (current.sessionId && state.panels.some(p => p.id === current.sessionId)) state.activePanel = current.sessionId;
    lastActivePanel = state.activePanel;
    shell.dataset.page = current.page.id;
    const collaboration = current.page.id === "collaboration";
    const team=currentTeam(),isWorker=!!team?.workflow.workers.some(w=>w.worker_id===state.activePanel);
    shell.classList.toggle('worker-active',collaboration&&isWorker);
    if(workerPanel){workerPanel.node.hidden=!collaboration||!isWorker;if(isWorker)try{workerPanel.update(team,state.activePanel);}catch{workerPanel.key=undefined;workerPanel.body.replaceChildren(el('p','详情渲染失败，后台任务继续。可切换会话后重试。','commander-risk'));}}
    center.hidden = inspector.hidden = logs.hidden = !collaboration;
    commander.node.hidden = current.sessionId !== "gpt";
    inspectorHandle.hidden = !collaboration;
    outlet.hidden = collaboration;
    outletTitle.textContent = current.page.title;
    outlet.dataset.route = current.page.id;
    if(futurePages)for(const [id,page] of futurePages)page.node.hidden=id!==current.page.id;
    for (const [id, item] of navButtons) {
      item.classList.toggle("selected", id === current.page.id);
      if (id === current.page.id) item.setAttribute("aria-current", "page"); else item.removeAttribute("aria-current");
    }
    pageTitle.textContent = current.page.title;
    const selected = state.panels.find(p => p.id === state.activePanel);
    pageStatus.textContent = " /  " + (selected?.title || "GPT 总指挥");
    select.value = state.mode;
    add.disabled = state.panels.length >= 12;
    logToggle.textContent = state.logsCollapsed ? "⌃" : "⌄";
    inspectorToggle.setAttribute(
      "aria-pressed",
      String(!state.inspectorCollapsed),
    );
    for (const [id, panel] of panels)
      if (!state.panels.some((p) => p.id === id)) {
        panel.node.remove();
        panels.delete(id);
        delete drafts[id];
      }
    let visible =
      state.mode === "auto"
        ? state.panels.slice()
        : state.panels.slice(
            0,
            { single: 1, dual: 2, triple: 3, quad: 4 }[state.mode],
          );
    const active = state.panels.find((p) => p.id === state.activePanel);
    if (active && !visible.includes(active))
      visible[visible.length - 1] = active;
    if (state.maximized)
      visible = state.panels.filter((p) => p.id === state.maximized);
    visible = visible.filter(p => !state.navigation.hiddenTabs.includes(p.id));
    // Real Worker conversations are always independent; legacy layout options remain available for existing panels.
    if(isWorker)visible=[];
    else visible=visible.filter(p=>p.kind!=='team-worker');
    grid.dataset.count = String(visible.length);
    grid.dataset.mode = state.maximized ? "single" : state.mode;
    tabs.replaceChildren();
    reopenMenu.replaceChildren();
    state.panels.forEach((slot) => {
      let panel = panels.get(slot.id);
      if(slot.kind==='team-worker')panel={node:workerPanel.node};
      if (!panel) {
        panel = new window.ATMChatPanel(
          slot,
          post,
          {
            collapse: (id) => layout.toggleCollapse(id),
            maximize: (id) => layout.maximize(id),
            remove: (id) => layout.remove(id),
            activate: (id) => {
              if (layout.state.activePanel !== id) focus(id);
            },
            draft: (id, text) => {
              drafts[id] = text;
            },
          },
          drafts[slot.id],
        );
        panels.set(slot.id, panel);
        grid.append(panel.node);
      }
      if(slot.kind!=='team-worker'){
      panel.node.hidden = !visible.includes(slot);
      panel.node.style.order = String(state.panels.indexOf(slot));
      panel.update(
        slot,
        slot.id === "claude-01" ? snapshot.claude || {} : snapshot.chat || {},
        state,
        snapshot.dispatch?.task,
      );
      if (slot.kind === "gpt") panel.setCommanderContext(commander.timeline);
      }
      const tab = button(
        slot.title,
        slot.title,
        () => {
          if (state.maximized && state.maximized !== slot.id)
            state.maximized = slot.id;
          focus(slot.id);
        },
        "panel-tab",
      );
      if(slot.kind!=='team-worker')panel.node.id = "session-" + slot.id;
      tab.dataset.sessionId = slot.id;
      tab.setAttribute("role", "tab");
      tab.setAttribute("aria-controls", panel.node.id);
      const active = collaboration && slot.id === state.activePanel;
      tab.setAttribute("aria-selected", String(active));
      tab.tabIndex = slot.id === state.activePanel ? 0 : -1;
      // Badge slots are intentionally empty until real per-Worker events are connected.
      for (const kind of ["unread", "error"]) {
        const badge = el("span", undefined, "session-badge session-" + kind);
        const worker=team?.workflow.workers.find(w=>w.worker_id===slot.id);
        badge.textContent=kind==='unread'?String(worker?.unread||''):'!';
        badge.hidden = kind==='unread'?!worker?.unread:!['FAILED','INTERRUPTED'].includes(worker?.status);
        badge.dataset.sessionId = slot.id;
        tab.append(badge);
      }
      const tabItem = el("div", undefined, "session-tab");
      const close = button("×", `关闭 ${slot.title} 标签（后台任务继续）`, () => closeTab(slot.id), "tab-close");
      close.disabled = state.panels.filter(p => !state.navigation.hiddenTabs.includes(p.id)).length <= 1;
      tabItem.hidden = state.navigation.hiddenTabs.includes(slot.id);
      tabItem.append(tab, close);
      tabs.append(tabItem);
      if (tabItem.hidden) reopenMenu.append(button(slot.title, "重新打开 " + slot.title, () => { reopen.open = false; focus(slot.id); }, "menu-item"));
    });
    if (!reopenMenu.childElementCount) reopenMenu.append(el("p", "没有隐藏的会话", "muted-text"));
    if(isWorker)selectTeamWorker(state.activePanel);
    updateControls();
  }
  // Presentation-only countdown: never post a host message or alter task state.
  const retryClock=setInterval(()=>{workerPanel?.tick();commander.tick();},1000);
  window.addEventListener('unload',()=>clearInterval(retryClock),{once:true});
  function activeChatId() {
    return layout?.state.activePanel === "claude-01" ? "claude-01" : "gpt";
  }
  function updateControls() {
    const state =
      activeChatId() === "claude-01" ? snapshot.claude : snapshot.chat;
    const chatAvailable = shell.dataset.page === "collaboration" && ["gpt", "claude-01"].includes(layout.state.activePanel);
    pause.disabled = !chatAvailable || !state?.busy;
    start.disabled = !chatAvailable || !!state?.busy;
    resume.disabled = !chatAvailable;
    start.title = `检测 ${activeChatId() === "claude-01" ? "Claude" : "Codex"} CLI，不自动发送消息`;
  }
  function renderLogs() {
    if (!snapshot) return;
    const key = JSON.stringify([
      snapshot.logs?.length,
      snapshot.logs?.at(-1)?.id,
      clearedThrough,
      level.value,
      source.value,
      search.value,
      paused,
    ]);
    if (key === lastLogRender) return;
    lastLogRender = key;
    const query = search.value.toLowerCase(),
      entries = (snapshot.logs || []).filter(
        (log) =>
          log.id > clearedThrough &&
          (level.selectedIndex === 0 || log.level === level.value) &&
          (source.selectedIndex === 0 || log.source === source.value) &&
          `${log.message} ${log.source}`.toLowerCase().includes(query),
      );
    const top = logBody.scrollTop;
    logBody.replaceChildren();
    entries.forEach((log) => {
      const line = el("div", undefined, "log-line");
      line.append(
        el("time", log.time),
        el("span", log.level, `log-level ${log.level.toLowerCase()}`),
        el("span", `[${log.source}]`, "log-source"),
        el("span", log.message, "log-message"),
      );
      logBody.append(line);
    });
    if (!entries.length)
      logBody.append(el("p", "当前筛选下没有日志。", "log-empty"));
    logBody.scrollTop = paused ? top : logBody.scrollHeight;
  }
  function update(data) {
    snapshot = data;
    if (!initialized) {
      initialized = true;
      layout = new window.ATMLayoutManager(
        shell,
        data.layout || saved.layout,
        (state) => {
          syncLayout();
          persist();
          post({ type: "layout", layout: state });
        },
      );
      layout.bindResize(navHandle, "navWidth", () => 1, 180, 300);
      layout.bindResize(
        inspectorHandle,
        "inspectorWidth",
        () => 1,
        250,
        520,
        -1,
      );
      layout.bindResize(logHandle, "logHeight", () => 1, 100, 420, -1);
      layout.bindResize(
        columnHandle,
        "split",
        () => 100 / grid.clientWidth,
        25,
        75,
      );
      layout.bindResize(
        rowHandle,
        "rowSplit",
        () => 100 / grid.clientHeight,
        25,
        75,
      );
    }
    const chat = data.chat || {};
    if(workerPanel){
      const team=currentTeam(),workers=team?.workflow.workers||[],ids=new Set(workers.map(w=>w.worker_id));
      layout.state.panels=layout.state.panels.filter(p=>p.kind!=='team-worker'||ids.has(p.id));
      for(const worker of workers)if(!layout.state.panels.some(p=>p.id===worker.worker_id))layout.state.panels.push({id:worker.worker_id,kind:'team-worker',title:'Claude '+worker.worker_id.slice(0,8),collapsed:false});
      if(!layout.state.panels.some(p=>p.id===layout.state.activePanel)){layout.state.activePanel='gpt';layout.state.navigation.route='collaboration';}
      if(!team)requestedWorker=undefined;
    }
    project.textContent = `▱  ${chat.projectName || data.project || "未选择项目"}`;
    project.title = chat.projectPath || "选择项目";
    const gptConnection = window.ATMGPTCommanderPanel.connection(chat);
    gptStatus.textContent = "GPT：" + gptConnection.text;
    gptStatus.title = gptConnection.detail;
    gptStatus.classList.toggle(
      "connected",
      gptConnection.kind === "online",
    );
    try { commander.update(data); for(const page of futurePages?.values()||[])page.update?.(data); }
    catch {
      commander.key = undefined;
      commander.current = undefined;
      for (const section of Object.values(commander.sections)) section.replaceChildren();
      commander.timeline.replaceChildren();
      commander.error.hidden = false;
      commander.error.textContent = "任务详情暂时无法显示，请查看业务操作与历史。";
      commander.timeline.hidden = true;
    }
    phase.querySelector("p").textContent = commander.current?.task
      ? "当前任务：" + commander.current.task.state
      : commander.current?.proposal ? "计划已验证，尚未执行" : "尚未建立任务";
    const teamTasks=currentTeam()?.workflow.tasks;
    if(teamTasks){const counts=[teamTasks.filter(t=>t.status==='COMPLETED').length,teamTasks.filter(t=>['RUNNING','REVIEWING','REPAIRING'].includes(t.status)).length,teamTasks.filter(t=>['PENDING','READY','WAITING_DEPENDENCY','WAITING_FILE_LOCK','WAITING_APPROVAL','WAITING_REVIEW','PAUSED'].includes(t.status)).length];[...phase.querySelectorAll('.count-line span:last-child')].forEach((node,i)=>node.textContent=String(counts[i]));}
    else for(const node of phase.querySelectorAll('.count-line span:last-child'))node.textContent='—';
    const claude = data.claude || {};
    permissionPanel?.update(claude.permissions || {},!!claude.busy);
    reviewPanel?.update(data.review || {});
    repairPanel?.update(data.repair || {},data.review || {});
    dispatchPanel?.update(data.dispatch || {});
    claudeStatus.textContent = `Claude：${claude.connectedAdapter ? (claude.busy ? "运行中" : claude.connection || "尚未检测") : "未接入"}`;
    claudeStatus.title = claudeStatus.textContent;
    claudeStatus.classList.toggle(
      "connected",
      !!claude.busy || String(claude.connection).includes("已连接"),
    );
    const teamWorkers=currentTeam()?.workflow.workers;
    if(teamWorkers?.length){const active=teamWorkers.filter(w=>['RUNNING','RETRYING','QUEUED','PAUSING'].includes(w.status)).length;claudeStatus.textContent=`Claude：${teamWorkers.length} 个隔离 Worker · ${active} 个活跃`;claudeStatus.title=claudeStatus.textContent;claudeStatus.classList.toggle('connected',teamWorkers.some(w=>!!w.session_id));}
    projectItems.replaceChildren();
    for (const item of data.projects || []) {
      const p = button(
        `▱  ${item.name}`,
        "在已有工作区中选择项目",
        () => post({ type: "project", panelId: "gpt" }),
        "project-item",
      );
      if (item.name === chat.projectName) p.classList.add("current");
      projectItems.append(p);
    }
    if (!projectItems.children.length)
      projectItems.append(el("p", "未打开工作区", "muted-text"));
    gptAgent.replaceChildren(
      el("span", "◎ GPT 总指挥"),
      el(
        "span",
        chat.busy ? "响应中" : chat.connection || "未检测",
        "status-pill",
      ),
    );
    claudeAgent.replaceChildren(
      el("span", "✳ Claude Worker 01"),
      el(
        "span",
        claude.connectedAdapter
          ? claude.busy
            ? "执行中"
            : claude.conversation?.status || "未启动"
          : "未接入",
        "status-pill muted",
      ),
    );
    tokenCard.replaceChildren(
      el("small", "GPT Token / 预算"),
      el("strong", `${chat.usage?.total?.toLocaleString() ?? "—"} / 未设置`),
      el("small", `活跃 Agent：${Number(!!chat.busy) + Number(!!claude.busy)}`),
    );
    claudeCard.replaceChildren(
      el("small", "Claude 会话"),
      el("strong", String(claude.sessionCount || 0)),
      el(
        "small",
        claude.connectedAdapter ? "Claude Worker 01" : "CLI Adapter 未接入",
      ),
    );
    syncLayout();
    renderLogs();
  }
  window.addEventListener("message", (event) => {
    if (event.data?.type === "snapshot") update(event.data);
  });
  post({ type: "ready" });
})();
