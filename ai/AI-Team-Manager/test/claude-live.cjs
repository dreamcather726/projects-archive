// Explicit real-model acceptance test. Writes only its newly created .tools fixture.
const fs = require("node:fs/promises"),
  path = require("node:path"),
  assert = require("node:assert/strict");
const { randomUUID, createHash } = require("node:crypto");
const {
  ClaudeService,
  ClaudeController,
  ClaudeStorage,
  resolveClaudeExecutable,
  launch,
  PermissionPolicy,
  PermissionHost,
} = require("../.tools/task02-test/api.cjs");
const root = path.resolve(__dirname, ".."),
  dir = path.join(root, ".tools", `task03-live-${Date.now()}`),
  project = path.join(dir, "project"),
  records = [];
const hash = (text) => createHash("sha256").update(text).digest("hex");
(async () => {
  await fs.mkdir(project, { recursive: true });
  const marker = `ATM_${randomUUID().slice(0, 8)}`;
  await fs.writeFile(
    path.join(project, "README.md"),
    `Isolated AI Team Manager acceptance fixture.\nSession marker: ${marker}\nOnly motor_control.c may be modified.\n`,
  );
  await fs.writeFile(
    path.join(project, "motor_control.c"),
    "int motor_limit(int speed) { return speed; }\n",
  );
  const protectedFiles = {
    "protocol.c": "int protocol_version(void) { return 1; }\n",
    "main.c": "int main(void) { return 0; }\n",
  };
  for (const [name, text] of Object.entries(protectedFiles))
    await fs.writeFile(path.join(project, name), text);
  let controller;
  const events = [],
    pids = [];
  function create() {
    const service = new ClaudeService(
      process.env.ATM_CLAUDE_EXECUTABLE || "claude",
      (command, args, cwd) => {
        const child = launch(command, args, cwd);
        if (child.pid) pids.push(child.pid);
        return child;
      },
      (command) =>
        resolveClaudeExecutable(command, {
          extensionDirectory: () => undefined,
        }),
    );
    const run = service.run.bind(service);
    service.run = (cwd, prompt, sid, emit, timeout, settings) =>
      run(
        cwd,
        prompt,
        sid,
        (event) => {
          events.push({ ...event, time: new Date().toISOString() });
          emit(event);
        },
        timeout,
        settings,
      );
    return new ClaudeController(
      new ClaudeStorage(path.join(dir, "storage"), project),
      service,
      () => {},
      async () => {},
      () => 180000,
      (taskId,sid)=>new PermissionHost(new PermissionPolicy(project,{allowedFiles:['motor_control.c'],safeCommands:[]}),taskId,async()=> 'DENY',async()=>{},90000,sid).start(),
    );
  }
  async function capture(step) {
    const state = controller.snapshot();
    const record = { step, time: new Date().toISOString(), ...state };
    records.push(record);
    await fs.writeFile(
      path.join(dir, "records.json"),
      JSON.stringify(records, null, 2),
    );
    await fs.writeFile(
      path.join(dir, "events.json"),
      JSON.stringify(events, null, 2),
    );
    console.log(
      JSON.stringify({
        step,
        status: state.conversation.status,
        session: state.conversation.threadId,
        error: state.error,
        events: events.length,
      }),
    );
    return state;
  }
  controller = create();
  await controller.initialize();
  await controller.check();
  assert(!controller.snapshot().error, controller.snapshot().error);
  await capture("CLI_CHECK");
  await controller.send(
    "这是隔离验收工程。只允许修改 motor_control.c，禁止修改 README.md、protocol.c、main.c，禁止执行终端命令。请使用 Read 工具读取 README.md 和 motor_control.c，再用 Edit 工具把 motor_limit 改成 speed 为负数时返回 0，否则返回 speed。不要更改其他文件。最后输出 README 中的 Session marker 及 CLAUDE_FILE_OK。",
  );
  const first = await capture("READ_AND_EDIT");
  assert.equal(first.conversation.status, "COMPLETED", first.error);
  assert(first.conversation.threadId);
  assert(
    first.conversation.messages.some(
      (m) => m.role === "assistant" && m.text.includes(marker),
    ),
    "Read result marker missing",
  );
  assert.match(
    await fs.readFile(path.join(project, "motor_control.c"), "utf8"),
    /0/,
  );
  assert(events.some((e) => e.type === "tool" && e.name === "Read"));
  assert(events.some((e) => e.type === "tool" && e.name === "Edit"));
  assert(
    events.some((e) => e.type === "text" && e.delta),
    "Must receive actual streaming deltas",
  );
  const session = first.conversation.threadId;
  controller = create();
  await controller.initialize();
  assert.equal(controller.snapshot().conversation.threadId, session);
  await controller.send(
    "不要使用工具或修改文件。请根据已有对话，回答刚才 README 的 Session marker，以及你对 motor_limit 做的修改。最后加上 CLAUDE_RESUME_OK。",
  );
  const restored = await capture("RESTART_AND_RESUME");
  assert.equal(restored.conversation.status, "COMPLETED", restored.error);
  assert.equal(restored.error, undefined);
  assert.equal(restored.conversation.threadId, session);
  assert(restored.conversation.messages.at(-1).text.includes(marker));
  assert(
    restored.conversation.messages.at(-1).text.includes("CLAUDE_RESUME_OK"),
  );
  await controller.newConversation();
  await controller.send(
    "不要使用工具或修改文件。只回答 CLAUDE_NEW_SESSION_OK。",
  );
  const fresh = await capture("NEW_SESSION");
  assert.equal(fresh.conversation.status, "COMPLETED", fresh.error);
  assert.notEqual(fresh.conversation.threadId, session);
  const before = events.length;
  const pending = controller.send(
    "不要使用任何工具或修改文件。请逐行输出从 1 到 2000 的整数，每行写整数后解释它的奇偶性，不要省略或总结。",
  );
  let sawDelta = false;
  for (let i = 0; i < 1200; i++) {
    if (events.slice(before).some((e) => e.type === "text" && e.delta)) {
      sawDelta = true;
      break;
    }
    if (!controller.snapshot().busy) break;
    await new Promise((r) => setTimeout(r, 100));
  }
  assert(sawDelta, "A real response must start before cancellation");
  await controller.stop();
  await pending;
  const stopped = await capture("STOP_STREAM");
  assert.equal(stopped.conversation.status, "CANCELLED");
  assert(
    stopped.conversation.messages.some((m) => m.role === "assistant" && m.text),
  );
  const hashes = {};
  for (const [name, text] of Object.entries(protectedFiles)) {
    const actual = await fs.readFile(path.join(project, name), "utf8");
    assert.equal(actual, text);
    hashes[name] = { before: hash(text), after: hash(actual) };
  }
  for (const pid of pids) {
    let alive = false;
    try {
      process.kill(pid, 0);
      alive = true;
    } catch {}
    assert(!alive, `CLI PID ${pid} must have exited`);
  }
  const result = {
    ok: true,
    modelCalls: 4,
    sessionRestored: true,
    newSessionIndependent: true,
    realStreaming: true,
    stopped: true,
    processIds: pids,
    noResidualProcesses: true,
    protectedFiles: hashes,
    finalCode: await fs.readFile(path.join(project, "motor_control.c"), "utf8"),
  };
  await fs.writeFile(
    path.join(dir, "result.json"),
    JSON.stringify(result, null, 2),
  );
  console.log("Real Claude acceptance passed:", dir);
  await require("./claude-real-faults.cjs").run(dir);
})().catch(async (error) => {
  await fs.mkdir(dir, { recursive: true });
  await fs.writeFile(
    path.join(dir, "failure.json"),
    JSON.stringify({ error: String(error), stack: error.stack }, null, 2),
  );
  console.error(String(error));
  console.error("Evidence:", dir);
  process.exitCode = 1;
});
