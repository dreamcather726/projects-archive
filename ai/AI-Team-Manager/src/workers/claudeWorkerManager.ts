import * as path from 'node:path';
import { realpath } from 'node:fs/promises';
import { randomUUID } from 'node:crypto';
import { ClaudeController, type ClaudeSendOptions } from '../claude/claudeController';
import { ClaudeService } from '../claude/claudeService';
import { ClaudeStorage } from '../claude/claudeStorage';
import { ReviewStorage } from '../review/reviewStorage';
import { safeReviewText } from '../review/reviewSafety';
import type { WorkerRecord, WorkerStatus, WorkerEvent, WorkerDetail } from './workerTypes';
import type { ClaudeRetryPolicy } from '../claude/claudeRetry';

interface WorkerRuntime { record: WorkerRecord; controller: ClaudeController; run?: Promise<void>; queued?: { prompt: string; options?: ClaudeSendOptions; resolve: () => void; reject: (e: unknown) => void }; stopTarget?: 'PAUSED' | 'CANCELLED' }
export interface WorkerManagerOptions {
  project_id: string;
  directory: string;
  maxConcurrent?: number;
  service: () => ClaudeService;
  verify: (worker: WorkerRecord) => Promise<void>;
  timeout?: () => number;
  retryPolicy?: () => Partial<ClaudeRetryPolicy>;
  changed?: () => void;
}
/** Backend-owned runtimes: closing or switching a Webview never disposes these controllers. */
export class ClaudeWorkerManager {
  private workers = new Map<string, WorkerRuntime>();
  private storage: ReviewStorage;
  private saves: Promise<void> = Promise.resolve();
  private pumping = false;
  private disposed = false;
  private selected?: string;
  private maxConcurrent: number;
  constructor(private options: WorkerManagerOptions) {
    this.storage = new ReviewStorage(options.directory);
    this.maxConcurrent = options.maxConcurrent ?? 3;
    if (!Number.isInteger(this.maxConcurrent) || this.maxConcurrent < 1 || this.maxConcurrent > 8) throw Error('并发数量必须为1–8。');
  }
  private async persist(notify=true) {
    const records = structuredClone([...this.workers.values()].map(w => w.record));
    const save = this.saves.then(() => this.storage.write(this.options.project_id, 'worker-history.json', records));
    this.saves = save; await save; if(notify)this.options.changed?.();
  }
  private async transition(w: WorkerRuntime, status: WorkerStatus, reason?: string) {
    w.record.status = status; w.record.updated_at = new Date().toISOString();
    this.append(w, { type: 'lifecycle', status, reason }); await this.persist();
  }
  private append(w: WorkerRuntime, event: WorkerEvent['event']) {
    const r = w.record;
    r.events.push({ sequence: r.events.length, time: new Date().toISOString(), project_id: r.project_id, task_id: r.task_id, worker_id: r.worker_id, session_id: r.session_id, event: JSON.parse(safeReviewText(JSON.stringify(event))) });
    if (this.selected !== r.worker_id && (event.type === 'text' && event.completed || event.type === 'lifecycle' && ['COMPLETED','FAILED','PAUSED','INTERRUPTED'].includes(event.status))) r.unread++;
  }
  private async runtime(record: WorkerRecord): Promise<WorkerRuntime> {
    let w!: WorkerRuntime;
    const controller = new ClaudeController(new ClaudeStorage(path.join(this.options.directory, 'workers', record.worker_id), record.workspace_path), this.options.service(), () => this.options.changed?.(), () => this.options.verify(w.record), this.options.timeout || (() => 300000), undefined, this.options.retryPolicy);
    w = { record, controller }; await controller.initialize();
    const session = controller.active?.threadId;
    if (record.session_id && session !== record.session_id) throw Error('Worker元数据与原Claude Session不一致，未创建替代会话。');
    record.session_id = session;
    return w;
  }
  async load() {
    const records = await this.storage.read<WorkerRecord[]>(this.options.project_id, 'worker-history.json') || [];
    if (!Array.isArray(records) || records.length > 1000 || records.some(r => r.project_id !== this.options.project_id || !/^[a-f0-9-]{36}$/.test(r.worker_id) || !Array.isArray(r.events) || !Array.isArray(r.allowed_files))) throw Error('Worker历史损坏，未覆盖或重发。');
    for (const record of records) {
      const w = await this.runtime(record); this.workers.set(record.worker_id, w);
      if (['RUNNING','RETRYING','QUEUED','PAUSING'].includes(record.status)) {
        record.status = 'INTERRUPTED';
        if(record.retry&&['RETRY_SCHEDULED','RETRY_STARTED'].includes(record.retry.kind)){
          record.retry={...record.retry,kind:'RETRY_CANCELLED',time:new Date().toISOString(),delayMs:0,nextRetryAt:undefined};this.append(w,record.retry);
        }
        this.append(w, { type: 'lifecycle', status: 'INTERRUPTED', reason: '插件退出中断，不自动重发或复用旧审批。' });
      }
    }
    await this.persist();
  }
  async create(taskId: string, workspace: string, branch: string, allowedFiles: string[] = []): Promise<WorkerRecord> {
    if (this.disposed || this.workers.size >= 1000) throw Error('Worker管理器已关闭或历史数量过多。');
    const root = await realpath(workspace);
    const record: WorkerRecord = { worker_id: randomUUID(), project_id: this.options.project_id, task_id: taskId, workspace_path: root, branch_name: branch, allowed_files: [...allowedFiles], status: 'IDLE', created_at: new Date().toISOString(), updated_at: new Date().toISOString(), unread: 0, events: [] };
    const w = await this.runtime(record); this.workers.set(record.worker_id, w);
    try { await this.persist(); } catch (e) { this.workers.delete(record.worker_id); throw e; }
    return structuredClone(record);
  }
  retryEvents() { return [...this.workers.values()].flatMap(w=>w.record.events.filter(e=>e.event.type==='retry')).map(e=>structuredClone(e)); }
  progressEvents(){return [...this.workers.values()].flatMap(w=>w.record.events.filter(e=>e.event.type==='tool'&&e.event.name.endsWith('run_authorized_test')).slice(-1).map(e=>({...e,event:e.event.type==='tool'?{type:'tool' as const,id:e.event.id,name:e.event.name,phase:e.event.phase}:e.event})));}
  list() { return [...this.workers.values()].map(w => { const {events, ...summary} = w.record; return structuredClone({...summary, event_count: events.length}); }); }
  detail(id: string): WorkerDetail { const w = this.require(id); return { worker: structuredClone(w.record), chat: structuredClone(w.controller.snapshot()) }; }
  events(id: string, cursor = 0, limit = 100) {
    const w = this.require(id); if (!Number.isInteger(cursor) || cursor < 0 || !Number.isInteger(limit) || limit < 1 || limit > 200) throw Error('Worker日志分页参数无效。');
    return { events: structuredClone(w.record.events.slice(cursor, cursor + limit)), next_cursor: Math.min(cursor + limit, w.record.events.length), has_more: cursor + limit < w.record.events.length };
  }
  private require(id: string) { const w = this.workers.get(id); if (!w) throw Error('Worker不存在或不属于当前项目。'); return w; }
  async select(id: string,notify=true) { const w = this.require(id); this.selected = id; w.record.unread = 0; await this.persist(notify); }
  async reassign(id: string, taskId: string, workspace: string, branch: string, files: string[]) {
    const w = this.require(id);
    if (w.run || w.queued || ['RUNNING','RETRYING','PAUSING'].includes(w.record.status) || await realpath(workspace) !== w.record.workspace_path || branch !== w.record.branch_name) throw Error('只能复用已停止且工作区/分支不变的Worker；不会把Session移到其他工作区。');
    w.record.task_id = taskId; w.record.allowed_files = [...files]; w.record.execution_result = undefined; await this.transition(w, 'IDLE');
  }
  run(id: string, prompt: string, options?: ClaudeSendOptions): Promise<void> {
    const w = this.require(id);
    if (this.disposed || w.run || w.queued || !prompt.trim() || prompt.length > 100000) return Promise.reject(Error('Worker忙碌、已关闭或任务内容无效。'));
    return new Promise<void>((resolve, reject) => {
      w.queued = { prompt, options, resolve, reject };
      void this.transition(w, 'QUEUED').then(() => this.pump(), e => { w.queued = undefined; reject(e); });
    });
  }
  private pump() {
    if (this.pumping || this.disposed) return;
    this.pumping = true;
    try {
      for (const w of this.workers.values()) {
        if ([...this.workers.values()].filter(x => x.run).length >= this.maxConcurrent) break;
        if (!w.queued) continue;
        const q = w.queued; w.queued = undefined;
        w.run = this.execute(w, q.prompt, q.options).then(q.resolve, q.reject).finally(() => { w.run = undefined; this.pump(); });
      }
    } finally { this.pumping = false; }
  }
  private async execute(w: WorkerRuntime, prompt: string, options?: ClaudeSendOptions) {
    w.stopTarget = undefined;
    w.record.retry = undefined;
    try {
      await this.options.verify(w.record); await this.transition(w, 'RUNNING');
      if (w.stopTarget || this.disposed) throw Error('Worker已停止。');
      await w.controller.send(prompt, { ...options, runOptions: { ...options?.runOptions, workerLabel: w.record.worker_id }, observe: event => {
        const e = event as WorkerEvent['event'];
        if (e.type === 'session') { if (w.record.session_id && w.record.session_id !== e.id) { w.stopTarget = 'CANCELLED'; void w.controller.stop(); } else w.record.session_id = e.id; }
        if (e.type === 'retry') {
          w.record.retry=e;
          if(!w.stopTarget)w.record.status=e.kind==='RETRY_SCHEDULED'?'RETRYING':'RUNNING';
          w.record.updated_at=e.time;
        }
        if (e.type !== 'text' || e.completed) this.append(w, e); options?.observe?.(e);
        // Text deltas stay in the controller's own debounced storage; completed events flush durable metadata.
        if (e.type !== 'text' || e.completed) void this.persist().catch(() => { w.stopTarget = 'CANCELLED'; void w.controller.stop(); });
      } });
      const snapshot = w.controller.snapshot(), request = snapshot.conversation?.requests.at(-1);
      w.record.session_id = snapshot.conversation?.threadId;
      w.record.execution_result = { final_answer: snapshot.conversation?.messages.filter(m => m.role === 'assistant' && m.requestId === request?.id).at(-1)?.text || '', error: snapshot.error, finished_at: new Date().toISOString(), request_id: request?.id };
      const state = w.stopTarget || (snapshot.conversation?.status === 'COMPLETED' ? 'COMPLETED' : snapshot.conversation?.status === 'CANCELLED' ? 'CANCELLED' : 'FAILED');
      await this.transition(w, state);
      if (state !== 'COMPLETED') throw Error(snapshot.error || `Worker ${state}`);
    } catch (e) {
      if (!['PAUSED','CANCELLED','FAILED'].includes(w.record.status)) await this.transition(w, w.stopTarget || 'FAILED', (e as Error).message);
      throw e;
    }
  }
  async stop(id: string, pause = false) {
    const w = this.require(id), target = pause ? 'PAUSED' : 'CANCELLED'; w.stopTarget = target;
    if (w.queued) { const q = w.queued; w.queued = undefined; await this.transition(w, target, '尚未启动的排队任务已停止。'); q.reject(Error(`Worker ${target}`)); return; }
    if (!w.run) return;
    const stopping=w.controller.stop(); // Abort backoff before waiting for any persistence I/O.
    await this.transition(w, 'PAUSING', '停止当前CLI后保留候选文件和Session；不是冻结正在执行的工具。');
    await stopping; await w.run.catch(() => {});
  }
  resume(id: string, prompt: string, options?: ClaudeSendOptions) {
    const w = this.require(id); if (!['PAUSED','INTERRUPTED','FAILED','CANCELLED'].includes(w.record.status)) throw Error('Worker当前状态不能恢复。');
    return this.run(id, prompt, options);
  }
  async setConcurrency(value: number) { if (!Number.isInteger(value) || value < 1 || value > 8) throw Error('并发数量必须为1–8。'); this.maxConcurrent = value; await this.persist(); this.pump(); }
  async dispose() { this.disposed = true; await Promise.allSettled([...this.workers.keys()].map(id => this.stop(id))); await this.saves; }
}
