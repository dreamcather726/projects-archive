import * as vscode from "vscode";
import * as fs from "node:fs/promises";
import * as path from "node:path";
import { randomUUID } from "node:crypto";
import { CodexService } from "../services/codexService";
import { resolveCodexExecutable } from "../process/codexExecutableResolver";
import { ReviewGit } from "./reviewGit";
import { ReviewEngine } from "./reviewEngine";
import { ReviewStorage } from "./reviewStorage";
import { ReviewTestRunner, type TestCommand } from "./reviewTests";
import { safeReviewText, relativeFile, excludedPath } from "./reviewSafety";
import type {
  ReviewReport,
  ReviewTask,
  TaskCapture,
  ClaudeDelivery,
} from "./reviewTypes";
export class ReviewCoordinator {
  private storage: ReviewStorage;
  private collector: ReviewGit;
  private runner: ReviewTestRunner;
  private engine: ReviewEngine;
  private report?: ReviewReport;
  private history: ReviewReport[] = [];
  private error?: string;
  private captureBusy = false;
  private testsBusy = false;
  private preparing = false;
  private preparationCancelled = false;
  constructor(
    private context: vscode.ExtensionContext,
    private claude: () => any,
    private changed: () => void,
    private log: (message: string, level?: string, source?: string) => void,
    private externalBusy: () => boolean = () => false,
  ) {
    this.storage = new ReviewStorage(
      path.join(context.globalStorageUri.fsPath, "reviews"),
    );
    this.collector = new ReviewGit(
      path.join(this.storage.directory, "temporary"),
    );
    this.runner = new ReviewTestRunner(this.collector);
    const service = new CodexService(
      vscode.workspace
        .getConfiguration("aiTeamManager")
        .get<string>("codexExecutable", "codex"),
      undefined,
      (configured) =>
        resolveCodexExecutable(configured, {
          extensionDirectory: () =>
            vscode.extensions.getExtension("openai.chatgpt")?.extensionUri
              .fsPath,
        }),
    );
    this.engine = new ReviewEngine(
      service,
      this.collector,
      this.storage,
      changed,
    );
  }
  snapshot() {
    return {
      busy:
        this.engine.busy ||
        this.testsBusy ||
        this.captureBusy ||
        this.preparing,
      canStop: this.engine.busy || this.testsBusy || this.preparing,
      progress: this.testsBusy
        ? "正在执行用户选择的测试"
        : this.engine.progress,
      error: this.error,
      report: this.report ? { ...this.report, evidence: undefined } : undefined,
      history: this.history.map((r) => ({
        review_id: r.review_id,
        created_at: r.created_at,
        verdict: r.verdict,
        task: r.task.task_description.slice(0, 100),
        issues: r.issues.length,
        project_path: r.project_path,
      })),
    };
  }
  private async root(): Promise<string> {
    if (!vscode.workspace.isTrusted)
      throw new Error("请先信任工作区后执行审查。");
    const current = this.claude(),
      candidate =
        current.projectPath ||
        vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
    if (!candidate) throw new Error("请先选择本地项目文件夹。");
    const root = await fs.realpath(candidate),
      folders = vscode.workspace.workspaceFolders || [];
    if (
      !folders.some(
        (f) =>
          f.uri.scheme === "file" &&
          path.resolve(f.uri.fsPath).toLowerCase() === root.toLowerCase(),
      )
    )
      throw new Error("所选审查项目不在当前工作区。");
    return root;
  }
  private delivery(state: any): ClaudeDelivery {
    const c = state.conversation,
      r = c?.requests?.at(-1);
    return {
      conversation_id: c?.id,
      session_id: c?.threadId,
      request_id: r?.id,
      status: r?.status || c?.status || "UNKNOWN",
      final_answer: safeReviewText(
        (c?.messages || [])
          .filter((m: any) => m.role === "assistant" && m.requestId === r?.id)
          .map((m: any) => m.text)
          .join("\n"),
      ).slice(0, 16000),
    };
  }
  async withClaudeTask(
    text: string,
    execute: () => Promise<void>,
  ): Promise<void> {
    if (
      this.externalBusy() || this.captureBusy ||
      this.engine.busy ||
      this.testsBusy ||
      this.preparing
    )
      throw new Error("请先完成当前审查或测试，再发送 Claude 开发任务。");
    if (this.claude().busy) {
      await execute();
      return;
    }
    this.captureBusy = true;
    this.error = undefined;
    this.changed();
    let capture: TaskCapture | undefined;
    try {
      const root = await this.root(),
        previous = await this.storage.capture(root),
        state = this.claude();
      const same =
        previous?.delivery?.conversation_id === state.conversation?.id;
      capture =
        same && previous
          ? {
              ...previous,
              task: {
                ...previous.task,
                follow_up_instructions: [
                  ...(previous.task.follow_up_instructions || []),
                  safeReviewText(text),
                ],
              },
            }
          : {
              id: randomUUID(),
              task: {
                task_id: randomUUID(),
                task_description: safeReviewText(text),
                technical_requirements: "",
              },
              before: await this.collector.snapshot(root),
            };
      await this.storage.saveCapture(capture);
    } catch {
      this.error =
        "未能保存 Claude 执行前 Git 证据；本轮开发仍可运行，后续审查不能声称有完整归因基线。";
      this.log(this.error, "WARN", "System");
    }
    try {
      await execute();
      if (capture) {
        capture.delivery = this.delivery(this.claude());
        await this.storage.saveCapture(capture);
        this.log(
          "已保存 Claude 原始任务、执行前快照及最终状态；未自动启动审查。",
          "INFO",
          "System",
        );
      }
    } finally {
      this.captureBusy = false;
      this.changed();
    }
  }
  async load(): Promise<void> {
    try {
      const root = await this.root();
      this.history = await this.storage.reports(root);
      this.report = this.history[0];
    } catch {
      /* No project at activation is a normal empty state. */
    }
    this.changed();
  }
  async runTests(command?: TestCommand): Promise<void> {
    if (
      this.externalBusy() ||
      this.engine.busy ||
      this.testsBusy ||
      this.captureBusy ||
      this.preparing ||
      this.claude().busy
    )
      throw new Error("请先等待 Claude / 审查 / 测试结束。");
    const choice =
      command ||
      (await vscode.window.showQuickPick(["npm test", "node --test"], {
        placeHolder:
          "在所选项目实际执行测试（会运行项目脚本）；只在你选择后执行",
      }));
    if (!choice) return;
    const root = await this.root();
    this.testsBusy = true;
    this.error = undefined;
    this.changed();
    try {
      const result = await this.runner.run(root, choice as TestCommand),
        previous = (await this.storage.tests(root)) || [];
      await this.storage.write(
        root,
        "tests.json",
        [...previous, result].slice(-30),
      );
      this.log(
        `审查测试 ${result.status} · exit=${result.exit_code}`,
        result.status === "PASSED" ? "INFO" : "WARN",
        "Test",
      );
    } finally {
      this.testsBusy = false;
      this.changed();
    }
  }
  async start(options?: {
    task?: ReviewTask;
  }): Promise<ReviewReport | undefined> {
    if (
      this.externalBusy() ||
      this.engine.busy ||
      this.testsBusy ||
      this.captureBusy ||
      this.preparing ||
      this.claude().busy
    )
      throw new Error("请先等待 Claude / 审查 / 测试结束。");
    this.preparing = true;
    this.preparationCancelled = false;
    this.changed();
    try {
      return await this.prepareReview(options);
    } catch (error) {
      const root = await this.root(),
        reason =
          error instanceof Error
            ? safeReviewText(error.message)
            : "审查准备失败";
      const now = new Date().toISOString(),
        source = {
          root,
          time: now,
          head: "UNAVAILABLE",
          digest: "UNAVAILABLE",
          files: [],
          gaps: [reason],
        };
      const task = options?.task || {
        task_id: randomUUID(),
        task_description: "",
        technical_requirements: "",
      };
      const report = await this.engine.run({
        id: randomUUID(),
        task,
        before: source,
        after: source,
        baseline: "GIT_HEAD",
        delivery: this.delivery(this.claude()),
        changes: [],
        context: [],
        tests: [],
        gaps: [reason, "审查准备不完整，不能确认代码、原始需求及测试证据"],
        warnings: [],
      });
      this.report = report;
      this.history = await this.storage.reports(root);
      return report;
    } finally {
      this.preparing = false;
      this.changed();
    }
  }
  private async prepareReview(options?: {
    task?: ReviewTask;
  }): Promise<ReviewReport | undefined> {
    const root = await this.root(),
      after = await this.collector.snapshot(root),
      capture = await this.storage.capture(root);
    const active = this.claude(),
      matches =
        capture?.delivery?.conversation_id === active.conversation?.id &&
        capture?.delivery?.request_id ===
          active.conversation?.requests?.at(-1)?.id;
    let task = options?.task || (matches ? capture?.task : undefined);
    if (!task) {
      const description = await vscode.window.showInputBox({
        title: "原始任务（没有执行前快照，将比较 Git HEAD）",
        prompt: "HEAD 差异可能包含用户原有修改；报告需人工确认归因",
        ignoreFocusOut: true,
        validateInput: (v) =>
          v.trim() && v.length <= 20000
            ? undefined
            : "请输入 1–20000 字符的原始需求",
      });
      if (description === undefined) return;
      task = {
        task_id: randomUUID(),
        task_description: description,
        technical_requirements: "",
      };
    }
    if (!options?.task) {
      const requirements = await vscode.window.showInputBox({
        title: "GPT 技术要求（可选）",
        prompt: "粘贴与本任务相关的技术要求；不自动混入无关 GPT 对话",
        value: task.technical_requirements,
        ignoreFocusOut: true,
        validateInput: (v) =>
          v.length <= 20000 ? undefined : "最多 20000 字符",
      });
      if (requirements === undefined) return;
      task = { ...task, technical_requirements: requirements };
    }
    if (
      !task.task_description.trim() ||
      task.task_description.length > 20000 ||
      task.technical_requirements.length > 20000
    )
      throw new Error("原始需求或技术要求无效 / 超过长度限制。");
    task = {
      ...task,
      task_description: safeReviewText(task.task_description),
      technical_requirements: safeReviewText(task.technical_requirements),
    };
    const baseline = capture && matches ? "CLAUDE_CAPTURE" : "GIT_HEAD",
      before =
        baseline === "CLAUDE_CAPTURE"
          ? capture!.before
          : await this.collector.headBaseline(after);
    const evidence = await this.collector.evidence(
      before,
      after,
      task!,
      matches ? capture!.delivery! : this.delivery(active),
      (await this.storage.tests(root)) || [],
      baseline,
    );
    this.error = undefined;
    if (this.preparationCancelled)
      throw new Error("审查准备已停止，未调用模型。");
    const report = await this.engine.run(evidence);
    this.report = report;
    this.history = await this.storage.reports(root);
    this.log(
      `独立 GPT 审查 ${report.verdict} · ${report.issues.length} 个问题`,
      "INFO",
      "GPT",
    );
    this.changed();
    return report;
  }
  private async openIssue(reviewId: string, issueId: string): Promise<void> {
    const report = this.history.find((r) => r.review_id === reviewId),
      issue = report?.issues.find((i) => i.issue_id === issueId);
    if (
      !report ||
      !issue ||
      !issue.line_number ||
      issue.evidence_status !== "VERIFIED" ||
      !relativeFile(issue.file_path) ||
      excludedPath(issue.file_path)
    )
      throw new Error("此问题没有可安全定位的文件行。");
    const root = await this.root();
    if (root !== report.project_path) throw new Error("该报告属于其他项目。");
    const current = await this.collector.snapshot(root);
    if (current.digest !== report.evidence.after.digest)
      throw new Error(
        "报告之后代码已变化；请重新审查后定位，避免跳转到错误代码行。",
      );
    if (issue.basis === "before") {
      const source = report.evidence.before.files.find(
        (f) => f.path === issue.file_path,
      );
      if (source?.text === undefined) throw new Error("缺少修改前代码。");
      const doc = await vscode.workspace.openTextDocument({
        content: source.text,
        language:
          path.extname(issue.file_path) === ".ts" ? "typescript" : "plaintext",
      });
      await vscode.window.showTextDocument(doc, {
        selection: new vscode.Range(
          issue.line_number - 1,
          0,
          issue.line_number - 1,
          0,
        ),
      });
      return;
    }
    const target = await fs.realpath(path.join(root, issue.file_path)),
      relative = path.relative(root, target);
    if (relative.startsWith("..") || path.isAbsolute(relative))
      throw new Error("文件超出项目根目录。");
    const doc = await vscode.workspace.openTextDocument(target);
    if (issue.line_number > doc.lineCount)
      throw new Error("报告行号超过当前文件范围。");
    await vscode.window.showTextDocument(doc, {
      selection: new vscode.Range(
        issue.line_number - 1,
        0,
        issue.line_number - 1,
        0,
      ),
    });
  }
  async handle(message: any): Promise<void> {
    try {
      this.error = undefined;
      if (message?.type === "reviewStart") await this.start();
      else if (message?.type === "reviewTests") await this.runTests();
      else if (message?.type === "reviewStop") {
        this.preparationCancelled = true;
        await this.engine.stop();
        await this.runner.stop();
      } else if (message?.type === "reviewHistory") {
        const root = await this.root();
        this.history = await this.storage.reports(root);
        const selected = await vscode.window.showQuickPick(
          this.history.map((r) => ({
            label: `${r.verdict} · ${r.task.task_description.slice(0, 60)}`,
            description: r.created_at,
            id: r.review_id,
          })),
          { placeHolder: "选择只读审查历史" },
        );
        if (selected)
          this.report = this.history.find((r) => r.review_id === selected.id);
      } else if (
        message?.type === "reviewSelect" &&
        typeof message.reviewId === "string"
      ) {
        this.report = this.history.find(
          (r) => r.review_id === message.reviewId,
        );
      } else if (
        message?.type === "reviewOpenIssue" &&
        typeof message.reviewId === "string" &&
        typeof message.issueId === "string"
      )
        await this.openIssue(message.reviewId, message.issueId);
    } catch (e) {
      this.error =
        e instanceof Error ? safeReviewText(e.message) : "审查操作失败";
      this.log(this.error, "ERROR", "GPT");
    } finally {
      this.changed();
    }
  }
  async dispose(): Promise<void> {
    await Promise.allSettled([this.engine.stop(), this.runner.stop()]);
  }
}
