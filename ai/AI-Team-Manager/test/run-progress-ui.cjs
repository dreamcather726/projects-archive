const {buildSync}=require('esbuild'),{spawnSync}=require('node:child_process');
for(const [entry,out] of [['test/progress-api.ts','.tools/progress/api.cjs'],['test/task0710-api.ts','.tools/r1-ui/api.cjs']])buildSync({entryPoints:[entry],bundle:true,platform:'node',format:'cjs',outfile:out});
for(const args of [['--test','test/progress-ui.test.cjs'],['test/progress-browser.cjs']]){const r=spawnSync(process.execPath,args,{stdio:'inherit'});if(r.status!==0){process.exitCode=r.status??1;break;}}
