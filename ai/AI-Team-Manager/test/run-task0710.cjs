const {buildSync}=require('esbuild'),{spawnSync}=require('node:child_process');
buildSync({entryPoints:['test/task0710-api.ts'],bundle:true,platform:'node',format:'cjs',outfile:'.tools/task0710/api.cjs'});
const result=spawnSync(process.execPath,['--test','test/task0710.test.cjs'],{stdio:'inherit'});process.exitCode=result.status??1;
