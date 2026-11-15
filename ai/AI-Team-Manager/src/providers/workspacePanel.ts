import * as vscode from "vscode";
import { randomBytes } from "node:crypto";
import { validateLayout } from "../workspace/workspaceLayout";

export class MultiChatWorkspace implements vscode.Disposable {
  static readonly viewType = "aiTeamManager.workspace";
  private panel?: vscode.WebviewPanel;
  private ready = false;
  private layout;
  private disposables: vscode.Disposable[] = [];
  constructor(
    private context: vscode.ExtensionContext,
    private snapshot: () => unknown,
    private handle: (message: any) => Promise<void>,
  ) {
    this.layout = validateLayout(
      context.workspaceState.get("workspace.layout.v1"),
    );
  }
  get status() {
    return {
      open: !!this.panel,
      ready: this.ready,
      viewType: MultiChatWorkspace.viewType,
      layout: this.layout,
    };
  }
  async saveLayout(value: unknown): Promise<void> {
    this.layout = validateLayout(value);
    await this.context.workspaceState.update(
      "workspace.layout.v1",
      this.layout,
    );
    this.refresh();
  }
  open(): void {
    if (this.panel) {
      this.panel.reveal(vscode.ViewColumn.Active);
      return;
    }
    this.attach(
      vscode.window.createWebviewPanel(
        MultiChatWorkspace.viewType,
        "AI Team Manager",
        vscode.ViewColumn.Active,
        { enableScripts: true, retainContextWhenHidden: true },
      ),
    );
  }
  restore(panel: vscode.WebviewPanel, state: unknown): void {
    if (this.panel) {
      panel.dispose();
      this.panel.reveal();
      return;
    }
    // workspaceState is authoritative; serialized webview state can lag behind a close/reload.
    if (
      this.context.workspaceState.get("workspace.layout.v1") === undefined &&
      state &&
      typeof state === "object" &&
      "layout" in state
    )
      this.layout = validateLayout((state as any).layout);
    this.attach(panel);
  }
  private attach(panel: vscode.WebviewPanel): void {
    this.panel = panel;
    this.ready = false;
    panel.iconPath = vscode.Uri.joinPath(
      this.context.extensionUri,
      "media",
      "icon.svg",
    );
    const webview = panel.webview;
    const root = vscode.Uri.joinPath(
      this.context.extensionUri,
      "src",
      "webview",
    );
    webview.options = {
      enableScripts: true,
      localResourceRoots: [
        root,
        vscode.Uri.joinPath(this.context.extensionUri, "media"),
      ],
    };
    const nonce = randomBytes(16).toString("hex");
    const asset = (file: string) =>
      webview.asWebviewUri(vscode.Uri.joinPath(root, file));
    const scripts = [
      "markdown.js",
      "workspace/layoutManager.js",
      "workspace/chatPanel.js",
      "workspace/outputFormatter.js",
      "workspace/workerPanel.js",
      "workspace/futurePages.js",
      "workspace/reviewPanel.js",
      'workspace/permissionPanel.js',
        'workspace/repairPanel.js',
        'workspace/dispatchPanel.js',
      "workspace/workspaceRouter.js",
      "workspace/gptCommanderPanel.js",
      "workspace/workspace.js",
    ];
    webview.html = `<!doctype html><html lang="zh-CN"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1"><meta http-equiv="Content-Security-Policy" content="default-src 'none'; img-src ${webview.cspSource} data:; style-src ${webview.cspSource}; script-src 'nonce-${nonce}';"><title>AI Team Manager</title><link rel="stylesheet" href="${asset("workspace/workspace.css")}"></head><body><main id="workspace" aria-label="AI Team Manager 工作台"></main>${scripts.map((file) => `<script nonce="${nonce}" src="${asset(file)}"></script>`).join("")}</body></html>`;
    this.disposables.push(
      webview.onDidReceiveMessage(async (message) => {
        if (message?.type === "ready") {
          this.ready = true;
          this.refresh();
          return;
        }
        if (message?.type === "layout") {
          await this.saveLayout(message.layout);
          return;
        }
        await this.handle(message);
      }),
      panel.onDidDispose(() => {
        this.panel = undefined;
        this.ready = false;
        this.disposables.splice(0).forEach((d) => d.dispose());
      }),
    );
  }
  refresh(): void {
    if (this.ready)
      void this.panel?.webview
        .postMessage({
          type: "snapshot",
          ...(this.snapshot() as object),
          layout: this.layout,
        })
        .then(undefined, () => {});
  }
  dispose(): void {
    this.panel?.dispose();
    this.disposables.splice(0).forEach((d) => d.dispose());
  }
}
