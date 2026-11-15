const {buildSync}=require('esbuild'),{spawnSync}=require('node:child_process');
buildSync({entryPoints:['test/retry-api.ts'],bundle:true,platform:'node',format:'cjs',outfile:'.tools/retry/api.cjs'});
const r=spawnSync(process.execPath,['--test','test/claude-retry.test.cjs'],{stdio:'inherit'});process.exitCode=r.status??1;
