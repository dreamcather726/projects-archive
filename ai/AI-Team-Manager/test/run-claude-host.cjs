// Creates a fresh fixture and uses a normal isolated Development Host twice.
const fs = require("node:fs/promises"),
  path = require("node:path"),
  { spawn } = require("node:child_process");
(async () => {
  const root = path.resolve(__dirname, ".."),
    dir = path.join(root, ".tools", `task03-fixture-${Date.now()}`),
    project = path.join(dir, "project");
  await fs.mkdir(project, { recursive: true });
  await fs.writeFile(
    path.join(project, "README.md"),
    "Isolated Claude Extension Host fixture.\nSession marker: CLAUDE_NATIVE_HOST_FIXTURE\nOnly motor_control.c may be edited.\n",
  );
  await fs.writeFile(
    path.join(project, "motor_control.c"),
    "int motor_limit(int speed) { return speed < 0 ? 0 : speed; }\n",
  );
  await fs.writeFile(
    path.join(project, "protocol.c"),
    "int protocol_version(void) { return 1; }\n",
  );
  await fs.writeFile(
    path.join(project, "main.c"),
    "int main(void) { return 0; }\n",
  );
  const env = {
    ...process.env,
    ATM_HOST_TEST_MODULE: path.join(root, "test", "task03-host-live.cjs"),
    ATM_HOST_WORKSPACE: project,
  };
  const code = await new Promise((resolve, reject) => {
    const child = spawn(
      process.execPath,
      [path.join(root, "test", "run-host.cjs")],
      { env, stdio: "inherit", windowsHide: true },
    );
    child.on("error", reject);
    child.on("close", resolve);
  });
  assertExit(code);
  for (const [file, text] of [
    ["protocol.c", "int protocol_version(void) { return 1; }\n"],
    ["main.c", "int main(void) { return 0; }\n"],
  ])
    if ((await fs.readFile(path.join(project, file), "utf8")) !== text)
      throw new Error("Protected fixture changed: " + file);
  console.log("Claude native host fixture verified:", project);
})().catch((e) => {
  console.error(e);
  process.exitCode = 1;
});
function assertExit(code) {
  if (code !== 0) throw new Error("Claude native host tests failed: " + code);
}
