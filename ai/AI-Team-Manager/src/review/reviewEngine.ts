import * as fs from "node:fs/promises";
import * as path from "node:path";
import { randomUUID } from "node:crypto";
import { CodexService, type RunningRequest } from "../services/codexService";
import { git, ReviewGit } from "./reviewGit";
import { ReviewStorage } from "./reviewStorage";
import { makeBatches, reviewPrompt } from "./reviewPrompt";
import { overallVerdict, validateBatch } from "./reviewValidation";
import { safeReviewText } from "./reviewSafety";
import {
  categories,
  type ReviewEvidence,
  type ReviewReport,
  type BatchResult,
} from "./reviewTypes";
import type { Usage } from '../chat/chatTypes';
export interface ReviewUsageHooks {
  begin(id:string):Promise<void>;
  finish(id:string,status:'COMPLETED'|'FAILED'|'CANCELLED',usage?:Usage):Promise<void>;
}
export class ReviewEngine {
  private request?: RunningRequest;
  private abort?: AbortController;
  private cancelled = false;
  busy = false;
  progress = "";
  constructor(
    private service: CodexService,
    private collector: ReviewGit,
    private storage: ReviewStorage,
    private changed: () => void = () => {},
    private timeout = 240000,
    private usageHooks?: ReviewUsageHooks,
  ) {}
  async stop(): Promise<void> {
    this.cancelled = true;
    this.abort?.abort();
    await this.request?.stop();
  }
  async run(evidence: ReviewEvidence): Promise<ReviewReport> {
    if (this.busy) throw new Error("已有独立审查正在运行。");
    this.busy = true;
    this.cancelled = false;
    this.abort = new AbortController();
    const created_at = new Date().toISOString(),
      review_id = randomUUID();
    const results: BatchResult[] = [],
      gaps = [...evidence.gaps];
    let stale = false;
    const bundle = path.join(this.storage.directory, "bundles", review_id);
    let version = "";
    try {
      await fs.mkdir(bundle, { recursive: true });
      await git(bundle, ["init", "--quiet"]);
      await fs.writeFile(
        path.join(bundle, "README.md"),
        "Isolated read-only review bundle. All evidence is provided via stdin. Do not read outside this directory or execute tools.\n",
        { mode: 0o600 },
      );
      const batches = makeBatches(evidence,evidence.workflow_context?120000:22000,!!evidence.workflow_context);
      if (batches.length > 100)
        throw new Error(
          "审查超过 100 批次限制，请缩小任务范围；未截取后判为通过。",
        );
      if (!batches.length) gaps.push("没有代码批次可提交 GPT");
      if (batches.length) {
        this.progress = "检测现有 Codex CLI";
        this.changed();
        version = await this.service.check(bundle, this.abort.signal);
      }
      for (const batch of batches) {
        if (this.cancelled) throw new Error("审查已停止；未判为通过。");
        this.progress = `审查 ${batch.id}/${batches.length} · 六个维度`;
        this.changed();
        const messages = new Map<string, string>();
        let session_id: string | undefined,
          usage: any,
          invalid = false,
          toolUsed = false;
        const prompt = reviewPrompt(evidence, batch, batches.length);
        if (prompt.length > 180000)
          throw new Error("单批审查证据超过 180000 字符限制；未静默丢弃证据。");
        await fs.writeFile(path.join(bundle, `${batch.id}-input.txt`), prompt, {
          mode: 0o600,
        });
        const attempt: BatchResult = {
          batch_id: batch.id,
          verdict: "BLOCKED",
          summary: "审查尝试尚未完成",
          dimensions: categories.map((category) => ({
            category,
            status: "INSUFFICIENT_EVIDENCE",
            summary: "未获得完整结构化结果",
          })),
          issues: [],
          missing_evidence: [],
        };
        results.push(attempt);
        const requestId=`${review_id}:${batch.id}`;
        let registered=false,completed=false;
        try {
        if(this.usageHooks){await this.usageHooks.begin(requestId);registered=true;}
        if(this.cancelled)throw Error('审查已停止；未启动新的GPT请求。');
        this.request = this.service.run(
          bundle,
          prompt,
          undefined,
          (event) => {
            if (event.type === "message" && event.completed)
              messages.set(event.id, event.text);
            if (event.type === "thread") {
              session_id = event.id;
              attempt.session_id = event.id;
            }
            if (event.type === "usage") {
              usage = event.usage;
              attempt.usage = event.usage;
            }
            if (event.type === "parseError") invalid = true;
            if (event.type === "activity") toolUsed = true;
          },
          this.timeout,
        );
        await this.request.done;
        completed=true;
        } finally {
          if(registered)await this.usageHooks!.finish(requestId,this.cancelled?'CANCELLED':completed?'COMPLETED':'FAILED',usage);
        }
        this.request = undefined;
        if (invalid || toolUsed)
          throw new Error(
            "审查输出不完整，或模型执行了未要求的工具操作；需要人工核对。",
          );
        const text = [...messages.values()].at(-1) || "";
        await fs.writeFile(
          path.join(bundle, `${batch.id}-output.json`),
          safeReviewText(text),
          { mode: 0o600 },
        );
        const result = validateBatch(text, evidence, batch);
        results[results.length - 1] = { ...result, session_id, usage };
      }
      const current = await this.collector.snapshot(evidence.after.root);
      stale = current.digest !== evidence.after.digest;
      if (stale) gaps.push("审查期间代码已改变，报告不适用于当前快照");
      if (evidence.tests.some((t) => t.status === "FAILED")) {
        if (results.every((r) => r.verdict === "PASS"))
          gaps.push("实际测试失败而模型仍判通过，结论冲突，需要复核");
      }
    } catch (error) {
      const reason =
        error instanceof Error &&
        /审查|GPT|代码|批次|结果|模型|Git/.test(error.message)
          ? safeReviewText(error.message)
          : "审查进程、证据校验或本地存储失败；未判为通过。";
      gaps.push(reason);
      if (results.at(-1)?.summary === "审查尝试尚未完成") {
        results.at(-1)!.error = reason;
        results.at(-1)!.missing_evidence.push(reason);
      }
    } finally {
      this.request = undefined;
      this.abort = undefined;
      this.progress = "审查结束";
    }
    const issues = results
      .flatMap((b) => b.issues)
      .filter(
        (issue, index, all) =>
          all.findIndex(
            (i) =>
              i.file_path === issue.file_path &&
              i.line_number === issue.line_number &&
              i.category === issue.category &&
              i.problem_description === issue.problem_description,
          ) === index,
      );
    const missing = [
      ...new Set([...gaps, ...results.flatMap((b) => b.missing_evidence)]),
    ];
    const verdict = overallVerdict(
      results,
      missing,
      evidence.baseline === "GIT_HEAD" || results.length > 1,
    );
    const report: ReviewReport = {
      version: 1,
      review_id,
      project_path: evidence.after.root,
      created_at,
      finished_at: new Date().toISOString(),
      task: evidence.task,
      verdict,
      summary: `${verdict} · ${issues.length} 个问题 · ${results.length} 个审查批次`,
      issues,
      dimensions: categories.map((category) => {
        const items = results.map((b) =>
          b.dimensions.find((d) => d.category === category)!,
        );
        return {
          category,
          status: missing.length
            ? "INSUFFICIENT_EVIDENCE"
            : items.some((d) => d.status === "ISSUES")
              ? "ISSUES"
              : items.some((d) => d.status === "INSUFFICIENT_EVIDENCE")
                ? "INSUFFICIENT_EVIDENCE"
                : items.some((d) => d.status === "NEEDS_HUMAN_REVIEW")
                  ? "NEEDS_HUMAN_REVIEW"
                  : "PASS",
          summary: items.map((d) => d.summary).join("\n") || "未完成模型审查",
        };
      }),
      missing_evidence: missing,
      warnings: [
        ...evidence.warnings,
        `Codex CLI：${version || "检查未完成"}`,
        ...(results.length > 1
          ? [
              "分批审查覆盖全部已采集片段；跨批接口与整体架构仍需人工复核，不自动 PASS。",
            ]
          : []),
      ],
      batches: results,
      evidence,
      stale,
    };
    try {
      await this.storage.saveReport(report);
      return report;
    } finally {
      this.busy = false;
      this.changed();
    }
  }
}
