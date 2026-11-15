const test=require('node:test'),assert=require('node:assert/strict'),fs=require('node:fs/promises'),path=require('node:path'),os=require('node:os');
const {AdaptiveTaskPlanner,validateDevelopmentPlan,plannedPath,ReviewGit,ReviewStorage,git}=require('../.tools/task02-test/api.cjs');
const context={evidence_id:'E',base_commit:'abc',original_request:'创建 ratio.py，不得修改 protocol.c。',continuous_authorized:false};
const task=(id,file,worker=id)=>({task_id:id,parent_task_id:null,task_description:'Implement and independently test the module',assigned_worker:worker,allowed_files:[file],read_only_files:[],forbidden_files:['protocol.c'],dependencies:[],interface_contracts:['pure interface, no cross-module writes'],acceptance_criteria:['boundary tests pass'],test_commands:['python3 -B test_ratio.py'],base_commit:'abc',status:'PENDING'});
const proposal=()=>({evidence_id:'E',task_type:'SMALL_CHANGE',goal:'requested function',current_project_state:'baseline',technical_approach:'pure implementation',worker_count:1,parallel_rationale:'one small module',tasks:[task('T1','ratio.py','W1')],shared_file_owners:[],out_of_scope:['unrequested features'],completion_policy:'STOP_AFTER_REQUEST'});
test('adaptive planning uses actual task graph rather than domain or category to validate Worker allocation',()=>{
 for(const kind of ['SMALL_CHANGE','MODULE_TASK','MULTI_MODULE','PROJECT_PHASE','FULL_PROJECT'])assert.equal(validateDevelopmentPlan({...proposal(),task_type:kind},context).worker_count,1);
 const p=proposal();p.worker_count=2;p.tasks.push(task('T2','parse.py','W2'));p.parallel_rationale='independent modules, distinct file ownership';assert.equal(validateDevelopmentPlan(p,context).worker_count,2);
 p.tasks[1].dependencies=['T1'];assert.throws(()=>validateDevelopmentPlan(p,context),/并行收益/);
});
test('planner rejects stale IDs, cycles, fake completion, shared-file writers and unauthorized scope expansion',()=>{
 for(const mutate of [p=>p.evidence_id='other',p=>p.tasks[0].dependencies=['T1'],p=>p.tasks[0].status='COMPLETED',p=>p.tasks[0].allowed_files=['protocol.c'],p=>p.tasks[0].base_commit='wrong',p=>p.tasks[0].test_commands=['git reset --hard'],p=>p.completion_policy='CONTINUE_APPROVED_PLAN',p=>p.tasks[0].allowed_files=['Ratio.py','ratio.py']]){const p=proposal();mutate(p);assert.throws(()=>validateDevelopmentPlan(p,context));}
 const p=proposal();p.worker_count=2;p.tasks.push(task('T2','ratio.py','W2'));p.shared_file_owners=[{file:'ratio.py',task_id:'T1'}];assert.throws(()=>validateDevelopmentPlan(p,context),/公共文件/);
 for(const f of ['../a.py','src//a.py','src/link/../a.py','C:/a.py','src/CON.txt','file.py.','.env','a/.git/config','src/*.py'])assert.throws(()=>plannedPath(f));
});
test('real Git planning captures dirty state without changing files, binds owner and turn, and persists validated reports',async()=>{
 const dir=await fs.mkdtemp(path.join(os.tmpdir(),'atm-plan-')),root=path.join(dir,'project');await fs.mkdir(root);await git(root,['init','--quiet']);await fs.writeFile(path.join(root,'README.md'),'Calibration converter library\n');await git(root,['add','README.md']);await git(root,['-c','user.name=Fixture','-c','user.email=fixture@example.invalid','commit','--quiet','-m','fixture']);
 await fs.writeFile(path.join(root,'README.md'),'USER DIRTY CONTENT\n');const collector=new ReviewGit(path.join(dir,'temporary')),storage=new ReviewStorage(path.join(dir,'history')),planner=new AdaptiveTaskPlanner(root,collector,storage);await planner.load();
 const c=await planner.capture('owner','turn',context.original_request);assert(c.dirty_files.includes('README.md'));assert(c.contexts[0].text.includes('USER DIRTY CONTENT'));
 const p=proposal();p.evidence_id=c.evidence_id;p.tasks[0].base_commit=c.base_commit;
 await assert.rejects(()=>planner.submit('other','turn',p));await assert.rejects(()=>planner.submit('owner','other',p));
 const report=await planner.submit('owner','turn',p);assert.equal(report.validation,'VALIDATED_NOT_EXECUTED');assert.equal((await planner.submit('owner','turn',p)).plan_id,report.plan_id);assert.equal(planner.snapshot().execution_enabled,false);
 const restored=new AdaptiveTaskPlanner(root,collector,storage);await restored.load();assert.equal(restored.snapshot().plans[0].plan_id,report.plan_id);
 await fs.appendFile(path.join(root,'README.md'),'MORE USER CONTENT\n');await assert.rejects(()=>planner.submit('owner','turn',p),/失效/);assert.equal(await fs.readFile(path.join(root,'README.md'),'utf8'),'USER DIRTY CONTENT\nMORE USER CONTENT\n');
});
