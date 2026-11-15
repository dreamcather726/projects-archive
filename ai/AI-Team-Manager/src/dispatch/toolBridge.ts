import * as http from 'node:http';
import { randomBytes, timingSafeEqual } from 'node:crypto';
import type { CodexRunOptions } from '../process/cliLauncher';

const str = { type:'string', minLength:1, maxLength:12000 };
const task = { type:'string', minLength:1, maxLength:128 };
export interface BridgeTool { name:string; description:string; inputSchema:object }
export const dispatcherTools = [
  { name:'dispatch_claude_task', description:'Actually dispatch a development task to Claude Worker 01. Returns task_id immediately. Requires exact relative allowed_files and exact test_commands. Poll output until terminal; never claim success without evidence.', inputSchema:{type:'object',properties:{description:str,allowed_files:{type:'array',items:str,maxItems:100},forbidden_files:{type:'array',items:str,maxItems:100},expected_commands:{type:'array',items:str,maxItems:10},test_commands:{type:'array',items:str,maxItems:10},acceptance_criteria:{type:'array',items:str,maxItems:30}},required:['description','allowed_files','forbidden_files','test_commands','acceptance_criteria'],additionalProperties:false}},
  { name:'get_claude_task_status', description:'Get real task status and review result.',inputSchema:{type:'object',properties:{task_id:task},required:['task_id'],additionalProperties:false}},
  { name:'get_claude_task_output', description:'Get actual Claude output/tool events/diff/test results/review. wait_ms up to 25000 waits for new events or completion. Continue polling until task is terminal.', inputSchema:{type:'object',properties:{task_id:task,cursor:{type:'integer',minimum:0},wait_ms:{type:'integer',minimum:0,maximum:25000}},required:['task_id'],additionalProperties:false}},
  { name:'cancel_claude_task',description:'Cancel a task, its Claude process, tests, approvals and GPT review.',inputSchema:{type:'object',properties:{task_id:task},required:['task_id'],additionalProperties:false}},
  { name:'get_claude_session',description:'Get the real active Claude Session ID and busy status.',inputSchema:{type:'object',properties:{},additionalProperties:false}},
];

/** Minimal stateless Streamable HTTP MCP server. No SSE/notifications emitted. Bound to one GPT turn. */
export class ToolBridge {
  private server?: http.Server;
  private token = randomBytes(32).toString('hex');
  private closed = false;
  private pendingRequests = 0;
  private toolCallSeen = false;
  /** Handshakes are harmless; incomplete HTTP requests and any tools/call forbid replay. */
  canRetry() { return !this.closed && this.pendingRequests===0 && !this.toolCallSeen; }
  constructor(private call:(name:string,args:any)=>Promise<unknown>, private onClose:()=>Promise<void> = async()=>{}, private tools:BridgeTool[]=dispatcherTools) {}
  async start():Promise<CodexRunOptions> {
    this.server=http.createServer((req,res)=>{
      const address=this.server?.address();
      const auth=Buffer.from(req.headers.authorization || '');
      const expected=Buffer.from(`Bearer ${this.token}`);
      if(this.closed || !address || typeof address==='string' || req.headers.host!==`127.0.0.1:${address.port}` || req.url!=='/mcp' || req.headers.origin || auth.length!==expected.length || !timingSafeEqual(auth,expected)) {res.writeHead(403).end();return;}
      if(req.method!=='POST'){res.writeHead(405).end();return;}
      this.pendingRequests++;
      let finished=false;
      const finish=()=>{if(!finished){finished=true;this.pendingRequests--;}};
      res.once('finish',finish);res.once('close',finish);
      const chunks:Buffer[]=[];let size=0;
      req.on('data',b=>{size+=b.length;if(size>256000){res.writeHead(413).end();req.destroy();}else chunks.push(b);});
      req.on('error',()=>{});
      req.on('end',()=>{void(async()=>{
        let data:any;
        try {
          data=JSON.parse(Buffer.concat(chunks).toString('utf8'));
          if(data.jsonrpc!=='2.0' || typeof data.method!=='string')throw Error('Invalid JSON-RPC');
          if(data.method==='tools/call')this.toolCallSeen=true;
          if(data.id===undefined){res.writeHead(202).end();return;}
          let result:unknown;
          if(data.method==='initialize')result={protocolVersion:'2025-03-26',capabilities:{tools:{}},serverInfo:{name:'AI Team Manager Dispatcher',version:'1.0.0'},instructions:'Use only the registered tools. Planning is not execution or authorization. Only report actual tool results. Never use shell to write or bypass approvals.'};
          else if(data.method==='ping')result={};
          else if(data.method==='tools/list')result={tools:this.tools};
          else if(data.method==='tools/call'){
            if(!this.tools.some(t=>t.name===data.params?.name))throw Error('Unknown tool');
            const output=await this.call(data.params.name,data.params.arguments || {});
            result={content:[{type:'text',text:JSON.stringify(output)}],isError:false};
          }else throw Error('Method not supported');
          if(!res.destroyed)res.writeHead(200,{'Content-Type':'application/json'}).end(JSON.stringify({jsonrpc:'2.0',id:data.id,result}));
        }catch(e){if(!res.destroyed)res.writeHead(200,{'Content-Type':'application/json'}).end(JSON.stringify({jsonrpc:'2.0',id:data?.id??null,error:{code:-32603,message:e instanceof Error?e.message:'Tool bridge error'}}));}
      })();});
    });
    await new Promise<void>((resolve,reject)=>{this.server!.once('error',reject);this.server!.listen(0,'127.0.0.1',resolve);});
    const port=(this.server.address() as any).port;
    return {env:{ATM_DISPATCH_CAPABILITY:this.token},config:[
      `mcp_servers.ai_team_manager.url="http://127.0.0.1:${port}/mcp"`,
      'mcp_servers.ai_team_manager.bearer_token_env_var="ATM_DISPATCH_CAPABILITY"',
      'mcp_servers.ai_team_manager.required=true',
      'mcp_servers.ai_team_manager.tool_timeout_sec=90',
      // The plugin enforces task/tool approvals. Only these five bounded gateway tools bypass a second CLI prompt.
      'mcp_servers.ai_team_manager.default_tools_approval_mode="approve"',
    ]};
  }
  async close(){if(this.closed)return;this.closed=true;await this.onClose();const s=this.server;this.server=undefined;this.token='';if(s){s.closeAllConnections();await new Promise<void>(resolve=>s.close(()=>resolve()));}}
  async startClaude(): Promise<string> {
    await this.start();
    const address=this.server!.address() as {port:number};
    return JSON.stringify({mcpServers:{atm_worker:{type:'http',url:`http://127.0.0.1:${address.port}/mcp`,headers:{Authorization:`Bearer ${this.token}`}}}});
  }
}
