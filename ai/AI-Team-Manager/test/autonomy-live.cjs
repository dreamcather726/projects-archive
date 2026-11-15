const fs=require('node:fs/promises'),path=require('node:path'),assert=require('node:assert/strict');
const {TeamTaskExecutor,IsolatedTestRunner,git}=require('../.tools/task02-test/api.cjs');const {claude}=require('./real-services.cjs');
(async()=>{
 const dir=path.resolve('.tools',`task077-autonomy-${Date.now()}`),root=path.join(dir,'project');await fs.mkdir(root,{recursive:true});
 await fs.writeFile(path.join(root,'conversion.py'),'def convert(value):\n    return None\n');
 await fs.writeFile(path.join(root,'test_conversion.py'),"import unittest\nfrom conversion import convert\nclass ConversionTest(unittest.TestCase):\n def test_values(self):\n  for value in [-2, 0, 3]: self.assertEqual(convert(value), value * 1000)\nif __name__ == '__main__': unittest.main()\n");
 await git(root,['init','--quiet']);await git(root,['add','conversion.py','test_conversion.py']);await git(root,['-c','user.name=Fixture','-c','user.email=fixture@example.invalid','commit','--quiet','-m','fixture']);
 const base=(await git(root,['rev-parse','HEAD'])).trim(),task={task_id:'conversion',parent_task_id:null,task_description:'修复 conversion.py 的 convert(value)，纯函数将输入乘以1000。读取源码及已有测试，实际运行测试，仅修复 conversion.py，不修改测试。',assigned_worker:'developer',allowed_files:['conversion.py'],read_only_files:['test_conversion.py'],forbidden_files:[],dependencies:[],interface_contracts:['convert(number) -> number * 1000'],acceptance_criteria:['已有unittest全部通过'],test_commands:['python -B test_conversion.py'],base_commit:base,status:'PENDING'};
 const plan={plan_id:'autonomy',project_id:'autonomy',project_path:root,original_request:task.task_description,tasks:[task],shared_file_owners:[],max_concurrency:1};
 const before=await new IsolatedTestRunner(path.join(dir,'baseline-tests')).run(root,task.test_commands[0],task.test_commands);assert.notEqual(before.exit_code,0);
 const executor=new TeamTaskExecutor(plan,path.join(dir,'runtime'),claude,async()=> 'DENY');await executor.load();
 try{const result=await executor.execute(task,new AbortController().signal),a=result.candidate;assert(a.evidence.tests.every(t=>t.status==='PASSED'&&t.test_count>0));assert(a.evidence.changes.every(c=>c.path==='conversion.py'));assert.equal((await git(root,['status','--porcelain'])).trim(),'');await fs.writeFile(path.join(dir,'result.json'),JSON.stringify({passed:true,realClaude:true,gptCalls:0,baselineFailure:before,candidate:a,worker:executor.workers.detail(result.worker_id)},null,2));console.log(dir);}finally{await executor.dispose();}
})().catch(e=>{console.error(e);process.exitCode=1});
