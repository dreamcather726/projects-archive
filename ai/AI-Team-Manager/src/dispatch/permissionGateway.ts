import { randomUUID } from 'node:crypto';
import { PermissionPolicy, operationFingerprint, validCommandInput } from '../permissions/permissionPolicy';
import type { PermissionDecision } from '../permissions/permissionTypes';
import type { PermissionEvaluator } from '../permissions/permissionHost';
import type { DispatchRecord, ApprovalRequest, ApprovalDecision } from './dispatchTypes';
import { safeReviewText } from '../review/reviewSafety';

// Parse only a deliberately small argv grammar. All metacharacters, extra switches and changed targets fail closed.
export function pythonTest(command:string):string|undefined {
  const m=/^python -B ([a-zA-Z0-9_][a-zA-Z0-9_./-]*\.py)$/.exec(command);
  return m && !m[1].split('/').some(s=>s==='..'||s==='.') ? m[1] : undefined;
}
export const dependencyCommand=(s:string)=>/^npm install --ignore-scripts [a-z0-9][a-z0-9._-]*@[0-9]+\.[0-9]+\.[0-9]+$/.test(s);
export class DispatchPolicy extends PermissionPolicy {
  constructor(root:string,files:string[],private commands:string[]){super(root,{allowedFiles:files,safeCommands:[]});}
  async check(tool:string,input:any):Promise<PermissionDecision>{
    if(tool==='Bash' && typeof input?.command==='string'){
      if(!validCommandInput(input))return {behavior:'deny',operation:'COMMAND',target:'',reason:'命令参数包含未知选项或工作目录覆盖，拒绝。'};
      const file=pythonTest(input.command);
      if(file && this.commands.includes(input.command) && !input.run_in_background && !input.dangerouslyDisableSandbox){
        const access=await super.check('Read',{file_path:file});
        if(access.behavior==='deny')return access;
        return {behavior:'allow',operation:'TEST_COMMAND',target:input.command,reason:'当前任务精确授权的Python测试命令；仍需GPT审批。'};
      }
      // Dependency changes require explicit scope and manual approval, even in automatic modes.
      if(dependencyCommand(input.command) && !input.run_in_background && !input.dangerouslyDisableSandbox){
        if(!this.commands.includes(input.command))return {behavior:'deny',operation:'DEPENDENCY',target:input.command,reason:'当前任务计划没有列出此精确依赖命令；需要新的人工授权任务。'};
        if(!['package.json','package-lock.json'].every(f=>this.scope.allowedFiles.includes(f)))return {behavior:'deny',operation:'DEPENDENCY',target:input.command,reason:'安装依赖需要人工批准package.json及package-lock.json范围；请先建立新的授权任务。'};
        for(const f of ['package.json','package-lock.json']){const check=await super.check('Write',{file_path:f,content:'dependency scope validation'});if(check.behavior==='deny')return check;}
        return {behavior:'ask',operation:'DEPENDENCY',target:input.command,reason:'安装依赖属于L2；必须逐次人工审批，不缓存通用授权。'};
      }
    }
    const d=await super.check(tool,input);
    if(d.operation==='DELETE')return {...d,behavior:'deny',reason:'自动调度任务不授权删除文件；GPT及普通工具审批不能扩大为删除权限。'};
    if(tool==='Bash' && d.operation==='TEST_COMMAND' && this.commands.includes(input?.command))return {...d,behavior:'allow'};
    return d;
  }
}
export class PermissionGateway {
  constructor(private record:DispatchRecord,private judge:(r:ApprovalRequest)=>Promise<ApprovalDecision>,private audit:(kind:string,data:unknown)=>Promise<void>){}
  readonly evaluate:PermissionEvaluator=async(base,tool,input,session,signal)=>{
    const deny=(reason:string):PermissionDecision=>({...base,behavior:'deny',reason});
    if(signal.aborted || this.record.state==='CANCELLED')return deny('任务已停止。');
    const readonly=base.operation==='READ' || tool==='Bash' && ['node --version','npm --version','git status --short','git diff --stat'].includes(base.target);
    if(readonly){await this.audit('GATEWAY_L0',{tool,session,target:base.target,decision:'APPROVE'});return {...base,behavior:'allow'};}
    if(this.record.mode==='MANUAL')return {...base,behavior:'ask',reason:'MANUAL模式：需要用户批准当前具体操作。'};
    const risk=base.behavior==='ask'?'L2':'L1';
    const request:ApprovalRequest={approval_id:randomUUID(),task_id:this.record.task_id,session_id:session,request_type:'TOOL',tool_name:tool,tool_arguments:JSON.parse(safeReviewText(JSON.stringify(input))),working_directory:this.record.project_path,target_files:tool==='Bash'?[]:[base.target],command:tool==='Bash'?base.target:'',allowed_files:this.record.input.allowed_files,forbidden_files:this.record.input.forbidden_files,task_description:this.record.user_request,risk_level:risk,evidence:{plan:this.record.rounds.at(-1)?.plan,fingerprint:operationFingerprint(tool,input)}};
    try{
      // L2 is assessed by GPT, but APPROVE cannot replace the required human decision.
      const result=await this.judge(request);
      await this.audit('GPT_TOOL_APPROVAL',{request,result});
      if(signal.aborted || Date.parse(result.expires_at)<=Date.now())return deny('审批已取消或过期。');
      if(result.risk_level==='L3')return deny('GPT识别出L3风险；不能覆盖拒绝规则。');
      if(result.decision==='DENY'||result.decision==='REVISE')return deny(`GPT拒绝：${result.reason}`);
      if(risk==='L2'||result.risk_level==='L2'||result.decision==='NEEDS_HUMAN')return {...base,behavior:'ask',reason:`需要人工审批：${result.reason}`};
      return {...base,behavior:'allow',reason:`GPT_APPROVE L1：${result.reason}`};
    }catch(e){await this.audit('GPT_APPROVAL_FAILED',{tool,session,fingerprint:operationFingerprint(tool,input),reason:e instanceof Error?e.message:'审批失败'});return deny('GPT审批不可用或返回无效决策；本操作未执行。');}
  };
}
