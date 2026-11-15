const fs = require('node:fs');
const path = require('node:path');

// Synthetic UI scenarios only. No model endpoint or persisted user checkpoint is read.
const ids = ['11111111-1111-4111-8111-111111111111', '22222222-2222-4222-8222-222222222222'];
const scripts = ['markdown.js', 'workspace/layoutManager.js', 'workspace/chatPanel.js', 'workspace/outputFormatter.js', 'workspace/workerPanel.js', 'workspace/futurePages.js', 'workspace/reviewPanel.js', 'workspace/permissionPanel.js', 'workspace/repairPanel.js', 'workspace/dispatchPanel.js', 'workspace/workspaceRouter.js', 'workspace/gptCommanderPanel.js', 'workspace/workspace.js'];
const { defaultLayout } = require('../.tools/r1-ui/api.cjs');
const retry = (kind = 'RETRY_SCHEDULED') => ({ type: 'retry', kind, requestId: 'ui-request-a', time: '2026-11-15T08:30:00.000Z', retry: 2, maxRetries: 3, reason: 'HTTP_503', delayMs: kind === 'RETRY_SCHEDULED' ? 4000 : 0, nextRetryAt: kind === 'RETRY_SCHEDULED' ? new Date(Date.now() + 4000).toISOString() : undefined, sessionId: ids[0] });
function fixture(kind = 'RETRY_SCHEDULED') {
  const tasks = ids.map((id, i) => ({
    task: { task_id: 'task-' + i, task_name: ['输入校验与测试', '接口集成验证'][i], task_description: ['实现输入校验，保留现有数据接口，并验证边界条件。', '在上游审查通过后验证接口集成，不修改其他文件。'][i], assigned_worker: 'worker-' + i, priority: i + 1, allowed_files: ['src/task-' + i + '.py'], read_only_files: [], forbidden_files: ['README.md'], dependencies: i ? ['task-0'] : [], interface_contracts: ['保持现有返回值'], acceptance_criteria: ['边界条件测试通过', '独立审查通过后再交付'], test_commands: ['python -B test_task.py'] },
    status: i ? 'WAITING_DEPENDENCY' : 'RUNNING', events: [{ time: '2026-11-15T08:30:00.000Z', from: 'READY', to: i ? 'WAITING_DEPENDENCY' : 'RUNNING' }],
  }));
  const workers = ids.map((id, i) => ({ worker_id: id, project_id: 'ui-project', task_id: 'task-' + i, status: i ? 'IDLE' : ['RETRY_SCHEDULED', 'RETRY_STARTED'].includes(kind) ? 'RETRYING' : kind === 'RUNNING' ? 'RUNNING' : kind === 'RETRY_SUCCEEDED' ? 'COMPLETED' : kind === 'RETRY_CANCELLED' ? 'PAUSED' : 'FAILED', unread: 0, session_id: id, workspace_path: '/ui-fixture/worker-' + i, allowed_files: ['src/task-' + i + '.py'], updated_at: '2026-11-15T08:30:00.000Z', ...(!i && kind !== 'RUNNING' ? { retry: retry(kind) } : {}) }));
  tasks[0].status = ['RUNNING', 'RETRY_SCHEDULED', 'RETRY_STARTED'].includes(kind) ? 'RUNNING' : kind === 'RETRY_SUCCEEDED' ? 'REVIEWING' : kind === 'RETRY_CANCELLED' ? 'PAUSED' : 'FAILED';
  const supervision = [{ id: 'retry-event-a', worker_id: ids[0], task_id: 'task-0', kind: 'RETRY_SCHEDULED', time: '2026-11-15T08:30:00.000Z', reason: 'HTTP 503 · 等待第二次自动重试，原 Session 保留。', decision: 'CONTINUE', evidence: ['ui-request-a'] }, { id: 'wait-event-b', worker_id: ids[1], task_id: 'task-1', kind: 'WAITING_DEPENDENCY', time: '2026-11-15T08:30:00.000Z', reason: '等待上游候选审查通过。', decision: 'WAIT', evidence: [] }];
  const candidate = { task_id: 'task-0', worker_id: ids[0], files: [{ kind: 'MODIFIED', path: 'src/task-0.py' }], tests: [{ id: 'ui-test-result', status: 'PASSED', command: 'python -B test_task.py', test_count: 4, output: 'UI fixture: 4 tests passed. Not a live model acceptance.' }] };
  const snapshot = {
    type: 'snapshot', layout: defaultLayout(), projects: [{ name: 'UI 验收夹具', path: '/ui-fixture/project' }], project: 'UI 验收夹具', logs: [], diagnostics: [],
    chat: { projectPath: '/ui-fixture/project', projectName: 'UI 验收夹具', connection: '已连接 · 空闲', busy: false, conversation: { id: 'ui-owner', threadId: 'ui-gpt-session', status: 'COMPLETED', requests: [], messages: [{ id: 'user-ui', role: 'user', text: '请实现输入校验，并在测试和独立审查后交付。', status: 'COMPLETED', time: '2026-11-15T08:29:00.000Z' }, { id: 'gpt-ui', role: 'assistant', text: '## 任务安排\n先验证输入，再进行接口集成。\n\n- Worker 1：输入校验与测试\n- Worker 2：等待上游独立审查\n\n```python\ndef validate(value):\n    return value is not None\n```\n\n这是 UI 自动化夹具，不是真实模型响应。', status: 'COMPLETED', time: '2026-11-15T08:29:30.000Z' }] } },
    claude: { connectedAdapter: true, busy: false, connection: '尚未检测', conversation: { id: 'ui-legacy', messages: [], requests: [], status: 'READY' }, activities: [] },
    dispatch: { settings: { teamMaxConcurrent: 3, teamMaxRepairRounds: 2, claudeRetryMaxRetries: 3, claudeRetryBaseDelayMs: 2000, claudeExecutable: 'claude', codexExecutable: 'codex' }, team: { busy: true, workflow: { plan: { plan_id: 'ui-plan', owner_id: 'ui-owner', project_id: 'ui-project', project_path: '/ui-fixture/project', goal: '输入校验与接口验证', max_concurrency: 2, tasks: tasks.map(r => r.task) }, tasks, workers, candidates: [candidate], retry_stats: { running: 0, retrying: 1, recovered: 0, exhausted: 0, needs_attention: 0, delivered: 0, total_tasks: 2 }, record: { reviews: {}, supervision, repairs: [] } }, usage: { budget: 100000, near_budget: false } } },
  };
  return snapshot;
}
function selectDetail(snapshot, id, cursor = {}) {
  const team = snapshot.dispatch.team;
  if (!id) { delete team.selected_worker; delete team.detail; return; }
  const i = ids.indexOf(id), worker = team.workflow.workers.find(w => w.worker_id === id);
  if (!worker) throw Error('Unknown fixture worker');
  const events = [{ sequence: 0, worker_id: id, task_id: worker.task_id, time: '2026-11-15T08:30:00.000Z', event: worker.retry || { type: 'status', message: '等待上游审查' } }];
  team.selected_worker = id;
  team.detail = { worker, task: team.workflow.tasks[i].task, chat: { conversation: { id: 'ui-chat-' + i, threadId: worker.session_id, messages: [{ id: 'ui-worker-text-' + i, role: 'assistant', status: 'COMPLETED', time: '2026-11-15T08:30:00.000Z', text: `ONLY_WORKER_${i}\n\n本页只展示当前 Worker 的独立记录。` }] } }, events: { events, has_more: false, next_cursor: 1 }, message_start: cursor.messageStart || 0, message_total: 1, event_total: 1, candidate: team.workflow.candidates.find(c => c.task_id === worker.task_id), reviews: [], repairs: [] };
}
function verifyProductionAssets() {
  const provider = fs.readFileSync(path.resolve('src/providers/workspacePanel.ts'), 'utf8');
  const list = provider.match(/const scripts = \[([\s\S]*?)\];/)[1];
  const actual = [...list.matchAll(/["']([^"']+\.js)["']/g)].map(m => m[1]);
  require('node:assert/strict').deepEqual(scripts, actual, 'The UI test must load the complete production script list in order');
}
module.exports = { ids, scripts, fixture, retry, selectDetail, verifyProductionAssets };
