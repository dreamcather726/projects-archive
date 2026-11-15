const esbuild = require('esbuild');
const path = require('node:path');
const { spawnSync } = require('node:child_process');
esbuild.buildSync({entryPoints:['test/api.ts'],bundle:true,platform:'node',format:'cjs',outfile:'.tools/task02-test/api.cjs'});
esbuild.buildSync({entryPoints:['test/retry-api.ts'],bundle:true,platform:'node',format:'cjs',outfile:'.tools/retry/api.cjs'});
const result=spawnSync(process.execPath,['--test','test/claude-retry.test.cjs','test/task02.test.cjs','test/webview.test.cjs','test/workspace.test.cjs','test/claude.test.cjs','test/review.test.cjs','test/permissions.test.cjs','test/repair.test.cjs','test/dispatch.test.cjs','test/planning.test.cjs','test/workers.test.cjs','test/isolation.test.cjs','test/protection.test.cjs','test/scheduler.test.cjs','test/usage.test.cjs','test/autonomy.test.cjs','test/integration.test.cjs'],{stdio:'inherit',env:process.env});
process.exitCode=result.status ?? 1;
