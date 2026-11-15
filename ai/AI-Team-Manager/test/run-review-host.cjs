const path = require("node:path"),
  { spawn } = require("node:child_process"),
  { create } = require("./review-fixture.cjs");
(async () => {
  const root = path.resolve(__dirname, ".."),
    project = path.join(root, ".tools", `task04-host-fixture-${Date.now()}`);
  await create(project, require("./fixtures/review/cases.cjs")[1]);
  const code = await new Promise((resolve, reject) => {
    const child = spawn(
      process.execPath,
      [path.join(root, "test", "run-host.cjs")],
      {
        windowsHide: true,
        stdio: "inherit",
        env: {
          ...process.env,
          ATM_HOST_CODEX_REAL: "1",
          ATM_HOST_WORKSPACE: project,
          ATM_HOST_TEST_MODULE: path.join(root, "test", "task04-host-live.cjs"),
        },
      },
    );
    child.on("error", reject);
    child.on("close", resolve);
  });
  if (code !== 0) throw new Error("Task04 native host acceptance failed");
  console.log("Review native host fixture:", project);
})().catch((e) => {
  console.error(e);
  process.exitCode = 1;
});
