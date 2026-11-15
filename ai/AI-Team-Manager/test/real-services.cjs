// Discover an installed CLI without reading credentials or changing routing.
const fs=require('node:fs'),path=require('node:path'),os=require('node:os');
const {ClaudeService,resolveClaudeExecutable,CodexService}=require('../.tools/task02-test/api.cjs');
function claude(){const extensions=path.join(os.homedir(),'.vscode','extensions');const installed=fs.existsSync(extensions)?fs.readdirSync(extensions).filter(n=>/^anthropic\.claude-code-/.test(n)).sort((a,b)=>b.localeCompare(a,undefined,{numeric:true})):[];return new ClaudeService(process.env.ATM_CLAUDE_EXECUTABLE||'claude',undefined,c=>resolveClaudeExecutable(c,{extensionDirectory:()=>installed[0]?path.join(extensions,installed[0]):undefined}));}
module.exports={claude,codex:()=>new CodexService(process.env.ATM_CODEX_EXECUTABLE||'codex')};
