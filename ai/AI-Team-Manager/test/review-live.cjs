// Explicit real Codex model acceptance. npm test never invokes this file.
const fs = require("node:fs/promises"),
  path = require("node:path"),
  assert = require("node:assert/strict");
const {
  ReviewGit,
  ReviewStorage,
  ReviewEngine,
  ReviewTestRunner,
  CodexService,
  digest,
} = require("../.tools/task02-test/api.cjs");
const samples = require("./fixtures/review/cases.cjs"),
  { create } = require("./review-fixture.cjs");
const root = path.resolve(__dirname, ".."),
  dir = path.join(root, ".tools", `task04-live-${Date.now()}`);
(async () => {
  await fs.mkdir(dir, { recursive: true });
  const results = [];
  for (const sample of samples) {
    const project = path.join(dir, sample.name),
      collector = new ReviewGit(path.join(dir, "temporary")),
      storage = new ReviewStorage(path.join(dir, "history"));
    await create(project, sample);
    const before = await collector.snapshot(project);
    await fs.writeFile(path.join(project, "worker.cjs"), sample.after);
    const after = await collector.snapshot(project);
    const test = await new ReviewTestRunner(collector).run(
      project,
      "node --test",
    );
    assert.equal(test.status, sample.name === "correct" ? "PASSED" : "FAILED");
    const evidence = await collector.evidence(
      before,
      after,
      {
        task_id: sample.name,
        task_description: sample.task,
        technical_requirements:
          "复用现有 CommonJS 导出接口，不新增依赖；按原始需求审查，不添加范围外功能。",
        allowed_files: ["worker.cjs"],
        forbidden_files: ["sample.test.cjs"],
      },
      {
        status: "FIXTURE",
        final_answer: "这是人为构造的验收代码样例，不是 Claude 的真实输出。",
      },
      [test],
      "CLAUDE_CAPTURE",
    );
    assert.equal(evidence.gaps.length, 0, evidence.gaps.join(";"));
    const engine = new ReviewEngine(
      new CodexService(),
      collector,
      storage,
      () => console.log(sample.name, engine.progress),
      240000,
    );
    const report = await engine.run(evidence);
    await fs.writeFile(
      path.join(dir, `${sample.name}-report.json`),
      JSON.stringify(report, null, 2),
    );
    assert.equal(
      digest(await fs.readFile(path.join(project, "worker.cjs"))),
      after.files.find((f) => f.path === "worker.cjs").hash,
      "Review must not modify fixture",
    );
    const verified = report.issues.filter(
      (i) => i.evidence_status === "VERIFIED",
    );
    if (sample.name === "correct") {
      assert.equal(
        report.verdict,
        "PASS",
        JSON.stringify({
          verdict: report.verdict,
          gaps: report.missing_evidence,
          issues: report.issues,
        }),
      );
      assert(!verified.some((i) => ["CRITICAL", "HIGH"].includes(i.severity)));
    } else {
      assert.equal(
        report.verdict,
        "REQUEST_CHANGES",
        JSON.stringify({
          verdict: report.verdict,
          gaps: report.missing_evidence,
          issues: report.issues,
        }),
      );
      assert(
        verified.some(
          (i) =>
            i.file_path === "worker.cjs" &&
            i.line_number &&
            i.suggested_fix &&
            ["REQUIREMENTS", "LOGIC"].includes(i.category),
        ),
        JSON.stringify(report.issues),
      );
    }
    assert.equal(report.dimensions.length, 6);
    assert.equal(report.batches.length, 1);
    assert(report.batches[0].session_id);
    assert((await storage.reports(project)).length === 1);
    results.push({
      sample: sample.name,
      verdict: report.verdict,
      test: test.status,
      session: report.batches[0].session_id,
      verified: verified.map((i) => ({
        line: i.line_number,
        severity: i.severity,
        category: i.category,
        problem: i.problem_description,
        suggested_fix: i.suggested_fix,
      })),
      unchanged: true,
    });
    await fs.writeFile(
      path.join(dir, "results.json"),
      JSON.stringify(results, null, 2),
    );
    console.log("REAL REVIEW", JSON.stringify(results.at(-1)));
  }
  console.log("Five real review cases passed:", dir);
})().catch((e) => {
  console.error(e);
  console.error("Evidence:", dir);
  process.exitCode = 1;
});
