import { randomUUID } from 'node:crypto';
import { ReviewGit, git, digest } from '../review/reviewGit';
import { ReviewStorage } from '../review/reviewStorage';
import { safeReviewText, relativeFile, excludedPath } from '../review/reviewSafety';
import { userAuthority } from '../dispatch/taskDispatcher';
import type { DevelopmentPlan, PlanProposal, PlanningContext, PlannedTask } from './planningTypes';

const kinds = ['SMALL_CHANGE', 'MODULE_TASK', 'MULTI_MODULE', 'PROJECT_PHASE', 'FULL_PROJECT'];
const canonical = (s: string) => process.platform === 'win32' ? s.toLowerCase() : s;
function text(v: unknown, label: string, max = 12000): asserts v is string {
  if (typeof v !== 'string' || !v.trim() || v.length > max) throw Error(`规划字段${label}缺失或过长。`);
}
function strings(v: unknown, label: string, max = 100): asserts v is string[] {
  if (!Array.isArray(v) || v.length > max || v.some(x => typeof x !== 'string' || !x.trim() || x.length > 4000)) throw Error(`规划列表${label}无效。`);
  if (new Set(v).size !== v.length) throw Error(`规划列表${label}包含重复条目。`);
}
export function plannedPath(file: string): string {
  if (!relativeFile(file) || file.length > 240 || file.endsWith('/') || file.split('/').some(p => !p || /[. ]$/.test(p) || /^(?:con|prn|aux|nul|com[1-9]|lpt[1-9])(?:\.|$)/i.test(p)) || /[*?<>|]/.test(file) || excludedPath(file)) throw Error('规划包含不安全、敏感、生成目录或非精确文件路径。');
  return canonical(file);
}
/** Pure validation; never interprets a model report as a shell command or as user authorization. */
export function validateDevelopmentPlan(raw: unknown, context: PlanningContext, maxConcurrency = 3): PlanProposal {
  if (!raw || typeof raw !== 'object' || JSON.stringify(raw).length > 180000) throw Error('开发报告无效或超过180000字符。');
  const p = structuredClone(raw) as PlanProposal;
  if (p.evidence_id !== context.evidence_id || !kinds.includes(p.task_type)) throw Error('计划未绑定当前项目证据或任务分类无效。');
  for (const key of ['goal', 'current_project_state', 'technical_approach'] as const) text(p[key], key);
  strings(p.out_of_scope, 'out_of_scope', 40);
  if (!['STOP_AFTER_REQUEST', 'CONTINUE_APPROVED_PLAN'].includes(p.completion_policy) || p.completion_policy === 'CONTINUE_APPROVED_PLAN' && !context.continuous_authorized) throw Error('用户尚未授权连续阶段开发，必须完成当前需求后停止。');
  if (!Number.isInteger(maxConcurrency) || maxConcurrency < 1 || maxConcurrency > 8 || !Array.isArray(p.tasks) || !p.tasks.length || p.tasks.length > 40) throw Error('任务数量或并发配置无效。');
  const ids = new Set<string>(), workers = new Set<string>();
  const authority = userAuthority(context.original_request);
  for (const task of p.tasks) {
    if (!task || !/^[A-Za-z0-9_-]{1,80}$/.test(task.task_id) || ids.has(task.task_id)) throw Error('任务编号无效或重复。');
    ids.add(task.task_id);
    if (!/^[A-Za-z0-9_-]{1,80}$/.test(task.assigned_worker)) throw Error('Worker槽位无效。');
    workers.add(task.assigned_worker);
    text(task.task_description, 'task_description');
    for (const k of ['allowed_files', 'read_only_files', 'forbidden_files', 'dependencies', 'interface_contracts', 'acceptance_criteria', 'test_commands'] as const) strings(task[k], k);
    if (!task.allowed_files.length || !task.acceptance_criteria.length || !task.test_commands.length) throw Error('任务必须有精确文件范围、验收标准和实际测试要求。');
    const allow = task.allowed_files.map(plannedPath), read = task.read_only_files.map(plannedPath), forbid = [...task.forbidden_files, ...authority.forbidden].map(plannedPath);
    if (new Set(allow).size !== allow.length || allow.some(f => read.includes(f) || forbid.includes(f))) throw Error('任务的读写或禁止范围冲突。');
    if (task.base_commit !== context.base_commit) throw Error('任务基准提交与真实项目不一致。');
    if (task.status !== 'PENDING') throw Error('模型只能提出PENDING任务，不能自行标记运行或完成。');
    if (task.parent_task_id !== null && typeof task.parent_task_id !== 'string') throw Error('父任务无效。');
    // The executor supports only its own argv catalog; the planner never executes these strings.
    for (const command of task.test_commands) if (!/^(?:node --test|npm test|npm run compile|python(?:3)? -B [A-Za-z0-9_][A-Za-z0-9_./-]*\.py)$/.test(command) || command.includes('..')) throw Error('当前执行目录仅支持node --test、npm test、npm run compile、python3 -B test_example.py（实际任务脚本路径，可用python）。Python测试脚本必须包含unittest.main；不支持-m等额外参数。其他工具链可列为缺少的执行能力，不能擅自执行。');
    task.forbidden_files = [...new Set([...task.forbidden_files, ...authority.forbidden])];
  }
  if (!Number.isInteger(p.worker_count) || p.worker_count !== workers.size || workers.size > maxConcurrency) throw Error('Worker数量必须等于真实任务分配槽位数且不超过配置上限。');
  if (p.orchestration_version !== undefined && p.orchestration_version !== 2) throw Error('Unsupported orchestration version');
  if (p.orchestration_version === 2) {
    const a = p.analysis;
    if (!a || !['SIMPLE','MEDIUM','LARGE','EXTRA_LARGE'].includes(a.scale) || !['LOW','MEDIUM','HIGH'].includes(a.technical_complexity) || typeof a.parallelizable !== 'boolean') throw Error('Invalid structured task analysis');
    for (const key of ['technical_domain','estimated_work','dependency_reasoning','worker_rationale'] as const) text(a[key], key, 6000);
    for (const key of ['modules','expected_files','risks','human_confirmation'] as const) strings(a[key], key);
    a.expected_files.forEach(plannedPath);
    if (workers.size > maxConcurrency || (a.scale === 'SIMPLE' || !a.parallelizable) && workers.size !== 1) throw Error('Worker allocation exceeds configured limits or task parallelism');
    for (const task of p.tasks) {
      text(task.task_name, 'task_name', 200);
      if (!Number.isInteger(task.priority) || task.priority! < 1 || task.priority! > 5) throw Error('Task priority must be 1..5');
    }
  }
  if (workers.size > 1) text(p.parallel_rationale, 'parallel_rationale', 6000);
  else if (typeof p.parallel_rationale !== 'string') throw Error('缺少Worker分配说明。');
  const byId = new Map(p.tasks.map(t => [t.task_id, t]));
  const visiting = new Set<string>(), visited = new Set<string>();
  const visit = (t: PlannedTask) => {
    if (visiting.has(t.task_id)) throw Error('任务包含循环依赖。');
    if (visited.has(t.task_id)) return;
    visiting.add(t.task_id);
    for (const id of t.dependencies) { const dep = byId.get(id); if (!dep) throw Error('任务依赖不存在。'); visit(dep); }
    visiting.delete(t.task_id); visited.add(t.task_id);
  };
  for (const t of p.tasks) { if (t.parent_task_id && (!ids.has(t.parent_task_id) || t.parent_task_id === t.task_id)) throw Error('父任务不存在或指向自己。'); visit(t); }
  if (!Array.isArray(p.shared_file_owners) || p.shared_file_owners.length > 100) throw Error('共享文件所有权列表无效。');
  const owned = new Set<string>();
  for (const entry of p.shared_file_owners) {
    const file = plannedPath(entry.file), owner = byId.get(entry.task_id);
    if (owned.has(file) || !owner?.allowed_files.some(f => plannedPath(f) === file)) throw Error('共享文件必须有唯一且实际拥有写权限的任务。');
    owned.add(file);
    if (p.tasks.some(t => t.task_id !== entry.task_id && t.allowed_files.some(f => plannedPath(f) === file))) throw Error('公共文件只能由指定任务修改，其他任务应为只读。');
  }
  const ancestors = new Map<string, Set<string>>();
  const allDependencies = (t: PlannedTask): Set<string> => {
    const cached = ancestors.get(t.task_id); if (cached) return cached;
    const result = new Set<string>();
    for (const id of t.dependencies) { result.add(id); for (const parent of allDependencies(byId.get(id)!)) result.add(parent); }
    ancestors.set(t.task_id, result); return result;
  };
  const depends = (a: PlannedTask, b: string): boolean => allDependencies(a).has(b);
  const parallel = p.tasks.some((a, i) => p.tasks.slice(i + 1).some(b => a.assigned_worker !== b.assigned_worker && !depends(a, b.task_id) && !depends(b, a.task_id) && !a.allowed_files.some(f => b.allowed_files.some(g => plannedPath(f) === plannedPath(g)))));
  if (workers.size > 1 && !parallel) throw Error('没有互相独立的任务，多个Worker无并行收益；请合并Worker或重新划分。');
  if (parallel && p.tasks.some(t => !t.interface_contracts.length)) throw Error('并行前必须明确公共接口、协议与边界。');
  return p;
}

