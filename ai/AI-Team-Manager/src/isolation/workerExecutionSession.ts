import { ToolBridge, type BridgeTool } from '../dispatch/toolBridge';
import { PermissionHost } from '../permissions/permissionHost';
import type { PermissionChoice, PermissionLog, PermissionOperation } from '../permissions/permissionTypes';
import type { ClaudeRunOptions } from '../claude/claudeService';
import { ScopedWorkerPolicy } from './scopedWorkerPolicy';
import { FileLockManager } from './fileLockManager';
import { FileOwnershipManager } from './fileOwnershipManager';
import { IsolatedTestRunner, type IsolatedTestResult } from './isolatedTestRunner';
import { git } from '../review/reviewGit';
import { safeReviewText } from '../review/reviewSafety';

export const workerTools: BridgeTool[] = [
  {name:'run_authorized_test',description:'Run an exact task-authorized Python test in an isolated read-only source snapshot. No network, host project, other Worker, credentials or Git metadata access. Correct ordinary failures using Edit/Write, then call again. Never use shell.',inputSchema:{type:'object',properties:{command:{type:'string'}},required:['command'],additionalProperties:false}},
  {name:'get_candidate_diff',description:'Return the real candidate Git diff. Does not stage, commit, reset or merge.',inputSchema:{type:'object',properties:{},additionalProperties:false}},
];
export class NativeRestrictedWorkerExecutionSession {
  private host?: PermissionHost;
  private bridge?: ToolBridge;
  private registered: Promise<void>;
  private register!: () => void;
  private registrationError?: Error;
  private closed=false;
  private active=false;
  readonly tests: IsolatedTestResult[]=[];
  runOptions?: ClaudeRunOptions;
  constructor(private root: string, private task: string, private worker: string, private lease: string,
    private ownership: FileOwnershipManager, private locks: FileLockManager, private runner: IsolatedTestRunner,
    private approve: (op: PermissionOperation,signal: AbortSignal)=>Promise<PermissionChoice>, private audit: (e: PermissionLog | {kind:string;data:unknown})=>Promise<void>, private session?: string) {
    this.registered=new Promise(resolve=>{this.register=resolve;});
  }
  async start() {
    const tools=workerTools.map(t=>'mcp__atm_worker__'+t.name);
    const policy=new ScopedWorkerPolicy(this.root,this.ownership,this.locks,this.task,this.worker,this.lease,async()=>{
      await this.registered; if(this.registrationError||this.closed)throw this.registrationError||Error('执行入口已关闭。');
    },tools);
    this.host=await new PermissionHost(policy,this.task,this.approve,e=>this.audit(e),30000,this.session,undefined,tools).start();
    this.bridge=new ToolBridge(async(name,args)=>{
      if(this.closed||!this.active)throw Error('当前Worker执行身份已失效。');
      if(name==='run_authorized_test') {
        if(!args||Object.keys(args).length!==1||typeof args.command!=='string')throw Error('测试参数无效。');
        const task=this.ownership.task(this.task);
        const result=await this.runner.run(this.root,args.command,task.test_commands,undefined,[...task.allowed_files,...task.read_only_files]);
        this.tests.push(result);await this.audit({kind:'ISOLATED_TEST_RESULT',data:result});return result;
      }
      if(name==='get_candidate_diff') {
        const allowed=this.ownership.task(this.task).allowed_files;
        return {diff:safeReviewText(await git(this.root,['diff','--no-ext-diff','--no-textconv','--no-renames','HEAD','--',...allowed.map(f=>':(literal)'+f)])),untracked:(await git(this.root,['ls-files','--others','--exclude-standard','-z'])).split('\0').filter(f=>f&&this.ownership.mayWrite(this.task,f))};
      }
      throw Error('未知隔离工具。');
    },async()=>{},workerTools);
    try {
      const mcpConfig=await this.bridge.startClaude();
      this.runOptions={restricted:true,workerLabel:this.worker,mcpConfig,onProcess:async pid=>{
        try { await this.locks.attachProcess(this.lease,pid);this.active=true; }
        catch(e){this.registrationError=e instanceof Error?e:Error('写入进程登记失败。');throw e;}
        finally{this.register();}
      }};
      return this;
    } catch(e) {await this.close();throw e;}
  }
  get settings() { if(!this.host)throw Error('执行权限入口尚未启动。');return this.host.settings; }
  async close() { this.closed=true;this.active=false;this.register();await this.host?.close();await this.bridge?.close();await this.runner.drain(); }
}
export { BoundedWorkerExecutionSession as WorkerExecutionSession } from './boundedWorkerExecution';
