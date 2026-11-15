const fs = require("node:fs/promises"),
  path = require("node:path"),
  { execFileSync } = require("node:child_process");
exports.create = async (root, sample) => {
  await fs.mkdir(root, { recursive: true });
  for (const [name, text] of Object.entries({
    "worker.cjs": sample.before,
    "sample.test.cjs":
      "const test=require('node:test'),assert=require('node:assert/strict');\n" +
      sample.test +
      "\n",
    "README.md": sample.task + "\n",
    "input.txt": "A",
  }))
    await fs.writeFile(path.join(root, name), text);
  const git = (args) =>
    execFileSync("git", ["-C", root, ...args], {
      windowsHide: true,
      stdio: "pipe",
    });
  git(["init", "--quiet"]);
  git(["add", "--", "worker.cjs", "sample.test.cjs", "README.md", "input.txt"]);
  git([
    "-c",
    "user.name=ATM Test",
    "-c",
    "user.email=atm-test@example.invalid",
    "commit",
    "--quiet",
    "-m",
    "fixture baseline",
  ]);
};
