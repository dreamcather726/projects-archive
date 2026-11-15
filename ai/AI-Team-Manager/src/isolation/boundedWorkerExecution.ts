import * as fs from 'node:fs/promises';
import * as path from 'node:path';
import { randomUUID } from 'node:crypto';
import { ToolBridge, type BridgeTool } from '../dispatch/toolBridge';
import { operationFingerprint } from '../permissions/permissionPolicy';
import type { PermissionChoice, PermissionLog, PermissionOperation } from '../permissions/permissionTypes';
import type { ClaudeRunOptions } from '../claude/claudeService';
import { ScopedWorkerPolicy } from './scopedWorkerPolicy';
import { FileLockManager } from './fileLockManager';
import { FileOwnershipManager } from './fileOwnershipManager';
import { IsolatedTestRunner, type IsolatedTestResult } from './isolatedTestRunner';
import { git, digest } from '../review/reviewGit';
import { safeReviewText } from '../review/reviewSafety';

const file={type:'string',minLength:1,maxLength:240},text={type:'string',maxLength:512000};
export const boundedWorkerTools: BridgeTool[] = [
  {name:'read_task_file',description:'Read an exact task-authorized file with one-based line pagination.',inputSchema:{type:'object',properties:{file_path:file,start_line:{type:'integer',minimum:1},line_count:{type:'integer',minimum:1,maximum:300}},required:['file_path'],additionalProperties:false}},
  {name:'write_task_file',description:'Create or replace an exact task-authorized file. Actual path, ownership and live lease are checked before writing. Forbidden files cannot be approved.',inputSchema:{type:'object',properties:{file_path:file,content:text},required:['file_path','content'],additionalProperties:false}},
  {name:'edit_task_file',description:'Replace one exact unique string in an authorized file; refuses stale or ambiguous replacements.',inputSchema:{type:'object',properties:{file_path:file,old_string:text,new_string:text},required:['file_path','old_string','new_string'],additionalProperties:false}},
  {name:'list_task_files',description:'List only this task\'s authorized readable and writable paths.',inputSchema:{type:'object',properties:{},additionalProperties:false}},
  {name:'run_authorized_test',description:'Run an exact task-authorized Python test in an isolated read-only snapshot. No network, host project, other Worker, credentials or Git metadata access. Correct ordinary failures using edit_task_file/write_task_file, then call again.',inputSchema:{type:'object',properties:{command:{type:'string'}},required:['command'],additionalProperties:false}},
  {name:'get_candidate_diff',description:'Return real authorized candidate Git diff. Does not stage, commit, reset or merge.',inputSchema:{type:'object',properties:{},additionalProperties:false}},
];
/** No native tools/hooks/customizations. MCP handlers form the pre-execution permission boundary. */
export class BoundedWorkerExecutionSession {
  private bridge?: ToolBridge;
  private policy: ScopedWorkerPolicy;
  private registered: Promise<void>;
  private register!: () => void;
  private registrationError?: Error;
  private closed=false;
  private active=false;
  private processId?:number;
  private operations: Promise<unknown>=Promise.resolve();
  private operationSeen = false;
  canRetry() { return !this.closed && !this.active && !this.operationSeen && !this.registrationError && this.aborts.size===0 && !!this.bridge?.canRetry(); }

