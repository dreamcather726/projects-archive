// Actual GPT verification of fault-injected source. No fake model output or claimed Claude execution.
const fs=require('node:fs/promises'),path=require('node:path'),assert=require('node:assert/strict');
const {ReviewGit,ReviewStorage,ReviewEngine,ReviewTestRunner,CodexService,resolveChecks}=require('../.tools/task02-test/api.cjs');
const {create,sample}=require('./repair-fixture.cjs');
(async()=>{
  if(!process.argv[2])throw Error('Supply completed real Task05 host evidence directory');
  const host=path.resolve(process.argv[2]),record=JSON.parse(await fs.readFile(path.join(host,'repair-record.json'),'utf8')),original=JSON.parse(await fs.readFile(path.join(host,'original-review.json'),'utf8'));
  const dir=path.resolve('.tools',`task05-verification-${Date.now()}`);await fs.mkdir(dir,{recursive:true});const results=[];
  for(const expected of ['NOT_FIXED','REGRESSION','UNVERIFIED']){
    const reuse=expected==='NOT_FIXED'?process.argv[3]:expected==='REGRESSION'?process.argv[4]:undefined;
    if (reuse) {
      const report=JSON.parse(await fs.readFile(path.resolve(reuse),'utf8'));
      assert.equal((await new ReviewGit(path.join(dir,'temp')).snapshot(report.project_path)).digest,report.evidence.after.digest);
      const checks=resolveChecks({plan:record.plan,original_issues:record.original_issues,event_gaps:[]},report);
      assert.equal(checks[0].status,expected);assert(report.batches.some(b=>b.session_id&&b.usage));
      await fs.writeFile(path.join(dir,expected+'-report.json'),JSON.stringify(report,null,2));
      results.push({scenario:expected,actual:checks[0].status,checks,verdict:report.verdict,review_id:report.review_id,realGPT:true,test:report.evidence.tests[0].status,usage:report.batches.map(b=>b.usage),sourceChangedBy:'TEST_HARNESS_FAULT_INJECTION_NOT_CLAUDE',reusedActualGPTReport:reuse});continue;
    }
    const root=path.join(dir,expected);await create(root);await fs.writeFile(path.join(root,'worker.cjs'),sample.after);
    const git=new ReviewGit(path.join(dir,'temp')),storage=new ReviewStorage(path.join(dir,'history')),before=await git.snapshot(root);
    const code=expected==='NOT_FIXED'?sample.after:expected==='REGRESSION'?sample.after.replace('Math.abs(value)','0').replace('Math.min(100, value)','value'):sample.after.replace('Math.abs(value)','0');
    await fs.writeFile(path.join(root,'worker.cjs'),code);
    const after=await git.snapshot(root),tests=expected==='UNVERIFIED'?[]:[await new ReviewTestRunner(git).run(root,'node --test')];
    const evidence=await git.evidence(before,after,{...original.task,repair_context:{repair_id:record.repair_id,review_id:record.review_id,issues:record.original_issues,plan:record.plan}},
      {status:'FAULT_INJECTION_TEST',final_answer:'测试程序直接写入场景代码；本场景没有Claude执行或用户审批，不能把此说明当作修复或测试证据。'},tests,'CLAUDE_CAPTURE');
    evidence.builds=[await new ReviewTestRunner(git).run(root,'npm run compile')];
    if(expected==='NOT_FIXED'){evidence.inspection_files=after.files.filter(f=>f.path==='worker.cjs');evidence.gaps=evidence.gaps.filter(g=>g!=='没有可审查的实际文本变更');}
    const engine=new ReviewEngine(new CodexService(),git,storage,()=>console.log(expected,engine.progress));
    const report=await engine.run(evidence),checks=resolveChecks({plan:record.plan,original_issues:record.original_issues,event_gaps:[]},report);
    await fs.writeFile(path.join(dir,expected+'-report.json'),JSON.stringify(report,null,2));
    results.push({scenario:expected,actual:checks[0].status,checks,verdict:report.verdict,review_id:report.review_id,realGPT:report.batches.some(b=>b.session_id&&b.usage),test:tests[0]?.status||'NOT_RUN',test_count:tests[0]?.test_count,usage:report.batches.map(b=>b.usage),sourceChangedBy:'TEST_HARNESS_FAULT_INJECTION_NOT_CLAUDE',approval:'NOT_APPLICABLE_NO_CLAUDE_DISPATCH'});
    await fs.writeFile(path.join(dir,'results.json'),JSON.stringify(results,null,2));assert(results.at(-1).realGPT,'Must call actual GPT');assert.equal(checks[0].status,expected,JSON.stringify(results.at(-1)));assert.equal((await git.snapshot(root)).digest,after.digest,'Reviewer must not modify code');
  }
  console.log('Actual GPT verification scenarios passed:',dir);
})().catch(e=>{console.error(e);process.exitCode=1;});
