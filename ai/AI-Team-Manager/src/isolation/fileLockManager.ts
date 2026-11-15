import * as fs from 'node:fs/promises';
import * as path from 'node:path';
import { randomUUID } from 'node:crypto';
import { plannedPath } from '../planning/adaptiveTaskPlanner';
import { digest } from '../review/reviewGit';

export interface FileLease { id: string; project_id: string; task_id: string; worker_id: string; files: string[]; process_ids: number[]; acquired_at: string }
const alive = (pid: number) => { try { process.kill(pid, 0); return true; } catch (e: any) { return e.code !== 'ESRCH'; } };
const overlap = (a: string, b: string) => a === b || a.startsWith(b + '/') || b.startsWith(a + '/');
/** Cross-process atomic metadata transaction. Never expires a live writer's lease merely due to elapsed time. */
export class FileLockManager {
  private directory: string;
  constructor(directory: string, readonly projectId: string) { this.directory = path.join(directory, digest(projectId)); }
  watchDirectory() { return this.directory; }
  private async transaction<T>(fn: (leases: FileLease[]) => Promise<T>): Promise<T> {
    await fs.mkdir(this.directory, { recursive: true });
    const mutex = path.join(this.directory, 'mutex');
    let locked = false;
    for (let n = 0; n < 100; n++) {
      try { await fs.mkdir(mutex); locked = true; await fs.writeFile(path.join(mutex, 'owner.json'), JSON.stringify({ pid: process.pid, nonce: randomUUID() })); break; }
      catch (e: any) {
        if (e.code !== 'EEXIST') throw e;
        // An orphaned mutex is never guessed safe. Recovery of a mutex requires a known dead owner.
        const owner = await fs.readFile(path.join(mutex, 'owner.json'), 'utf8').then(text => {
          try { return JSON.parse(text); } catch { return undefined; } // Another live owner may still be writing its identity.
        }, () => undefined);
        if (owner && Number.isInteger(owner.pid) && owner.pid > 0 && !alive(owner.pid)) {
          try { await fs.rename(mutex, path.join(this.directory, 'orphan-' + randomUUID())); } catch { /* Another process recovered it first. */ }
        }
        await new Promise(r => setTimeout(r, 30));
      }
    }
    if (!locked) throw Error('文件锁事务不可用；保守停止，不抢占未知或仍存活的持有者。');
    const file = path.join(this.directory, 'leases.json');
    try {
      const leases: FileLease[] = await fs.readFile(file, 'utf8').then(JSON.parse, (e: any) => { if (e.code === 'ENOENT') return []; throw e; });
      if (!Array.isArray(leases) || leases.some(l => !l || l.project_id !== this.projectId || !Array.isArray(l.process_ids) || l.process_ids.some(p=>!Number.isInteger(p)||p<=0) || !Array.isArray(l.files) || l.files.some(f=>typeof f!=='string'||plannedPath(f)!==f) || typeof l.id!=='string' || typeof l.task_id!=='string' || typeof l.worker_id!=='string')) throw Error('文件锁记录损坏，未覆盖。');
      const result = await fn(leases), temporary = file + '.' + randomUUID() + '.tmp';
      try { await fs.writeFile(temporary, JSON.stringify(leases), { mode: 0o600 }); await fs.rename(temporary, file); }
      finally { await fs.unlink(temporary).catch(() => {}); }
      return result;
    } finally { await fs.unlink(path.join(mutex, 'owner.json')).catch(() => {}); await fs.rmdir(mutex); }
  }
  private audit(kind: string, data: unknown) { return fs.appendFile(path.join(this.directory, 'events.jsonl'), JSON.stringify({ time: new Date().toISOString(), kind, data }) + '\n', { mode: 0o600 }); }
  acquire(taskId: string, workerId: string, files: string[]): Promise<FileLease> {
    const names = [...new Set(files.map(f => plannedPath(f.replace(/\/$/, ''))))].sort();
    if (!names.length || !taskId || !workerId) return Promise.reject(Error('文件锁范围或归属无效。'));
    return this.transaction(async leases => {
      await this.audit('LOCK_REQUEST', { taskId, workerId, files: names });
      const conflict = leases.find(l => l.files.some(a => names.some(b => overlap(a, b))));
      if (conflict) { await this.audit('LOCK_DENIED', { taskId, workerId, conflict: conflict.id, files: names }); throw Error('WAITING_FILE_LOCK：存在相同或目录重叠的逻辑文件写锁。'); }
      const lease: FileLease = { id: randomUUID(), project_id: this.projectId, task_id: taskId, worker_id: workerId, files: names, process_ids: [], acquired_at: new Date().toISOString() };
      leases.push(lease); await this.audit('LOCK_GRANTED', lease); return structuredClone(lease);
    });
  }
  attachProcess(id: string, pid: number) {
    if (!Number.isInteger(pid) || pid <= 0) return Promise.reject(Error('写入进程编号无效。'));
    return this.transaction(async leases => { const lease = leases.find(l => l.id === id); if (!lease) throw Error('文件锁已失效。'); if (!lease.process_ids.includes(pid)) lease.process_ids.push(pid); await this.audit('LOCK_PROCESS', { id, pid }); });
  }
  assertHeld(id: string, task: string, worker: string, file: string) {
    const name = plannedPath(file);
    return this.transaction(async leases => { const lease = leases.find(l => l.id === id && l.task_id === task && l.worker_id === worker); if (!lease || !lease.files.some(f => name === f || name.startsWith(f + '/'))) throw Error('缺少本任务/Worker的有效文件写锁。'); });
  }
  release(id: string) {
    return this.transaction(async leases => { const index = leases.findIndex(l => l.id === id); if (index < 0) return; const lease = leases[index]; if (lease.process_ids.some(alive)) { await this.audit('LOCK_RELEASE_REFUSED', { id }); throw Error('写入进程尚未确认退出，不释放文件锁。'); } leases.splice(index, 1); await this.audit('LOCK_RELEASED', { id, reason: '所有已登记写入进程均已退出' }); });
  }
  list() { return this.transaction(async leases => structuredClone(leases)); }
}
