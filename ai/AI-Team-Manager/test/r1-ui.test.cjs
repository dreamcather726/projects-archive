const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const { JSDOM } = require('jsdom');
const { ids, scripts, fixture, selectDetail, retry, verifyProductionAssets } = require('./r1-ui-fixture.cjs');
function ui() {
  const dom = new JSDOM('<main id="workspace"></main>', { runScripts: 'outside-only' });
  const w = dom.window, sent = [], errors = [];
  w.addEventListener('error', e => errors.push(e.message));
  w.acquireVsCodeApi = () => ({ getState: () => ({}), setState: s => { w.saved = s; }, postMessage: m => sent.push(m) });
  for (const file of scripts) w.eval(fs.readFileSync('src/webview/' + file, 'utf8'));
  const update = s => w.dispatchEvent(new w.MessageEvent('message', { data: s }));
  const by = name => w.document.querySelector(`[aria-label="${name}"]`);
  const click = id => w.document.querySelector(`[role=tab][data-session-id="${id}"]`).click();
  return { dom, w, sent, errors, update, by, click };
}
test('R1 loads every production Webview resource and preserves all five routes', () => {
  verifyProductionAssets();
  const { dom, w, sent, update, errors } = ui();
  try {
    update(fixture()); assert.equal(w.document.querySelectorAll('.nav-link').length, 5);
    const before = sent.length;
    for (const link of w.document.querySelectorAll('.nav-link')) link.click();
    assert(sent.slice(before).every(m => ['layout', 'teamSelect'].includes(m.type)));
    assert.deepEqual(errors, []);
  } finally { dom.window.close(); }
});
test('R1 task list filters backend statuses, opens drawer, and uses backend statistics verbatim', () => {
  const { dom, w, update, by } = ui(), s = fixture();
  try {
    s.dispatch.team.workflow.retry_stats.running = 7;
    update(s); by('任务调度中心').click();
    assert.equal(w.document.querySelectorAll('.task-table tbody tr').length, 2);
    assert(by('后台重试统计').textContent.includes('执行 7'));
    assert(!w.document.querySelector('.task-dependencies').open);
    assert(!w.document.querySelector('.task-event-log').open);
    by('筛选任务状态').value = 'RETRYING'; by('筛选任务状态').dispatchEvent(new w.Event('input'));
    assert.equal(w.document.querySelectorAll('.task-table tbody tr').length, 1);
    by('查看任务 task-0').click(); assert(!by('任务详情').hidden);
    assert(by('任务详情').textContent.includes('README.md')); by('关闭任务详情').click();
    delete s.dispatch.team.workflow.retry_stats; update(s);
    assert(by('后台重试统计').textContent.includes('后端尚未提供'));
    assert(!w.document.querySelector('.gpt-task-details').textContent.includes('100%'));
  } finally { dom.window.close(); }
});
test('R1 worker five-tab isolation, backend operations and hide/reopen never simulate execution', () => {
  const { dom, w, update, by, click, sent, errors } = ui(), s = fixture();
  try {
    selectDetail(s, ids[0]); update(s); click(ids[0]);
    assert.equal(w.document.querySelectorAll('.worker-subtabs [role=tab]').length, 5);
    for (const tab of w.document.querySelectorAll('.worker-subtabs [role=tab]')) {
      tab.click(); assert.equal(w.document.querySelectorAll('.worker-subtabs [aria-selected=true]').length, 1);
    }
    by('暂停当前 Worker').click();
    assert.deepEqual(JSON.parse(JSON.stringify(sent.at(-1))), { type: 'teamControl', projectPath: s.chat.projectPath, planId: 'ui-plan', workerId: ids[0], action: 'pause' });
    assert.equal(s.dispatch.team.workflow.tasks[0].status, 'RUNNING');
    assert(by('恢复当前 Worker').disabled);
    s.dispatch.team.workflow.tasks[0].status = 'PAUSED'; s.dispatch.team.workflow.workers[0].status = 'PAUSED'; update(s);
    assert(!by('恢复当前 Worker').disabled); assert(!by('当前 Worker 补充指令').disabled);
    by('当前 Worker 补充指令').value = '只检查已有文件'; by('保存补充指令，恢复时经过原权限网关').click();
    assert.equal(sent.at(-1).type, 'teamInstruct'); assert.equal(sent.at(-1).workerId, ids[0]);
    const before = sent.length;
    click(ids[1]); selectDetail(s, ids[1]); update(s);
    assert(w.document.querySelector('.worker-body').textContent.includes('ONLY_WORKER_1'));
    assert(!w.document.querySelector('.worker-body').textContent.includes('ONLY_WORKER_0'));
    by('关闭 Claude ' + ids[1].slice(0, 8) + ' 标签（后台任务继续）').click();
    by('重新打开 Claude ' + ids[1].slice(0, 8)).click();
    assert(sent.slice(before).every(m => ['layout', 'teamSelect'].includes(m.type)));
    assert.deepEqual(errors, []);
  } finally { dom.window.close(); }
});
test('R1 retry statuses show reason, count, session and collapsed diagnostics without host requests', () => {
  const { dom, w, sent } = ui();
  try {
    const initialMessages=sent.length;
    for (const [kind, tone] of [['RETRY_SCHEDULED', 'warning'], ['RETRY_STARTED', 'warning'], ['RETRY_SUCCEEDED', 'success'], ['RETRY_EXHAUSTED', 'danger'], ['RETRY_CANCELLED', 'neutral'], ['RETRY_BLOCKED_SIDE_EFFECT', 'danger']]) {
      const r = retry(kind), view = w.ATMOutputFormatter.retryBar(r, 'Claude fixture', text => sent.push({ type: 'copy', text }));
      assert.equal(view.node.dataset.tone, tone, kind); assert.equal(view.node.open, false);
      assert(view.node.querySelector('summary').textContent.includes('Claude fixture'));
      assert(view.node.textContent.includes('HTTP 503')); assert(view.node.textContent.includes('第 2 / 3 次'));
      assert(view.node.textContent.includes('原会话已保留'));
      view.tick(); view.tick(); assert.equal(sent.length, initialMessages);
    }
  } finally { dom.window.close(); }
});
test('R1 retry events deduplicate display but preserve original audit and copy contents', () => {
  const { dom, w, update, by, sent } = ui(), s = fixture();
  try {
    const events = s.dispatch.team.workflow.record.supervision;
    events.push(structuredClone(events[0])); update(s); update(s);
    by('任务调度中心').click(); assert.equal(w.document.querySelectorAll('.task-event').length, 2);
    by('筛选事件 Worker').value = ids[0]; by('筛选事件 Worker').dispatchEvent(new w.Event('input'));
    assert.equal(w.document.querySelectorAll('.task-event').length, 1);
    by('复制筛选后的执行事件').click(); assert.equal(JSON.parse(sent.at(-1).text).length, 2);
    assert.equal(events.length, 3);
    assert.equal(w.document.querySelectorAll('.commander-retry-events .retry-status').length, 1);
    by('搜索执行事件').value = 'not-present'; by('搜索执行事件').dispatchEvent(new w.Event('input'));
    assert.equal(w.document.querySelectorAll('.task-event').length, 0);
  } finally { dom.window.close(); }
});
test('R1 same-project foreign owner cannot keep prior worker sessions or expose another conversation', () => {
  const { dom, w, update, click } = ui(), s = fixture();
  try {
    selectDetail(s, ids[0]); update(s); click(ids[0]);
    s.chat.conversation.id = 'another-owner'; update(s);
    assert(w.document.querySelector('.worker-workbench').hidden);
    assert.equal(w.document.querySelectorAll('[role=tab][data-session-id="' + ids[0] + '"]').length, 0);
  } finally { dom.window.close(); }
});
test('R1 Worker labels filter tasks and task drawer retains keyboard focus during unchanged snapshots', () => {
  const { dom, w, update, by } = ui(), s = fixture();
  try {
    update(s); by('任务调度中心').click();
    by('搜索任务').value = ids[0].slice(0, 8); by('搜索任务').dispatchEvent(new w.Event('input'));
    assert.equal(w.document.querySelectorAll('.task-table tbody tr').length, 1);
    by('查看任务 task-0').click(); const close = by('关闭任务详情'); close.focus(); update(s);
    assert.equal(w.document.activeElement, close);
  } finally { dom.window.close(); }
});

test('R1 an unchanged Worker detail is rendered again after the loading placeholder',()=>{
 const {dom,w}=ui(),s=fixture(),panel=new w.ATMWorkerPanel(()=>{},()=>{});
 try{selectDetail(s,ids[0]);panel.update(s.dispatch.team,ids[0]);assert(panel.node.textContent.includes('ONLY_WORKER_0'));
  const original=s.dispatch.team.detail;delete s.dispatch.team.detail;panel.update(s.dispatch.team,ids[0]);s.dispatch.team.detail=original;panel.update(s.dispatch.team,ids[0]);
  assert(panel.body.textContent.includes('ONLY_WORKER_0'));assert(panel.retrySlot.querySelector('.retry-status'));
 }finally{dom.window.close();}
});
