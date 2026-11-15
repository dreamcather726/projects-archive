import * as vscode from 'vscode';
import * as fs from 'node:fs/promises';
import * as path from 'node:path';
import { createHash } from 'node:crypto';
import { PermissionPolicy, validateScope, defaultScope, commandCatalog } from './permissionPolicy';
import { PermissionHost, type PermissionEvaluator } from './permissionHost';
import { safeReviewText } from '../review/reviewSafety';
import type { TaskPermissionScope, PermissionOperation, PermissionLog, PermissionChoice } from './permissionTypes';
export class PermissionManager {
  private scope = defaultScope();
  private pending = new Map<string,{ operation: PermissionOperation; resolve: (choice: PermissionChoice) => void }>();
  private logs: PermissionLog[] = [];
  private host?: PermissionHost;
  private taskId?: string;
  private activeScope?: TaskPermissionScope;
  private queue: Promise<void> = Promise.resolve();
  private error?: string;
  private activeMode?: string;
  private auditPath: string;
  private scopeKey: string;
  constructor(private context: vscode.ExtensionContext, readonly root: string, private changed: () => void) {
    const id = createHash('sha256').update(process.platform === 'win32' ? root.toLowerCase() : root).digest('hex');
    this.scopeKey = `claude.permissions.${id}`;
    this.auditPath = path.join(context.globalStorageUri.fsPath,'permissions',`${id}.jsonl`);
  }
  async load() {
    this.scope = validateScope(this.context.workspaceState.get<TaskPermissionScope>(this.scopeKey) || defaultScope());
    const file = await fs.open(this.auditPath,'r').catch((e: any) => { if (e.code === 'ENOENT') return undefined; throw e; });
    if (file) try {
      const size = (await file.stat()).size, start = Math.max(0,size - 256000), data = Buffer.alloc(size-start);
      await file.read(data,0,data.length,start);
      const lines = data.toString('utf8').split('\n'); if (start) lines.shift();
      this.logs = lines.filter(Boolean).slice(-100).map(s => JSON.parse(s));
    } finally { await file.close(); }
  }
  snapshot() {
    return { mode: this.activeMode || 'manual + task hooks', scope: this.activeScope || this.scope, taskId: this.taskId, active: !!this.host,
      pending: [...this.pending.values()].map(p => p.operation), logs: this.logs, error: this.error, auditPath: this.auditPath,
      note: '授权文件外的修改直接拒绝；本次任务授权在当前请求结束后失效。白名单脚本会执行项目代码，并非操作系统沙箱。' };
  }
  async configure(scope: TaskPermissionScope) {
    if (this.host) throw new Error('请先结束Claude当前任务再修改授权范围。');
    if (!vscode.workspace.isTrusted) throw new Error('请先信任工作区。');
    const valid = validateScope(scope); this.scope = valid;
    await this.context.workspaceState.update(this.scopeKey,valid); this.changed();
  }
  async configureUI() {
    if (this.host) throw new Error('请先结束Claude当前任务再修改授权范围。');
    const files = await vscode.window.showInputBox({ title: 'Claude任务允许修改的文件',
      prompt: '以逗号或换行分隔精确相对路径；留空为只读。其他文件禁止修改，不接受通配符。此范围用于后续任务，直到你主动修改。',
      value: this.scope.allowedFiles.join(', '), ignoreFocusOut: true,
      validateInput: v => { try { validateScope({allowedFiles:splitFiles(v),safeCommands:[]});return undefined; } catch(e) {return (e as Error).message;} } });
    if (files === undefined) return;
    const selected = await vscode.window.showQuickPick(commandCatalog.map(c => ({label:c,picked:this.scope.safeCommands.includes(c)})),
      { title:'精确命令白名单',placeHolder:'测试/构建命令会执行项目脚本；选择即授权在任务中执行。复合命令始终拒绝。',canPickMany:true,ignoreFocusOut:true });
    if (!selected) return;
    await this.configure({allowedFiles:splitFiles(files),safeCommands:selected.map(c=>c.label)});
  }
  private approve = (operation: PermissionOperation, signal: AbortSignal): Promise<PermissionChoice> => new Promise(resolve => {
    const cancel = new vscode.CancellationTokenSource();
    const settle = (choice: PermissionChoice) => { if (!this.pending.has(operation.id)) return; this.pending.delete(operation.id);signal.removeEventListener('abort',abort);cancel.cancel();cancel.dispose();resolve(choice);this.changed(); };
    const abort = () => settle('DENY');
    this.pending.set(operation.id,{operation,resolve:settle}); this.changed();
    signal.addEventListener('abort',abort,{once:true});
    if (signal.aborted) { abort();return; }
    void vscode.window.showQuickPick([
      {label:'允许一次',choice:'ONCE' as const,description:'仅批准当前工具调用'},
      {label:'允许本次任务',choice:'TASK' as const,description:'仅批准当前任务对同一文件的同类操作或同一精确命令'},
      {label:'拒绝',choice:'DENY' as const,description:'该操作不得执行'},
    ],{title:`Claude权限审批 · ${operation.tool} · ${operation.operation}`,placeHolder:`${operation.target} — ${operation.reason}（可在工作台查看操作预览）`,ignoreFocusOut:true},cancel.token)
      .then(choice => settle(choice?.choice || 'DENY'),() => settle('DENY'));
  });
  answer(id: string, choice: PermissionChoice) {
    if (!['ONCE','TASK','DENY'].includes(choice)) return;
    this.pending.get(id)?.resolve(choice);
  }
  private record = async (entry: PermissionLog) => {
    const sanitized = JSON.parse(safeReviewText(JSON.stringify(entry))) as PermissionLog;
    const pending = this.queue.then(async () => { await fs.mkdir(path.dirname(this.auditPath),{recursive:true});await fs.appendFile(this.auditPath,JSON.stringify(sanitized)+'\n',{mode:0o600}); });
    this.queue = pending;
    try { await pending;this.logs.push(sanitized);if(this.logs.length>100)this.logs.shift();this.changed(); }
    catch { this.error = '授权日志写入失败；当前工具调用被阻止。';this.changed();throw new Error(this.error); }
  };
  async start(taskId: string, session?: string, override?: TaskPermissionScope, repair?: { deleteFiles: string[] }, observe?: (entry: PermissionLog) => void,
    gateway?: {mode:string;policy:PermissionPolicy;evaluate:PermissionEvaluator}) {
    if (this.host) throw new Error('已有Claude任务权限主机。');
    this.error = undefined;this.taskId = taskId;this.activeScope = validateScope(structuredClone(override || this.scope));
    await this.record({time:new Date().toISOString(),taskId,sessionId:session,tool:'SYSTEM',operation:'TASK_SCOPE',target:this.root,decision:'ALLOW',source:'TASK_START',reason:'已锁定本次任务授权范围；不代表批准任何具体工具调用。',scope:this.activeScope});
    this.activeMode=gateway?.mode;
    const host = await new PermissionHost(gateway?.policy || new PermissionPolicy(this.root,this.activeScope,repair),taskId,this.approve,async entry => { await this.record(entry);observe?.(entry); },90000,session,gateway?.evaluate).start();
    this.host = host;this.changed();
    return { settings:host.settings,canRetry:()=>host.canRetry(),close:async () => {
      await host.close();await this.queue.catch(()=>{});
      if(this.host===host){this.host=undefined;this.activeScope=undefined;this.activeMode=undefined;this.changed();}
    } };
  }
  async dispose() { for (const p of [...this.pending.values()])p.resolve('DENY');await this.host?.close();this.host=undefined;await this.queue.catch(()=>{}); }
}
const splitFiles = (text: string) => text.split(/[,\r\n]+/).map(v=>v.trim()).filter(Boolean);
