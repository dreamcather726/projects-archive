import { launch, type Launcher } from "../process/cliLauncher";
import { terminateTree } from "../process/processManager";
import { ClaudeEventParser, validClaudeSession } from "./claudeEventParser";
import { ClaudeError, classifyClaudeError } from "./claudeErrors";
import type { ClaudeEvent } from "./claudeTypes";
import type { ResolvedClaude } from "./claudeExecutableResolver";
export interface ClaudeRunningRequest {
  done: Promise<void>;
  stop: () => Promise<void>;
  /** True only after confirmed child close and completed process/permission lifecycle. */
  replaySafe?: () => boolean;
}
export interface ClaudeRunOptions { workerLabel?: string; restricted?: boolean; boundedMcp?: boolean; allowedMcpTools?: string[]; mcpConfig?: string; env?: NodeJS.ProcessEnv; onProcess?: (pid: number) => Promise<void>; onProcessExit?: (pid: number) => Promise<void> }
export function claudeArgs(sessionId?: string, permissionSettings?: string, options?: ClaudeRunOptions): string[] {
  if (options?.boundedMcp && (!options.mcpConfig || !options.allowedMcpTools?.length || options.allowedMcpTools.some(t=>!/^mcp__atm_worker__[a-z_]+$/.test(t)))) throw new ClaudeError('PERMISSION_DENIED');
  if (options?.boundedMcp && options.restricted) throw new ClaudeError('PERMISSION_DENIED');
  if (options?.workerLabel && !/^[A-Za-z0-9_-]{1,80}$/.test(options.workerLabel)) throw new Error('Worker标识无效。');
  if (sessionId && !validClaudeSession(sessionId))
    throw new ClaudeError("SESSION_RESUME_FAILED");
  const args = [
    "--print",
    "--verbose",
    "--output-format",
    "stream-json",
    "--include-partial-messages",
    "--permission-mode",
    "manual",
    "--permission-prompts",
    "none",
    "--tools",
    options?.boundedMcp ? '' : permissionSettings ? options?.restricted ? 'Read,Edit,Write,Glob,Grep' : "Read,Edit,Write,Glob,Grep,Bash" : 'Read,Glob,Grep',
    "--strict-mcp-config",
    "--mcp-config",
    options?.mcpConfig || '{"mcpServers":{}}',
    "--append-system-prompt",
    `你是 AI Team Manager 的 Claude ${options?.workerLabel || 'Worker 01'}。仅执行当前用户明确授权的任务。不得读取密钥、认证文件或 CC Switch 配置；不得修改这些配置。不启动其他 Agent，不自行提交或推送 Git，不扩大工作目录或绕过权限。遇到权限拒绝请明确报告，不换用其他工具绕过。`,
  ];
  if (options?.restricted) args.push('--restricted', '--disable-slash-commands', '--no-chrome');
  if (options?.boundedMcp) args.push('--bare','--disable-slash-commands','--no-chrome','--allowedTools',options.allowedMcpTools!.join(','));
  if (permissionSettings) args.push('--settings', permissionSettings);
  return sessionId ? [...args, "--resume", sessionId] : args;
}
export class ClaudeService {
  private resolved?: ResolvedClaude;
  private restrictedSupported = false;
  private bareSupported = false;
  get executableInfo() {
    return this.resolved;
  }
  constructor(
    private executable = "claude",
    private launcher: Launcher = launch,
    private resolver?: (configured: string) => Promise<ResolvedClaude>,
  ) {}
  private capture(
    args: string[],
    cwd: string,
    signal?: AbortSignal,
  ): Promise<string> {
    return new Promise((resolve, reject) => {
      if (signal?.aborted) {
        reject(new ClaudeError("REQUEST_CANCELLED"));
        return;
      }
      let child;
      try {
        child = this.launcher(
          this.resolved?.command || this.executable,
          args,
          cwd,
        );
      } catch {
        reject(new ClaudeError("PROCESS_START_FAILED"));
        return;
      }
      let output = "",
        stderr = "",
        settled = false;
      const finish = (error?: ClaudeError) => {
        if (settled) return;
        settled = true;
        clearTimeout(timer);
        signal?.removeEventListener("abort", abort);
        error ? reject(error) : resolve(output);
      };
      const abort = () => {
        void terminateTree(child).then(
          () => finish(new ClaudeError("REQUEST_CANCELLED")),
          () => finish(new ClaudeError("PROCESS_EXIT_NONZERO")),
        );
      };
      const timer = setTimeout(() => {
        void terminateTree(child).then(
          () => finish(new ClaudeError("REQUEST_TIMEOUT")),
          () => finish(new ClaudeError("PROCESS_EXIT_NONZERO")),
        );
      }, 10000);
      signal?.addEventListener("abort", abort, { once: true });
      child.stdout?.on("data", (b) => {
        output = (output + b.toString()).slice(0, 128000);
      });
      child.stderr?.on("data", (b) => {
        stderr = (stderr + b.toString()).slice(-16000);
      });
      child.on("error", (e: NodeJS.ErrnoException) =>
        finish(
          new ClaudeError(
            e.code === "ENOENT"
              ? "CLAUDE_NOT_FOUND"
              : e.code === "EACCES" || e.code === "EPERM"
                ? "PERMISSION_DENIED"
                : "PROCESS_START_FAILED",
          ),
        ),
      );
      child.on("close", (code) =>
        finish(
          signal?.aborted
            ? new ClaudeError("REQUEST_CANCELLED")
            : code === 0
              ? undefined
              : classifyClaudeError(stderr),
        ),
      );
      child.stdin?.on("error", () => {});
      child.stdin?.end();
    });
  }
  async check(cwd: string, signal?: AbortSignal): Promise<string> {
    this.resolved = this.resolver
      ? await this.resolver(this.executable)
      : { command: this.executable, source: "settings" };
    const version = await this.capture(["--version"], cwd, signal),
      help = await this.capture(["--help"], cwd, signal);
    const semver = version.match(/^(\d+)\.(\d+)\.(\d+)/);
    const supportedHooks = !!semver && (Number(semver[1]) > 2 || Number(semver[1]) === 2 && (Number(semver[2]) > 1 || Number(semver[2]) === 1 && Number(semver[3]) >= 295));
    if (
      !supportedHooks ||
      !/Claude Code/i.test(version) ||
      ![
        "--print",
        "--verbose",
        "--output-format",
        "--include-partial-messages",
        "--permission-mode",
        "manual",
        "--permission-prompts",
        "--resume",
        "--strict-mcp-config",
        '--settings',
      ].every((flag) => help.includes(flag))
    )
      throw new ClaudeError("CLAUDE_VERSION_UNSUPPORTED");
    this.resolved.version = version.trim().slice(0, 200);
    this.restrictedSupported = ['--restricted','--disable-slash-commands','--no-chrome'].every(flag => help.includes(flag));
    this.bareSupported = ['--bare','--disable-slash-commands','--no-chrome','--allowedTools'].every(flag => help.includes(flag));
    return this.resolved.version;
  }
  run(
    cwd: string,
    prompt: string,
    sessionId: string | undefined,
    onEvent: (e: ClaudeEvent) => void,
    timeout: number,
    permissionSettings?: string,
    options?: ClaudeRunOptions,
  ): ClaudeRunningRequest {
    let child;
    try {
      if (options?.restricted && !this.restrictedSupported) throw new ClaudeError('CLAUDE_VERSION_UNSUPPORTED');
      if (options?.boundedMcp && !this.bareSupported) throw new ClaudeError('CLAUDE_VERSION_UNSUPPORTED');
      child = this.launcher(
        this.resolved?.command || this.executable,
        claudeArgs(sessionId, permissionSettings, options),
        cwd,
        options?.env,
      );
    } catch (error) {
      return {
        done: Promise.reject(error instanceof ClaudeError ? error : new ClaudeError("PROCESS_START_FAILED")),
        stop: async () => {},
      };
    }
    let stderr = "",
      cancelled = false,
      timedOut = false,
      settled = false,
      resultSeen = false,
      success = false,
      parseFailed = false,
      denied = false,
      errorText = "",
      actualSession: string | undefined,
      termination: Promise<void> | undefined;
    let closed = false, registrationFailed = false;
    let settle!: (error?: ClaudeError) => void;
    const done = new Promise<void>((resolve, reject) => {
      settle = (error) => {
        if (settled) return;
        settled = true;
        clearTimeout(timer);
        error ? reject(error) : resolve();
      };
    });
    const terminate = () => (termination ||= terminateTree(child));
    // Do not send the prompt until the current PID owns the existing permission boundary.
    const registration = Promise.resolve().then(async () => {
      if (options?.onProcess) {
        if (!child.pid) throw Error('Missing child PID');
        await options.onProcess(child.pid);
      }
      if (!cancelled && !timedOut && !settled) child.stdin?.end(prompt,'utf8');
    }).catch(async () => {
      registrationFailed = true;
      try { await terminate(); settle(new ClaudeError('PERMISSION_DENIED')); }
      catch { settle(new ClaudeError('PROCESS_EXIT_NONZERO')); }
    });
    const timer = setTimeout(() => {
      timedOut = true;
      void terminate().then(() => settle(new ClaudeError('REQUEST_TIMEOUT')), () => settle(new ClaudeError('PROCESS_EXIT_NONZERO')));
    }, timeout);
    const parser = new ClaudeEventParser((event) => {
      if (settled) return; // Late output after stop/timeout must not revive a completed request.
      if (event.type === "session") {
        actualSession = event.id;
        if (sessionId && sessionId !== event.id) {
          errorText = "Session resume failed: ID changed";
          void terminate().catch(() => {});
        }
      }
      if (event.type === "parseError") parseFailed = true;
      if (event.type === "result") {
        resultSeen = true;
        success = event.ok;
        denied ||= event.denied;
        if(event.error)errorText=(errorText+'\n'+event.error).slice(-32768);
      }
      const visible = event.type === 'result' && event.error ? {...event,error:classifyClaudeError(event.error).message} : event;
      try { onEvent(visible); }
      catch { parseFailed=true; void terminate().catch(()=>{}); }
    });
    child.stdout?.on("data", (chunk) => parser.push(chunk));
    child.stderr?.on("data", (chunk) => {
      stderr = (stderr + chunk.toString()).slice(-32768);
    });
    child.stdin?.on("error", () => {});
    child.on("error", (e: NodeJS.ErrnoException) =>
      settle(
        new ClaudeError(
          e.code === "ENOENT"
            ? "CLAUDE_NOT_FOUND"
            : e.code === "EACCES" || e.code === "EPERM"
              ? "PERMISSION_DENIED"
              : "PROCESS_START_FAILED",
        ),
      ),
    );
    child.on("close", async (code) => {
      parser.end();
      await registration;
      try { if (child.pid) await options?.onProcessExit?.(child.pid); closed = true; }
      catch { registrationFailed = true; }
      if (termination) {
        try {
          await termination;
        } catch {
          settle(new ClaudeError("PROCESS_EXIT_NONZERO"));
          return;
        }
      }
      if (cancelled || timedOut)
        settle(
          new ClaudeError(timedOut ? "REQUEST_TIMEOUT" : "REQUEST_CANCELLED"),
        );
      else if (registrationFailed) settle(new ClaudeError('PERMISSION_DENIED'));
      else if (sessionId && actualSession && actualSession !== sessionId)
        settle(new ClaudeError("SESSION_RESUME_FAILED"));
      else if (parseFailed) settle(new ClaudeError("OUTPUT_PARSE_FAILED"));
      else if (denied) settle(new ClaudeError("PERMISSION_DENIED"));
      else if (code !== 0 || !resultSeen || !success || !actualSession)
        settle(
          classifyClaudeError(
            `${stderr}\n${errorText}`,
            !!sessionId && !actualSession,
          ),
        );
      else settle();
      stderr = "";
      errorText = "";
    });
    return {
      done,
      replaySafe: () => !!child.pid && closed && !registrationFailed && !parseFailed && !denied && !cancelled && !timedOut,
      stop: async () => {
        if (settled) return;
        cancelled = true;
        await terminate();
        settle(new ClaudeError('REQUEST_CANCELLED'));
        await done.catch(() => {});
      },
    };
  }
}
