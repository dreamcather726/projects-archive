// Real GPT boundary acceptance using an already-created isolated fixture.
const fs = require("node:fs/promises"),
  path = require("node:path"),
  assert = require("node:assert/strict");
const {
  ReviewEngine,
  ReviewGit,
  ReviewStorage,
  CodexService,
} = require("../.tools/task02-test/api.cjs");
(async () => {
  const root = path.resolve(__dirname, ".."),
    names = (await fs.readdir(path.join(root, ".tools")))
      .filter((n) => /^task04-live-\d+$/.test(n))
      .sort()
      .reverse(),
    dir = path.join(root, ".tools", names[0]);
  const original = JSON.parse(
      await fs.readFile(path.join(dir, "correct-report.json"), "utf8"),
    ),
    collector = new ReviewGit(path.join(dir, "boundary-temp")),
    storage = new ReviewStorage(path.join(dir, "boundary-history"));
  const missing = structuredClone(original.evidence);
  missing.tests = [];
  missing.gaps = ["缺少针对当前代码快照实际执行的测试证据"];
  const blocked = await new ReviewEngine(
    new CodexService(),
    collector,
    storage,
  ).run(missing);
  assert.equal(blocked.verdict, "BLOCKED");
  assert(blocked.batches.length === 1 && blocked.batches[0].session_id);
  await fs.writeFile(
    path.join(dir, "missing-evidence-report.json"),
    JSON.stringify(blocked, null, 2),
  );
  let changed = false;
  const project = original.evidence.after.root;
  const engine = new ReviewEngine(
    new CodexService(),
    collector,
    storage,
    () => {
      if (!changed && engine.progress.startsWith("审查 B")) {
        changed = true;
        require("node:fs").appendFileSync(
          path.join(project, "worker.cjs"),
          "// concurrent edit from acceptance harness\n",
        );
      }
    },
  );
  try {
    const stale = await engine.run(original.evidence);
    assert.equal(stale.verdict, "BLOCKED");
    assert.equal(stale.stale, true);
    assert(stale.batches[0].session_id);
    await fs.writeFile(
      path.join(dir, "stale-evidence-report.json"),
      JSON.stringify(stale, null, 2),
    );
  } finally {
    await fs.writeFile(
      path.join(project, "worker.cjs"),
      require("./fixtures/review/cases.cjs")[0].after,
    );
  }
  console.log(
    "Real GPT boundaries passed: missing tests BLOCKED; concurrent source change BLOCKED",
    dir,
  );
})().catch((e) => {
  console.error(e);
  process.exitCode = 1;
});
