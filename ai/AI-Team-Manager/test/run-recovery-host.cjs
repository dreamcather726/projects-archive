const fs=require('node:fs'),path=require('node:path'),os=require('node:os');
const {resolveCodexExecutable,resolveClaudeExecutable}=require('../.tools/recovery/api.cjs');
(async()=>{
 const base=path.join(os.homedir(),'.vscode','extensions'),names=fs.readdirSync(base),directory=prefix=>path.join(base,names.filter(n=>n.startsWith(prefix)).sort((a,b)=>b.localeCompare(a,undefined,{numeric:true}))[0]);
 process.env.ATM_HOST_CODEX_EXE=(await resolveCodexExecutable('codex',{env:{PATH:''},extensionDirectory:()=>directory('openai.chatgpt-')})).command;
 process.env.ATM_HOST_CLAUDE_EXE=(await resolveClaudeExecutable('claude',{env:{PATH:''},extensionDirectory:()=>directory('anthropic.claude-code-')})).command;
 process.env.ATM_HOST_CODEX_REAL='1';process.env.ATM_HOST_TEST_MODULE=path.resolve('test/recovery-host.cjs');process.env.ATM_HOST_LABEL='codex-recovery-host';process.env.ATM_HOST_TIMEOUT_MS='240000';
 process.env.ATM_HOST_VSIX=process.env.ATM_HOST_VSIX||path.resolve('test-results','ai-team-manager-'+require('../package.json').version+'.vsix');require('./run-host.cjs');
})().catch(e=>{console.error(e);process.exitCode=1;});
