// Deliberately terminate a genuine native Claude process without calling stop().
const fs = require("node:fs/promises"),
  path = require("node:path"),
  assert = require("node:assert/strict");
const {
  ClaudeController,
  ClaudeService,
  ClaudeStorage,
  resolveClaudeExecutable,
  launch,
  terminateTree,
} = require("../.tools/task02-test/api.cjs");
exports.run = async (dir) => {
  const project = path.join(dir, "project"),
    events = [];
  let child;
  const service = new ClaudeService(
    process.env.ATM_CLAUDE_EXECUTABLE || "claude",
    (command, args, cwd) => {
      const p = launch(command, args, cwd);
      if (args.includes("--print")) child = p;
      return p;
    },
    (command) =>
      resolveClaudeExecutable(command, { extensionDirectory: () => undefined }),
  );
  const make = () =>
    new ClaudeController(
      new ClaudeStorage(path.join(dir, "storage"), project),
      service,
      () => {},
      async () => {},
      () => 180000,
    );
  let controller = make();
  await controller.initialize();
  const sid = controller.snapshot().conversation.threadId;
  const originalRun = service.run.bind(service);
  service.run = (cwd, prompt, id, emit, timeout) =>
    originalRun(
      cwd,
      prompt,
      id,
      (e) => {
        events.push({ ...e, time: new Date().toISOString() });
        emit(e);
      },
      timeout,
    );
  const pending = controller.send(
    "不要使用工具或修改文件。逐行列出 1 到 2000，每行解释对应数字的奇偶性，不要省略。",
  );
  let started = false;
  for (let i = 0; i < 1200; i++) {
    if (events.some((e) => e.type === "text" && e.delta)) {
      started = true;
      break;
    }
    if (!controller.snapshot().busy) break;
    await new Promise((r) => setTimeout(r, 100));
  }
  assert(started, "Real response must start before external termination");
  const pid = child.pid;
  await terminateTree(child);
  await pending;
  const crashed = controller.snapshot();
  assert.equal(crashed.conversation.status, "FAILED");
  assert(crashed.error.includes("PROCESS_EXIT_NONZERO"), crashed.error);
  assert.equal(crashed.conversation.threadId, sid);
  controller = make();
  await controller.initialize();
  await controller.send(
    "不要使用工具或修改文件。只回复 CLAUDE_AFTER_CRASH_OK。",
  );
  const restored = controller.snapshot();
  assert.equal(restored.conversation.status, "COMPLETED", restored.error);
  assert.equal(restored.error, undefined);
  assert.equal(restored.conversation.threadId, sid);
  assert(
    restored.conversation.messages
      .at(-1)
      .text.includes("CLAUDE_AFTER_CRASH_OK"),
  );
  let alive = false;
  try {
    process.kill(pid, 0);
    alive = true;
  } catch {}
  assert(!alive);
  await fs.writeFile(
    path.join(dir, "faults.json"),
    JSON.stringify(
      {
        unexpectedExit: true,
        resumedAfterCrash: true,
        processExited: true,
        pid,
        session: sid,
        crashed,
        restored,
        events,
      },
      null,
      2,
    ),
  );
  console.log("Real unexpected-exit and recovery passed:", dir);
};
if (require.main === module)
  exports.run(path.resolve(process.argv[2])).catch((e) => {
    console.error(e);
    process.exitCode = 1;
  });
