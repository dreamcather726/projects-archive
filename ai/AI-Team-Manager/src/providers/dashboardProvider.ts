import * as vscode from 'vscode';
import { randomBytes } from 'node:crypto';
export class DashboardProvider implements vscode.WebviewViewProvider {
  private view?: vscode.WebviewView;
  private ready = false;
  get status() {return {section:this.section,ready:this.ready,hasCsp:!!this.view?.webview.html.includes('http-equiv="Content-Security-Policy"')};}
  constructor(private uri: vscode.Uri, private section: string, private snapshot: () => unknown, private message?: (message: unknown) => void) {}
  resolveWebviewView(view: vscode.WebviewView): void {
    this.view = view;
    const webview = view.webview;
    const root = vscode.Uri.joinPath(this.uri,'src','webview');
    webview.options = {enableScripts:true,localResourceRoots:[root]};
    const nonce = randomBytes(16).toString('hex');
    const asset = (file: string) => webview.asWebviewUri(vscode.Uri.joinPath(root,file));
    webview.html = `<!doctype html><html lang="zh-CN"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1"><meta http-equiv="Content-Security-Policy" content="default-src 'none'; style-src ${webview.cspSource}; script-src 'nonce-${nonce}';"><link rel="stylesheet" href="${asset('dashboard.css')}"></head><body data-section="${this.section}"><main id="root" aria-live="polite"></main><script nonce="${nonce}" src="${asset('markdown.js')}"></script><script nonce="${nonce}" src="${asset('dashboard.js')}"></script></body></html>`;
    webview.onDidReceiveMessage(message => {if(message?.type === 'ready') {this.ready = true;this.refresh();}else this.message?.(message);});
    view.onDidDispose(() => {this.view = undefined;this.ready = false;});
  }
  refresh(): void {void this.view?.webview.postMessage(this.snapshot()).then(undefined,() => {});}
}
export class AgentProvider implements vscode.TreeDataProvider<vscode.TreeItem> {
  private emitter = new vscode.EventEmitter<void>();
  readonly onDidChangeTreeData = this.emitter.event;
  constructor(private snapshot?:()=>any) {}
  refresh():void {this.emitter.fire();}
  dispose():void {this.emitter.dispose();}
  getTreeItem(item: vscode.TreeItem): vscode.TreeItem {return item;}
  getChildren(): vscode.TreeItem[] {
    const state=this.snapshot?.();
    const item = new vscode.TreeItem(state?'Claude Worker 01':'未启动 · 0 Agents');
    item.description = state?(state.busy?'运行中':state.conversation?.status||'未启动'):'空列表';
    item.tooltip = state?`单 Claude Worker · ${state.projectName}\n${state.conversation?.threadId||'尚未建立 Session'}\n仅用户发送任务时启动 CLI`:'暂无 Agent';
    if(state)item.command={command:'aiTeamManager.openDashboard',title:'打开工作台'};
    item.iconPath = new vscode.ThemeIcon('circle-outline');
    return [item];
  }
}
