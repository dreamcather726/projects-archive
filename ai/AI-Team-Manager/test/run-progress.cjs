const {buildSync}=require('esbuild'),{spawnSync}=require('node:child_process');
buildSync({entryPoints:['test/progress-api.ts'],bundle:true,platform:'node',format:'cjs',outfile:'.tools/progress/api.cjs'});
const r=spawnSync(process.execPath,['--test','test/progress.test.cjs'],{stdio:'inherit'});process.exitCode=r.status??1;
