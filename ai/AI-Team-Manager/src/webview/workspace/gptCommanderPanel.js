// Read-only projection of the existing dispatcher/planner snapshot. No execution API.
(() => {
  const { el, button } = window.ATMElements;
  const labels = {
    PLANNING: "制定计划", APPROVING: "等待审批", RUNNING: "执行中", REVIEWING: "审查中",
    CORRECTING: "定向修复中", PAUSED: "已暂停", PASSED: "已通过验收", FAILED: "执行失败",
    CANCELLED: "已取消", INTERRUPTED: "已中断", PENDING: "待执行", READY: "待执行",
    WAITING_DEPENDENCY: "等待依赖", WAITING_REVIEW:'等待补证复审', WAITING_APPROVAL:'等待人工确认', WAITING_FILE_LOCK:'等待文件锁', WAITING_INTEGRATION:'等待集成', RETRYING:'自动重试中', REPAIRING:'定向修复', COMPLETED: "已完成", PASS: "通过", REQUEST_CHANGES: "需要修复",
    BLOCKED: "证据不足或受阻", NEEDS_HUMAN_REVIEW: "需要人工确认",
  };
  const status = value => labels[value] || value || "未提供";
  const list = value => Array.isArray(value) ? value : [];
  const pathKey = value => typeof value === "string" ?
    (/^[a-z]:/i.test(value) ? value.replace(/\\/g, "/").toLowerCase() : value) : "";
  function connection(chat = {}) {
    const text = chat.connection || "尚未检测";
    if (chat.error?.includes('[WAITING_APPROVAL]')) return {kind:'idle',text:'等待用量确认',detail:chat.error};
    if (chat.error?.includes('[TEAM_PREPARE_FAILED]')) return {kind:'idle',text:'团队准备受阻',detail:chat.error};
    if (chat.error || /不可用|失败|异常/.test(text)) return { kind: "error", text: "连接异常", detail: chat.error || text };
    if (/已连接/.test(text)) return { kind: "online", text: chat.busy ? "在线 · 响应中" : "在线 · 空闲", detail: text };
    if (chat.busy || /检测中|启动.*响应/.test(text)) return { kind: "connecting", text: "连接中", detail: text };
    return { kind: "idle", text: /codex/i.test(text) ? "CLI 已检测 · 模型未验证" : "空闲 · " + text, detail: text };
  }
  function model(snapshot = {}) {
    const chat = snapshot.chat || {}, dispatch = snapshot.dispatch || {};
    const belongs = value => !!chat.projectPath && !!chat.conversation?.id &&
      pathKey(value?.project_path) === pathKey(chat.projectPath) && value?.owner_id === chat.conversation.id;
    const task = belongs(dispatch.task) ? dispatch.task : undefined;
    // Planning is not execution. Do not join an unrelated adaptive plan to a dispatched task.
    const proposal = !task ? list(dispatch.planning?.plans).find(belongs) : undefined;
    const round = list(task?.rounds).at(-1);
    const report = snapshot.review?.report;
    const review = round?.review || (task && report?.task?.task_id === task.task_id &&
      pathKey(report.project_path) === pathKey(task.project_path) ? report : undefined);
    const tests = list(round?.tests);
    const accepted = task?.state === "PASSED" && review?.verdict === "PASS" &&
      tests.some(t => t.status === "PASSED" && t.command !== "npm run compile") && tests.every(t => t.status === "PASSED");
    const pending = task && dispatch.pending?.task_id === task.task_id ? dispatch.pending : undefined;
    const workerLinked = !!task?.session_id && pathKey(snapshot.claude?.projectPath) === pathKey(chat.projectPath) &&
      task.session_id === snapshot.claude?.conversation?.threadId && task.conversation_id === snapshot.claude?.conversation?.id;
    const risks = list(round?.plan?.risks).slice();
    if (task?.error) risks.push(task.error);
    if (pending) risks.push("待人工审批：" + pending.reason);
    for (const test of tests.filter(t => t.status !== "PASSED")) risks.push("测试 " + test.status + "：" + test.command);
    if (review && review.verdict !== "PASS") risks.push("GPT 审查：" + status(review.verdict));
    if (proposal) risks.push(dispatch.planning?.note || "规划不等于执行授权；多 Worker 执行尚未接通。");
    if (task && !workerLinked) risks.push("当前 Claude Session 与该任务不匹配；不能跳转为当前执行会话。");
    for (const event of list(task?.events)) {
      if (event.kind === "GPT_APPROVAL_FAILED") risks.push("权限审批异常：" + (event.data?.reason || "审批未通过"));
      if (event.kind === "GPT_TOOL_APPROVAL" || event.kind === "GPT_PLAN_APPROVAL") {
        const decision = event.data?.result || event.data?.decision;
        if (decision?.decision === "DENY" || decision?.decision === "NEEDS_HUMAN")
          risks.push("权限决策 " + decision.decision + "：" + (decision.reason || "请查看审批记录"));
      }
    }
    const nextByState = {
      PLANNING: "等待真实开发计划返回。", APPROVING: "等待现有审批链决策。", RUNNING: "等待 Claude 执行及授权测试结果。",
      REVIEWING: "等待 GPT 独立审查结果。", CORRECTING: "等待授权范围内定向修复和重新验证。",
      PASSED: "当前任务已结束，等待用户确认下一项需求。", PAUSED: "查看限制原因，由用户确认后续操作。",
      FAILED: "检查失败证据，再决定是否重新派发。", CANCELLED: "任务已取消，保留已有记录。", INTERRUPTED: "查看中断记录，不自动重发任务。",
    };
    return { chat, task, proposal, round, review, tests, accepted, pending, workerLinked, risks,
      connection: connection(chat),
      title: task?.input?.description || proposal?.goal || "尚未建立任务",
      criteria: list(task?.input?.acceptance_criteria || proposal?.tasks?.flatMap(t => list(t.acceptance_criteria))),
      next: task ? nextByState[task.state] || "等待后端状态更新。" : proposal ? "等待明确的执行授权；计划步骤尚未执行。" : "在 GPT 对话中描述需求，或继续讨论方案。",
    };
  }
  class GPTCommanderPanel {
    constructor(post, selectWorker) {
      this.post = post; this.selectWorker = selectWorker;
      this.node = el("div", undefined, "gpt-task-details");
      this.node.setAttribute("aria-label", "GPT 当前任务详情");
      this.error = el("p", undefined, "commander-risk"); this.error.hidden = true;
      this.error.setAttribute("role", "alert"); this.node.append(this.error);
      this.sections = {};
      for (const [id, title, icon] of [
        ["summary", "当前任务摘要", "▤"], ["workers", "Worker 分配情况", "✳"],
        ["steps", "任务拆分与进度", "☷"], ["acceptance", "验收标准", "☑"],
        ["risks", "风险提示", "△"], ["next", "下一步计划", "↗"],
      ]) {
        const card = el("details", undefined, "commander-card commander-" + id);
        card.open=['summary','workers','next'].includes(id);
        card.append(el("summary", icon + "  " + title));
        const body = el("div", undefined, "commander-card-body"); card.append(body);
        this.sections[id] = body; this.node.append(card);
      }
      this.timeline = el("section", undefined, "commander-notices");
      this.timeline.setAttribute("aria-label", "当前任务通知与规划");
    }
    update(snapshot) {
      const team=snapshot.dispatch?.team,teamPlan=team?.workflow?.plan;
      if(teamPlan&&pathKey(teamPlan.project_path)===pathKey(snapshot.chat?.projectPath)&&teamPlan.owner_id===snapshot.chat?.conversation?.id){this.updateTeam(team,snapshot.chat);return;}
      const m = model(snapshot); this.current = m; this.error.hidden = true;
      // Avoid replacing focused controls or scrolling through identical streaming snapshots.
      const key = JSON.stringify([m.task, m.proposal, m.review, m.pending, m.workerLinked, m.chat.projectName]);
      if (key === this.key) return; this.key = key;
      for (const section of Object.values(this.sections)) section.replaceChildren();
      this.timeline.replaceChildren(); this.timeline.hidden = !m.task && !m.proposal;
      const { summary, workers, steps, acceptance, risks, next } = this.sections;
      summary.append(el("strong", m.title, "commander-task-title"));
      const facts = el("dl", undefined, "commander-facts");
      for (const [label, value] of [
        ["Task ID", m.task?.task_id || m.proposal?.plan_id || "未提供"],
        ["项目", m.chat.projectName || "未选择项目"],
        ["任务目标", m.round?.plan?.goal || m.proposal?.goal || m.task?.input?.description || "未提供"],
        ["优先级", "后端未提供"],
        ["当前状态", m.task ? status(m.task.state) : m.proposal ? "计划已验证 · 尚未执行" : "等待需求"],
      ]) facts.append(el("dt", label), el("dd", value));
      summary.append(facts);
      const progress = el("div", undefined, "commander-progress");
      progress.setAttribute("role", "progressbar"); progress.setAttribute("aria-label", "总体任务进度");
      progress.setAttribute("aria-valuetext", m.accepted ? "总体已通过验收" : "后端未提供逐步进度");
      if (m.accepted) { progress.setAttribute("aria-valuenow", "100"); progress.setAttribute("aria-valuemin", "0"); progress.setAttribute("aria-valuemax", "100"); }
      const fill = el("span"); fill.hidden = !m.accepted; progress.append(fill);
      summary.append(progress, el("small", m.accepted ? "100% · 总体验收通过" : "总体进度：未提供，不估算百分比"));
      if (m.task) {
        workers.append(el("p", "任务执行通道：1"));
        const worker = button("✳ Claude Worker 01", "查看当前任务的 Claude Worker 01", () => this.selectWorker("claude-01"), "commander-worker");
        worker.disabled = !m.workerLinked;
        workers.append(worker, el("p", status(m.task.state) + " · 进度未提供 · 独立 Session：" + (m.task.session_id || "尚未建立"), "muted-text"));
      } else if (m.proposal) {
        workers.append(el("p", "规划槽位：" + m.proposal.worker_count + " · 未启动执行"));
        for (const id of new Set(list(m.proposal.tasks).map(t => t.assigned_worker)))
          workers.append(el("p", id + " · 仅规划", "muted-text"));
      } else workers.append(el("p", "尚无任务分配记录", "muted-text"));
      const planned = m.task ? list(m.round?.plan?.steps).map(text => ({ text })) :
        list(m.proposal?.tasks).map(t => ({ text: t.task_description, dependencies: list(t.dependencies), id: t.task_id }));
      if (!planned.length) steps.append(el("p", "暂无任务拆分数据", "muted-text"));
      const ordered = el("ol", undefined, "commander-step-list");
      for (const item of planned) {
        const li = el("li"); li.append(el("span", item.text), el("small", m.proposal ? "仅规划 · 尚未执行" : "步骤结果未验证"));
        if (item.id) li.append(el("small", "Task ID：" + item.id));
        if (item.dependencies?.length) li.append(el("small", "依赖：" + item.dependencies.join("、")));
        ordered.append(li);
      }
      steps.append(ordered);
      for (const criterion of m.criteria) {
        const row = el("div", undefined, "criterion"); row.dataset.result = "UNVERIFIED";
        row.append(el("span", "○ " + criterion), el("small", "未验证 · 未提供逐项验收结果")); acceptance.append(row);
      }
      if (!m.criteria.length) acceptance.append(el("p", "尚无验收标准 · 未验证", "muted-text"));
      if (m.review) acceptance.append(el("p", "总体审查：" + status(m.review.verdict)), el("p", m.review.summary || ""));
      for (const test of m.tests) {
        const row = el("p", "测试证据 · " + test.status + " · " + test.command, "test-evidence");
        row.dataset.result = test.status; acceptance.append(row);
      }
      if (m.task && !m.tests.length) acceptance.append(el("p", "尚无实际测试结果", "muted-text"));
      for (const risk of m.risks) risks.append(el("p", risk, "commander-risk"));
      if (!m.risks.length) risks.append(el("p", "暂无已上报风险；不代表安全验证已通过。", "muted-text"));
      next.append(el("p", m.next));
      if (m.task) {
        const record = button("查看完整任务记录", "查看当前 GPT 派发任务记录", () => this.post({ type: "dispatchReport" }), "secondary-button"); next.append(record);
        this.notice("task", "任务状态 · " + status(m.task.state), m.task.input.description);
        if (m.pending) this.notice("system", "等待人工审批", m.pending.reason);
        if (m.review) this.notice("review", "GPT 审查 · " + status(m.review.verdict), m.review.summary);
      }
      const plan = m.round?.plan || (m.proposal && { goal: m.proposal.goal, approach: m.proposal.technical_approach });
      if (plan) {
        this.notice("plan", "开发计划 · " + plan.goal, plan.approach);
        if (m.task) {
          const scope = el("p", "允许文件：" + list(m.task.input?.allowed_files).join("、") + "\n禁止文件：" + (list(m.task.input?.forbidden_files).join("、") || "所有未授权文件"));
          this.timeline.lastElementChild.append(scope);
        }
      }
    }
    notice(kind, title, text) {
      const card = el("article", undefined, "commander-notice notice-" + kind); card.dataset.messageType = kind;
      card.append(el("strong", title), el("p", text || "")); this.timeline.append(card);
    }
    updateTeam(team,chat){
      const w=team.workflow,plan=w.plan,key=JSON.stringify([w.tasks,w.workers,w.candidates,w.record,team.integration,team.busy,team.error,team.usage,team.progress,chat.projectName]);
      this.current={task:{state:team.integration?.status||'团队执行中'},team};this.error.hidden=true;
      if(key===this.key)return;this.key=key;for(const section of Object.values(this.sections))section.replaceChildren();this.timeline.replaceChildren();this.timeline.hidden=false;
      const {summary,workers,steps,acceptance,risks,next}=this.sections,fmt=window.ATMOutputFormatter;
      summary.append(el('strong',plan.goal,'commander-task-title'));if(!team.progress)summary.append(el('p','当前阶段：'+(team.integration?fmt.stateLabel(team.integration.status):team.busy?'执行与独立审查':'等待处理')));
      const stats=w.retry_stats;
      summary.append(el('p',stats?`${stats.delivered} / ${stats.total_tasks} 个任务已交付`:'已验证进度：后端未提供'),el('small','候选通过审查不等于交付。'));
      this.progressView ||= fmt.progressView('GPT 整体任务');summary.append(this.progressView.node);this.progressView.update(team.progress?{...team.progress,name:'整体验证进度'}:undefined);
      const timeline=fmt.progressTimeline({steps:team.progress?.tasks.flatMap(t=>t.steps.slice(0,3).map(s=>({...s,name:t.name+' · '+s.name}))).concat(team.progress?.tasks[0]?.steps.slice(3)||[])||[]});
      summary.append(timeline);if(team.progress)timeline.append(el('small',team.progress.basis));
      const meta=el('details',undefined,'team-plan-details');meta.append(el('summary','计划与分配依据'),el('p','计划编号：'+plan.plan_id));
      if(plan.analysis){meta.append(el('p','规模：'+plan.analysis.scale+' · 技术复杂度：'+plan.analysis.technical_complexity),el('p',plan.analysis.worker_rationale));for(const risk of plan.analysis.risks||[])risks.append(el('p',risk,'commander-risk'));}
      meta.append(fmt.render(JSON.stringify(plan,null,2),text=>this.post({type:'copy',panelId:'gpt',text})));steps.append(meta);
      workers.append(el('small','实际 Worker '+w.workers.length+' · 并发上限 '+plan.max_concurrency));
      for(const worker of w.workers){
        const row=w.tasks.find(t=>t.task.task_id===worker.task_id),item=el('div',undefined,'commander-worker-row');
        item.append(button('Claude '+worker.worker_id.slice(0,8),'打开 Worker '+worker.worker_id,()=>this.selectWorker(worker.worker_id),'commander-worker'),fmt.badge(worker.status==='RETRYING'?'RETRYING':row?.status||worker.status),el('small',row?.task.task_name||row?.task.task_description||worker.task_id));workers.append(item);
      }
      const list=el('ol',undefined,'commander-step-list');
      for(const row of w.tasks){const li=el('li');li.append(el('span',row.task.task_name||row.task.task_id),el('small',status(row.status)+' · 依赖：'+(row.task.dependencies.join('、')||'无')));list.append(li);
        for(const criterion of row.task.acceptance_criteria)acceptance.append(el('p','○ '+criterion+' · 逐项结果未提供'));
        for(const t of w.candidates.find(c=>c.task_id===row.task.task_id)?.tests||[])acceptance.append(el('p',`${row.task.task_id} · ${fmt.stateLabel(t.status)} · ${t.test_count??'未知'} 项 · ${t.command}`,'test-evidence'));
        const review=w.record.reviews[row.task.task_id]?.at(-1);if(review){acceptance.append(el('p','独立审查：'+status(review.verdict)+' · '+review.summary));for(const reason of review.block?.reasons||[])risks.append(el('p',reason,'commander-risk'));}
      }steps.prepend(list);
      for(const r of w.record.repairs||[])acceptance.append(el('p','第 '+r.round+' 轮修复 · '+fmt.stateLabel(r.status)+' · '+r.description));
      if(team.usage?.near_budget)risks.append(el('p','已知用量接近预算，请检查用量与预算。','commander-risk'));
      for(const risk of [team.error,w.record.error,team.integration?.error,...(team.integration?.conflicts||[])].filter(Boolean))risks.append(el('p',risk,'commander-risk'));
      if(!risks.childElementCount)risks.append(el('p','暂无已上报风险；不代表安全验证已通过。','muted-text'));
      const blocked=w.tasks.find(t=>['WAITING_REVIEW','WAITING_APPROVAL','FAILED','PAUSED'].includes(t.status)),block=blocked&&w.record.reviews[blocked.task.task_id]?.at(-1)?.block;
      next.append(el('p',block?.next_action||(team.integration?.status==='DELIVERED'?'本次需求已交付。':team.busy?'等待实际执行、测试和独立审查。':blocked?'查看受阻任务，确认后恢复；不会自动重发旧审批。':'等待下一步操作。')));
      const action=type=>this.post({type,projectPath:plan.project_path,planId:plan.plan_id});
      if(team.integration?.evidence)next.append(button('查看集成补丁','查看实际集成补丁',()=>action('teamPreview'),'secondary-button'));
      if(team.integration?.status==='READY')next.append(button('授权安全交付','授权安全交付',()=>action('teamDeliver'),'primary-button'));
      this.retryViews=[];
      const retries=w.workers.filter(worker=>worker.retry);
      if(retries.length){const events=el('details',undefined,'commander-retry-events');events.append(el('summary',`${retries.length} 个 Worker 有重试记录`));
        for(const worker of retries){const view=fmt.retryBar(worker.retry,'Claude '+worker.worker_id.slice(0,8),text=>this.post({type:'copy',panelId:'gpt',text}));this.retryViews.push(view);events.append(view.node);}this.timeline.append(events);
      }else this.timeline.hidden=true;
    }
    tick(){if(!this.node.hidden)for(const view of this.retryViews||[])view.tick();}
  }
  GPTCommanderPanel.model = model;
  GPTCommanderPanel.connection = connection;
  window.ATMGPTCommanderPanel = GPTCommanderPanel;
})();
