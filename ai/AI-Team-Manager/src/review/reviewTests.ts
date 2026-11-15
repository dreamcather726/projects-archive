import { randomUUID } from "node:crypto";
import type { Launcher } from "../process/cliLauncher";
import crossSpawn from 'cross-spawn';
import { terminateTree } from "../process/processManager";
import { safeReviewText } from "./reviewSafety";
import { ReviewGit } from "./reviewGit";
import type { TestEvidence } from "./reviewTypes";
export type TestCommand = "npm test" | "node --test" | "npm run compile" | `python -B ${string}.py`;
const launchTest:Launcher=(command,args,cwd)=>{
  const env={...process.env};delete env.NODE_TEST_CONTEXT;
  return crossSpawn(command,args,{cwd,env,windowsHide:true,detached:process.platform!=='win32',stdio:['pipe','pipe','pipe']});
};
export class ReviewTestRunner {
  private stopCurrent?: () => Promise<void>;
  constructor(
    private collector: ReviewGit,
    private launcher: Launcher = launchTest,
  ) {}
  async run(root: string, command: TestCommand): Promise<TestEvidence> {
    const python=/^python -B ([a-zA-Z0-9_][a-zA-Z0-9_./-]*\.py)$/.exec(command);
    if(python&&python[1].split('/').some(p=>p==='..'||p==='.'))throw new Error('Python测试路径无效。');
    if (!["npm test", "node --test", "npm run compile"].includes(command) && !python)
      throw new Error("只支持用户明确选择的 npm test 或 node --test。");
    if (this.stopCurrent) throw new Error("已有审查测试正在执行。");
    const before = await this.collector.snapshot(root),
      started_at = new Date().toISOString();
    const child = this.launcher(
      python ? 'python' : command === "node --test" ? "node" : "npm",
      python ? ['-B',python[1]] : command === "node --test" ? ["--test"] : command === 'npm run compile' ? ['run','compile'] : ['test'],
      root,
    );
    let output = "",
      tail = "",
      truncated = false,
      cancelled = false,
      timedOut = false,
      termination: Promise<void> | undefined;
    const terminate = () => (termination ||= terminateTree(child));
    this.stopCurrent = async () => {
      cancelled = true;
      await terminate();
    };
    const timer = setTimeout(() => {
      timedOut = true;
      void terminate().catch(() => {});
    }, 120000);
    const record = (chunk: Buffer) => {
      const text = chunk.toString("utf8");
      tail = (tail + text).slice(-16000);
      output += text;
      if (output.length > 48000) {
        truncated = true;
        output = output.slice(0, 48000);
      }
    };
    child.stdout?.on("data", record);
    child.stderr?.on("data", record);
    child.stdin?.end();
    let exit: number | null = null;
    try {
      exit = await new Promise<number | null>((resolve, reject) => {
        child.on("error", () =>
          reject(new Error("测试进程启动失败，请检查 Node/npm 和权限。")),
        );
        child.on("close", resolve);
      });
      if (termination) await termination;
    } finally {
      clearTimeout(timer);
      this.stopCurrent = undefined;
    }
    const after = await this.collector.snapshot(root);
    const scan = (output + "\n" + tail).replace(/\x1b\[[0-9;]*m/g, "");
    const observed =
      scan.match(/(?:^|\n)(?:#|ℹ)\s*tests\s+(\d+)/m) ||
      scan.match(/Tests:\s*(\d+)\s+passed/i) ||
      scan.match(/(?:^|\n)\s*(\d+)\s+passing\b/i) ||
      scan.match(/\bTests\s+(\d+)\s+passed/i) || scan.match(/\bRan (\d+) tests? in /);
    const test_count = observed ? Number(observed[1]) : undefined;
    return {
      id: randomUUID(),
      command,
      started_at,
      finished_at: new Date().toISOString(),
      exit_code: exit,
      status: cancelled
        ? "CANCELLED"
        : timedOut
          ? "TIMEOUT"
          : before.digest !== after.digest
            ? "STALE"
            : exit === 0
              ? command === 'npm run compile' || test_count && test_count > 0
                ? "PASSED"
                : "UNVERIFIED"
              : "FAILED",
      test_count,
      output: safeReviewText(
        truncated
          ? output.slice(0, 36000) +
              "\n[输出已截取，保留结尾]\n" +
              tail.slice(-12000)
          : output,
      ),
      output_truncated: truncated,
      source_digest: before.digest,
    };
  }
  async stop(): Promise<void> {
    await this.stopCurrent?.();
  }
}
