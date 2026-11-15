const { test } = require("node:test"),
  assert = require("node:assert/strict"),
  fs = require("node:fs/promises"),
  path = require("node:path"),
  os = require("node:os");
const api = require("../.tools/task02-test/api.cjs"),
  { create } = require("./review-fixture.cjs"),
  sample = require("./fixtures/review/cases.cjs")[0];
const cats = [
  "REQUIREMENTS",
  "ARCHITECTURE",
  "LOGIC",
  "SECURITY",
  "PERFORMANCE",
  "TESTING",
];
async function fixture() {
  const dir = await fs.mkdtemp(path.join(os.tmpdir(), "atm-review-")),
    root = path.join(dir, "project");
  await create(root, sample);
  const collector = new api.ReviewGit(path.join(dir, "temp"));
  const before = await collector.snapshot(root);
  await fs.writeFile(path.join(root, "worker.cjs"), sample.after);
  const after = await collector.snapshot(root);
  const tests = await new api.ReviewTestRunner(collector).run(
    root,
    "node --test",
  );
  const evidence = await collector.evidence(
    before,
    after,
    { task_id: "T", task_description: sample.task, technical_requirements: "" },
    { status: "FIXTURE", final_answer: "" },
    [tests],
    "CLAUDE_CAPTURE",
  );
  return { dir, root, collector, before, after, evidence };
}
const response = () => ({
  verdict: "PASS",
  summary: "Controlled validation test, not a model review",
  dimensions: cats.map((category) => ({
    category,
    status: "PASS",
    summary: "schema validation only",
  })),
  issues: [],
  missing_evidence: [],
});
test("Claude capture baseline preserves existing dirty user content and context includes actual test source", async () => {
  const f = await fixture();
  await fs.appendFile(
    path.join(f.root, "worker.cjs"),
    "// user-owned dirty line\n",
  );
  const before = await f.collector.snapshot(f.root);
  await fs.appendFile(
    path.join(f.root, "worker.cjs"),
    "// later task change\n",
  );
  const after = await f.collector.snapshot(f.root),
    e = await f.collector.evidence(
      before,
      after,
      f.evidence.task,
      f.evidence.delivery,
      [],
      "CLAUDE_CAPTURE",
    );
  assert(
    e.context.some(
      (c) => c.path === "sample.test.cjs" && c.text.includes("assert.equal"),
    ),
  );
  assert(e.changes[0].diff.includes("+// later task change"));
  assert(!/^[-+]\/\/ user-owned/m.test(e.changes[0].diff));
});
test("review collects real Git changes including dirty baseline, untracked files and deletion without touching source", async () => {
  const f = await fixture();
  assert.deepEqual(
    f.evidence.changes.map((c) => c.path),
    ["worker.cjs"],
  );
  assert(f.evidence.changes[0].diff.includes("Math.max"));
  assert.equal(f.evidence.tests[0].status, "PASSED");
  await fs.writeFile(path.join(f.root, "new.c"), "new file\n");
  await fs.unlink(path.join(f.root, "input.txt"));
  const after = await f.collector.snapshot(f.root),
    e = await f.collector.evidence(
      f.before,
      after,
      f.evidence.task,
      f.evidence.delivery,
      [],
      "CLAUDE_CAPTURE",
    );
  assert(e.changes.some((c) => c.path === "new.c" && c.kind === "ADDED"));
  assert(e.changes.some((c) => c.path === "input.txt" && c.kind === "DELETED"));
  assert(e.gaps.some((g) => g.includes("测试")));
});
test("review validation prevents high severity PASS, invented locations and incomplete dimensions", async () => {
  const { evidence } = await fixture(),
    batch = api.makeBatches(evidence)[0],
    data = response();
  data.issues = [
    {
      severity: "HIGH",
      category: "LOGIC",
      file_path: "worker.cjs",
      line_number: 3,
      basis: "after",
      problem_description: "validation case",
      evidence: "return Math.max(0, Math.min(100, value));",
      impact: "validation only",
      suggested_fix: "validation only",
      confidence: 0.8,
    },
  ];
  let parsed = api.validateBatch(JSON.stringify(data), evidence, batch);
  assert.equal(parsed.verdict, "REQUEST_CHANGES");
  assert.equal(parsed.issues[0].evidence_status, "VERIFIED");
  data.issues[0].line_number=2;
  assert.equal(api.validateBatch(JSON.stringify(data),evidence,batch).issues[0].evidence_status,'UNVERIFIED','Evidence from the next line cannot validate an incorrect line number');
  data.issues[0].line_number = 999;
  parsed = api.validateBatch(JSON.stringify(data), evidence, batch);
  assert.equal(parsed.verdict, "NEEDS_HUMAN_REVIEW");
  assert.equal(parsed.issues[0].evidence_status, "UNVERIFIED");
  data.dimensions.pop();
  assert.throws(() => api.validateBatch(JSON.stringify(data), evidence, batch));
});
test("review redacts secret values with stable lines, rejects traversal, and excludes configuration credentials", () => {
  const text = 'const password = "unit-test-only-password";\nline2\n';
  const masked = api.safeReviewText(text);
  assert(!masked.includes("unit-test-only-password"));
  assert.equal(masked.split("\n").length, text.split("\n").length);
  assert.equal(api.excludedPath(".env.local"), "sensitive");
  assert.equal(api.excludedPath(".claude/settings.json"), "sensitive");
  assert.equal(api.excludedPath("node_modules/lib/a.js"), "generated");
  for (const bad of ["../a", "/a", "C:/a", "a/../../b", "a\\b"])
    assert(!api.relativeFile(bad));
});
test("review batches large code with actual line labels and never passes missing evidence or unreviewed batches", async () => {
  const { evidence } = await fixture();
  evidence.changes[0].after.text = Array.from(
    { length: 500 },
    (_, i) => `const variable${i} = ${i};`,
  ).join("\n");
  const batches = api.makeBatches(evidence, 2000);
  assert(batches.length > 2);
  assert(batches.some((b) => b.content.includes("500:")));
  assert.equal(api.overallVerdict([], []), "BLOCKED");
  assert.equal(
    api.overallVerdict([{ verdict: "PASS", issues: [] }], ["no tests"]),
    "BLOCKED",
  );
  assert.equal(
    api.overallVerdict([{ verdict: "PASS", issues: [] }], [], true),
    "NEEDS_HUMAN_REVIEW",
  );
});
test('zero-test command exits are not marked as passing tests',async()=>{
  const f=await fixture();await fs.unlink(path.join(f.root,'sample.test.cjs'));const evidence=await new api.ReviewTestRunner(f.collector).run(f.root,'node --test');assert.equal(evidence.exit_code,0);assert.equal(evidence.test_count,0);assert.equal(evidence.status,'UNVERIFIED');
});
test("review history persists independently and malformed history is not silently replaced", async () => {
  const f = await fixture(),
    storage = new api.ReviewStorage(path.join(f.dir, "history"));
  await storage.write(f.root, "tests.json", f.evidence.tests);
  assert.equal((await storage.tests(f.root))[0].status, "PASSED");
  await fs.writeFile(path.join(storage.project(f.root), "tests.json"), "{bad");
  await assert.rejects(() => storage.tests(f.root));
  assert.equal(
    await fs.readFile(path.join(storage.project(f.root), "tests.json"), "utf8"),
    "{bad",
  );
});
