import {
  categories,
  type BatchResult,
  type ReviewEvidence,
  type ReviewIssue,
  type Verdict,
  type DimensionResult,
} from "./reviewTypes";
import type { ReviewBatch } from "./reviewPrompt";
import { relativeFile, safeReviewText } from "./reviewSafety";
import type { RepairCheck } from '../repair/repairTypes';
const verdicts: Verdict[] = [
  "PASS",
  "REQUEST_CHANGES",
  "BLOCKED",
  "NEEDS_HUMAN_REVIEW",
];
const string = (value: unknown, max = 8000): string => {
  if (typeof value !== "string" || !value.trim() || value.length > max)
    throw new Error("审查JSON缺少必要文本或超出限制。");
  return safeReviewText(value);
};
export function validateBatch(
  text: string,
  evidence: ReviewEvidence,
  batch: ReviewBatch,
): BatchResult {
  let data: any;
  try {
    data = JSON.parse(
      text
        .trim()
        .replace(/^```(?:json)?\s*/, "")
        .replace(/\s*```$/, ""),
    );
  } catch {
    throw new Error("GPT 未返回有效结构化 JSON。");
  }
  if (
    !verdicts.includes(data.verdict) ||
    !Array.isArray(data.dimensions) ||
    data.dimensions.length !== 6 ||
    !Array.isArray(data.issues) ||
    data.issues.length > 100 ||
    !Array.isArray(data.missing_evidence) ||
    data.missing_evidence.length > 100
  )
    throw new Error("GPT 审查结果不符合结构。");
  const dimensions: DimensionResult[] = categories.map((category) => {
    const list = data.dimensions.filter((d: any) => d.category === category);
    if (
      list.length !== 1 ||
      ![
        "PASS",
        "ISSUES",
        "INSUFFICIENT_EVIDENCE",
        "NEEDS_HUMAN_REVIEW",
      ].includes(list[0].status)
    )
      throw new Error("六维审查结果缺失或重复。");
    return {
      category,
      status: list[0].status,
      summary: string(list[0].summary),
    };
  });
  const issues: ReviewIssue[] = data.issues.map((i: any, index: number) => {
    if (
      !["CRITICAL", "HIGH", "MEDIUM", "LOW", "INFO"].includes(i.severity) ||
      !categories.includes(i.category) ||
      typeof i.file_path !== "string" ||
      !relativeFile(i.file_path) ||
      !["before", "after"].includes(i.basis) ||
      !(
        i.line_number === null ||
        (Number.isInteger(i.line_number) && i.line_number > 0)
      ) ||
      typeof i.confidence !== "number" ||
      i.confidence < 0 ||
      i.confidence > 1
    )
      throw new Error("问题字段或文件位置无效。");
    const code = string(i.evidence),
      file = evidence.changes.find((c) => c.path === i.file_path)?.[
        i.basis as "before" | "after"
      ] || (i.basis === 'after' ? evidence.inspection_files?.find(f => f.path === i.file_path) : undefined);
    const lines = file?.text?.split("\n"),
      present =
        batch.files.includes(i.file_path) &&
        !!lines &&
        !!i.line_number &&
        batch.content.includes(`${i.line_number}: ${lines[i.line_number - 1]}`);
    const excerpt =
      lines && i.line_number
        ? lines.slice(i.line_number - 1, i.line_number + 7).join("\n")
        : "";
    const offset = excerpt.indexOf(code),
      exactLine = offset >= 0 && !excerpt.slice(0, offset).includes("\n");
    const verified = !!(
      present &&
      lines &&
      i.line_number &&
      exactLine &&
      code.trim().length >= 4
    );
    return {
      issue_id: `${batch.id}-${index + 1}`,
      severity: i.severity,
      category: i.category,
      file_path: i.file_path,
      line_number: i.line_number,
      basis: i.basis,
      problem_description: string(i.problem_description),
      evidence: code,
      impact: string(i.impact),
      suggested_fix: string(i.suggested_fix),
      confidence: i.confidence,
      evidence_status: verified ? "VERIFIED" : "UNVERIFIED",
    };
  });
  const missing_evidence = data.missing_evidence.map((x: unknown) => string(x));
  let verdict: Verdict = data.verdict;
  if (issues.some((i) => i.evidence_status === "UNVERIFIED"))
    verdict = "NEEDS_HUMAN_REVIEW";
  if (
    issues.some(
      (i) =>
        i.evidence_status === "VERIFIED" &&
        ["CRITICAL", "HIGH"].includes(i.severity),
    )
  )
    verdict = "REQUEST_CHANGES";
  if (dimensions.some((d) => d.status === "ISSUES") && verdict === "PASS")
    verdict = "REQUEST_CHANGES";
  if (
    dimensions.some((d) => d.status === "NEEDS_HUMAN_REVIEW") &&
    verdict === "PASS"
  )
    verdict = "NEEDS_HUMAN_REVIEW";
  if (
    missing_evidence.length ||
    dimensions.some((d) => d.status === "INSUFFICIENT_EVIDENCE")
  )
    verdict = "BLOCKED";
  let repair_checks: RepairCheck[] | undefined;
  if (evidence.task.repair_context) {
    const ids = evidence.task.repair_context.issues.map(i => i.issue_id);
    if (!Array.isArray(data.repair_checks) || data.repair_checks.length !== ids.length || ids.some(id => data.repair_checks.filter((c: any) => c.issue_id === id).length !== 1))
      throw new Error('GPT缺少原始审查问题的逐项修复验证结果。');
    repair_checks = data.repair_checks.map((c: any) => {
      if (!['FIXED','NOT_FIXED','REGRESSION','UNVERIFIED'].includes(c.status) || !relativeFile(c.file_path) || !(c.line_number === null || Number.isInteger(c.line_number) && c.line_number > 0)) throw new Error('修复验证字段无效。');
      const code = typeof c.evidence === 'string' ? safeReviewText(c.evidence) : '';
      const file = evidence.after.files.find(f => f.path === c.file_path), lines = file?.text?.split('\n');
      const excerpt = lines && c.line_number ? lines.slice(c.line_number - 1, c.line_number + 7).join('\n') : '';
      const offset = excerpt.indexOf(code);
      const exact = code.trim().length >= 4 && offset >= 0 && !excerpt.slice(0,offset).includes('\n') && batch.files.includes(c.file_path) && !!lines && batch.content.includes(`${c.line_number}: ${lines[c.line_number - 1]}`);
      return { issue_id: c.issue_id, status: exact ? c.status : 'UNVERIFIED', explanation: string(c.explanation), file_path: c.file_path, line_number: c.line_number, evidence: code };
    });
  }
  return {
    ...(repair_checks ? { repair_checks } : {}),
    batch_id: batch.id,
    verdict,
    summary: string(data.summary),
    dimensions,
    issues,
    missing_evidence,
  };
}
export function overallVerdict(
  batches: BatchResult[],
  gaps: string[],
  human = false,
): Verdict {
  if (
    gaps.length ||
    !batches.length ||
    batches.some((b) => b.verdict === "BLOCKED" || b.error)
  )
    return "BLOCKED";
  if (
    batches.some(
      (b) =>
        b.verdict === "REQUEST_CHANGES" ||
        b.issues.some(
          (i) =>
            i.evidence_status === "VERIFIED" &&
            ["CRITICAL", "HIGH"].includes(i.severity),
        ),
    )
  )
    return "REQUEST_CHANGES";
  if (human || batches.some((b) => b.verdict === "NEEDS_HUMAN_REVIEW"))
    return "NEEDS_HUMAN_REVIEW";
  return "PASS";
}
