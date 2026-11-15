import { launch, buildArgs, type Launcher, type CodexRunOptions } from '../process/cliLauncher';
import { terminateTree } from '../process/processManager';
import { CodexEventParser } from '../parsers/codexEventParser';
import { CodexError, classifyError } from './codexErrors';
import type { CodexEvent } from '../chat/chatTypes';
import type { ResolvedCodex } from '../process/codexExecutableResolver';

export interface RunningRequest { done: Promise<void>; stop: () => Promise<void> }
export class CodexService {
  private resolved?: ResolvedCodex;
  get executableInfo(): ResolvedCodex | undefined {return this.resolved;}
  constructor(private executable = 'codex', private launcher: Launcher = launch,
    private resolveExecutable?: (configured: string) => Promise<ResolvedCodex>) {}
  private capture(args: string[], cwd: string, signal?: AbortSignal): Promise<string> {
    return new Promise((resolve,reject) => {
      if(signal?.aborted) {reject(new CodexError('REQUEST_CANCELLED'));return;}
      let child;
      try { child = this.launcher(this.resolved?.command || this.executable,args,cwd); } catch(error) { reject(new CodexError('PROCESS_START_FAILED',error)); return; }
      let text = ''; let settled = false;
      const finish = (error?: Error) => { if (settled) return; settled = true; clearTimeout(timer); signal?.removeEventListener('abort',abort);error ? reject(error) : resolve(text); };
      const abort = () => {void terminateTree(child).then(() => finish(new CodexError('REQUEST_CANCELLED')),() => finish(new CodexError('PROCESS_EXIT_NONZERO')));};
      const timer = setTimeout(() => { void terminateTree(child).then(() => finish(new CodexError('REQUEST_TIMEOUT')),() => finish(new CodexError('PROCESS_EXIT_NONZERO'))); },10000);
      signal?.addEventListener('abort',abort,{once:true});
      child.stdout?.on('data',chunk => { text = (text + chunk.toString()).slice(0,128000); });
      child.stderr?.on('data',() => {});
      child.on('error',(e: NodeJS.ErrnoException) => finish(new CodexError(e.code === 'ENOENT' ? 'CODEX_NOT_FOUND' : 'PROCESS_START_FAILED',e)));
      child.on('close',code => finish(signal?.aborted ? new CodexError('REQUEST_CANCELLED') : code === 0 ? undefined : new CodexError('CODEX_VERSION_UNSUPPORTED')));
      child.stdin?.on('error',() => {}); child.stdin?.end();
    });
  }
  async check(cwd: string, signal?: AbortSignal): Promise<string> {
    this.resolved = undefined;
    if(this.resolveExecutable) this.resolved = await this.resolveExecutable(this.executable);
    const version = await this.capture(['--version'],cwd,signal);
    const help = await this.capture(['exec','--help'],cwd,signal);
    const resume = await this.capture(['exec','--json','--sandbox','read-only','--cd',cwd,'resume','--help'],cwd,signal);
    if (!version.includes('codex') || !['--json','--sandbox','read-only','--cd','--config'].every(flag => help.includes(flag)) || !resume.includes('SESSION_ID') || !resume.includes('stdin')) throw new CodexError('CODEX_VERSION_UNSUPPORTED');
    return version.trim().slice(0,200);
  }
  run(cwd: string, prompt: string, threadId: string | undefined, onEvent: (event: CodexEvent) => void, timeout: number, options?: CodexRunOptions): RunningRequest {
    let child;
    try { child = this.launcher(this.resolved?.command || this.executable,buildArgs(cwd,threadId,options),cwd,options?.env); }
    catch { return {done:Promise.reject(new CodexError('PROCESS_START_FAILED')),stop:async () => {}}; }
    let stderr = ''; let cancelled = false; let timedOut = false; let completed = false;
    let eventError = ''; let streamFailed = false; let termination: Promise<void> | undefined;
    let settled = false; let settle!: (error?: Error) => void;
    const done = new Promise<void>((resolve,reject) => { settle = error => {if (settled) return; settled = true; clearTimeout(timer); error ? reject(error) : resolve();}; });
    const terminate = () => termination ||= terminateTree(child);
    const timer = setTimeout(() => {timedOut = true; void terminate().then(() => settle(new CodexError('REQUEST_TIMEOUT')),() => settle(new CodexError('PROCESS_EXIT_NONZERO')));},timeout);
    const events = new CodexEventParser(event => {
      if (event.type === 'turn' && event.state === 'completed') completed = true;
      if (event.type === 'turn' && event.state === 'failed') {streamFailed = true;eventError = event.error || '';}
      if (event.type === 'error') eventError = event.message;
      onEvent(event);
    });
    child.stdout?.on('data',chunk => events.push(chunk));
    child.stderr?.on('data',chunk => {stderr = (stderr + chunk.toString()).slice(-32768);});
    child.stdin?.on('error',() => {});
    child.on('error',(e: NodeJS.ErrnoException) => settle(new CodexError(e.code === 'ENOENT' ? 'CODEX_NOT_FOUND' : 'PROCESS_START_FAILED')));
    child.on('close',async code => {
      events.end();
      if (termination) {try {await termination;} catch {settle(new CodexError('PROCESS_EXIT_NONZERO'));return;}}
      if (cancelled || timedOut) settle(new CodexError(timedOut ? 'REQUEST_TIMEOUT' : 'REQUEST_CANCELLED'));
      else if (code !== 0 || streamFailed || !completed) settle(classifyError(`${stderr}\n${eventError}`,!!threadId));
      else settle();
      stderr = '';eventError = '';
    });
    // Prompt is never an argv element, even for .cmd launchers on Windows.
    child.stdin?.end(prompt,'utf8');
    return {done,stop:async () => {if (settled) return;cancelled = true;await terminate();settle(new CodexError('REQUEST_CANCELLED'));await done.catch(() => {});}};
  }
}
