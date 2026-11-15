import * as fs from 'node:fs/promises';
import { randomUUID } from 'node:crypto';
import { ReviewStorage } from '../review/reviewStorage';
import { ReviewGit } from '../review/reviewGit';
import { ReviewEngine } from '../review/reviewEngine';
import { ReviewTestRunner, type TestCommand } from '../review/reviewTests';
import { safeReviewText } from '../review/reviewSafety';
import { sumUsage } from '../services/codexUsageService';
import { RepairModel, planHash, planRisks, claudeRepairPrompt } from './repairModel';
import type { ReviewReport, ReviewTask } from '../review/reviewTypes';
import type { RepairRecord, RepairCheck, IssueDisposition, RepairPlan } from './repairTypes';
export interface ClaudeRepairPort {
  snapshot(): any;
  sendRepair(text: string, conversation: string, session: string, plan: RepairPlan, observe: (event: any) => void): Promise<void>;
  stop(): Promise<void>;
}
export type Approver = (record: RepairRecord, risks: string[]) => Promise<{ approved: boolean; risks: string[]; actor: string }>;
export class RepairWorkflow {
  records: RepairRecord[] = [];
  dispositions: IssueDisposition[] = [];
  busy = false;
  phase = '';
  private current?: RepairRecord;
  private stopped = false;
  constructor(public root: string, private storage: ReviewStorage, private git: ReviewGit,
    private model: RepairModel, private engine: ReviewEngine, private tests: ReviewTestRunner,
    private claude: ClaudeRepairPort, private changed: () => void = () => {}) {}
  private async save(r: RepairRecord) {
    r.updated_at = new Date().toISOString();
    await this.storage.write(this.root, `repair-${r.repair_id}.json`, r); this.changed();
  }
  async load() {
    const names = await fs.readdir(this.storage.project(this.root)).catch((e: any) => { if (e.code === 'ENOENT') return []; throw e; });
    this.records = [];
    for (const n of names.filter(n => /^repair-[\w-]+\.json$/.test(n))) {
      const r = await this.storage.read<RepairRecord>(this.root, n);
      if (!r || r.version !== 1 || r.project_path !== this.root || !Array.isArray(r.approvals)) throw new Error('修复历史无效，未覆盖。');
      if (['SENDING', 'VERIFYING'].includes(r.state) || (!r.plan && r.state === 'DRAFT')) {
        r.state = 'INTERRUPTED'; r.error = '上次操作中断；不会自动重发。请检查代码与会话后新建单轮修复。'; await this.save(r);
      }
      this.records.push(r);
    }
    this.records.sort((a,b) => b.created_at.localeCompare(a.created_at));
    this.dispositions = await this.storage.read<IssueDisposition[]>(this.root, 'issue-dispositions.json') || [];
  }
  async disposition(report: ReviewReport, ids: string[], value: IssueDisposition['disposition']) {
    if (!['OPEN','IGNORED','HUMAN'].includes(value) || !ids.length || ids.some(id => !report.issues.some(i => i.issue_id === id))) throw new Error('无效问题选择。');
    for (const id of new Set(ids)) {
      this.dispositions = this.dispositions.filter(x => !(x.review_id === report.review_id && x.issue_id === id));
      this.dispositions.push({ review_id: report.review_id, issue_id: id, disposition: value, time: new Date().toISOString() });
    }
    await this.storage.write(this.root, 'issue-dispositions.json', this.dispositions); this.changed();
  }
  private async exclusive<T>(phase: string, action: () => Promise<T>): Promise<T> {
    if (this.busy || this.claude.snapshot().busy) throw new Error('请先等待当前修复或Claude任务结束。');
    this.busy = true; this.phase = phase; this.stopped = false; this.changed();
    try { return await action(); } finally { this.busy = false; this.phase = ''; this.current = undefined; this.changed(); }
  }
  async generate(report: ReviewReport, ids: string[], requirements = '') {
    return this.exclusive('GPT生成修复方案（尚未批准）', async () => {
      if (report.project_path !== this.root || report.stale || !ids.length || ids.length > 20 || new Set(ids).size !== ids.length) throw new Error('报告或选择无效；最多选择20个问题。');
      const issues = ids.map(id => report.issues.find(i => i.issue_id === id));
      if (issues.some(i => !i || i.evidence_status !== 'VERIFIED' || i.basis !== 'after')) throw new Error('仅支持有可验证当前代码位置的问题；其他问题请标记人工处理。');
      if (requirements.length > 16000) throw new Error('补充修复要求最多16000字符。');
      const before = await this.git.snapshot(this.root);
      if (before.digest !== report.evidence.after.digest || before.gaps.length) throw new Error('代码已不同于原始审查快照或采集不完整；请先重新审查。');
      const now = new Date().toISOString(), delivery = report.evidence.delivery;
      const r: RepairRecord = { version: 1, repair_id: randomUUID(), review_id: report.review_id, task_id: report.task.task_id,
        project_path: this.root, created_at: now, updated_at: now, state: 'DRAFT', original_issues: issues as any,
        before, requirements: safeReviewText(requirements), original_session: delivery.session_id,
        original_conversation: delivery.conversation_id, approvals: [], events: [], event_gaps: [], checks: [], sent_once: false, gpt_calls: [] };
      this.records.unshift(r); this.current = r; await this.save(r);
      try {
        r.plan = await this.model.plan(report, r, () => this.save(r)); r.plan_hash = planHash(r.plan);
        if (this.stopped) throw new Error('修复方案生成已停止。');
        if ((await this.git.snapshot(this.root)).digest !== before.digest) throw new Error('方案生成期间代码已变化，请重新审查。');
      } catch (e) { r.state = 'FAILED'; r.error = safeReviewText(e instanceof Error ? e.message : '方案生成失败'); }
      await this.save(r); return r;
    });
  }
  get(id: string) { const r = this.records.find(r => r.repair_id === id); if (!r) throw new Error('修复任务不存在。'); return r; }
  async defer(id: string, rejected = false) {
    if (this.busy) throw new Error('请先等待当前操作结束。');
    const r = this.get(id); if (r.sent_once || !['DRAFT','DEFERRED'].includes(r.state)) throw new Error('此修复任务不能更改审批状态。');
    r.state = rejected ? 'REJECTED' : 'DEFERRED';
    r.approvals.push({ time: new Date().toISOString(), actor: 'USER', decision: r.state, plan_hash: r.plan_hash || '', source_digest: r.before.digest, risks: [] });
    await this.save(r);
  }
  async execute(id: string, approve: Approver) {
    return this.exclusive('等待人工审批 / Claude单轮修复', async () => {
      const r = this.get(id); this.current = r;
      if (r.sent_once || !['DRAFT','DEFERRED'].includes(r.state) || !r.plan || r.plan_hash !== planHash(r.plan)) throw new Error('此方案不可发送或已经发送；不会重复执行。');
      if (r.plan.missing_context.length) throw new Error('GPT要求补充上下文，不能批准发送。');
      if (!r.original_session || !r.original_conversation) throw new Error('缺少原始Claude Session，不能静默创建新会话。请先恢复原会话并重新审查。');
      const verify = async () => {
        if (!r.plan || r.plan_hash !== planHash(r.plan)) throw new Error('审批期间方案已变化，未发送。');
        if (this.stopped || (await this.git.snapshot(this.root)).digest !== r.before.digest) throw new Error('审批前后代码发生变化或操作已停止，未发送。');
        const c = this.claude.snapshot();
        if (c.busy || c.projectPath !== this.root || c.conversation?.id !== r.original_conversation || c.conversation?.threadId !== r.original_session)
          throw new Error('原Claude Session未恢复或当前会话已变化；请在Claude历史中恢复原会话，未发送。');
      };
      await verify();
      const risks = planRisks(r), approval = await approve(structuredClone(r), risks);
      r.approvals.push({ time: new Date().toISOString(), actor: safeReviewText(approval.actor), decision: approval.approved ? 'APPROVED' : 'DECLINED', plan_hash: r.plan_hash!, source_digest: r.before.digest, risks: approval.risks });
      await this.save(r);
      if (!approval.approved) return r;
      if (risks.some(x => !approval.risks.includes(x))) throw new Error('高风险操作缺少单独审批，未发送。');
      await verify();
      r.sent_once = true; r.state = 'SENDING'; r.received_at = new Date().toISOString();
      r.events.push({ time: r.received_at, kind: 'DISPATCH_COMMITTED', detail: { plan_hash: r.plan_hash } });
      await this.save(r); // durable at-most-once marker BEFORE touching Claude
      r.started_at = new Date().toISOString(); await this.save(r);
      let journal: Promise<void> = Promise.resolve();
      try {
        await this.claude.sendRepair(claudeRepairPrompt(r), r.original_conversation, r.original_session, r.plan, event => {
          if (event.type === 'text' && !event.completed) return; // UI still streams; journal keeps final text, not repeated cumulative deltas.
          if (r.events.length >= 5000) { if (!r.event_gaps.length) r.event_gaps.push('工具/状态事件超过5000条，后续事件未保存'); return; }
          r.events.push({ time: new Date().toISOString(), kind: 'CLAUDE_EVENT', detail: JSON.parse(safeReviewText(JSON.stringify(event))) });
          journal = journal.then(() => this.save(r)).catch(() => { if (!r.event_gaps.includes('执行日志持久化失败')) r.event_gaps.push('执行日志持久化失败'); });
        });
        const c = this.claude.snapshot(), request = c.conversation?.requests?.at(-1);
        r.request_id = request?.id; r.session_id = c.conversation?.threadId;
        r.final_answer = safeReviewText((c.conversation?.messages || []).filter((m: any) => m.role === 'assistant' && m.requestId === request?.id).map((m: any) => m.text).join('\n')).slice(0,32000);
        if (c.error || request?.status !== 'COMPLETED' || r.session_id !== r.original_session) throw new Error(c.error || 'Claude修复未成功结束或Session恢复失败。');
        r.state = 'EXECUTED';
      } catch (e) { r.state = 'FAILED'; r.error = safeReviewText(e instanceof Error ? e.message : 'Claude发送或执行失败；不会自动重发。'); }
      finally {
        await journal; r.finished_at = new Date().toISOString();
        try { await this.collect(r); } catch (e) { r.state = 'FAILED'; r.event_gaps.push('修复后证据采集失败：' + safeReviewText(e instanceof Error ? e.message : '未知错误')); }
        await this.save(r);
      }
      return r;
    });
  }
  private async collect(r: RepairRecord) {
    r.evidence = await this.git.evidence(r.before, await this.git.snapshot(this.root),
      { task_id: r.task_id, task_description: r.plan!.problem_description, technical_requirements: r.plan!.repair_strategy,
        allowed_files: r.plan!.allowed_files, forbidden_files: r.plan!.forbidden_files },
      { status: r.state, final_answer: r.final_answer || '', session_id: r.session_id, request_id: r.request_id, conversation_id: r.original_conversation },
      r.checks.filter(c => c.command !== 'npm run compile'), 'CLAUDE_CAPTURE');
  }
  async runCheck(id: string, command: TestCommand) {
    return this.exclusive('执行用户选择的编译/测试', async () => {
      const r = this.get(id); this.current = r;
      if (!r.sent_once || !['EXECUTED','FAILED'].includes(r.state)) throw new Error('请先执行已批准的修复。');
      const result = await this.tests.run(this.root, command); r.checks.push(result);
      await this.collect(r); await this.save(r); return result;
    });
  }
  async verify(id: string, original: ReviewReport) {
    return this.exclusive('GPT独立复审选中问题', async () => {
      const r = this.get(id); this.current = r;
      if (!r.sent_once || !['EXECUTED','FAILED'].includes(r.state) || !r.plan || original.review_id !== r.review_id) throw new Error('此任务不可复审；每个修复只运行一轮复审。');
      r.state = 'VERIFYING'; await this.save(r);
      try {
        await this.collect(r);
        const evidence = structuredClone(r.evidence!);
        evidence.builds = r.checks.filter(c => c.command === 'npm run compile' && c.source_digest === evidence.after.digest);
        const selectedFiles = new Set(r.original_issues.map(i => i.file_path));
        const relevant = new Set([...selectedFiles, ...r.plan.allowed_files, ...evidence.changes.map(c => c.path)]);
        evidence.inspection_files = evidence.after.files.filter(f => selectedFiles.has(f.path) && !evidence.changes.some(c => c.path === f.path));
        if (evidence.inspection_files.length && !evidence.changes.length)
          evidence.gaps = evidence.gaps.filter(g => g !== '没有可审查的实际文本变更');
        evidence.context = [...new Set([...original.evidence.context, ...evidence.context].map(f => f.path))]
          .flatMap(p => evidence.after.files.filter(f => f.path === p))
          .filter(f => f.text !== undefined);
        if (evidence.context.reduce((n,f) => n + (f.text?.length || 0),0) > 64000) throw new Error('必要复审上下文超过64000字符，请人工处理；未静默截断。');
        evidence.task = { ...original.task, technical_requirements: original.task.technical_requirements + '\n单轮修复计划：' + JSON.stringify(r.plan),
          allowed_files: r.plan.allowed_files, forbidden_files: r.plan.forbidden_files,
          repair_context: { repair_id: r.repair_id, review_id: r.review_id, issues: r.original_issues, plan: r.plan } };
        evidence.gaps.push(...r.event_gaps);
        if (r.state === 'VERIFYING' && r.error) evidence.warnings.push('Claude执行存在错误：' + r.error);
        const buildRequired = r.plan.test_requirements.some(s => /编译|compile|build/i.test(s));
        if (buildRequired && !r.checks.some(c => c.command === 'npm run compile' && c.status === 'PASSED' && c.source_digest === evidence.after.digest))
          evidence.gaps.push('方案要求编译，但缺少本次快照实际编译成功记录');
        const report = await this.engine.run(evidence);
        r.verification_review_id = report.review_id;
        r.gpt_calls.push(...report.batches.map(b => ({ phase: 'VERIFY', session_id: b.session_id, usage: b.usage, error: b.error })));
        r.verification = resolveChecks(r, report);
        r.verification_summary = `${report.verdict}；${r.verification.map(c => `${c.issue_id}=${c.status}`).join('，')}`;
        r.state = 'FINISHED';
      } catch (e) {
        r.state = 'FINISHED'; r.error = safeReviewText(e instanceof Error ? e.message : '复审失败');
        r.verification = r.original_issues.map(i => ({ issue_id: i.issue_id, status: 'UNVERIFIED', explanation: r.error!, file_path: i.file_path, line_number: null, evidence: '' }));
      }
      await this.save(r); return r;
    });
  }
  usage(r: RepairRecord) { return sumUsage(r.gpt_calls); }
  async stop() { this.stopped = true; await Promise.allSettled([this.model.stop(), this.engine.stop(), this.tests.stop(), ...(this.current?.state === 'SENDING' ? [this.claude.stop()] : [])]); }
}
export function resolveChecks(r: RepairRecord, report: ReviewReport): RepairCheck[] {
  const unauthorized = report.evidence.changes.filter(c => !r.plan!.allowed_files.includes(c.path) || r.plan!.forbidden_files.includes(c.path) || (c.kind === 'DELETED' && !r.plan!.delete_files.includes(c.path)));
  return r.original_issues.map(issue => {
    const checks = report.batches.flatMap(b => b.repair_checks || []).filter(c => c.issue_id === issue.issue_id);
    let check: RepairCheck = checks.find(c => c.status === 'REGRESSION') || checks.find(c => c.status === 'NOT_FIXED') || checks[0] ||
      { issue_id: issue.issue_id, status: 'UNVERIFIED', explanation: 'GPT没有提供该原始问题的结构化验证结论。', file_path: issue.file_path, line_number: null, evidence: '' };
    if (unauthorized.length) return { ...check, status: 'REGRESSION', explanation: '发现实际越权/未批准删除：' + unauthorized.map(c => c.path).join(', ') };
    if (report.stale || report.batches.some(b => b.error) || report.batches.length !== 1)
      return { ...check, status: 'UNVERIFIED', explanation: '证据不完整、代码过期或跨批结论需要人工确认。' };
    if (check.status === 'REGRESSION' && (!report.issues.some(i => i.evidence_status === 'VERIFIED' && i.basis === 'after' && i.file_path === check.file_path &&
      !!i.line_number && !!check.line_number && i.line_number >= check.line_number && i.line_number <= check.line_number + check.evidence.split('\n').length - 1 && check.evidence.includes(i.evidence)) ||
      !report.evidence.changes.some(c => c.path === check.file_path && c.kind !== 'DELETED' && !c.before?.text?.includes(check.evidence))))
      return { ...check, status: 'UNVERIFIED', explanation: 'GPT未给出可验证的新问题代码位置，回归判断需人工确认。' };
    if (check.status === 'FIXED' && (report.verdict !== 'PASS' || report.missing_evidence.length || r.event_gaps.length || !report.evidence.tests.some(t => t.status === 'PASSED') || report.evidence.tests.some(t => t.status !== 'PASSED') || r.error))
      check = { ...check, status: 'UNVERIFIED', explanation: 'GPT声称修复，但独立审查、测试或Claude执行证据未全部通过。' };
    return check;
  });
}
