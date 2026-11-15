import * as fs from 'node:fs/promises';
import * as path from 'node:path';
import { randomUUID } from 'node:crypto';
import { TeamWorkflow } from '../team/teamWorkflow';
import { WorkerWorkspaceManager, type WorkerWorkspace } from '../isolation/workerWorkspaceManager';
import { IsolatedTestRunner } from '../isolation/isolatedTestRunner';
import { UserEditProtectionManager } from '../protection/userEditProtectionManager';
import { ReviewStorage } from '../review/reviewStorage';
import { git, digest } from '../review/reviewGit';
import { plannedPath } from '../planning/adaptiveTaskPlanner';
import type { ReviewEvidence, ReviewReport, TestEvidence } from '../review/reviewTypes';

export interface IntegrationRecord {
  integration_id:string;plan_id:string;status:'PREPARING'|'CONFLICT'|'BLOCKED'|'READY'|'DELIVERED';
  workspace?:WorkerWorkspace;evidence?:ReviewEvidence;review?:ReviewReport;patch_file?:string;
  candidates:Record<string,{digest:string;review_id:string}>;conflicts:string[];error?:string;
  commit?:string;push?:{branch:string;success:boolean;error?:string};delivered_at?:string;
}
/** Merge in a separate worktree. Only a fresh, one-shot user grant may apply the reviewed patch. */
export class IntegrationManager {
  private record?:IntegrationRecord;
  private busy=false;
  constructor(private workflow:TeamWorkflow,readonly protection:UserEditProtectionManager,private memory:ReviewStorage=workflow.storage,private changed:()=>void=()=>{}){}
  async load(){this.record=await this.workflow.storage.read<IntegrationRecord>(this.workflow.plan.project_path,'integration.json');if(this.record&&this.record.plan_id!==this.workflow.plan.plan_id)throw Error('集成记录与计划不一致。');}
  snapshot(){const r=this.record;return r?{...r,evidence:r.evidence?{changes:r.evidence.changes.map(c=>({path:c.path,kind:c.kind,diff:c.diff})),tests:r.evidence.tests}:undefined,review:r.review?{review_id:r.review.review_id,verdict:r.review.verdict,summary:r.review.summary,issues:r.review.issues,missing_evidence:r.review.missing_evidence}:undefined,busy:this.busy}:undefined;}
  progressSource():Readonly<IntegrationRecord>|undefined{return this.record;}
  private async save(){if(this.record)await this.workflow.storage.write(this.workflow.plan.project_path,'integration.json',this.record);this.changed();}
  private latest(taskId:string){return this.workflow.reviewHistory(taskId).at(-1);}
  private async verifyCandidates(){
    const state=this.workflow.snapshot();if(!state.ready_for_integration)throw Error('候选尚未全部通过独立审查。');
    if(state.workers.some(w=>['RUNNING','RETRYING','PAUSING','QUEUED'].includes(w.status)))throw Error('Worker仍在写入，不能合并。');
    const proofs:IntegrationRecord['candidates']={};
    for(const task of this.workflow.plan.tasks){const a=this.workflow.executor.candidate(task.task_id),review=this.latest(task.task_id);
      if(!a.after||!a.evidence||!review||review.verdict!=='PASS'||review.stale||review.missing_evidence.length||review.evidence.after.digest!==a.after.digest||a.evidence.tests.some(t=>t.status!=='PASSED')||!a.evidence.tests.length)throw Error('候选缺少当前代码、真实测试或独立审查证据。');
      if((await this.workflow.collector.snapshot(a.workspace.workspace_path)).digest!==a.after.digest)throw Error('候选在审查后发生变化，必须重新测试及审查。');
      proofs[task.task_id]={digest:a.after.digest,review_id:review.review_id};
    }
    return proofs;
  }
  async prepare(){
    if(this.busy)throw Error('已有集成操作运行中。');this.busy=true;
    const r:IntegrationRecord={integration_id:randomUUID(),plan_id:this.workflow.plan.plan_id,status:'PREPARING',candidates:{},conflicts:[]};this.record=r;
    try{
      r.candidates=await this.verifyCandidates();await this.save();
      const plan=this.workflow.plan,base=plan.tasks[0].base_commit;
      const manager=new WorkerWorkspaceManager(plan.project_path,path.join(this.workflow.directory,'integration-worktrees'));
      r.workspace=await manager.create(base,'integration');const before=await this.workflow.collector.snapshot(r.workspace.workspace_path),current=new Map(before.files.map(f=>[plannedPath(f.path),f]));
      const ordered:string[]=[],seen=new Set<string>();const visit=(id:string)=>{if(seen.has(id))return;seen.add(id);for(const dep of plan.tasks.find(t=>t.task_id===id)!.dependencies)visit(dep);ordered.push(id);};for(const task of plan.tasks)visit(task.task_id);
      for(const id of ordered){const candidate=this.workflow.executor.candidate(id),task=plan.tasks.find(t=>t.task_id===id)!;
        for(const c of candidate.evidence!.changes){const name=plannedPath(c.path),old=current.get(name);if(!task.allowed_files.some(f=>plannedPath(f)===name))throw Error('集成候选包含越权文件。');
          if(old?.hash!==c.before?.hash){if(old?.hash===c.after?.hash)continue;r.conflicts.push(c.path);continue;}
          const full=path.join(r.workspace.workspace_path,c.path);
          if(c.after){if(c.after.text===undefined||c.after.redacted||digest(c.after.text)!==c.after.hash)throw Error('候选源码被省略或脱敏，不能集成。');await fs.mkdir(path.dirname(full),{recursive:true});await fs.writeFile(full,c.after.text);current.set(name,c.after);}
          else{await fs.unlink(full);current.delete(name);}
        }
      }
      if(r.conflicts.length){r.status='CONFLICT';throw Error('候选内容冲突；所有原始Worktree保留，需要重新协调。');}
      const after=await this.workflow.collector.snapshot(r.workspace.workspace_path),commands=new Set(plan.tasks.flatMap(t=>t.test_commands));
      for(const f of after.files){if(/(?:^|\/)test_[^/]+\.py$|_test\.py$/.test(f.path))commands.add('python -B '+f.path);if(f.path==='package.json'&&f.text){const pkg=JSON.parse(f.text);if(pkg.scripts?.test)commands.add('npm test');if(pkg.scripts?.compile)commands.add('npm run compile');}}
      const tests:TestEvidence[]=[],runner=new IsolatedTestRunner(path.join(this.workflow.directory,'integration-tests'));
      for(const command of commands){const t=await runner.run(r.workspace.workspace_path,command,[...commands],after);tests.push({id:t.id,command,started_at:t.started_at,finished_at:t.finished_at,status:t.exit_code!==0?'FAILED':t.test_count&&t.test_count>0?'PASSED':'UNVERIFIED',test_count:t.test_count,exit_code:t.exit_code,output:t.output,output_truncated:t.truncated,source_digest:t.snapshot_digest});}
      // A sibling's owned file is forbidden to this Worker, but belongs to the combined integration.
      // This projection changes review context only; each candidate's ownership was checked above.
      const allowed=[...new Set(plan.tasks.flatMap(t=>t.allowed_files))],allowedKeys=new Set(allowed.map(plannedPath));
      const forbidden=[...new Set(plan.tasks.flatMap(t=>t.forbidden_files))].filter(f=>!allowedKeys.has(plannedPath(f)));
      r.evidence=await this.workflow.collector.evidence(before,after,{task_id:'integration-'+r.integration_id,task_description:plan.original_request,technical_requirements:plan.tasks.flatMap(t=>t.interface_contracts.concat(t.acceptance_criteria)).join('\n'),allowed_files:allowed,forbidden_files:forbidden},{status:'PLUGIN_INTEGRATION',final_answer:'插件在独立集成工作区按依赖顺序合并已审查候选；此文本不替代真实代码和测试证据。'},tests,'CLAUDE_CAPTURE');
      if(!tests.length||tests.some(t=>t.status!=='PASSED'))throw Error('集成回归测试失败或缺少真实测试数量。');
      r.review=await this.workflow.review(r.evidence,'INTEGRATION_REVIEW');
      if(r.review.verdict!=='PASS'||r.review.stale||r.review.missing_evidence.length)throw Error('集成候选未通过GPT独立审查；保留全部候选，未应用主项目。');
      await manager.verify(r.workspace);
      for(const c of r.evidence.changes){if(c.after){const oid=(await git(r.workspace.workspace_path,['hash-object','-w','--no-filters','--',c.path])).trim(),entry=await git(r.workspace.workspace_path,['ls-tree',base,'--',':(literal)'+c.path]),mode=entry.startsWith('100755')?'100755':'100644';await git(r.workspace.workspace_path,['update-index','--add','--cacheinfo',mode,oid,c.path]);}else await git(r.workspace.workspace_path,['update-index','--force-remove','--',c.path]);}
      const patch=await git(r.workspace.workspace_path,['diff','--cached','--binary','--no-ext-diff','--no-textconv','--no-renames',base,'--',...r.evidence.changes.map(c=>':(literal)'+c.path)]);if(!patch.trim())throw Error('没有实际可交付的补丁。');
      r.patch_file=path.join(this.workflow.directory,r.integration_id+'.patch');await fs.writeFile(r.patch_file,patch,{mode:0o600});r.status='READY';await this.save();return this.snapshot();
    }catch(e){r.error=(e as Error).message;if(r.status!=='CONFLICT')r.status='BLOCKED';await this.save();throw e;}finally{this.busy=false;this.changed();}
  }
  async deliver(externalEditorsConfirmed:boolean,syncGit=false){
    if(this.busy)throw Error('已有集成操作运行中。');const r=this.record;if(!r||r.status!=='READY'||!r.evidence||!r.review||!r.patch_file||!r.workspace)throw Error('没有经过测试和审查的可交付候选。');
    this.busy=true;
    try{
      r.error=undefined;
      const root=this.workflow.plan.project_path,base=this.workflow.plan.tasks[0].base_commit,files=r.evidence.changes.map(c=>c.path);
      // Ignore unrelated global filter definitions; only effective filters on delivered paths can execute here.
      if(files.some(f=>/(?:^|\/)\.gitattributes$/.test(f)))throw Error('Git属性变更需要人工集成，不能在自动应用中改变内容过滤规则。');
      const attributes=(await git(root,['check-attr','-z','filter','--',...files])).split('\0');
      if(attributes.some((v,i)=>i%3===2&&!['unspecified','unset',''].includes(v)))throw Error('交付文件启用了Git内容过滤器，自动应用保持关闭，需人工检查补丁。');
      const destination=syncGit?await this.gitDestination():undefined;
      if(syncGit&&(await git(root,['diff','--cached','--name-only'])).trim())throw Error('暂存区存在用户改动，不能代为提交；请处理后重新授权。');
      const grant=await this.protection.authorize(base,files,externalEditorsConfirmed);
      await this.protection.withDelivery(grant.id,async snapshot=>{
        if(JSON.stringify(await this.verifyCandidates())!==JSON.stringify(r.candidates))throw Error('Worker审查或候选已改变，旧集成结果失效。');
        if((await this.workflow.collector.snapshot(r.workspace!.workspace_path)).digest!==r.evidence!.after.digest)throw Error('集成工作区已改变，旧审查失效。');
        const expected=await git(r.workspace!.workspace_path,['diff','--cached','--binary','--no-ext-diff','--no-textconv','--no-renames',base,'--',...files.map(f=>':(literal)'+f)]);if(await fs.readFile(r.patch_file!,'utf8')!==expected)throw Error('交付补丁发生变化，未应用。');
        await git(root,['apply','--index','--check','--',r.patch_file!]);
        const fresh=await this.protection.capture(base,files);if(!fresh.safe||fresh.fingerprint!==snapshot.fingerprint)throw Error('最终应用前用户文件或编辑状态已改变。');
        await git(root,['apply','--index','--',r.patch_file!]);
      });
      r.status='DELIVERED';r.delivered_at=new Date().toISOString();await this.save();await this.workflow.markIntegrated(r.integration_id,r.review.review_id);
      await this.memory.write(root,'project-context.json',{version:1,plan_id:this.workflow.plan.plan_id,goal:this.workflow.plan.goal,base_commit:base,head:(await git(root,['rev-parse','HEAD'])).trim(),delivered_at:r.delivered_at,completed_tasks:this.workflow.plan.tasks.map(t=>({task_id:t.task_id,description:t.task_description,interfaces:t.interface_contracts})),pending_tasks:[],out_of_scope:this.workflow.plan.out_of_scope,technical_approach:this.workflow.plan.technical_approach,tests:r.evidence.tests.map(t=>({command:t.command,status:t.status,test_count:t.test_count})),review_id:r.review.review_id,known_issues:r.review.issues});
      if(destination){try{await git(root,['commit','--only','-m','feat: '+this.workflow.plan.goal.slice(0,100),'--',...files.map(f=>':(literal)'+f)]);r.commit=(await git(root,['rev-parse','HEAD'])).trim();await git(root,destination.args);r.push={branch:destination.branch,success:true};}catch(e){r.push={branch:destination.branch,success:false,error:(e as Error).message};}await this.save();}
      return this.snapshot();
    }catch(e){r.error=(e as Error).message;await this.save();throw e;}finally{this.busy=false;this.changed();}
  }
  private async gitDestination(){
    const root=this.workflow.plan.project_path,branch=(await git(root,['branch','--show-current'])).trim();if(!branch)throw Error('Detached HEAD不能自动推送。');
    const configured=(await git(root,['config','--get',`branch.${branch}.remote`],true)).trim(),remote=configured||'origin',url=(await git(root,['remote','get-url',remote])).trim();
    if(!/^(?:https:\/\/github\.com\/|git@github\.com:)[A-Za-z0-9_.-]+\/[A-Za-z0-9_.-]+(?:\.git)?$/.test(url))throw Error('没有明确GitHub远端，禁止猜测推送地址。');
    const merge=(await git(root,['config','--get',`branch.${branch}.merge`],true)).trim();
    if(configured&&(!merge.startsWith('refs/heads/')||remote==='.'))throw Error('GitHub upstream无效。');
    return {branch:configured?merge.slice(11):branch,args:configured?['push',remote,`HEAD:${merge}`]:['push','-u','origin',branch]};
  }
}