  private aborts=new Set<AbortController>();
  readonly tests: IsolatedTestResult[]=[];
  localApprovals=0;
  runOptions?: ClaudeRunOptions;
  constructor(private root: string, private task: string, private worker: string, private lease: string,
    private ownership: FileOwnershipManager, private locks: FileLockManager, private runner: IsolatedTestRunner,
    private approve: (op: PermissionOperation,signal: AbortSignal)=>Promise<PermissionChoice>, private audit: (e: PermissionLog | {kind:string;data:unknown})=>Promise<void>, private session?: string) {
    this.registered=new Promise(resolve=>{this.register=resolve;});
    this.policy=new ScopedWorkerPolicy(root,ownership,locks,task,worker,lease,async()=>{
      await this.registered;if(this.registrationError||this.closed)throw this.registrationError||Error('执行入口已关闭。');
    });
  }
  private async authorize(tool:string,input:any) {
    let d=await this.policy.check(tool,input),allow=d.behavior==='allow',source='LOCAL_POLICY';
    if(d.behavior==='ask') {
      const abort=new AbortController();this.aborts.add(abort);let timer:ReturnType<typeof setTimeout>|undefined;
      try {const choice=await Promise.race([this.approve({id:randomUUID(),taskId:this.task,tool,operation:d.operation,target:d.target,reason:d.reason,preview:d.preview,fingerprint:operationFingerprint(tool,input)},abort.signal),new Promise<PermissionChoice>(resolve=>{timer=setTimeout(()=>{abort.abort();resolve('DENY');},30000);abort.signal.addEventListener('abort',()=>resolve('DENY'),{once:true});})]);allow=!this.closed&&!abort.signal.aborted&&['ONCE','TASK'].includes(choice);source='HUMAN_'+choice;}
      finally{clearTimeout(timer);this.aborts.delete(abort);}
    }
    const fresh=await this.policy.check(tool,input);if(this.closed||fresh.behavior==='deny'){allow=false;d=fresh;}
    await this.audit({time:new Date().toISOString(),taskId:this.task,sessionId:this.session,tool,operation:d.operation,target:d.target,decision:allow?'ALLOW':'DENY',source,reason:d.reason,fingerprint:operationFingerprint(tool,input)});
    if(!allow)throw Error(d.reason||'操作未获批准。');if(source==='LOCAL_POLICY')this.localApprovals++;
    return d;
  }
  private async call(name:string,args:any) {
    if(this.closed||!this.active)throw Error('当前Worker执行身份已失效。');
    try {if(!this.processId)throw Error();process.kill(this.processId,0);}catch{throw Error('Worker进程已退出，旧工具凭据失效。');}
    if(!args||typeof args!=='object'||Array.isArray(args))throw Error('参数无效。');
    const task=this.ownership.task(this.task);
    if(name==='list_task_files')return {allowed_files:task.allowed_files,read_only_files:task.read_only_files,forbidden_files:task.forbidden_files};
    if(name==='read_task_file') {
      if(Object.keys(args).some(k=>!['file_path','start_line','line_count'].includes(k)))throw Error('未知读取参数。');
      const start=args.start_line??1,count=args.line_count??200;if(!Number.isInteger(start)||start<1||!Number.isInteger(count)||count<1||count>300)throw Error('分页无效。');
      const d=await this.authorize('Read',args),full=path.join(this.root,d.target),stat=await fs.stat(full);if(stat.size>512000)throw Error('文件过大，需人工补充受限上下文。');
      const bytes=await fs.readFile(full),lines=safeReviewText(bytes.toString('utf8')).split('\n');return {file_path:d.target,hash:digest(bytes),start_line:start,total_lines:lines.length,text:lines.slice(start-1,start-1+count).join('\n')};
    }
    if(name==='write_task_file'||name==='edit_task_file') {
      const edit=name==='edit_task_file',keys=edit?['file_path','old_string','new_string']:['file_path','content'];if(Object.keys(args).some(k=>!keys.includes(k))||keys.some(k=>typeof args[k]!=='string')||keys.some(k=>args[k].length>512000))throw Error('写入参数无效或超限。');
      const tool=edit?'Edit':'Write',d=await this.authorize(tool,args),full=path.join(this.root,d.target);
      const before=await fs.readFile(full).catch((e:any)=>{if(e.code==='ENOENT')return undefined;throw e;});let content=args.content;
      if(edit){const old=before?.toString('utf8');if(old===undefined||!args.old_string||old.split(args.old_string).length!==2)throw Error('原文缺失或不唯一，未写入。');content=old.replace(args.old_string,args.new_string);}
      if(Buffer.byteLength(content)>512000)throw Error('目标文件超过当前限额。');
      if(before && digest(before)===digest(content)){const result={file_path:d.target,kind:'UNCHANGED',before_hash:digest(before),after_hash:digest(content)};await this.audit({kind:'FILE_WRITE_SKIPPED',data:result});return result;}
      await fs.mkdir(path.dirname(full),{recursive:true});const temporary=path.join(path.dirname(full),'.atm-write-'+randomUUID()+'.tmp');
      try {await fs.writeFile(temporary,content,{flag:'wx'});const fresh=await this.policy.check(tool,args);if(this.closed||fresh.behavior==='deny')throw Error(fresh.reason||'执行已结束。');const current=await fs.readFile(full).catch((e:any)=>{if(e.code==='ENOENT')return undefined;throw e;});if(digest(before||'')!==digest(current||'')||!!before!==!!current)throw Error('候选文件在写入前已改变，未覆盖。');await fs.rename(temporary,full);}
      finally{await fs.unlink(temporary).catch(()=>{});}
      const result={file_path:d.target,kind:before?'MODIFIED':'ADDED',before_hash:before?digest(before):null,after_hash:digest(content)};await this.audit({kind:'FILE_WRITE_RESULT',data:result});return result;
    }
    if(name==='run_authorized_test') {
      if(Object.keys(args).length!==1||typeof args.command!=='string')throw Error('测试参数无效。');
      const result=await this.runner.run(this.root,args.command,task.test_commands,undefined,[...task.allowed_files,...task.read_only_files]);this.localApprovals++;this.tests.push(result);await this.audit({kind:'ISOLATED_TEST_RESULT',data:result});return result;
    }
    if(name==='get_candidate_diff')return {diff:safeReviewText(await git(this.root,['diff','--no-ext-diff','--no-textconv','--no-renames','HEAD','--',...task.allowed_files.map(f=>':(literal)'+f)])),untracked:(await git(this.root,['ls-files','--others','--exclude-standard','-z'])).split('\0').filter(f=>f&&this.ownership.mayWrite(this.task,f))};
    throw Error('未知隔离工具。');
  }
  async start() {
    this.bridge=new ToolBridge((name,args)=>{this.operationSeen=true;const run=this.operations.then(async()=>{try{return await this.call(name,args);}catch(e){await this.audit({kind:'TOOL_FAILED',data:{tool:name,reason:safeReviewText((e as Error).message).slice(0,1000)}});throw e;}});this.operations=run.catch(()=>{});return run;},async()=>{},boundedWorkerTools);
    const mcpConfig=await this.bridge.startClaude();
    this.runOptions={boundedMcp:true,workerLabel:this.worker,mcpConfig,allowedMcpTools:boundedWorkerTools.map(t=>'mcp__atm_worker__'+t.name),onProcess:async pid=>{
      try {
        if(this.closed||this.active||this.processId||this.operationSeen||this.registrationError||!this.bridge?.canRetry())throw Error('前一次 Worker 执行尚未安全结束');
        await this.locks.attachProcess(this.lease,pid);
        if(this.closed)throw Error('进程登记期间 Worker 执行入口已关闭');
        this.processId=pid;this.active=true;
      }catch(e){this.registrationError=e instanceof Error?e:Error('写入进程登记失败。');throw e;}finally{this.register();}
    },onProcessExit:async pid=>{
      if(this.processId!==pid)throw Error('Worker 进程身份不匹配');
      this.active=false;this.processId=undefined;
      for(const a of this.aborts)a.abort();
      await this.operations;await this.runner.drain();
    }};
    return this;
  }
  get settings() {return '';}
  bindSession(id:string){if(!/^[a-f0-9-]{36}$/.test(id)||this.session&&this.session!==id){this.closed=true;throw Error('Worker Session与本次执行身份不一致。');}this.session=id;}
  async close(){this.closed=true;this.active=false;this.register();for(const a of this.aborts)a.abort();await this.bridge?.close();await this.operations;await this.runner.drain();}
}
