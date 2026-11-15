import * as vscode from 'vscode';
import * as fs from 'node:fs/promises';
import * as path from 'node:path';
import { CodexService } from '../services/codexService';
import { resolveCodexExecutable } from '../process/codexExecutableResolver';
import { ReviewGit } from '../review/reviewGit';
import { ReviewEngine } from '../review/reviewEngine';
import { ReviewStorage } from '../review/reviewStorage';
import { ReviewTestRunner, type TestCommand } from '../review/reviewTests';
import { safeReviewText } from '../review/reviewSafety';
import { RepairModel } from './repairModel';
import { RepairWorkflow, type ClaudeRepairPort, type Approver } from './repairWorkflow';
import type { RepairRecord } from './repairTypes';

export class RepairCoordinator {
  private workflow?: RepairWorkflow;
  private selected?: string;
  private storage: ReviewStorage;
  private error?: string;
  private preparing = false;
  constructor(private context: vscode.ExtensionContext, private claude: ClaudeRepairPort,
    private reviewBusy: () => boolean, private changed: () => void, private log: (message: string, level?: string, source?: string) => void) {
    this.storage = new ReviewStorage(path.join(context.globalStorageUri.fsPath, 'reviews'));
  }
  snapshot() {
    const w = this.workflow, r = w?.records.find(r => r.repair_id === this.selected) || w?.records[0];
    return { busy: !!(this.preparing || w?.busy), progress: w?.phase, error: this.error,
      record: r ? { ...r, before: undefined, evidence: r.evidence ? { changes: r.evidence.changes, gaps: r.evidence.gaps, warnings: r.evidence.warnings } : undefined, gpt_usage: w!.usage(r) } : undefined,
      dispositions: w?.dispositions || [], history: w?.records.map(r => ({ repair_id: r.repair_id, review_id: r.review_id, state: r.state, title: r.plan?.title || r.repair_id })) || [] };
  }
  get busy() { return !!(this.preparing || this.workflow?.busy); }
  private async ensure() {
    if (!vscode.workspace.isTrusted) throw new Error('请先信任当前工作区。');
    const candidate = this.claude.snapshot().projectPath;
    if (!candidate) throw new Error('请先选择Claude本地项目。');
    const root = await fs.realpath(candidate);
    if (!vscode.workspace.workspaceFolders?.some(f => f.uri.scheme === 'file' && path.resolve(f.uri.fsPath).toLowerCase() === root.toLowerCase())) throw new Error('修复项目不在当前工作区。');
    if (this.workflow?.root === root) return this.workflow;
    if (this.workflow?.busy) throw new Error('当前修复仍在执行，不能切换项目。');
    const service = () => new CodexService(vscode.workspace.getConfiguration('aiTeamManager').get<string>('codexExecutable','codex'), undefined,
      command => resolveCodexExecutable(command, { extensionDirectory: () => vscode.extensions.getExtension('openai.chatgpt')?.extensionUri.fsPath }));
    const git = new ReviewGit(path.join(this.storage.directory, 'temporary'));
    const w = new RepairWorkflow(root, this.storage, git, new RepairModel(service(), this.storage.directory),
      new ReviewEngine(service(), git, this.storage, this.changed), new ReviewTestRunner(git), this.claude, this.changed);
    await w.load(); this.workflow = w; return w;
  }
  async load() {
    try { await this.ensure(); } catch (e) {
      if (this.claude.snapshot().projectPath) { this.error = safeReviewText(e instanceof Error ? e.message : '修复历史加载失败，未覆盖。'); this.log(this.error,'ERROR','System'); }
    }
    this.changed();
  }
  // Trusted extension API, like ReviewCoordinator.runTests. Webview always uses the native command picker.
  async runCheck(id: string, command: TestCommand) {
    if (this.busy || this.reviewBusy()) throw new Error('请先等待当前操作结束。');
    this.preparing = true; this.changed();
    try { const result = await (await this.ensure()).runCheck(id, command); this.log(`修复 ${id} · ${command}: ${result.status} · exit=${result.exit_code}`,'INFO','Test'); return result; }
    finally { this.preparing = false; this.changed(); }
  }
  // The workbench click approves the exact preview hash. Higher risks require separate native approval.
  private approve: Approver = async (r, risks) => {
    const p = r.plan!;
    const approved: string[] = [];
    for (const risk of risks) {
      const choice = await vscode.window.showWarningMessage(`单独批准高风险行为：${risk}`, { modal: true,
        detail: `新增范围：${p.allowed_files.filter(f => !r.original_issues.some(i => i.file_path === f)).join(', ')}\n删除：${p.delete_files.join(', ')}\n公共接口变更：${p.public_interface_changes.join('; ')}\n风险：${p.risk_level}\n取消将不发送整个修复任务。` }, '单独批准');
      if (choice !== '单独批准') return { approved: false, risks: approved, actor: 'USER_NATIVE_DIALOG' };
      approved.push(risk);
    }
    return { approved: true, risks: approved, actor: 'USER_WORKBENCH_PREVIEW' };
  };
  async handle(m: any) {
    if (m?.type === 'repairStop') { await this.workflow?.stop(); return; }
    if (this.preparing) return;
    this.preparing = true; this.error = undefined; this.changed();
    try {
      if (this.reviewBusy()) throw new Error('请先等待原始审查、测试或证据采集完成。');
      const w = await this.ensure();
      const reports = await this.storage.reports(w.root);
      if (m.type === 'repairGenerate' || m.type === 'repairEdit') {
        const old = m.type === 'repairEdit' ? w.get(m.repairId) : undefined;
        const report = reports.find(r => r.review_id === (old?.review_id || m.reviewId));
        if (!report) throw new Error('原始审查报告不存在。');
        const requirements = m.type === 'repairEdit' ? await vscode.window.showInputBox({ title: '修改修复要求（生成新方案并重新审批）', value: old!.requirements, ignoreFocusOut: true, validateInput: v => v.length <= 16000 ? undefined : '最多16000字符' }) : m.requirements || '';
        if (requirements === undefined) return;
        if (old?.sent_once) throw new Error('已发送任务不能编辑或自动重试；请重新审查后手动选择新的修复。');
        const r = await w.generate(report, old?.plan?.issue_ids || m.issueIds, requirements);
        this.selected = r.repair_id;
        if (old) await w.defer(old.repair_id);
        this.log(`修复方案 ${r.repair_id} · ${r.state}；尚未发送Claude`, 'INFO', 'GPT');
      } else if (m.type === 'repairDisposition') {
        const report = reports.find(r => r.review_id === m.reviewId); if (!report) throw new Error('原始报告不存在。');
        await w.disposition(report, m.issueIds, m.disposition);
      } else if (m.type === 'repairApprove') {
        const r = w.get(m.repairId);
        if (!m.planHash || m.planHash !== r.plan_hash) throw new Error('预览方案已变化，请重新查看后批准。');
        const result = await w.execute(r.repair_id, this.approve);
        this.log(`修复 ${result.repair_id} · ${result.state}`,result.error ? 'ERROR' : 'INFO','Claude');
      }
      else if (m.type === 'repairDefer' || m.type === 'repairReject') await w.defer(m.repairId, m.type === 'repairReject');
      else if (m.type === 'repairSelect') { w.get(m.repairId); this.selected = m.repairId; }
      else if (m.type === 'repairCheck') {
        const command = await vscode.window.showQuickPick(['node --test', 'npm test', 'npm run compile'], { title: '运行项目脚本（会执行当前项目代码）', placeHolder: '明确选择实际编译或测试；不会自动执行GPT生成的命令' });
        if (command) { const result = await w.runCheck(m.repairId || this.selected || w.records[0]?.repair_id, command as TestCommand); this.log(`修复检查 ${command}: ${result.status} · exit=${result.exit_code}`,result.status==='PASSED'?'INFO':'WARN','Test'); }
      } else if (m.type === 'repairVerify') {
        const id = m.repairId || this.selected || w.records[0]?.repair_id, r = w.get(id), report = reports.find(x => x.review_id === r.review_id);
        if (!report) throw new Error('缺少原始报告，不能复审。');
        const result = await w.verify(id, report);
        this.log(`修复复审 ${result.repair_id} · ${result.verification_summary || result.error}`,'INFO','GPT');
      } else if (m.type === 'repairReport') {
        const r = w.get(m.repairId), doc = await vscode.workspace.openTextDocument({ content: JSON.stringify({ ...r, gpt_usage: w.usage(r) }, null, 2), language: 'json' });
        await vscode.window.showTextDocument(doc);
      }
    } catch (e) { this.error = safeReviewText(e instanceof Error ? e.message : '修复操作失败'); this.log(this.error, 'ERROR', 'System'); }
    finally { this.preparing = false; this.changed(); }
  }
  async dispose() { await this.workflow?.stop(); }
}
