import * as http from 'node:http';
import { randomBytes, randomUUID } from 'node:crypto';
import { PermissionPolicy, operationFingerprint } from './permissionPolicy';
import type { PermissionChoice, PermissionLog, PermissionOperation, PermissionRun, PermissionDecision } from './permissionTypes';
export type PermissionEvaluator = (decision:PermissionDecision,tool:string,input:unknown,session:string,signal:AbortSignal)=>Promise<PermissionDecision>;

export class PermissionHost implements PermissionRun {
  settings = '';
  private server?: http.Server;
  private closed = false;
  private operationSeen = false;
  canRetry() { return !this.closed && !this.operationSeen && this.controllers.size === 0; }
  private grants = new Set<string>();
  private preDecisions = new Map<string, { allow: boolean; reason: string; expires:number;toolUseId?:string }>();
  private session?: string;
  private controllers = new Set<AbortController>();
  constructor(private policy: PermissionPolicy, private taskId: string,
    private approve: (operation: PermissionOperation, signal: AbortSignal) => Promise<PermissionChoice>,
    private record: (entry: PermissionLog) => Promise<void>, private timeoutMs = 90000,
    private expectedSession?: string, private evaluate?: PermissionEvaluator, private extraTools: string[] = []) {}
  async start(): Promise<this> {
    const token = randomBytes(32).toString('hex');
    this.server = http.createServer((req,res) => {
      const address = this.server?.address();
      if (!address || typeof address === 'string' || req.method !== 'POST' || req.url !== `/permission/${token}` || req.headers.host !== `127.0.0.1:${address.port}`) { res.writeHead(403).end(); return; }
      this.operationSeen = true; // Count before reading the body, including incomplete requests.
      const chunks: Buffer[] = []; let size = 0;
      req.on('data', b => { size += b.length; if (size > 2*1024*1024) { res.writeHead(413).end(); req.destroy(); } else chunks.push(b); });
      req.on('end', () => {
        void (async () => {
          try {
            if (size > 2*1024*1024) return;
            const data = JSON.parse(Buffer.concat(chunks).toString('utf8')), answer = await this.handle(data);
            if (!res.destroyed) res.writeHead(200, {'Content-Type':'application/json'}).end(JSON.stringify(answer));
          } catch { if (!res.destroyed) res.writeHead(500).end(); }
        })();
      });
      req.on('error', () => {});
    });
    await new Promise<void>((resolve,reject) => { this.server!.once('error',reject); this.server!.listen(0,'127.0.0.1',() => resolve()); });
    const url = `http://127.0.0.1:${(this.server.address() as any).port}/permission/${token}`;
    const hook = { matcher: '*', hooks: [{ type: 'http', url, timeout: Math.ceil(this.timeoutMs/1000)+10, onFailure: 'block' }] };
    // Ask rules ensure that if hooks are administratively disabled, print-mode's fallback denies every enabled tool.
    // No saved permissions/settings updates are returned. The CLI receives settings only for this request.
    this.settings = JSON.stringify({ disableAllHooks: false,
      permissions: { ask: ['Read','Edit','Write','Glob','Grep','Bash',...this.extraTools] },
      hooks: { PreToolUse: [hook], PermissionRequest: [hook] } });
    return this;
  }
  private response(event: string, allow: boolean, reason: string) {
    return event === 'PermissionRequest'
      ? { hookSpecificOutput: { hookEventName: event, decision: { behavior: allow ? 'allow' : 'deny', ...(allow ? {} : { message: reason }) } } }
      : { hookSpecificOutput: { hookEventName: 'PreToolUse', permissionDecision: allow ? 'allow' : 'deny', permissionDecisionReason: reason } };
  }
  private async handle(data: any) {
    const event = data?.hook_event_name, tool = data?.tool_name, input = data?.tool_input;
    if (!['PreToolUse','PermissionRequest'].includes(event) || typeof tool !== 'string' || typeof data.session_id !== 'string' || this.closed) throw new Error('Invalid permission request');
    const same = (a: string,b: string) => process.platform === 'win32' ? a.toLowerCase() === b.toLowerCase() : a === b;
    if (!same(data.cwd || '', this.policy.root) || (this.expectedSession && data.session_id !== this.expectedSession) || (this.session && data.session_id !== this.session)) return this.response(event,false,'请求工作区或Session与本次任务不一致。');
    this.session ||= data.session_id;
    const fingerprint = operationFingerprint(tool,input);
    // Re-check real paths for both events; a cached approval can never override a new path/scope denial.
    let decision = await this.policy.check(tool,input);
    const cached=event==='PermissionRequest'?this.preDecisions.get(fingerprint):undefined;
    const prior=cached&&cached.expires>Date.now()&&(!cached.toolUseId||!data.tool_use_id||cached.toolUseId===data.tool_use_id)?cached:undefined;
    if(this.evaluate && decision.behavior!=='deny' && prior)decision={...decision,behavior:prior.allow?'allow':'deny',reason:prior.reason};
    if(this.evaluate && decision.behavior!=='deny' && !prior) {
      const controller=new AbortController();this.controllers.add(controller);
      let timer:ReturnType<typeof setTimeout>|undefined;
      try {
        decision=await Promise.race([this.evaluate(decision,tool,input,this.session!,controller.signal),new Promise<PermissionDecision>(resolve=>{timer=setTimeout(()=>{controller.abort();resolve({...decision,behavior:'deny',reason:'审批服务超时；默认拒绝。'});},this.timeoutMs);})]);
        // Revalidate filesystem identity after the remote decision, before issuing an allow.
        const current=await this.policy.check(tool,input);
        if(current.behavior==='deny')decision=current;
        if(controller.signal.aborted)decision={...decision,behavior:'deny',reason:'任务停止或审批已过期。'};
      } finally {clearTimeout(timer);this.controllers.delete(controller);}
    }
    const grantKey = JSON.stringify([tool,decision.operation,decision.target]);
    let allow = decision.behavior === 'allow', source = this.evaluate&&prior?'SAME_TOOL_CALL':'POLICY', reason = decision.reason;
    if (decision.behavior === 'ask') {
      if (prior) { allow = prior.allow; reason = prior.reason; source = 'SAME_TOOL_CALL'; }
      else if (this.grants.has(grantKey)) { allow = true; source = 'TASK_GRANT'; }
      else {
        const controller = new AbortController(); this.controllers.add(controller);
        const operation: PermissionOperation = { id: randomUUID(), taskId: this.taskId, tool, operation: decision.operation, target: decision.target, reason, preview: decision.preview, fingerprint };
        let timer: ReturnType<typeof setTimeout> | undefined;
        const timed = new Promise<PermissionChoice>(resolve => { timer = setTimeout(() => { controller.abort(); resolve('DENY'); }, this.timeoutMs); controller.signal.addEventListener('abort',() => resolve('DENY'),{once:true}); });
        try {
          const choice = await Promise.race([this.approve(operation,controller.signal), timed]);
          allow = !this.closed && !controller.signal.aborted && (choice === 'ONCE' || choice === 'TASK');
          source = allow ? choice : 'USER_DENIED_OR_CANCELLED';
          reason = allow ? `用户${choice === 'ONCE' ? '允许一次' : '允许本次任务同类操作'}：${reason}` : '用户拒绝、取消、停止或审批超时；该操作未获授权。';
          if (allow && choice === 'TASK' && !this.evaluate) this.grants.add(grantKey);
        } finally { clearTimeout(timer); this.controllers.delete(controller); }
      }
    }
    if (this.closed) { allow = false; reason = '任务已结束，授权失效。'; }
    if(allow){const current=await this.policy.check(tool,input);if(current.behavior==='deny'){allow=false;reason=current.reason;source='PATH_REVALIDATION';}}
    // Persist the decision before letting the CLI act; storage failure => HTTP failure => onFailure:block.
    await this.record({ time: new Date().toISOString(), taskId: this.taskId, sessionId: this.session, tool, operation: decision.operation, target: decision.target, decision: allow ? 'ALLOW' : 'DENY', source, reason, fingerprint,toolUseId:typeof data.tool_use_id==='string'?data.tool_use_id:undefined });
    if (event === 'PreToolUse') {
      if (this.preDecisions.size >= 2000) this.preDecisions.clear();
      this.preDecisions.set(fingerprint,{allow,reason,expires:Date.now()+60000,toolUseId:typeof data.tool_use_id==='string'?data.tool_use_id:undefined});
    } else this.preDecisions.delete(fingerprint);
    return this.response(event,allow,reason);
  }
  async close(): Promise<void> {
    this.closed = true; this.grants.clear(); this.preDecisions.clear();
    for (const c of this.controllers) c.abort();
    const server = this.server; this.server = undefined;
    if (server) { server.closeIdleConnections(); await new Promise<void>(resolve => server.close(() => resolve())); }
  }
}
