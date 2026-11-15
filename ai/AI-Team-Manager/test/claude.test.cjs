const { test } = require("node:test"),
  assert = require("node:assert/strict"),
  fs = require("node:fs/promises"),
  path = require("node:path"),
  os = require("node:os");
const {
  ClaudeService,
  ClaudeController,
  ClaudeEventParser,
  ClaudeStorage,
  ConversationStorage,
  claudeArgs,
  resolveClaudeExecutable,
  launch,
} = require("../.tools/task02-test/api.cjs");
const helper = path.resolve("test/mock-claude.cjs"),
  sid = "25d7c0e0-71d5-465c-98a9-dcb8c8246387";
const service = () =>
  new ClaudeService("mock", (command, args, cwd) =>
    launch(process.execPath, [helper, ...args], cwd),
  );
test("Claude parser streams UTF8, deduplicates full assistant messages, keeps tool status and discards raw reasoning", () => {
  const events = [],
    parser = new ClaudeEventParser((e) => events.push(e));
  const rows = [
    { type: "system", subtype: "init", session_id: sid },
    {
      type: "stream_event",
      event: { type: "message_start", message: { id: "m" } },
    },
    {
      type: "stream_event",
      event: {
        type: "content_block_start",
        index: 0,
        content_block: { type: "text", text: "" },
      },
    },
    {
      type: "stream_event",
      event: {
        type: "content_block_delta",
        index: 0,
        delta: { type: "text_delta", text: "中文回复" },
      },
    },
    {
      type: "stream_event",
      event: {
        type: "content_block_delta",
        index: 1,
        delta: { type: "thinking_delta", thinking: "HIDDEN_REASONING" },
      },
    },
    {
      type: "assistant",
      message: {
        id: "m",
        content: [
          { type: "text", text: "中文回复" },
          {
            type: "tool_use",
            id: "t",
            name: "Edit",
            input: { file_path: "motor.c", new_string: "DO_NOT_LOG_CODE" },
          },
        ],
      },
    },
    {
      type: "user",
      message: {
        content: [
          { type: "tool_result", tool_use_id: "t", content: "DO_NOT_LOG_FILE" },
        ],
      },
    },
    {
      type: "result",
      subtype: "success",
      is_error: false,
      result: "中文回复",
      usage: {
        input_tokens: 5,
        cache_read_input_tokens: 3,
        cache_creation_input_tokens: 2,
        output_tokens: 4,
      },
    },
  ];
  const bytes = Buffer.from(rows.map((r) => JSON.stringify(r)).join("\n"));
  for (let i = 0; i < bytes.length; i += 3)
    parser.push(bytes.subarray(i, i + 3));
  parser.end();
  const texts = events.filter((e) => e.type === "text");
  assert.equal(texts.length, 2);
  assert.equal(texts[0].text, "中文回复");
  assert(texts[0].delta);
  assert.equal(texts[1].id, "m");
  assert(events.some((e) => e.type === "tool" && e.phase === "completed"));
  assert(!JSON.stringify(events).includes("HIDDEN_REASONING"));
  assert(!JSON.stringify(events).includes("DO_NOT_LOG"));
  assert.equal(events.at(-1).usage.total, 14);
});
test("Claude parser bounds JSONL and redacts secrets without logging malformed payloads", () => {
  const events = [],
    p = new ClaudeEventParser((e) => events.push(e));
  p.push("RAW_SECRET\n" + "x".repeat(2 * 1024 * 1024 + 1) + "\n");
  p.push(
    JSON.stringify({
      type: "result",
      subtype: "success",
      is_error: false,
      result: "password=topsecret Authorization: Bearer ABCDEFGHIJKLMNOPQRST",
      session_id: sid,
    }),
  );
  p.end();
  assert.equal(events.filter((e) => e.type === "parseError").length, 2);
  assert(!JSON.stringify(events).includes("topsecret"));
  assert(!JSON.stringify(events).includes("ABCDEFGHIJKLMNOPQRST"));
  assert.equal(events.at(-1).usage, undefined);
});
test("Claude uses stdin and explicit resume, manual permissions, and no permission bypass", () => {
  const args = claudeArgs(sid);
  assert(args.includes("--resume"));
  assert(args.includes(sid));
  assert(args.includes("manual"));
  assert(args.includes("none"));
  assert.equal(args[args.indexOf('--tools')+1],'Read,Glob,Grep');
  const gated=claudeArgs(sid,'{"hooks":{}}');
  assert(gated.includes('--settings'));assert(gated.includes('Read,Edit,Write,Glob,Grep,Bash'));
  assert(!args.some((a) => /skip-permission|bypassPermissions/.test(a)));
  assert(!args.includes("--continue"));
  assert.throws(() => claudeArgs("bad ID; touch x"));
});
test("Claude service runs real subprocesses and classifies auth/network/permission/parse/crash/resume failures", async () => {
  const s = service();
  await s.check(process.cwd());
  const events = [];
  await s.run(
    process.cwd(),
    "中文 input $(echo secret)",
    undefined,
    (e) => events.push(e),
    10000,
  ).done;
  assert(
    events.some((e) => e.type === "text" && e.text.includes("$(echo secret)")),
  );
  for (const [prompt, code, session] of [
    ["AUTH", "AUTHENTICATION_FAILED"],
    ["NETWORK", "NETWORK_ERROR"],
    ["DENIED", "PERMISSION_DENIED"],
    ["MALFORMED", "OUTPUT_PARSE_FAILED"],
    ["CRASH", "PROCESS_EXIT_NONZERO"],
    ["BAD_RESUME", "SESSION_RESUME_FAILED", sid],
    ["WRONG_SESSION", "SESSION_RESUME_FAILED", sid],
  ])
    await assert.rejects(
      s.run(process.cwd(), prompt, session, () => {}, 10000).done,
      (e) => e.code === code && !e.message.includes("raw secret"),
      prompt,
    );
});
test("Claude stop and timeout terminate the corresponding process and preserve partial output", async () => {
  const s = service(),
    events = [];
  let request;
  let seen;
  const started = new Promise((r) => (seen = r));
  request = s.run(
    process.cwd(),
    "HANG",
    undefined,
    (e) => {
      events.push(e);
      if (e.type === "text") seen();
    },
    10000,
  );
  await started;
  const rejected = assert.rejects(
    request.done,
    (e) => e.code === "REQUEST_CANCELLED",
  );
  await request.stop();
  await rejected;
  assert(events.some((e) => e.type === "text"));
  await assert.rejects(
    s.run(process.cwd(), "HANG", undefined, () => {}, 200).done,
    (e) => e.code === "REQUEST_TIMEOUT",
  );
});
test("missing Claude command fails clearly and explicit path never falls back", async () => {
  const missing = new ClaudeService(
    path.join(os.tmpdir(), "missing-claude-cli.exe"),
  );
  await assert.rejects(
    missing.check(process.cwd()),
    (e) => e.code === "CLAUDE_NOT_FOUND",
  );
  assert.equal(
    (
      await resolveClaudeExecutable("C:/missing.exe", {
        extensionDirectory: () => undefined,
      })
    ).source,
    "settings",
  );
  await assert.rejects(
    resolveClaudeExecutable("claude", {
      env: { PATH: "" },
      extensionDirectory: () => undefined,
    }),
    (e) => e.code === "CLAUDE_NOT_FOUND",
  );
});
test("Claude supports Windows .cmd paths with spaces and reports startup permission errors", async () => {
  if (process.platform === "win32") {
    const dir = await fs.mkdtemp(path.join(os.tmpdir(), "atm claude cmd ")),
      file = path.join(dir, "claude shim.cmd");
    await fs.writeFile(
      file,
      `@echo off\r\n"${process.execPath}" "${helper}" %*\r\n`,
    );
    const service = new ClaudeService(file);
    await service.check(process.cwd());
    const events = [];
    await service.run(
      process.cwd(),
      "CMD_LITERAL $(not_a_command)",
      undefined,
      (e) => events.push(e),
      10000,
    ).done;
    assert(
      events.some(
        (e) => e.type === "text" && e.text === "CMD_LITERAL $(not_a_command)",
      ),
    );
  }
  const { EventEmitter } = require("node:events"),
    { PassThrough } = require("node:stream");
  const denied = new ClaudeService("denied", () => {
    const child = Object.assign(new EventEmitter(), {
      stdout: new PassThrough(),
      stderr: new PassThrough(),
      stdin: new PassThrough(),
    });
    queueMicrotask(() =>
      child.emit(
        "error",
        Object.assign(new Error("secret raw error"), { code: "EACCES" }),
      ),
    );
    return child;
  });
  await assert.rejects(
    denied.check(process.cwd()),
    (e) =>
      e.code === "PERMISSION_DENIED" && !e.message.includes("secret raw error"),
  );
});
test("Claude controller preserves same Session across restart, isolates storage and new conversations", async () => {
  const dir = await fs.mkdtemp(path.join(os.tmpdir(), "atm-claude-")),
    project = path.join(dir, "project");
  await fs.mkdir(project);
  const make = () =>
    new ClaudeController(
      new ClaudeStorage(path.join(dir, "claude"), project),
      service(),
      () => {},
      async () => {},
      () => 10000,
    );
  const a = make();
  await a.initialize();
  await a.send("FIRST");
  assert.equal(a.snapshot().error, undefined);
  const first = a.snapshot().conversation.threadId;
  const b = make();
  await b.initialize();
  assert.equal(b.snapshot().conversation.threadId, first);
  await b.send("SECOND");
  assert.equal(b.snapshot().conversation.threadId, first);
  assert.equal(b.snapshot().conversation.requests.length, 2);
  await b.newConversation();
  await b.send("NEW");
  assert.notEqual(b.snapshot().conversation.threadId, first);
  assert.equal(await new ConversationStorage(dir, project).load(), undefined);
  const old = b.snapshot().conversation.threadId;
  await b.send("BAD_RESUME");
  assert.equal(b.snapshot().conversation.threadId, old);
  assert.equal(b.snapshot().conversation.status, "FAILED");
  assert(b.snapshot().error.includes("SESSION_RESUME_FAILED"));
});
test("Claude storage serializes retries and immutable snapshots, preserving corrupt records", async () => {
  const dir = await fs.mkdtemp(path.join(os.tmpdir(), "atm-claude-store-")),
    store = new ClaudeStorage(dir, dir);
  const original = ConversationStorage.prototype.save;
  let attempts = 0;
  const order = [];
  ConversationStorage.prototype.save = async function (data) {
    order.push(data.activeId);
    if (attempts++ < 2) throw new Error("transient Windows lock");
    return original.call(this, data);
  };
  try {
    const data = {
      version: 1,
      projectPath: dir,
      activeId: "a",
      conversations: [],
    };
    const a = store.save(data);
    data.activeId = "b";
    const b = store.save(data);
    data.activeId = "mutated";
    await Promise.all([a, b]);
    assert.deepEqual(order, ["a", "a", "a", "b"]);
    assert.equal((await store.load()).activeId, "b");
  } finally {
    ConversationStorage.prototype.save = original;
  }
  const files = await fs.readdir(path.join(dir, "conversations"));
  const file = path.join(dir, "conversations", files[0]);
  await fs.writeFile(file, "corrupt");
  await assert.rejects(store.load());
  assert.equal(await fs.readFile(file, "utf8"), "corrupt");
});
