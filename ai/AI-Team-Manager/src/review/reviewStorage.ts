import * as fs from "node:fs/promises";
import * as path from "node:path";
import { digest } from "./reviewGit";
import { randomUUID } from "node:crypto";
import type { ReviewReport, TaskCapture, TestEvidence } from "./reviewTypes";
export class ReviewStorage {
  private queue: Promise<void> = Promise.resolve();
  constructor(public readonly directory: string) {}
  project(root: string): string {
    return path.join(
      this.directory,
      digest(process.platform === "win32" ? root.toLowerCase() : root),
    );
  }
  async write(root: string, name: string, data: unknown): Promise<void> {
    if (!/^[\w-]+\.json$/.test(name)) throw new Error("无效审查记录名");
    const serialized = JSON.stringify(data, null, 2),
      file = path.join(this.project(root), name),
      temp = `${file}.${randomUUID()}.tmp`;
    const pending = this.queue
      .catch(() => {})
      .then(async () => {
        await fs.mkdir(path.dirname(file), { recursive: true });
        try {
          await fs.writeFile(temp, serialized, { mode: 0o600 });
          for (let i = 0; ; i++) {
            try {
              await fs.rename(temp, file);
              break;
            } catch (e) {
              if (i === 3) throw e;
              await new Promise((r) => setTimeout(r, 50 * (i + 1)));
            }
          }
        } finally {
          await fs.unlink(temp).catch(() => {});
        }
      });
    this.queue = pending;
    return pending;
  }
  async read<T>(root: string, name: string): Promise<T | undefined> {
    try {
      return JSON.parse(
        await fs.readFile(path.join(this.project(root), name), "utf8"),
      );
    } catch (e: any) {
      if (e.code === "ENOENT") return undefined;
      throw new Error("审查历史损坏或无法读取，未覆盖原记录。");
    }
  }
  async reports(root: string): Promise<ReviewReport[]> {
    const names = await fs.readdir(this.project(root)).catch((e: any) => {
      if (e.code === "ENOENT") return [];
      throw e;
    });
    const result: ReviewReport[] = [];
    for (const n of names.filter((n) => /^report-[\w-]+\.json$/.test(n))) {
      const r = await this.read<ReviewReport>(root, n);
      if (
        !r ||
        r.version !== 1 ||
        r.project_path !== root ||
        !Array.isArray(r.issues)
      )
        throw new Error("审查报告格式无效，未覆盖记录。");
      result.push(r);
    }
    return result.sort((a, b) => b.created_at.localeCompare(a.created_at));
  }
  saveReport(report: ReviewReport) {
    return this.write(
      report.project_path,
      `report-${report.review_id}.json`,
      report,
    );
  }
  saveCapture(capture: TaskCapture) {
    return this.write(capture.before.root, "capture.json", capture);
  }
  capture(root: string) {
    return this.read<TaskCapture>(root, "capture.json");
  }
  tests(root: string) {
    return this.read<TestEvidence[]>(root, "tests.json");
  }
}
