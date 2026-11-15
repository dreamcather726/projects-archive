import * as vscode from "vscode";
import { realpath } from "node:fs/promises";
import * as path from "node:path";
import { ClaudeStorage } from "./claudeStorage";
import { ClaudeController } from "./claudeController";
import { ClaudeService } from "./claudeService";
import { resolveClaudeExecutable } from "./claudeExecutableResolver";
import { ClaudeError } from "./claudeErrors";
import { PermissionManager } from '../permissions/permissionManager';
import type { TaskPermissionScope } from '../permissions/permissionTypes';
import type { RepairPlan } from '../repair/repairTypes';
import type { PermissionEvaluator } from '../permissions/permissionHost';
import type { PermissionPolicy } from '../permissions/permissionPolicy';
export class ClaudeCoordinator {
  private controller?: ClaudeController;
  private folder?: vscode.WorkspaceFolder;
  private initializing?: Promise<ClaudeController>;
  private error?: string;
  private permissions?: PermissionManager;
  constructor(
    private context: vscode.ExtensionContext,
    private changed: () => void,
  ) {}
  snapshot() {
    return {
      projectName: this.folder?.name || "未选择工作区",
      error: this.error,
      ...(this.controller?.snapshot() || {}),
      connectedAdapter: true,
      permissions: this.permissions?.snapshot(),
    };
  }
  private async ensure(): Promise<ClaudeController> {
    if (this.controller) return this.controller;
    if (this.initializing) return this.initializing;
    this.initializing = this.initialize();
    try {
      return await this.initializing;
    } finally {
      this.initializing = undefined;
    }
  }
  private async initialize(): Promise<ClaudeController> {
    const folders = vscode.workspace.workspaceFolders;
    if (!folders?.length) throw new Error("请先打开 Claude 工作区文件夹。");
    const selected = this.context.workspaceState.get<string>(
        "claude.selectedFolder",
      ),
      folder = folders.find((f) => f.uri.toString() === selected) || folders[0];
    if (folder.uri.scheme !== "file")
      throw new Error("Claude 仅支持本机文件夹工作区。");
    const root = await realpath(folder.uri.fsPath);
    const verify = async () => {
      if (!vscode.workspace.isTrusted)
        throw new ClaudeError("WORKSPACE_UNTRUSTED");
      if (
        !vscode.workspace.workspaceFolders?.some(
          (f) => f.uri.toString() === folder.uri.toString(),
        ) ||
        (await realpath(folder.uri.fsPath)) !== root
      )
        throw new ClaudeError("WORKSPACE_CHANGED");
    };
    const configured = vscode.workspace
      .getConfiguration("aiTeamManager")
      .get<string>("claudeExecutable", "claude");
    const service = new ClaudeService(configured, undefined, (command) =>
      resolveClaudeExecutable(command, {
        extensionDirectory: () =>
          vscode.extensions.getExtension("anthropic.claude-code")?.extensionUri
            .fsPath,
      }),
    );
    const permissions = new PermissionManager(this.context,root,this.changed);
    await permissions.load();
    const controller = new ClaudeController(
      new ClaudeStorage(
        path.join(this.context.globalStorageUri.fsPath, "claude"),
        root,
      ),
      service,
      this.changed,
      verify,
      () => {
        const value = vscode.workspace
          .getConfiguration("aiTeamManager")
          .get<number>("claudeRequestTimeoutMs", 300000);
        return Number.isFinite(value)
          ? Math.max(1000, Math.min(3600000, value))
          : 300000;
      },
      (taskId,session) => permissions.start(taskId,session),
      () => ({
        maxRetries:vscode.workspace.getConfiguration('aiTeamManager').get<number>('claudeRetryMaxRetries',3),
        baseDelayMs:vscode.workspace.getConfiguration('aiTeamManager').get<number>('claudeRetryBaseDelayMs',2000),
      }),
    );
    await controller.initialize();
    this.folder = folder;
    this.controller = controller;
    this.permissions = permissions;
    await this.context.workspaceState.update(
      "claude.selectedFolder",
      folder.uri.toString(),
    );
    this.changed();
    return controller;
  }
  async load(): Promise<void> {
    try {
      await this.ensure();
    } catch (e) {
      this.report(e);
    }
  }
  private report(error: unknown): void {
    this.error =
      error instanceof ClaudeError
        ? error.message
        : error instanceof Error &&
            /请先|工作区|历史会话|当前任务|请输入/.test(error.message)
          ? error.message
          : new ClaudeError("PROCESS_START_FAILED").message;
    this.changed();
  }
  async handle(message: any): Promise<void> {
    try {
      this.error = undefined;
      if (message?.type === "stop") {
        await this.controller?.stop();
        return;
      }
      if (message?.type === 'permissionAnswer' && typeof message.id === 'string') {
        if (vscode.workspace.isTrusted) this.permissions?.answer(message.id,message.choice);
        return;
      }
      if (message?.type === 'permissions') {
        await this.ensure();
        if (this.controller?.isBusy) throw new Error('请先停止Claude当前任务再修改权限范围。');
        await this.permissions?.configureUI();return;
      }
      if (
        message?.type === "copy" &&
        typeof message.text === "string" &&
        message.text.length <= 100000
      ) {
        await vscode.env.clipboard.writeText(message.text);
        return;
      }
      if (message?.type === "project") {
        if (this.controller?.isBusy)
          throw new Error("请先停止 Claude 当前任务。");
        const folder = await vscode.window.showWorkspaceFolderPick({
          placeHolder: "选择 Claude 执行任务的工作区",
        });
        if (folder) {
          await this.permissions?.dispose();this.permissions = undefined;
          this.controller = undefined;
          this.folder = undefined;
          await this.context.workspaceState.update(
            "claude.selectedFolder",
            folder.uri.toString(),
          );
          await this.ensure();
        }
        return;
      }
      const c = await this.ensure();
      if (message?.type === "send" && typeof message.text === "string")
        await c.send(message.text);
      else if (message?.type === "check") await c.check();
      else if (message?.type === "new") await c.newConversation();
      else if (message?.type === "history") {
        const items = c
          .snapshot()
          .history.map((h) => ({
            label: h.title,
            description: `${h.updatedAt} · ${h.status}`,
            id: h.id,
          }));
        const selected = await vscode.window.showQuickPick(items, {
          placeHolder: "恢复本项目 Claude 会话；不新建或复制 Session",
        });
        if (selected) await c.select(selected.id);
      }
    } catch (e) {
      this.report(e);
    } finally {
      this.changed();
    }
  }
  async configurePermissions(scope: TaskPermissionScope): Promise<void> {
    const c=await this.ensure();if(c.isBusy)throw new Error('请先停止Claude当前任务。');
    await this.permissions!.configure(scope);
  }
  async sendRepair(text: string, conversation: string, session: string, plan: RepairPlan, observe: (event: unknown) => void): Promise<void> {
    const c = await this.ensure(), state = c.snapshot();
    if (c.isBusy || state.conversation?.id !== conversation || state.conversation?.threadId !== session)
      throw new Error('原Claude Session未恢复或正在执行，修复未发送。');
    this.error = undefined;
    await c.send(text, { observe, preparePermissions: (taskId, sid) => this.permissions!.start(taskId, sid,
      { allowedFiles: plan.allowed_files, safeCommands: [] }, { deleteFiles: plan.delete_files }, entry => observe({ type: 'permission', ...entry })) });
  }
  async stop(): Promise<void> { await this.controller?.stop(); }
  async sendDispatched(text:string, root:string, expectedSession:string|undefined, files:string[], observe:(event:unknown)=>void,
    gateway:{mode:string;policy:PermissionPolicy;evaluate:PermissionEvaluator}):Promise<void> {
    const c=await this.ensure(),s=c.snapshot();
    if(c.isBusy || s.projectPath!==root || s.conversation?.threadId!==expectedSession)throw new Error('Claude工作区或Session已改变，任务未发送；请恢复原会话后重新派发。');
    this.error=undefined;
    await c.send(text,{observe,preparePermissions:(taskId,sid)=>this.permissions!.start(taskId,sid,{allowedFiles:files,safeCommands:[]},undefined,entry=>observe({type:'permission',...entry}),gateway)});
    const result=c.snapshot();
    if(result.conversation?.status!=='COMPLETED')throw new Error(result.error || 'Claude执行未完成。');
  }
  async dispose(): Promise<void> {
    await this.controller?.stop();
    await this.permissions?.dispose();
  }
}
