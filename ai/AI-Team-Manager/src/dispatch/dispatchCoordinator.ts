import * as vscode from 'vscode';
import * as path from 'node:path';
import { realpath } from 'node:fs/promises';
import { TaskDispatcher } from './taskDispatcher';
import { ApprovalModel } from './approvalModel';
import { ToolBridge, dispatcherTools } from './toolBridge';
import { AdaptiveTaskPlanner } from '../planning/adaptiveTaskPlanner';
import { planningTools, PLANNING_INSTRUCTIONS } from '../planning/planningTools';
import { ReviewGit } from '../review/reviewGit';
import { ReviewStorage } from '../review/reviewStorage';
import { ReviewEngine } from '../review/reviewEngine';
import { ReviewTestRunner } from '../review/reviewTests';
import { RepairModel } from '../repair/repairModel';
import { CodexService } from '../services/codexService';
import { resolveCodexExecutable } from '../process/codexExecutableResolver';
import type { ClaudeCoordinator } from '../claude/claudeCoordinator';
import type { ChatBridge } from '../chat/chatController';
import type { ApprovalMode, DispatchRecord } from './dispatchTypes';
import { safeReviewText } from '../review/reviewSafety';
import { configureGPTUsage, validUsageBudget } from '../usage/usageConfiguration';
import { TeamSession } from '../team/teamSession';
import { teamTools, TEAM_INSTRUCTIONS } from '../team/teamTools';
import { ClaudeService } from '../claude/claudeService';
import { resolveClaudeExecutable } from '../claude/claudeExecutableResolver';
import { editorFileState } from '../protection/vscodeEditorState';

