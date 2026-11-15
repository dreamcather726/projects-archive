import { watch } from 'node:fs';
import { FileLockManager, type FileLease } from '../isolation/fileLockManager';

/** Wait for an existing lease; never expires or steals a live writer's lock. */
export async function acquireQueuedLease(locks: FileLockManager, task: string, worker: string, files: string[], signal: AbortSignal, onWait: (waiting: boolean) => Promise<void> = async () => {}): Promise<FileLease> {
  let waiting = false;
  for (;;) {
    if (signal.aborted) throw Error('File lease wait cancelled');
    try {
      const lease = await locks.acquire(task, worker, files);
      try { if (waiting) await onWait(false); } catch (e) { await locks.release(lease.id); throw e; }
      return lease;
    } catch (e) {
      if (!(e as Error).message.includes('WAITING_FILE_LOCK')) throw e;
      if (!waiting) { waiting = true; await onWait(true); }
    }
    await new Promise<void>(resolve => {
      let watcher: ReturnType<typeof watch> | undefined;
      const done = () => { clearTimeout(timer); watcher?.close(); signal.removeEventListener('abort', done); resolve(); };
      const timer = setTimeout(done, 500);
      signal.addEventListener('abort', done, { once: true });
      try { watcher = watch(locks.watchDirectory(), (_event, file) => { if (file?.toString() === 'leases.json') done(); }); watcher.on('error', () => watcher?.close()); } catch { /* bounded retry if filesystem watching is unavailable */ }
      if (signal.aborted) done();
    });
  }
}
