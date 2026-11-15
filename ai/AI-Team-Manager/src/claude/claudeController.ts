import { randomUUID } from "node:crypto";
import type {
  Conversation,
  ConversationFile,
  RequestRecord,
} from "../chat/chatTypes";
import { ConversationStorage } from "../storage/conversationStorage";
import { ClaudeService, type ClaudeRunningRequest, type ClaudeRunOptions } from "./claudeService";
import { ClaudeError, safeClaudeText } from "./claudeErrors";
import type { ClaudeSnapshot, ClaudeActivity, ClaudeRetryEvent } from "./claudeTypes";
import { claudeRetryPolicy, claudeRetryDelay, waitForClaudeRetry, claudeRetryMessage, type ClaudeRetryPolicy } from './claudeRetry';
import { retryWorkspaceFingerprint } from './claudeRetrySnapshot';
import type { PermissionRun } from '../permissions/permissionTypes';
export interface ClaudeSendOptions {
  preparePermissions?: (taskId: string, session?: string) => Promise<PermissionRun>;
  observe?: (event: unknown) => void;
  runOptions?: ClaudeRunOptions;
}
export class ClaudeController {
  private data!: ConversationFile;
  private request?: ClaudeRunningRequest;
  private busy = false;
  private abort?: AbortController;
  private cancelled = false;
  private error?: string;
  private connection = "尚未检测";
  private activities: ClaudeActivity[] = [];
  private statusEvent?: string;
  private persistTimer?: ReturnType<typeof setTimeout>;
  private permissionRun?: PermissionRun;
  constructor(
    private storage: ConversationStorage,
    private service: ClaudeService,
    private changed: () => void,
    private verify: () => Promise<void>,
    private timeout: () => number,
    private preparePermissions?: (taskId: string, session?: string) => Promise<PermissionRun>,
    private retryPolicy: () => Partial<ClaudeRetryPolicy> = () => ({}),
  ) {}
  get isBusy() {
    return this.busy;
  }
  get active(): Conversation | undefined {
    return this.data?.conversations.find((c) => c.id === this.data.activeId);
  }
  async initialize(): Promise<void> {
    try {
      this.data = (await this.storage.load()) || {
        version: 1,
        projectPath: this.storage.projectPath,
        activeId: "",
        conversations: [],
      };
      for (const c of this.data.conversations) {
        if (c.status === "RUNNING") c.status = "CANCELLED";
        for (const r of c.requests)
          if (r.status === "RUNNING") {
            r.status = "CANCELLED";
            r.error = "上次请求因退出而中断，未自动重发。";
            const last=r.claudeRetries?.at(-1);
            if(last&&['RETRY_SCHEDULED','RETRY_STARTED'].includes(last.kind))r.claudeRetries!.push({...last,kind:'RETRY_CANCELLED',time:new Date().toISOString(),delayMs:0,nextRetryAt:undefined});
          }
        for (const m of c.messages)
          if (m.status === "RUNNING") m.status = "CANCELLED";
      }
      if (!this.active) this.create();
      await this.save();
    } catch {
      throw new ClaudeError("STORAGE_ERROR");
    }
  }
  snapshot(): ClaudeSnapshot {
    return {
      projectPath: this.storage.projectPath,
      connection: this.connection,
      busy: this.busy,
      error: this.error,
      cli: this.service.executableInfo,
      conversation: this.active,
      sessionCount:
        this.data?.conversations.filter((c) => c.threadId).length || 0,
      usage: this.active?.requests.at(-1)?.usage,
      activities: this.activities,
      statusEvent: this.statusEvent,
      retry: this.active?.requests.at(-1)?.claudeRetries?.at(-1),
      history:
        this.data?.conversations.map((c) => ({
          id: c.id,
          title:
            c.messages.find((m) => m.role === "user")?.text.slice(0, 60) ||
            "新 Claude 会话",
          status: c.status,
          updatedAt: c.updatedAt,
        })) || [],
    };
  }
  private create(): void {
    const time = new Date().toISOString(),
      id = randomUUID();
    this.data.conversations.push({
      id,
      projectPath: this.storage.projectPath,
      createdAt: time,
      updatedAt: time,
      status: "READY",
      messages: [],
      requests: [],
    });
    this.data.activeId = id;
    this.error = undefined;
    this.activities = [];
    this.statusEvent = undefined;
  }
  private async save(): Promise<void> {
    try {
      await this.storage.save(this.data);
    } catch {
      throw new ClaudeError("STORAGE_ERROR");
    }
  }
  private update(): void {
    this.changed();
    if (!this.persistTimer)
      this.persistTimer = setTimeout(() => {
        this.persistTimer = undefined;
        void this.save().catch(() => {
          this.error = new ClaudeError("STORAGE_ERROR").message;
          this.changed();
        });
      }, 200);
  }
  async check(): Promise<void> {
    if (this.busy) throw new Error("请先停止 Claude 当前任务。");
    this.connection = "检测中";
    this.changed();
    try {
      await this.verify();
      this.connection = await this.service.check(this.storage.projectPath);
      this.error = undefined;
    } catch (e) {
      this.connection = "不可用";
      this.error =
        e instanceof ClaudeError
          ? e.message
          : new ClaudeError("PROCESS_START_FAILED").message;
    }
    this.changed();
  }
  async newConversation(): Promise<void> {
    if (this.busy) throw new Error("请先停止 Claude 当前任务。");
    this.create();
    await this.save();
    this.changed();
  }
  async select(id: string): Promise<void> {
    if (this.busy) throw new Error("请先停止 Claude 当前任务。");
    if (!this.data.conversations.some((c) => c.id === id))
      throw new Error("Claude 历史会话不存在。");
    this.data.activeId = id;
    this.error = undefined;
    this.activities = [];
    this.changed();
    await this.save();
  }
  async send(text: string, options?: ClaudeSendOptions): Promise<void> {
    if (this.busy) throw new Error("Claude 当前任务尚未结束。");
    if (!text.trim() || text.length > 100000)
      throw new Error("请输入 1–100000 字符的 Claude 任务。");
    this.busy = true;
    this.cancelled = false;
    this.error = undefined;
    this.activities = [];
    this.abort = new AbortController();
    this.statusEvent = "检查 CLI 与工作区";
    this.changed();
    const conversation = this.active!;
    let record: RequestRecord | undefined;
    try {
      await this.verify();
      this.connection = await this.service.check(
        this.storage.projectPath,
        this.abort.signal,
      );
      if (this.cancelled) throw new ClaudeError("REQUEST_CANCELLED");
      if (conversation.requests.length && !conversation.threadId)
        throw new ClaudeError("SESSION_RESUME_FAILED");
      const time = new Date().toISOString(),
        id = randomUUID();
      record = { id, time, status: "RUNNING" };
      conversation.requests.push(record);
      conversation.status = "RUNNING";
      conversation.updatedAt = time;
      conversation.messages.push({
        id: randomUUID(),
        role: "user",
        text: safeClaudeText(text),
        time,
        status: "COMPLETED",
        requestId: id,
      });
      await this.save();
      this.changed();
      if (this.cancelled) throw new ClaudeError("REQUEST_CANCELLED");
      this.permissionRun = await (options?.preparePermissions || this.preparePermissions)?.(id, conversation.threadId);
      if (this.cancelled) throw new ClaudeError('REQUEST_CANCELLED');
      const policy = claudeRetryPolicy(this.retryPolicy());
      const settings = this.permissionRun?.settings;
      const runOptions = {...options?.runOptions,...(this.permissionRun as PermissionRun & {runOptions?:ClaudeRunOptions})?.runOptions};
      // Capture once, before any CLI attempt. Errors disable automatic replay, not the original task.
      const baseline = policy.maxRetries ? await retryWorkspaceFingerprint(this.storage.projectPath,this.abort!.signal).catch(()=>undefined) : undefined;
      let retry = 0;
      let lastHint: ClaudeError['retry'];
      const emitRetry = async (kind: ClaudeRetryEvent['kind'], delayMs=0, sideEffect?:boolean) => {
        const event: ClaudeRetryEvent = {type:'retry',kind,requestId:id,time:new Date().toISOString(),retry,maxRetries:policy.maxRetries,reason:lastHint!.reason,delayMs,sessionId:conversation.threadId,...(kind==='RETRY_SCHEDULED'?{nextRetryAt:new Date(Date.now()+delayMs).toISOString()}:{}),...(sideEffect===undefined?{}:{sideEffect})};
        (record!.claudeRetries ||= []).push(event);
        this.statusEvent = claudeRetryMessage(event);
        options?.observe?.(event);
        await this.save();this.changed();
      };
      const unchanged = async () => {
        if (!baseline || this.cancelled || this.abort!.signal.aborted) return false;
        if (this.permissionRun && (!this.permissionRun.canRetry || !await this.permissionRun.canRetry())) return false;
        await this.verify();
        const same = await retryWorkspaceFingerprint(this.storage.projectPath,this.abort!.signal) === baseline;
        // A hook/MCP request may arrive while the asynchronous file scan is in progress.
        return same && (!this.permissionRun || !!await this.permissionRun.canRetry?.());
      };
      for (;;) {
      if (this.cancelled) { if(lastHint)await emitRetry('RETRY_CANCELLED'); throw new ClaudeError('REQUEST_CANCELLED'); }
      let activity = false, positiveUsage = false, zeroUsage = false, executionUncertain = false;
      try {
      this.request = this.service.run(
        this.storage.projectPath,
        text,
        conversation.threadId,
        (event) => {
          if (event.type === 'text' && event.text.length || ['tool','execution','parseError'].includes(event.type)) activity = true;
          if (event.type === 'result' || event.type === 'usage') {
            if (event.usage && Object.values(event.usage).some(v=>typeof v==='number'&&v>0)) positiveUsage=true;
            if (event.usage && event.usage.total===null) executionUncertain=true;
            if (event.type==='result') zeroUsage=event.usage?.total===0;
            if (event.type==='result' && event.denied) activity=true;
          }
          options?.observe?.(event);
          if (event.type === "session") {
            if (!conversation.threadId) conversation.threadId = event.id;
          } else if (event.type === "text") {
            const messageId = `${id}:${event.id}`;
            let message = conversation.messages.find((m) => m.id === messageId);
            if (!message) {
              message = {
                id: messageId,
                role: "assistant",
                text: "",
                time: new Date().toISOString(),
                status: "RUNNING",
                requestId: id,
              };
              conversation.messages.push(message);
            }
            message.text = event.text;
            message.status = event.completed ? "COMPLETED" : "RUNNING";
            this.connection = "已连接 · 正在响应";
          } else if (event.type === "status") this.statusEvent = event.message;
          else if (event.type === "tool") {
            const previous = this.activities.find((a) => a.id === event.id);
            if (previous) {
              previous.phase = event.phase;
              if (event.detail) previous.detail = event.detail;
            } else
              this.activities.push({
                id: event.id,
                name: event.name,
                phase: event.phase,
                detail: event.detail,
              });
            if (this.activities.length > 100) this.activities.shift();
            this.statusEvent = `${event.name} · ${event.phase}`;
          } else if (event.type === "result") {
            record!.usage = event.usage;
          } else if (event.type === "parseError")
            this.statusEvent = "收到异常输出，等待安全终止结果";
          conversation.updatedAt = new Date().toISOString();
          this.update();
        },
        this.timeout(),
        settings,
        runOptions,
      );
      await this.request.done;
      if (this.cancelled) throw new ClaudeError('REQUEST_CANCELLED');
      if (retry) await emitRetry('RETRY_SUCCEEDED');
      break;
      } catch (error) {
        if (this.cancelled || this.abort!.signal.aborted) {
          if (lastHint) await emitRetry('RETRY_CANCELLED');
          throw new ClaudeError('REQUEST_CANCELLED');
        }
        if (this.cancelled) throw new ClaudeError('REQUEST_CANCELLED');
        const kind = activity || positiveUsage || executionUncertain ? 'RETRY_BLOCKED_SIDE_EFFECT' : 'RETRY_CANCELLED';
        if (!(error instanceof ClaudeError) || !error.retry || !policy.maxRetries) {
          if(lastHint)await emitRetry(kind,0,kind==='RETRY_BLOCKED_SIDE_EFFECT');
          throw error;
        }
        lastHint = error.retry;
        const safe = !activity && !positiveUsage && !executionUncertain && (zeroUsage || lastHint.routeRejected) && this.request?.replaySafe?.() && await unchanged().catch(()=>false);
        if (!safe) { await emitRetry('RETRY_BLOCKED_SIDE_EFFECT',0,true); throw error; }
        if (retry >= policy.maxRetries) { await emitRetry('RETRY_EXHAUSTED'); throw error; }
        const delay = claudeRetryDelay(lastHint,retry+1,policy);
        if (delay===undefined) { await emitRetry('RETRY_DEFERRED'); throw error; }
        retry++;
        this.connection = '重试等待中';
        await emitRetry('RETRY_SCHEDULED',delay,false);
        try { await waitForClaudeRetry(delay,this.abort!.signal); }
        catch { await emitRetry('RETRY_CANCELLED'); throw new ClaudeError('REQUEST_CANCELLED'); }
        if (!await unchanged().catch(()=>false)) {
          if (this.cancelled) { await emitRetry('RETRY_CANCELLED'); throw new ClaudeError('REQUEST_CANCELLED'); }
          await emitRetry('RETRY_BLOCKED_SIDE_EFFECT',0,true);throw error;
        }
        await emitRetry('RETRY_STARTED');
        this.connection = '正在重试';
      }
      }
      record.status = "COMPLETED";
      conversation.status = "COMPLETED";
      this.connection = "已连接 · 空闲";
      this.statusEvent = record.claudeRetries?.at(-1)?.kind==='RETRY_SUCCEEDED'
        ? 'Claude 已恢复连接，原任务执行已结束（未经 GPT 验收）。' : '任务执行已结束（未经 GPT 验收）';
    } catch (error) {
      const safe =
        error instanceof ClaudeError
          ? error
          : new ClaudeError("PROCESS_START_FAILED");
      this.error = safe.message;
      conversation.status =
        safe.code === "REQUEST_CANCELLED" ? "CANCELLED" : "FAILED";
      if (record) {
        record.status = conversation.status;
        record.error = safe.message;
      }
      this.connection =
        conversation.status === "CANCELLED" ? "已停止" : "请求失败";
      const lastRetry=record?.claudeRetries?.at(-1);
      this.statusEvent = lastRetry ? claudeRetryMessage(lastRetry) : this.connection;
    } finally {
      await this.permissionRun?.close();
      this.permissionRun = undefined;
      if (record)
        for (const m of conversation.messages)
          if (m.requestId === record.id && m.status === "RUNNING")
            m.status = record.status;
      for (const tool of this.activities)
        if (tool.phase === "running") tool.phase = "failed";
      clearTimeout(this.persistTimer);
      this.persistTimer = undefined;
      this.request = undefined;
      this.abort = undefined;
      this.busy = false;
      await this.save().catch(() => {
        this.error = new ClaudeError("STORAGE_ERROR").message;
        conversation.status = "FAILED";
        if (record) {
          record.status = "FAILED";
          record.error = this.error;
        }
      });
      this.changed();
    }
  }
  async stop(): Promise<void> {
    this.cancelled = true;
    this.abort?.abort();
    await Promise.allSettled([this.permissionRun?.close(), this.request?.stop()]);
  }
}
