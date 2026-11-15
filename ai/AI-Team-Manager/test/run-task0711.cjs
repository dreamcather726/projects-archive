const {buildSync}=require('esbuild'),{spawnSync}=require('node:child_process');
buildSync({entryPoints:['test/task0711-api.ts'],bundle:true,platform:'node',format:'cjs',outfile:'.tools/task0711/api.cjs'});
const r=spawnSync(process.execPath,['--test','test/task0711.test.cjs'],{stdio:'inherit'});process.exitCode=r.status??1;
