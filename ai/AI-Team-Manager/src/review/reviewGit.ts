import { execFile } from "node:child_process";
import * as fs from "node:fs/promises";
import * as path from "node:path";
import { createHash, randomUUID } from "node:crypto";
import { excludedPath, relativeFile, safeReviewText } from "./reviewSafety";
import { selectReviewContext } from './reviewContext';
import { snapshotScopeProof } from './reviewProof';
import type {
  SourceFile,
  SourceSnapshot,
  ReviewEvidence,
  ReviewTask,
  ClaudeDelivery,
  TestEvidence,
} from "./reviewTypes";
export const digest = (text: string | Buffer) =>
  createHash("sha256").update(text).digest("hex");
export function git(
  root: string,
  args: string[],
  allowDiff = false,
): Promise<string> {
  return new Promise((resolve, reject) =>
    execFile(
      "git",
      [
        "-c",
        "core.fsmonitor=false",
        "-c",
        "core.longpaths=true",
        "-c",
        "core.hooksPath=/dev/null",
        "-C",
        root,
        ...args,
      ],
      {
        windowsHide: true,
        timeout: 20000,
        maxBuffer: 16 * 1024 * 1024,
        encoding: "utf8",
      },
      (error, stdout, stderr) =>
        error && !(allowDiff && (error as any).code === 1)
          ? reject(
              new Error(
                "Git 操作失败（仓库、认证、权限、远端更新或超时）；本地成果保留。"+safeReviewText(stderr||'').slice(0,2000),
              ),
            )
          : resolve(stdout),
    ),
  );
}
export class ReviewGit {
  constructor(
    private temporaryDirectory: string,
    private fileLimit = 512 * 1024,
    private totalLimit = 16 * 1024 * 1024,
  ) {}
  private async names(
    root: string,
  ): Promise<{ names: string[]; gaps: string[] }> {
    const names = new Set(
        (await git(root, ["ls-files", "-z", "--cached"]))
          .split("\0")
          .filter(Boolean),
      ),
      gaps: string[] = [];
    const status = (
      await git(root, [
        "status",
        "--porcelain=v1",
        "-z",
        "--untracked-files=normal",
        "--ignore-submodules=none",
      ])
    ).split("\0");
    let visits = 0;
    const walk = async (file: string): Promise<void> => {
      if (++visits > 20000) {
        if (!gaps.includes("未跟踪文件枚举超过 20000 项"))
          gaps.push("未跟踪文件枚举超过 20000 项");
        return;
      }
      if (!relativeFile(file.replace(/\/$/, ""))) {
        gaps.push("不安全的 Git 文件路径");
        return;
      }
      if (excludedPath(file) === "generated") return;
      if (excludedPath(file) === "sensitive") {
        names.add(file.replace(/\/$/, ""));
        return;
      }
      const full = path.join(root, file),
        stat = await fs.lstat(full).catch(() => undefined);
      if (!stat) return;
      if (stat.isDirectory() && !stat.isSymbolicLink()) {
        for (const item of await fs.readdir(full))
          await walk(`${file.replace(/\/$/, "")}/${item}`);
      } else names.add(file);
    };
    for (let i = 0; i < status.length; i++) {
      const entry = status[i];
      if (entry.startsWith("?? ")) await walk(entry.slice(3));
      else if (/[RC]/.test(entry.slice(0, 2))) i++;
    }
    return { names: [...names].sort(), gaps };
  }
  async snapshot(directory: string): Promise<SourceSnapshot> {
    const root = await fs.realpath(directory),
      top = await git(root, ["rev-parse", "--show-toplevel"]);
    if (path.resolve(top.trim()).toLowerCase() !== root.toLowerCase())
      throw new Error(
        "审查工作区必须为 Git 仓库根目录，避免漏掉仓库外层变更。",
      );
    const head = (await git(root, ["rev-parse", "HEAD"])).trim(),
      { names, gaps } = await this.names(root),
      files: SourceFile[] = [];
    let total = 0;
    for (const file of names) {
      if (!relativeFile(file)) {
        gaps.push("Git 包含不安全路径");
        continue;
      }
      const exclusion = excludedPath(file);
      if (exclusion === "generated") continue;
      if (exclusion === "sensitive") {
        files.push({ path: file, hash: "EXCLUDED", omitted: "敏感路径不读取" });
        continue;
      }
      const full = path.join(root, file),
        stat = await fs.lstat(full).catch(() => undefined);
      if (!stat) continue;
      if (stat.isSymbolicLink() || !stat.isFile()) {
        files.push({
          path: file,
          hash: "OMITTED",
          omitted: "符号链接、子模块或非普通文件不读取",
        });
        continue;
      }
      const real = await fs.realpath(full),
        rel = path.relative(root, real);
      if (rel.startsWith("..") || path.isAbsolute(rel)) {
        gaps.push(`文件超出工作区：${file}`);
        continue;
      }
      if (stat.size > this.fileLimit || total + stat.size > this.totalLimit) {
        files.push({
          path: file,
          hash: `OMITTED:${stat.size}:${stat.mtimeMs}`,
          omitted: "文件或项目内容超过采集大小限制",
        });
        continue;
      }
      const bytes = await fs.readFile(real);
      const finalStat = await fs.stat(real);
      if (finalStat.size !== stat.size || finalStat.mtimeMs !== stat.mtimeMs) {
        gaps.push(`采集时文件发生变化：${file}`);
      }
      total += bytes.length;
      if (bytes.includes(0)) {
        files.push({
          path: file,
          hash: digest(bytes),
          omitted: "二进制文件不发送",
        });
        continue;
      }
      const raw = bytes.toString("utf8"),
        text = safeReviewText(raw);
      files.push({
        path: file,
        hash: digest(bytes),
        text,
        redacted: raw !== text,
      });
    }
    return {
      root,
      time: new Date().toISOString(),
      head,
      files,
      gaps,
      digest: digest(JSON.stringify(files.map((f) => [f.path, f.hash]))),
    };
  }
  async headBaseline(current: SourceSnapshot): Promise<SourceSnapshot> {
    const files: SourceFile[] = [];
    let total = 0;
    for (const file of (
      await git(current.root, [
        "ls-tree",
        "-r",
        "-z",
        "--name-only",
        current.head,
      ])
    )
      .split("\0")
      .filter(Boolean)) {
      if (!relativeFile(file) || excludedPath(file) === "generated") continue;
      if (excludedPath(file) === "sensitive") {
        files.push({ path: file, hash: "EXCLUDED", omitted: "敏感路径不读取" });
        continue;
      }
      const size = Number(
        (
          await git(current.root, ["cat-file", "-s", `${current.head}:${file}`])
        ).trim(),
      );
      if (size > this.fileLimit || total + size > this.totalLimit) {
        files.push({ path: file, hash: "OMITTED", omitted: "基线文件过大" });
        continue;
      }
      total += size;
      const raw = await git(current.root, ["show", `${current.head}:${file}`]);
      if (raw.includes("\0")) {
        files.push({
          path: file,
          hash: digest(raw),
          omitted: "二进制文件不发送",
        });
        continue;
      }
      const text = safeReviewText(raw);
      files.push({
        path: file,
        hash: digest(raw),
        text,
        redacted: raw !== text,
      });
    }
    return {
      ...current,
      time: new Date().toISOString(),
      files,
      digest: digest(JSON.stringify(files.map((f) => [f.path, f.hash]))),
    };
  }
  private async diff(
    file: string,
    before: string,
    after: string,
  ): Promise<string> {
    const dir = path.join(this.temporaryDirectory, randomUUID());
    await fs.mkdir(dir, { recursive: true });
    try {
      await fs.writeFile(path.join(dir, "before"), before);
      await fs.writeFile(path.join(dir, "after"), after);
      const patch = await git(
        dir,
        [
          "diff",
          "--no-index",
          "--no-ext-diff",
          "--no-textconv",
          "--unified=5",
          "--",
          "before",
          "after",
        ],
        true,
      );
      return patch
        .replace(/^diff --git .*$/m, `diff --git a/${file} b/${file}`)
        .replace(/^--- .*$/m, `--- a/${file}`)
        .replace(/^\+\+\+ .*$/m, `+++ b/${file}`);
    } finally {
      await fs.unlink(path.join(dir, "before")).catch(() => {});
      await fs.unlink(path.join(dir, "after")).catch(() => {});
      await fs.rmdir(dir).catch(() => {});
    }
  }
  async evidence(
    before: SourceSnapshot,
    after: SourceSnapshot,
    task: ReviewTask,
    delivery: ClaudeDelivery,
    tests: TestEvidence[],
    baseline: ReviewEvidence["baseline"],
  ): Promise<ReviewEvidence> {
    const changes: ReviewEvidence["changes"] = [],
      gaps = [...before.gaps, ...after.gaps],
      warnings: string[] = [];
    if (before.root !== after.root)
      throw new Error("审查基线与当前工作区不一致。");
    const old = new Map(before.files.map((f) => [f.path, f])),
      now = new Map(after.files.map((f) => [f.path, f]));
    for (const file of new Set([...old.keys(), ...now.keys()])) {
      const a = old.get(file),
        b = now.get(file);
      if (a?.hash === b?.hash && /^[a-f0-9]{64}$/.test(a?.hash || "")) continue;
      if (a?.omitted || b?.omitted) {
        gaps.push(`${file}：${a?.omitted || b?.omitted}，无法确认完整变更`);
        continue;
      }
      changes.push({
        path: file,
        kind: !a ? "ADDED" : !b ? "DELETED" : "MODIFIED",
        before: a,
        after: b,
        diff: await this.diff(file, a?.text || "", b?.text || ""),
      });
    }
    if (!task.task_description.trim()) gaps.push("缺少用户原始任务");
    if (!changes.length) gaps.push("没有可审查的实际文本变更");
    if (
      !tests.length ||
      !tests.some(
        (t) =>
          t.source_digest === after.digest &&
          ["PASSED", "FAILED"].includes(t.status),
      )
    )
      gaps.push("缺少针对当前代码快照实际执行的测试证据");
    if (baseline === "GIT_HEAD")
      warnings.push(
        "没有 Claude 执行前快照：HEAD 差异可能包含用户原有改动，不能断言全部由 Claude 修改。",
      );
    else
      warnings.push(
        "执行前快照只证明此时间段的变化；若用户或其他进程同时编辑，不能独占归因给 Claude。",
      );
    if (!delivery.final_answer)
      warnings.push("缺少 Claude 最终回答；仍以代码和测试证据为准。");
    const selected=selectReviewContext(after,changes,task,tests);
    const context=selected.context;gaps.push(...selected.gaps);warnings.push(...selected.warnings);
    const scope_proof=snapshotScopeProof(before,after,task);
    if(!scope_proof.binding_valid)gaps.push('前后快照摘要与实际文件清单不一致');
    if(scope_proof.outside_allowed.length)gaps.push('检测到允许范围外的真实变更：'+scope_proof.outside_allowed.join(', '));
    if(scope_proof.protected_files.some(f=>!f.unchanged))gaps.push('检测到受保护文件已改变');
    return {
      id: randomUUID(),
      task,
      delivery,
      baseline,
      before,
      after,
      changes,
      scope_proof,
      context,
      tests: tests.filter((t) => t.source_digest === after.digest),
      gaps: [...new Set(gaps)],
      warnings,
    };
  }
}
