import * as vscode from 'vscode';
import { realpath } from 'node:fs/promises';
import { execFile } from 'node:child_process';
import { ChatController, type ChatBridge } from './chatController';
import { ConversationStorage } from '../storage/conversationStorage';
import { CodexService } from '../services/codexService';
import { CodexError } from '../services/codexErrors';
import { resolveCodexExecutable } from '../process/codexExecutableResolver';
export class ChatCoordinator {
  private controller?: ChatController;
  private folder?: vscode.WorkspaceFolder;
  private error?: string;
  private initializing?: Promise<ChatController>;
  constructor(private context: vscode.ExtensionContext, private changed: () => void,
    private prepareBridge?:(root:string,text:string,owner:string,turn:string)=>Promise<ChatBridge>) {}
  snapshot(): unknown {return {projectName:this.folder?.name || '未选择工作区',error:this.error,...(this.controller?.snapshot() as object || {})};}
  private async ensure(): Promise<ChatController> {
    if(this.controller) return this.controller;
    if(this.initializing) return this.initializing;
    this.initializing = this.initialize();
    try {return await this.initializing;} finally {this.initializing = undefined;}
  }
  private async initialize(): Promise<ChatController> {
    const folders = vscode.workspace.workspaceFolders;
    if(!folders?.length) throw new Error('请先打开一个项目文件夹。');
    const selected = this.context.workspaceState.get<string>('gpt.selectedFolder');
    const folder = folders.find(f => f.uri.toString() === selected) || folders[0];
    if(folder.uri.scheme !== 'file') throw new Error('本阶段仅支持本机文件夹工作区，请在本机窗口中打开项目。');
    const root = await realpath(folder.uri.fsPath);
    const verify = async () => {
      if(!vscode.workspace.isTrusted) throw new CodexError('WORKSPACE_UNTRUSTED');
      if(!vscode.workspace.workspaceFolders?.some(f => f.uri.toString() === folder.uri.toString()) || await realpath(folder.uri.fsPath) !== root) throw new CodexError('WORKSPACE_CHANGED');
      await new Promise<void>((resolve,reject) => execFile('git',['-C',root,'rev-parse','--is-inside-work-tree'],{windowsHide:true,timeout:5000},(error,output) => error || output.trim() !== 'true' ? reject(new CodexError('NOT_GIT_REPOSITORY')) : resolve()));
    };
    const executable = vscode.workspace.getConfiguration('aiTeamManager').get<string>('codexExecutable','codex');
    const service = new CodexService(executable,undefined,configured => resolveCodexExecutable(configured,{
      extensionDirectory:() => {
        const extension = vscode.extensions.getExtension('openai.chatgpt');
        return extension?.extensionUri.scheme === 'file' ? extension.extensionUri.fsPath : undefined;
      }
    }));
    const controller = new ChatController(new ConversationStorage(this.context.globalStorageUri.fsPath,root),service,this.changed,verify,
      () => Math.max(1000,Math.min(3600000,vscode.workspace.getConfiguration('aiTeamManager').get<number>('requestTimeoutMs',180000) || 180000)),this.prepareBridge);
    await controller.initialize();
    this.folder = folder;this.controller = controller;
    await this.context.workspaceState.update('gpt.selectedFolder',folder.uri.toString());
    this.changed();return controller;
  }
  async load(): Promise<void> {try {await this.ensure();} catch(error) {this.report(error);}}
  private report(error: unknown): void {
    this.error = error instanceof CodexError ? error.message : error instanceof Error && /请先|工作区|本阶段|当前请求|请输入|历史会话/.test(error.message) ? error.message : 'GPT 操作失败，请检查本地存储和 CLI 环境。';
    this.changed();
  }
  async handle(message: any): Promise<void> {
    try {
      this.error = undefined;
      if(message?.type === 'stop') {await this.controller?.stop();return;}
      if(message?.type === 'copy' && typeof message.text === 'string' && message.text.length <= 100000) {await vscode.env.clipboard.writeText(message.text);return;}
      if(message?.type === 'project') {await this.chooseProject();return;}
      const controller = await this.ensure();
      if(message?.type === 'send' && typeof message.text === 'string') await controller.send(message.text);
      else if(message?.type === 'new') await controller.newConversation();
      else if(message?.type === 'check') await controller.check();
      else if(message?.type === 'history') {
        const snapshot = controller.snapshot() as any;
        const items = snapshot.history.map((c: any) => ({label:c.title,description:`${c.updatedAt} · ${c.status}`,id:c.id}));
        const selected = await vscode.window.showQuickPick<{label:string;id:string}>(items,{placeHolder:'选择本项目历史会话；不会新建 Codex 会话'});
        if(selected) await controller.select(selected.id);
      } else if(message?.type === 'clear') {
        if(controller.isBusy) throw new Error('请先停止当前请求。');
        const selected = await vscode.window.showWarningMessage('清空本项目在插件中的全部聊天记录？不会删除 Codex CLI 原始会话。',{modal:true},'清空插件记录');
        if(selected === '清空插件记录') await controller.clear();
      }
    } catch(error) {this.report(error);}
    finally {this.changed();}
  }
  async chooseProject(): Promise<void> {
    if(this.controller?.isBusy) throw new Error('请先停止当前请求。');
    const folder = await vscode.window.showWorkspaceFolderPick({placeHolder:'选择 GPT 分析的工作区文件夹'});
    if(!folder) return;
    this.controller = undefined;this.folder = undefined;
    await this.context.workspaceState.update('gpt.selectedFolder',folder.uri.toString());
    await this.ensure();
  }
  async dispose(): Promise<void> {await this.controller?.stop();}
}
