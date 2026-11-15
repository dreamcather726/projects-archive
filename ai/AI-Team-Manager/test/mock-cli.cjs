const fs = require('node:fs');
const { spawn } = require('node:child_process');
const args = process.argv.slice(2);
if (args.includes('--version')) {console.log('codex-cli mock');process.exit(0);}
if (args.includes('--help')) {console.log('--json --sandbox read-only --cd --config SESSION_ID stdin');process.exit(0);}
let input = '';
process.stdin.setEncoding('utf8');
process.stdin.on('data',chunk => input += chunk);
process.stdin.on('end',() => {
  const resume = args.includes('resume');
  const id = resume ? args[args.indexOf('resume') + 1] : 'mock-thread-1';
  const emit = e => process.stdout.write(JSON.stringify(e)+'\n');
  if(input==='route'){console.error('connection refused http://localhost:9999/?api_key=secret123');process.exit(1);}
  if(input==='auth'){console.error('401 Unauthorized authorization=secret123');process.exit(1);}
  if(input==='bad-resume'){console.error('session not found secret123');process.exit(1);}
  if(input==='sandbox'){console.error('sandbox setup failed secret123');process.exit(1);}
  emit({type:'thread.started',thread_id:id});emit({type:'turn.started'});
  if (input.startsWith('slow:')) {
    const child = spawn(process.execPath,['-e','setInterval(()=>{},1000)'],{stdio:'ignore'});
    fs.writeFileSync(input.slice(5),String(child.pid));
    emit({type:'item.updated',item:{id:'reply',type:'agent_message',text:'已收到部分内容'}});
    setInterval(()=>{},1000);return;
  }
  if(input==='malformed')process.stdout.write('invalid-json secret123\n');
  emit({type:'item.completed',item:{id:'reason',type:'reasoning',text:'PRIVATE_REASONING'}});
  emit({type:'item.completed',item:{id:'reply',type:'agent_message',text:resume?'CODEX_CONNECT_OK':input}});
  emit({type:'turn.completed',usage:{input_tokens:resume?40:20,cached_input_tokens:resume?20:10,output_tokens:resume?6:3}});
});