export class AdaptiveTaskPlanner {
  private contexts = new Map<string, PlanningContext>();
  private plans: DevelopmentPlan[] = [];
  constructor(readonly root: string, private collector: ReviewGit, private storage: ReviewStorage, private changed: () => void = () => {}, private maxConcurrency = 3, private maxRepairRounds = 2, private workerTimeout = 300000) {if(!Number.isInteger(maxRepairRounds)||maxRepairRounds<0||maxRepairRounds>5||!Number.isInteger(workerTimeout)||workerTimeout<1000||workerTimeout>3600000)throw Error('Invalid repair limit or Worker timeout');}
  async load() {
    const plans = await this.storage.read<DevelopmentPlan[]>(this.root, 'adaptive-plans.json') || [];
    if (!Array.isArray(plans) || plans.some(p => p.version !== 1 || p.project_path !== this.root || p.validation !== 'VALIDATED_NOT_EXECUTED')) throw Error('开发计划历史损坏，未覆盖。');
    this.plans = plans;
  }
  snapshot() { return { plans: this.plans, max_concurrency: this.maxConcurrency, execution_enabled: false, note: '规划保存不启动执行；执行通过 TeamSession 和隔离权限链。' }; }
  async capture(owner: string, turn: string, user: string): Promise<PlanningContext> {
    text(user, 'original_request', 24000);
    const before = await this.collector.snapshot(this.root);
    if (before.gaps.length) throw Error('项目快照有缺口，不能规划可执行任务；请缩小项目或处理采集缺口。');
    const status = await git(this.root, ['status', '--porcelain=v1', '-z', '--untracked-files=all']);
    const entries = status.split('\0').filter(Boolean), dirty: string[] = [];
    for (let i = 0; i < entries.length; i++) { const e = entries[i]; const f = e.slice(3); if (relativeFile(f) && !excludedPath(f)) dirty.push(f); if (/[RC]/.test(e.slice(0, 2))) { const previous = entries[++i]; if (previous && relativeFile(previous) && !excludedPath(previous)) dirty.push(previous); } }
    const contexts = before.files.filter(f => /(?:^|\/)(?:README\.md|package\.json|pyproject\.toml|Cargo\.toml|CMakeLists\.txt|go\.mod)$/i.test(f.path) && f.text !== undefined).slice(0, 6).map(f => ({ path: f.path, text: f.text!.slice(0, 8000) }));
    const evidence: PlanningContext = { evidence_id: randomUUID(), project_id: digest(this.root.toLowerCase()), project_path: this.root, owner_id: owner, turn_id: turn, original_request: safeReviewText(user), captured_at: new Date().toISOString(), base_commit: before.head, source_digest: before.digest, tracked_files: before.files.map(f => f.path), file_hashes: before.files.map(f => ({ path: f.path, hash: f.hash })), dirty_files: [...new Set(dirty)], contexts, gaps: [], continuous_authorized: false };
    if (JSON.stringify(evidence).length > 100000) throw Error('规划清单过大；需要分阶段缩小项目上下文，不发送整个项目。');
    this.contexts.set(evidence.evidence_id, evidence);
    evidence.orchestration_limits = { max_workers: this.maxConcurrency, max_repair_rounds: this.maxRepairRounds };
    const previous=await this.storage.read<any>(this.root,'project-context.json');
    if(previous){if(previous.version!==1||!Array.isArray(previous.completed_tasks)||!Array.isArray(previous.pending_tasks))throw Error('项目继承记录损坏，不能盲目复用。');evidence.previous_context={...previous,requires_revalidation:previous.head!==before.head,note:'继承记录只供参考，当前真实代码、Git状态和本次用户授权优先；不得自动重发旧任务。'};}
    await this.storage.write(this.root, `planning-context-${evidence.evidence_id}.json`, evidence);
    return evidence;
  }
  async submit(owner: string, turn: string, proposal: unknown): Promise<DevelopmentPlan> {
    const evidenceId = (proposal as any)?.evidence_id;
    const c = this.contexts.get(evidenceId);
    if (!c || c.owner_id !== owner || c.turn_id !== turn) throw Error('规划证据不属于当前GPT会话和消息，请重新采集。');
    if ((await this.collector.snapshot(this.root)).digest !== c.source_digest) throw Error('项目已改变，旧规划证据失效，请重新采集后规划。');
    const p = validateDevelopmentPlan(proposal, c, this.maxConcurrency);
    const existing = this.plans.find(x => x.evidence_id === evidenceId);
    if (existing) { if (digest(JSON.stringify(p)) !== digest(JSON.stringify(Object.fromEntries(Object.keys(p).map(k => [k, (existing as any)[k]]))))) throw Error('同一证据已经提交其他计划；重新采集证据后提交新版本。'); return existing; }
    const explicit = userAuthority(c.original_request).allowed.map(canonical);
    const plan: DevelopmentPlan = { ...p, version: 1, plan_id: randomUUID(), project_id: c.project_id, project_path: this.root, owner_id: owner, original_request: c.original_request, source_digest: c.source_digest, created_at: new Date().toISOString(), validation: 'VALIDATED_NOT_EXECUTED', user_approval_required: p.tasks.some(t => t.allowed_files.some(f => !explicit.includes(canonical(f)))), max_concurrency: this.maxConcurrency };
    this.plans.unshift(plan);
    plan.max_repair_rounds = this.maxRepairRounds;
    plan.worker_timeout_ms = this.workerTimeout;
    try { await this.storage.write(this.root, 'adaptive-plans.json', this.plans); } catch (e) { this.plans.shift(); throw e; }
    this.changed(); return plan;
  }
}
