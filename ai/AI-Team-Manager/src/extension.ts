import * as vscode from 'vscode';
import { DashboardProvider, AgentProvider } from './providers/dashboardProvider';
import { diagnose, Diagnostic } from './services/environmentService';
import { resolveCodexExecutable } from './process/codexExecutableResolver';
import { resolveClaudeExecutable } from './claude/claudeExecutableResolver';
import { ChatCoordinator } from './chat/chatCoordinator';
import { MultiChatWorkspace } from './providers/workspacePanel';
import { WorkspaceTelemetry, type WorkspaceLog } from './workspace/workspaceTelemetry';
import { ClaudeCoordinator } from './claude/claudeCoordinator';
import { ReviewCoordinator } from './review/reviewCoordinator';
import { RepairCoordinator } from './repair/repairCoordinator';
import { DispatchCoordinator } from './dispatch/dispatchCoordinator';
let activeChat: ChatCoordinator | undefined;
let activeClaude: ClaudeCoordinator | undefined;
let activeReview: ReviewCoordinator | undefined;
let activeRepair: RepairCoordinator | undefined;
let activeDispatch: DispatchCoordinator | undefined;
export async function activate(context: vscode.ExtensionContext) {
  const logs: WorkspaceLog[] = [];
  let logId = Date.now();
  let diagnostics: Diagnostic[] = [];
  let running = false;
  const log = (message:string, level='INFO',source='System') => {
    logs.push({id:++logId,time:new Date().toLocaleTimeString(),level,source,message});
    if(logs.length > 500) logs.shift();
  };
  log('系统初始化完成；GPT可通过受控MCP派发Claude，工具执行前需权限审批。尚未启动模型。');
  log('环境检测待执行：使用 Diagnose Environment 命令');
  const providers: DashboardProvider[] = [];
  const telemetry = new WorkspaceTelemetry(log);
  let workspace: MultiChatWorkspace;
  const refresh = () => {telemetry.observe(chat.snapshot());observeClaude();providers.forEach(p => p.refresh());workspace?.refresh();agents?.refresh();};
  let dispatch: DispatchCoordinator | undefined;
  const chat = new ChatCoordinator(context,refresh,(root,text,owner,turn)=>dispatch!.prepare(root,text,owner,turn));
  const claude = new ClaudeCoordinator(context,refresh);
  let repair: RepairCoordinator | undefined;
  const review = new ReviewCoordinator(context,() => claude.snapshot(),refresh,log,() => !!repair?.busy || !!dispatch?.busy);
  repair = new RepairCoordinator(context,claude,() => review.snapshot().busy || !!dispatch?.busy,refresh,log);
  dispatch = new DispatchCoordinator(context,claude,refresh,()=>review.snapshot().busy || !!repair?.busy);
  activeDispatch=dispatch;
  activeRepair = repair;
  activeReview = review;
  let previousClaudeRequest='',previousClaudeError='';
  const seenClaudeTools = new Map<string,string>();
  const observeClaude = () => {
    const state=claude.snapshot() as any,request=state.conversation?.requests?.at(-1);
    const key=request?`${state.projectPath}:${request.id}:${request.status}`:'';
    if(key&&key!==previousClaudeRequest){previousClaudeRequest=key;log(`Claude 请求 ${request.status}`,request.status==='FAILED'?'ERROR':'INFO','Claude');}
    if(state.error&&state.error!==previousClaudeError)log(state.error,'ERROR','Claude');previousClaudeError=state.error||'';
    for(const tool of state.activities||[]){const id=`${request?.id}:${tool.id}`;if(seenClaudeTools.get(id)!==tool.phase){seenClaudeTools.set(id,tool.phase);log(`${tool.name} · ${tool.phase}`,tool.phase==='failed'?'ERROR':'INFO','Claude');}}
    if(seenClaudeTools.size>500)seenClaudeTools.clear();
  };
  activeChat = chat;
  activeClaude = claude;
  let agents: AgentProvider;
  const snapshot = () => ({project:vscode.workspace.name || '未打开项目',projects:(vscode.workspace.workspaceFolders || []).map(f => ({name:f.name,uri:f.uri.toString()})),logs,diagnostics,running,chat:chat.snapshot(),claude:claude.snapshot(),review:review.snapshot(),repair:repair?.snapshot(),dispatch:dispatch?.snapshot()});
  const handleWorkspace = async (message: any) => {
    if(message?.type === 'settings') {await vscode.commands.executeCommand('workbench.action.openSettings','@ext:ai-team-manager.ai-team-manager');return;}
    if(message?.type === 'diagnose') {await vscode.commands.executeCommand('aiTeamManager.diagnoseEnvironment');return;}
    if(message?.type === 'openWorkspace') {workspace.open();return;}
    if(typeof message?.type==='string' && message.type.startsWith('team')){const configured=await dispatch!.handle(message);if(message.type==='teamUsageConfigure'&&configured)await chat.handle({type:'check'});return;}
    if(typeof message?.type==='string' && message.type.startsWith('dispatch')){await dispatch!.handle(message);return;}
    if(dispatch?.busy && message?.panelId==='claude-01' && !['stop','permissionAnswer','copy'].includes(message.type)){log('Claude由GPT调度任务占用，请先停止调度任务。','WARN');refresh();return;}
    if(message?.type==='stop' && (message.panelId==='claude-01'||message.panelId==='gpt'))await dispatch?.handle({type:'dispatchStop'});
    if(typeof message?.type === 'string' && message.type.startsWith('repair')) {await repair!.handle(message);return;}
    if(message?.panelId === 'claude-01' && repair?.busy && !['stop','permissionAnswer','copy'].includes(message.type)) {log('请先完成当前修复操作。','WARN');refresh();return;}
    if(typeof message?.type === 'string' && message.type.startsWith('review')) {await review.handle(message);return;}
    if(message?.panelId === 'claude-01') {try {if(message.type === 'send' && typeof message.text === 'string') await review.withClaudeTask(message.text,() => claude.handle(message));else await claude.handle(message);}catch {log('请先完成当前审查或证据采集后再发送 Claude 任务。','WARN');refresh();}return;}
    // Only the GPT window may send messages to the existing GPT controller.
    if(message?.panelId && message.panelId !== 'gpt') return;
    await chat.handle(message);
  };
  workspace = new MultiChatWorkspace(context,snapshot,handleWorkspace);
  context.subscriptions.push(workspace,vscode.window.registerWebviewPanelSerializer(MultiChatWorkspace.viewType,{deserializeWebviewPanel:async (panel,state) => {workspace.restore(panel,state);}}));
  for (const section of ['commander','tasks','logs']) {
    const provider = new DashboardProvider(context.extensionUri,section,() => ({project:vscode.workspace.name || '未打开项目',logs,diagnostics,running,chat:chat.snapshot()}),
      section === 'commander' ? message => {void handleWorkspace(message);} : undefined);
    context.subscriptions.push(vscode.window.registerWebviewViewProvider(`aiTeamManager.${section}`,provider));
    providers.push(provider);
  }
  agents = new AgentProvider(() => claude.snapshot());
  context.subscriptions.push(agents,vscode.window.createTreeView('aiTeamManager.agents',{treeDataProvider:agents}));
  const register = (name:string, callback:()=>unknown) => context.subscriptions.push(vscode.commands.registerCommand(`aiTeamManager.${name}`,callback));
  register('openDashboard',() => workspace.open());
  register('refreshDashboard',() => {log('仪表盘已刷新');refresh();});
  register('openSettings',() => vscode.commands.executeCommand('workbench.action.openSettings','@ext:ai-team-manager.ai-team-manager'));
  register('newGPTConversation',() => chat.handle({type:'new'}));
  register('stopGPTResponse',() => chat.handle({type:'stop'}));
  register('gptHistory',() => chat.handle({type:'history'}));
  register('clearGPTConversations',() => chat.handle({type:'clear'}));
  register('selectGPTProject',() => chat.handle({type:'project'}));
  register('checkCodexConnection',() => chat.handle({type:'check'}));
  register('checkClaudeConnection',() => claude.handle({type:'check'}));
  register('newClaudeConversation',() => handleWorkspace({type:'new',panelId:'claude-01'}));
  register('stopClaudeResponse',() => handleWorkspace({type:'stop',panelId:'claude-01'}));
  register('claudeHistory',() => handleWorkspace({type:'history',panelId:'claude-01'}));
  register('selectClaudeProject',() => handleWorkspace({type:'project',panelId:'claude-01'}));
  register('configureClaudePermissions',() => {workspace.open();return handleWorkspace({type:'permissions',panelId:'claude-01'});});
  register('openRepairs',() => workspace.open());
  register('configureDispatcher',()=>{workspace.open();return dispatch!.handle({type:'dispatchConfigure'});});
  register('stopDispatchedTask',()=>dispatch!.handle({type:'dispatchStop'}));
  register('startCodeReview',() => {workspace.open();return review.handle({type:'reviewStart'});});
  register('runReviewTests',() => review.handle({type:'reviewTests'}));
  register('stopCodeReview',() => review.handle({type:'reviewStop'}));
  register('codeReviewHistory',() => {workspace.open();return review.handle({type:'reviewHistory'});});
  context.subscriptions.push({dispose:() => {void dispatch?.dispose();void chat.dispose();void claude.dispose();void review.dispose();void repair?.dispose();}});
  register('diagnoseEnvironment',async () => {
    if(running) return;
    running = true; log('开始环境检测，仅执行 --version'); refresh();
    try {
      const configured = vscode.workspace.getConfiguration('aiTeamManager').get<number>('diagnosticTimeoutMs',10000);
      const timeout = Number.isFinite(configured) ? Math.max(1000,Math.min(60000,configured)) : 10000;
      const settings=vscode.workspace.getConfiguration('aiTeamManager');
      diagnostics = await diagnose(vscode.version,timeout,{
        codex:()=>resolveCodexExecutable(settings.get<string>('codexExecutable','codex'),{extensionDirectory:()=>vscode.extensions.getExtension('openai.chatgpt')?.extensionUri.fsPath}),
        claude:()=>resolveClaudeExecutable(settings.get<string>('claudeExecutable','claude'),{extensionDirectory:()=>vscode.extensions.getExtension('anthropic.claude-code')?.extensionUri.fsPath})
      });
      diagnostics.forEach(result => log(`${result.name}: ${result.detail}`,result.ok ? 'INFO' : 'ERROR'));
    } catch(error) {log(`环境检测失败: ${String(error)}`,'ERROR');}
    finally {running = false;refresh();}
  });
  context.subscriptions.push(vscode.workspace.onDidChangeWorkspaceFolders(() => {void dispatch?.dispose();void chat.dispose();void claude.dispose();refresh();}));
  await chat.load();
  await claude.load();
  await review.load();
  await repair.load();
  await dispatch.load();
  return { getDiagnostics: () => diagnostics.map(result => ({ ...result })), getViews: () => providers.map(p => p.status), getWorkspace: () => workspace.status, getWorkspaceSnapshot:snapshot, chat, claude, review, repair, dispatch, workspace };
}
export async function deactivate(): Promise<void> {await Promise.allSettled([activeDispatch?.dispose(),activeChat?.dispose(),activeClaude?.dispose(),activeReview?.dispose(),activeRepair?.dispose()]);activeChat = undefined;activeClaude = undefined;activeReview = undefined;activeRepair = undefined;activeDispatch=undefined;}
