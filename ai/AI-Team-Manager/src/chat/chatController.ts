import { randomUUID } from 'node:crypto';
import type { Conversation, ConversationFile, CodexEvent, ChatMessage, RequestRecord } from './chatTypes';
import { ConversationStorage } from '../storage/conversationStorage';
import { CodexService, type RunningRequest } from '../services/codexService';
import { CodexError, redact } from '../services/codexErrors';
import { sumUsage, usageDelta } from '../services/codexUsageService';
import type { CodexRunOptions } from '../process/cliLauncher';
export interface ChatBridge {options:CodexRunOptions;timeoutMs?:number;finalize?:(record:RequestRecord)=>Promise<void>;close():Promise<void>}
export class ChatController {
  private data!: ConversationFile;
  private request?: RunningRequest;
  private busy = false;
  private cancelRequested = false;
  private abort?: AbortController;
  private error?: string;
  private connection = '尚未检测';
  private activities: string[] = [];
  private bridge?:ChatBridge;
  constructor(private storage: ConversationStorage, private service: CodexService,
    private changed: () => void, private verify: () => Promise<void>, private timeout: () => number,
    private prepareBridge?:(root:string,text:string,owner:string,turn:string)=>Promise<ChatBridge>) {}
  async initialize(): Promise<void> {
    const saved = await this.storage.load();
    this.data = saved || {version:1,projectPath:this.storage.projectPath,activeId:'',conversations:[]};
    for (const c of this.data.conversations) {
      if (c.status === 'RUNNING') c.status = 'CANCELLED';
      for (const r of c.requests) if (r.status === 'RUNNING') {r.status = 'CANCELLED';r.error = '上次请求因插件退出中断，未自动重发。';}
      for (const m of c.messages) if (m.status === 'RUNNING') m.status = 'CANCELLED';
    }
    if (!this.active) this.create();
    await this.save();
  }
  get active(): Conversation | undefined {return this.data?.conversations.find(c => c.id === this.data.activeId);}
  get isBusy(): boolean {return this.busy;}
  snapshot(): unknown {
    const active = this.active;
    const last = active?.requests.at(-1);
    return {projectPath:this.storage.projectPath,connection:this.connection,cli:this.service.executableInfo,busy:this.busy,error:this.error,activities:this.activities,
      conversation:active,usage:last?.usageSnapshot || (active ? sumUsage(active.requests) : undefined),
      history:this.data?.conversations.map(c => ({id:c.id,threadId:c.threadId,createdAt:c.createdAt,updatedAt:c.updatedAt,status:c.status,title:c.messages.find(m => m.role === 'user')?.text.slice(0,60) || '新会话'})) || []};
  }
  private create(): void {
    const time = new Date().toISOString(); const id = randomUUID();
    this.data.conversations.push({id,projectPath:this.storage.projectPath,createdAt:time,updatedAt:time,status:'READY',messages:[],requests:[]});
    this.data.activeId = id;this.error = undefined;this.activities = [];
  }
  private async save(): Promise<void> {await this.storage.save(this.data);}
  private update(): void {this.changed();void this.save().catch(() => {this.error = new CodexError('STORAGE_ERROR').message;this.changed();});}
  async check(): Promise<void> {
    this.connection = '检测中'; this.changed();
    try {await this.verify();this.connection = await this.service.check(this.storage.projectPath);this.error = undefined;}
    catch(error) {this.connection = '不可用';this.error = error instanceof CodexError ? error.message : new CodexError('PROCESS_START_FAILED').message;}
    this.changed();
  }
  async newConversation(): Promise<void> {if(this.busy) throw new Error('请先停止当前请求。');this.create();await this.save();this.changed();}
  async select(id: string): Promise<void> {
    if(this.busy) throw new Error('请先停止当前请求。');
    if(!this.data.conversations.some(c => c.id === id)) throw new Error('历史会话不存在。');
    this.data.activeId = id;this.error = undefined;this.activities = [];await this.save();this.changed();
  }
  async clear(): Promise<void> {
    if(this.busy) throw new Error('请先停止当前请求。');
    this.data.conversations = [];this.create();await this.save();this.changed();
  }
  async send(text: string): Promise<void> {
    if(this.busy) throw new Error('当前请求尚未结束。');
    if(!text.trim() || text.length > 100000) throw new Error('请输入 1–100000 字符的消息。');
    this.busy = true;this.cancelRequested = false;this.error = undefined;this.activities = [];this.changed();
    this.abort = new AbortController();
    let record: RequestRecord | undefined;
    let protocolViolation = false;
    const conversation = this.active!;
    try {
      await this.verify();
      if(this.cancelRequested) throw new CodexError('REQUEST_CANCELLED');
      this.connection = await this.service.check(this.storage.projectPath,this.abort.signal);
      if(this.cancelRequested) throw new CodexError('REQUEST_CANCELLED');
      // Any previous request without a thread ID is ambiguous: never silently start over.
      if(conversation.requests.length && !conversation.threadId) throw new CodexError('SESSION_RESUME_FAILED');
      const time = new Date().toISOString(); const requestId = randomUUID();
      record = {id:requestId,time,status:'RUNNING'};
      try {this.bridge=await this.prepareBridge?.(this.storage.projectPath,text,conversation.id,requestId);}
      catch(error) {throw error instanceof CodexError?error:new CodexError('TEAM_PREPARE_FAILED',error);}
      if(this.cancelRequested) throw new CodexError('REQUEST_CANCELLED');
      // A rejected budget/bridge preparation is not a sent message or an ambiguous CLI session.
      conversation.requests.push(record);conversation.status = 'RUNNING';conversation.updatedAt = time;
      conversation.messages.push({id:randomUUID(),role:'user',text:redact(text),time,status:'COMPLETED',requestId});
      await this.save();this.changed();
      if(this.cancelRequested) throw new CodexError('REQUEST_CANCELLED');
      const request = this.service.run(this.storage.projectPath,text,conversation.threadId,event => {
        if(event.type === 'thread') {
          if(conversation.threadId && conversation.threadId !== event.id) {protocolViolation = true;void this.stop().catch(() => {});}
          else conversation.threadId = event.id;
        } else if(event.type === 'message') {
          this.connection = '已连接 · 正在响应';
          const id = `${requestId}:${event.id}`;
          let message = conversation.messages.find(m => m.id === id);
          if(!message) {message = {id,role:'assistant',text:'',time:new Date().toISOString(),status:'RUNNING',requestId};conversation.messages.push(message);}
          message.text = event.text;message.status = event.completed ? 'COMPLETED' : 'RUNNING';
        } else if(event.type === 'usage') {
          record!.usageSnapshot = event.usage;
          record!.usage = usageDelta(event.usage,conversation.requests.at(-2)?.usageSnapshot,conversation.requests.length === 1);
        }
        else if(event.type === 'activity') {
          if(event.kind==='mcp'){
            record!.toolCalls||=[];const prior=record!.toolCalls.find(t=>t.id===event.id);
            if(prior){prior.summary=event.text;prior.completed=event.completed;}
            else record!.toolCalls.push({id:event.id,summary:event.text,completed:event.completed});
          }
          if(event.kind === 'file') {protocolViolation = true;void this.stop().catch(() => {});}
          this.activities.push(`${event.kind === 'file' ? '只读违规' : event.kind==='mcp' ? '真实工具调用' : event.completed ? '命令完成' : '只读命令运行中'}: ${event.text}${event.exitCode !== undefined ? `（退出码 ${event.exitCode}）` : ''}${event.output ? `\n${event.output}` : ''}`);
          this.activities = this.activities.slice(-30);
        } else if(event.type === 'parseError') this.activities.push(new CodexError('JSONL_PARSE_ERROR').message);
        else if(event.type === 'turn' && event.state === 'started') this.connection = 'CLI 已启动 · 正在响应';
        // Never put raw error payloads or reasoning into UI/storage/logs.
        conversation.updatedAt = new Date().toISOString();this.update();
      },Math.max(this.timeout(),this.bridge?.timeoutMs||0),this.bridge?.options);
      this.request = request;
      await request.done;
      if(protocolViolation || !conversation.threadId) throw new CodexError('CODEX_VERSION_UNSUPPORTED');
      record.status = 'COMPLETED';conversation.status = 'COMPLETED';
    } catch(error) {
      const safe = protocolViolation ? new CodexError('CODEX_VERSION_UNSUPPORTED') : error instanceof CodexError ? error : new CodexError('PROCESS_START_FAILED');
      this.error = safe.message;
      conversation.status = safe.code === 'REQUEST_CANCELLED' ? 'CANCELLED' : 'FAILED';
      if(record) {record.status = conversation.status;record.error = safe.message;}
    } finally {
      if(record)await this.bridge?.finalize?.(record).catch(()=>{this.error='用量账本记录失败，后续自动开发需要重新检查。';});
      await this.bridge?.close().catch(()=>{});this.bridge=undefined;
      if(record) for(const m of conversation.messages) if(m.requestId === record.id && m.status === 'RUNNING') m.status = record.status;
      this.request = undefined;this.abort = undefined;this.busy = false;
      if(this.connection.includes('正在响应')) this.connection = record?.status === 'COMPLETED' ? '已连接 · 空闲' : record?.status === 'CANCELLED' ? '已停止' : '请求失败';
      await this.save().catch(() => {this.error = new CodexError('STORAGE_ERROR').message;});this.changed();
    }
  }
  async stop(): Promise<void> {this.cancelRequested = true;this.abort?.abort();await Promise.allSettled([this.bridge?.close(),this.request?.stop()]);}
}