export const DISPATCH_INSTRUCTIONS=`你是AI Team Manager的GPT技术负责人。可以讨论，也可根据用户需求通过ai_team_manager MCP的真实工具主动派发Claude Worker 01。开发工作必须使用dispatch_claude_task，不要自己修改文件，也不要求用户复制到Claude。你自身只读沙箱；禁止提交/推送Git、读取认证/密钥/CC Switch、使用其他MCP或Shell绕过权限。不能修改审批规则或扩大用户任务范围。\n派发时提供明确任务、精确allowed_files、forbidden_files、验收标准、test_commands（只支持node --test、npm test、npm run compile、python -B test.py（使用任务精确脚本路径））。用户明确要求运行测试时必须提供测试命令。安装依赖须在expected_commands列出npm install --ignore-scripts package@1.2.3（使用具体包名和精确版本），同时请求package.json与package-lock.json文件范围，需人工审批，不能自动安装。Python加法样例可在test.py中用unittest自测，命令python -B test.py。计划和具体工具审批由插件独立GPT完成，不是你口头批准。\n收到task_id后必须反复调用get_claude_task_output，携带next_cursor及wait_ms=25000，直到terminal=true且has_more=false；耗时任务不要提前结束或重复派发。实际工具返回才是执行事实。未执行/被拒绝/测试失败/审查BLOCKED必须如实汇报，不得声称成功。支持cancel_claude_task、get_claude_task_status、get_claude_session。普通对话不用派发。用户若明确要求仅讨论，不能派发。每次最多一个Claude任务，不能并行。`;
export class DispatchCoordinator {
  private dispatcher?:TaskDispatcher;
  private storage:ReviewStorage;
  private bridges=new Set<ToolBridge>();
  private error?:string;
  private mode:ApprovalMode;
  private maxRounds:number;
  private pending?:{record:DispatchRecord;reason:string};
  private planner?:AdaptiveTaskPlanner;
  private team?:TeamSession;
  private verifyTeam?:()=>Promise<void>;
  private previewedIntegration?:string;
  constructor(private context:vscode.ExtensionContext,private claude:ClaudeCoordinator,private changed:()=>void,private externalBusy:()=>boolean){
    this.storage=new ReviewStorage(path.join(context.globalStorageUri.fsPath,'reviews'));
    const mode=context.workspaceState.get<ApprovalMode>('dispatch.mode','GPT_HYBRID');this.mode=['MANUAL','GPT_HYBRID','GPT_SCOPED_AUTO'].includes(mode)?mode:'GPT_HYBRID';
    const rounds=context.workspaceState.get<number>('dispatch.maxRounds',2);this.maxRounds=Number.isInteger(rounds)&&rounds>=1&&rounds<=3?rounds:2;
  }
  get busy(){return !!this.dispatcher?.busy||!!this.team?.busy;}
  snapshot(){const records=this.dispatcher?.records||[],config=vscode.workspace.getConfiguration('aiTeamManager');const settings=Object.fromEntries(['teamMaxConcurrent','teamMaxRepairRounds','claudeRetryMaxRetries','claudeRetryBaseDelayMs','claudeExecutable','codexExecutable'].map(key=>[key,config.get(key)]));return {settings,team:this.team?.snapshot(),mode:this.mode,max_rounds:this.maxRounds,busy:this.busy,error:this.error,pending:this.pending?{task_id:this.pending.record.task_id,reason:this.pending.reason,input:this.pending.record.input}:undefined,
    gpt_usage:{calls:records[0]?.gpt_calls.length||0,known_total:records[0]?.gpt_calls.reduce((s,c)=>s+(c.usage?.total||0),0)||0,unknown_calls:records[0]?.gpt_calls.filter(c=>c.usage?.total==null).length||0},
    task:records[0]?{...records[0],rounds:records[0].rounds.map(r=>({...r,before:undefined,after:undefined,evidence:undefined,review:r.review?{review_id:r.review.review_id,verdict:r.review.verdict,summary:r.review.summary}:undefined})),events:records[0].events.slice(-25)}:undefined,
    history:records.map(r=>({task_id:r.task_id,state:r.state,description:r.input.description,created_at:r.created_at})),planning:this.planner?.snapshot(),note:'执行前Hook拦截已启用。测试脚本会执行项目代码；这不是操作系统沙箱。重启后中断任务不自动重发。'};}
  private authorize=async(r:DispatchRecord,reason:string,signal:AbortSignal):Promise<boolean>=>{
    this.pending={record:r,reason};this.changed();
    const cancellation=new vscode.CancellationTokenSource(),abort=()=>cancellation.cancel();signal.addEventListener('abort',abort,{once:true});if(signal.aborted)abort();
    try{const picked=await vscode.window.showQuickPick([{label:'批准当前精确任务范围',approved:true,detail:`允许：${r.input.allowed_files.join(', ')}；命令：${[...r.input.test_commands,...(r.input.expected_commands||[])].join(', ')}`},{label:'拒绝',approved:false}],{title:'GPT→Claude 任务人工审批',placeHolder:reason,ignoreFocusOut:true},cancellation.token);
      return !!picked?.approved;
    }finally{signal.removeEventListener('abort',abort);cancellation.dispose();this.pending=undefined;this.changed();}
  };
  private async ensure(root:string){
    if(this.dispatcher?.root===root&&this.team)return this.dispatcher;
    if(this.busy)throw Error('请先停止当前调度任务再切换项目。');
    const verify=async()=>{if(!vscode.workspace.isTrusted||!vscode.workspace.workspaceFolders?.some(f=>f.uri.scheme==='file'&&path.resolve(f.uri.fsPath).toLowerCase()===root.toLowerCase())||await realpath(root)!==root)throw Error('任务工作区信任或真实路径已失效。');};
    await verify();
    const service=()=>new CodexService(vscode.workspace.getConfiguration('aiTeamManager').get<string>('codexExecutable','codex'),undefined,c=>resolveCodexExecutable(c,{extensionDirectory:()=>vscode.extensions.getExtension('openai.chatgpt')?.extensionUri.fsPath}));
    const collector=new ReviewGit(path.join(this.storage.directory,'temporary'));
    const d=new TaskDispatcher(root,this.storage,collector,this.claude,new ApprovalModel(service(),this.storage.directory),new ReviewEngine(service(),collector,this.storage,this.changed),new ReviewTestRunner(collector),new RepairModel(service(),this.storage.directory),{mode:this.mode,maxRounds:this.maxRounds,authorize:this.authorize,verify},this.changed);
    await d.load();
    const planner=new AdaptiveTaskPlanner(root,collector,this.storage,this.changed,vscode.workspace.getConfiguration('aiTeamManager').get<number>('teamMaxConcurrent',3),vscode.workspace.getConfiguration('aiTeamManager').get<number>('teamMaxRepairRounds',2),vscode.workspace.getConfiguration('aiTeamManager').get<number>('claudeRequestTimeoutMs',300000));await planner.load();this.planner=planner;
    await this.team?.dispose();this.verifyTeam=verify;
    const team=new TeamSession({root,directory:path.join(this.context.globalStorageUri.fsPath,'team'),services:{claudeRetryPolicy:()=>({maxRetries:vscode.workspace.getConfiguration('aiTeamManager').get<number>('claudeRetryMaxRetries',3),baseDelayMs:vscode.workspace.getConfiguration('aiTeamManager').get<number>('claudeRetryBaseDelayMs',2000)}),codex:service,claude:()=>new ClaudeService(vscode.workspace.getConfiguration('aiTeamManager').get<string>('claudeExecutable','claude'),undefined,c=>resolveClaudeExecutable(c,{extensionDirectory:()=>vscode.extensions.getExtension('anthropic.claude-code')?.extensionUri.fsPath}))},editors:()=>editorFileState(root),changed:this.changed,
      authorize:async plan=>(await vscode.window.showWarningMessage(`执行计划：${plan.goal}\n允许文件：${plan.tasks.flatMap(t=>t.allowed_files).join(', ')}\n测试：${plan.tasks.flatMap(t=>t.test_commands).join(', ')}`,{modal:true},'批准当前范围'))==='批准当前范围',
      approve:async(op,signal)=>{const cancellation=new vscode.CancellationTokenSource(),abort=()=>cancellation.cancel();signal.addEventListener('abort',abort,{once:true});if(signal.aborted)abort();try{return await vscode.window.showQuickPick(['ONCE','DENY'],{title:`${op.tool}: ${op.target}`,placeHolder:op.reason,ignoreFocusOut:true},cancellation.token)==='ONCE'&&!signal.aborted?'ONCE':'DENY';}finally{signal.removeEventListener('abort',abort);cancellation.dispose();}}});
    await team.load();this.team=team;
    this.dispatcher=d;return d;
  }
  async load(){const root=(this.claude.snapshot() as any).projectPath;if(root)try{await this.ensure(root);}catch(e){this.error=safeReviewText((e as Error).message);this.changed();}}
  async prepare(root:string,text:string,owner:string,turn:string):Promise<ChatBridge>{
    const d=await this.ensure(root);
    const team=this.team!;
    let ownsTurn=false;
    const bridge=new ToolBridge(async(name,args)=>{
      if(this.externalBusy())throw Error('独立审查/修复正在运行，请先完成后派发。');
      if(name==='get_project_planning_context')return this.planner!.capture(owner,turn,text);
      if(name==='submit_adaptive_plan'){if(args.orchestration_version!==2)throw Error('新计划必须包含 Task 07.11 结构化分析。');return this.planner!.submit(owner,turn,args);}
      if(teamTools.some(t=>t.name===name)){
        await this.verifyTeam!();
        if(d.busy)throw Error('单 Worker 调度仍在运行，请先结束。');
        if(name==='execute_adaptive_plan'){const plan=this.planner!.snapshot().plans.find(p=>p.plan_id===args.plan_id);if(!plan)throw Error('计划不存在。');return team.execute(plan,owner,text);}
        return team.call(name,args,owner);
      }
      if(team.busy)throw Error('隔离团队正在执行，不能同时使用旧单 Worker 调度。');
      if((this.claude.snapshot() as any).projectPath!==root)throw Error('GPT和Claude选择的项目不同；请先选择同一个工作区。');
      return d.call(owner,turn,text,name,args);
    },()=>ownsTurn?d.cancelOwner(owner):Promise.resolve(),[...dispatcherTools,...planningTools,...teamTools]);
    this.bridges.add(bridge);
    let options;
    try {options=await bridge.start();await team.beginTurn(turn);ownsTurn=true;}
    catch(error){await bridge.close().catch(()=>{});this.bridges.delete(bridge);throw error;}
    options.instructions=PLANNING_INSTRUCTIONS+'\n'+TEAM_INSTRUCTIONS+'\n你是只读的 GPT 总指挥。开发通过隔离团队执行，自己不得写文件、提交 Git、读取凭据、改变审批或绕过 MCP 权限。旧 dispatch_claude_task 仅用于用户明确请求的旧单任务流程，禁止与团队并用。';
    return {options,timeoutMs:1800000,finalize:record=>team.finishTurn(turn,record),close:async()=>{await bridge.close();this.bridges.delete(bridge);}};
  }
  async handle(m:any){
    try{
      this.error=undefined;
      if(m.type==='teamUsageConfigure'){
        if(typeof m.projectPath!=='string')throw Error('请先选择 GPT 项目。');
        await this.ensure(m.projectPath);await this.verifyTeam!();
        if(this.busy)throw Error('请先暂停或完成当前团队任务。');
        return await configureGPTUsage(this.team!.ledger,{
          inputBudget:async state=>vscode.window.showInputBox({title:'GPT 用量与预算',value:String(state.budget),prompt:`已知累计 ${state.known_total} Token；${state.unknown_requests} 个请求用量未知。预算是请求前门禁，单次返回可能超出。`,ignoreFocusOut:true,validateInput:value=>validUsageBudget(value,state.known_total)?undefined:'请输入 1000–100000000 的整数，且大于已知累计用量。'}),
          confirmUnknown:async count=>(await vscode.window.showWarningMessage(`有 ${count} 个中断或缺少 usage 的请求，用量无法确定。确认后允许后续 GPT 请求；原账本和未知标记继续保留，不按 0 计费。`,{modal:true},'确认未知用量并继续'))==='确认未知用量并继续'
        });
      }
      if(m.type.startsWith('team')){
        if(!this.team)throw Error('当前项目没有团队会话。');await this.verifyTeam!();
        if(m.projectPath!==this.team.options.root||m.planId!==this.team.workflow?.plan.plan_id)throw Error('界面事件属于其他项目或旧计划。');
        if(m.type==='teamSelect')await this.team.select(m.workerId,m.messageStart,m.logStart);
        else if(m.type==='teamControl'){const worker=this.team.workflow!.executor.workers.detail(m.workerId).worker;await this.team.control(m.action,worker.task_id);}
        else if(m.type==='teamInstruct')await this.team.instruct(m.workerId,m.text);
        else if(m.type==='teamFile'){const doc=await vscode.workspace.openTextDocument({language:'diff',content:this.team.file(m.workerId,m.file)});await vscode.window.showTextDocument(doc);}
        else if(m.type==='teamPreview'){
          const integration=this.team.integration?.snapshot();if(!integration?.evidence)throw Error('尚无实际集成补丁。');
          const doc=await vscode.workspace.openTextDocument({language:'diff',content:integration.evidence.changes.map(c=>c.diff).join('\n')});await vscode.window.showTextDocument(doc);this.previewedIntegration=integration.integration_id;
        }
        else if(m.type==='teamDeliver'){
          if(this.previewedIntegration!==this.team.integration?.snapshot()?.integration_id)throw Error('请先查看当前实际集成补丁。');
          const picked=await vscode.window.showWarningMessage('已查看当前集成补丁，并确认外部编辑器没有未保存的目标文件？交付前将重新验证候选、审查和文件保护。',{modal:true},'确认安全交付');
          if(picked==='确认安全交付')await this.team.integration!.deliver(true,false);
        }
        return;
      }
      if(m.type==='dispatchStop'){await this.dispatcher?.cancel();return;}
      if(m.type==='dispatchReport'){
        const r=this.dispatcher?.records.find(r=>r.task_id===m.taskId)||this.dispatcher?.records[0];if(!r)return;
        const doc=await vscode.workspace.openTextDocument({language:'json',content:JSON.stringify(r,null,2)});await vscode.window.showTextDocument(doc);return;
      }
      if(m.type==='dispatchPlanReport'){
        const plan=this.planner?.snapshot().plans[0];if(!plan)return;
        const doc=await vscode.workspace.openTextDocument({language:'json',content:JSON.stringify(plan,null,2)});await vscode.window.showTextDocument(doc);return;
      }
      if(m.type==='dispatchConfigure'){
        if(this.busy)throw Error('请先停止当前调度任务。');
        const mode=await vscode.window.showQuickPick(['GPT_HYBRID','GPT_SCOPED_AUTO','MANUAL'],{title:'GPT→Claude审批模式',ignoreFocusOut:true});if(!mode)return;
        const rounds=await vscode.window.showQuickPick(['1','2','3'],{title:'最大开发/修复轮数（含首轮）；到达上限停止',ignoreFocusOut:true});if(!rounds)return;
        this.mode=mode as ApprovalMode;this.maxRounds=Number(rounds);await this.context.workspaceState.update('dispatch.mode',mode);await this.context.workspaceState.update('dispatch.maxRounds',this.maxRounds);this.dispatcher=undefined;await this.load();
      }
    }catch(e){this.error=safeReviewText(e instanceof Error?e.message:'调度操作失败');}finally{this.changed();}
  }
  async dispose(){await this.dispatcher?.cancel();await this.team?.dispose();await Promise.allSettled([...this.bridges].map(b=>b.close()));this.team=undefined;this.dispatcher=undefined;this.planner=undefined;}
}
