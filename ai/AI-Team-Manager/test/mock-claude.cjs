// Fault-injection process, never presented as a real Claude session.
const { randomUUID } = require("node:crypto");
const args = process.argv.slice(2);
if (args.includes("--version")) {
  console.log("2.1.322 (Claude Code)");
  process.exit(0);
}
if (args.includes("--help")) {
  console.log(
    "--print --verbose --output-format --include-partial-messages --permission-mode manual --permission-prompts --resume --strict-mcp-config --settings --bare --disable-slash-commands --no-chrome --allowedTools --restricted",
  );
  process.exit(0);
}
let prompt = "";
process.stdin.setEncoding("utf8");
process.stdin.on("data", (text) => (prompt += text));
process.stdin.on("end", async () => {
  const resume = args.includes("--resume"),
    session = resume ? args[args.indexOf("--resume") + 1] : randomUUID();
  if (prompt === "BAD_RESUME") {
    process.stderr.write("No conversation found with session ID.");
    process.exit(1);
  }
  const id = prompt === "WRONG_SESSION" ? randomUUID() : session;
  const emit = (o) => console.log(JSON.stringify({ session_id: id, ...o }));
  const fault=process.env.ATM_MOCK_RETRY ? JSON.parse(process.env.ATM_MOCK_RETRY) : undefined;
  if(fault?.noSession){
    console.log(JSON.stringify({type:'result',subtype:'error_during_execution',is_error:true,errors:['API Error: 503 No available channel for model test-model']}));process.exitCode=1;return;
  }
  emit({ type: "system", subtype: "init" });
  if(fault){
    const zero={input_tokens:0,cache_read_input_tokens:0,cache_creation_input_tokens:0,output_tokens:0};
    const error=fault.error||'API Error: 503 No available channel for model test-model';
    if(fault.text)emit({type:'assistant',message:{id:'partial',content:[{type:'text',text:fault.text}]}});
    if(fault.tool)emit({type:'assistant',message:{id:'tool-message',content:[{type:'tool_use',id:'tool-attempt',name:'Read',input:{file_path:'source.py'}}]}});
    if(fault.write)require('node:fs').writeFileSync(require('node:path').join(process.cwd(),fault.write),'changed by fault fixture');
    if(fault.partialUsage)emit({type:'stream_event',event:{type:'message_delta',usage:{input_tokens:1}}});
    if(fault.thinking)emit({type:'stream_event',event:{type:'content_block_start',index:0,content_block:{type:'thinking',thinking:'PRIVATE_REASONING'}}});
    if(fault.child)emit({type:'assistant',parent_tool_use_id:'child',message:{content:[{type:'text',text:'PRIVATE_CHILD'}]}});
    if(fault.parse)console.log('malformed fixture output');
    if(fault.wrongSession)console.log(JSON.stringify({type:'system',subtype:'init',session_id:randomUUID()}));
    if(fault.permission){const config=JSON.parse(args[args.indexOf('--settings')+1]),url=config.hooks.PreToolUse[0].hooks[0].url;const response=await fetch(url,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({hook_event_name:'PreToolUse',session_id:id,cwd:process.cwd(),tool_name:'Read',tool_input:{file_path:'source.py'}})});await response.text();}
    if(fault.gateway){const config=JSON.parse(args[args.indexOf('--mcp-config')+1]).mcpServers.atm_worker;const response=await fetch(config.url,{method:'POST',headers:{...config.headers,'Content-Type':'application/json'},body:JSON.stringify({jsonrpc:'2.0',id:1,method:'tools/call',params:{name:fault.gatewayWrite?'write_task_file':'list_task_files',arguments:fault.gatewayWrite?{file_path:fault.gatewayWrite,content:'written after reconnect'}:{}}})});const answer=await response.json();if(fault.gatewayWrite&&answer.error){emit({type:'result',subtype:'error_during_execution',is_error:true,permission_denials:[{tool_name:'write_task_file'}],errors:['permission denied by gateway']});process.exitCode=1;return;}}
    if(fault.success){emit({type:'assistant',message:{id:'success',content:[{type:'text',text:prompt}]}});emit({type:'result',subtype:'success',is_error:false,result:prompt,usage:{...zero,input_tokens:1,output_tokens:1}});return;}
    if(fault.hang){setInterval(()=>{},1000);return;}
    if(fault.diagnostic)emit({type:'assistant',error:'api_error',message:{id:'api-error',content:[{type:'text',text:error}]}});
    emit({type:'result',subtype:'error_during_execution',is_error:true,...(fault.diagnostic?{result:error}:{}),errors:[error],permission_denials:fault.denied?[{tool_name:'Read'}]:[],...(fault.modelUsage?{modelUsage:{mock:{inputTokens:5}}}:{}),...(fault.cost?{total_cost_usd:0.001}:{}),...(fault.usage==='missing'?{}:{usage:fault.usage==='invalid'?{input_tokens:-1}:fault.usage==='partial'?{input_tokens:0}:{...zero,...(fault.usage==='positive'?{input_tokens:1}:{})}})});
    process.exitCode=1;return;
  }
  if (prompt === "AUTH" || prompt === "NETWORK") {
    process.stderr.write("password=SECRET_SHOULD_NOT_APPEAR");
    emit({
      type: "result",
      subtype: "error_during_execution",
      is_error: true,
      errors: [
        prompt === "AUTH" ? "401 authentication failed" : "ECONNREFUSED",
      ],
    });
    process.exit(1);
  }
  if (prompt === "CRASH") {
    process.stderr.write("raw secret");
    process.exit(17);
  }
  if (prompt === "MALFORMED") console.log("raw invalid secret payload");
  emit({
    type: "stream_event",
    event: { type: "message_start", message: { id: "message-one" } },
  });
  emit({
    type: "stream_event",
    event: {
      type: "content_block_start",
      index: 0,
      content_block: { type: "text", text: "" },
    },
  });
  emit({
    type: "stream_event",
    event: {
      type: "content_block_delta",
      index: 0,
      delta: { type: "text_delta", text: prompt },
    },
  });
  if (prompt === "HANG") {
    setInterval(() => {}, 1000);
    return;
  }
  emit({
    type: "assistant",
    message: { id: "message-one", content: [{ type: "text", text: prompt }] },
  });
  emit({
    type: "result",
    subtype: "success",
    is_error: false,
    result: prompt,
    permission_denials: prompt === "DENIED" ? [{ tool_name: "Bash" }] : [],
    usage: {
      input_tokens: 5,
      cache_read_input_tokens: 7,
      cache_creation_input_tokens: 2,
      output_tokens: 3,
    },
  });
});
