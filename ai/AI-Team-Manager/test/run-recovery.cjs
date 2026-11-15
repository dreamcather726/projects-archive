const {buildSync}=require('esbuild'),{spawnSync}=require('node:child_process');
buildSync({entryPoints:['test/recovery-api.ts'],bundle:true,platform:'node',format:'cjs',outfile:'.tools/recovery/api.cjs'});
const r=spawnSync(process.execPath,['--test','test/recovery.test.cjs','test/environment.test.cjs'],{stdio:'inherit'});process.exitCode=r.status??1;
