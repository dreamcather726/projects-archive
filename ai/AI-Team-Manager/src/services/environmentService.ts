import * as os from 'node:os';
import { launch, type Launcher } from '../process/cliLauncher';
import { terminateTree } from '../process/processManager';
import { resolveCodexExecutable, type ResolvedCodex } from '../process/codexExecutableResolver';
import { resolveClaudeExecutable, type ResolvedClaude } from '../claude/claudeExecutableResolver';
import { CodexError, redact } from './codexErrors';
import { ClaudeError } from '../claude/claudeErrors';
export interface Diagnostic { name:string;ok:boolean;detail:string;command?:string;source?:string }
export interface DiagnosticOptions {
  codex?:()=>Promise<ResolvedCodex>;
  claude?:()=>Promise<ResolvedClaude>;
  launcher?:Launcher;
}
const commands=[['Node.js','node'],['npm','npm'],['Git','git'],['Codex CLI','codex'],['Claude Code CLI','claude']];
/** Use the chat adapters' discovery rules; only --version is executed, never a model request. */
export async function diagnose(version:string,timeout:number,options:DiagnosticOptions={}):Promise<Diagnostic[]> {
  const limit=Number.isFinite(timeout)?Math.max(100,Math.min(60000,timeout)):10000;
  const cwd=process.platform==='win32'?process.env.SystemRoot||'C:\\Windows':os.tmpdir();
  return [{name:'VS Code',ok:true,detail:version},...await Promise.all(commands.map(async([name,command]):Promise<Diagnostic>=>{
    let selected:{command:string;source:string}={command,source:'path'};
    try {
      if(command==='codex')selected=await (options.codex?.()||resolveCodexExecutable('codex',{extensionDirectory:()=>undefined}));
      if(command==='claude')selected=await (options.claude?.()||resolveClaudeExecutable('claude',{extensionDirectory:()=>undefined}));
    } catch(error) {
      return {name,ok:false,detail:error instanceof CodexError||error instanceof ClaudeError?error.message:'无法定位 CLI，请检查已有可执行文件设置或扩展状态。'};
    }
    return new Promise<Diagnostic>(resolve=>{
      let child;
      const source=selected.source==='extension'?'已安装扩展':selected.source==='settings'?'插件设置':'PATH';
      const displayCommand=redact(selected.command);
      const facts={name,command:displayCommand,source:selected.source};
      const detail=(text:string)=>text+'\n来源：'+source+'\n路径：'+displayCommand;
      try {child=(options.launcher||launch)(selected.command,['--version'],cwd);}
      catch {resolve({...facts,ok:false,detail:detail('版本检测无法启动，请检查当前可执行文件。')});return;}
      let bytes=Buffer.alloc(0),settled=false,timedOut=false;
      const finish=(ok:boolean,text:string)=>{if(settled)return;settled=true;clearTimeout(timer);resolve({...facts,ok,detail:detail(text)});};
      const timer=setTimeout(()=>{timedOut=true;void terminateTree(child).then(()=>finish(false,'版本检测超时。'),()=>finish(false,'版本检测超时，进程结束状态无法确认。'));},limit);
      const capture=(chunk:Buffer)=>{if(bytes.length<65536)bytes=Buffer.concat([bytes,Buffer.from(chunk).subarray(0,65536-bytes.length)]);};
      child.stdout?.on('data',capture);
      // stderr may contain credentials/config diagnostics; never put raw stderr in logs.
      child.stderr?.on('data',()=>{});
      child.stdin?.on('error',()=>{});child.stdin?.end();
      child.on('error',(error:NodeJS.ErrnoException)=>finish(false,'版本检测无法启动（'+(/^E[A-Z0-9_]+$/.test(error.code||'')?error.code:'UNKNOWN')+'），请检查当前可执行文件。'));
      child.on('close',code=>{
        if(timedOut){finish(false,'版本检测超时。');return;}
        const output=redact(bytes.toString('utf8').trim()).slice(0,2000);
        finish(code===0&&!!output,code!==0?'版本检测失败（退出码 '+code+'）。':output||'未返回版本信息。');
      });
    });
  }))];
}
