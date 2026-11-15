// Read-only offline regression against a retained candidate. Does not resume, modify it, or call a model.
const fs=require('node:fs/promises'),path=require('node:path'),assert=require('node:assert/strict'),api=require('../.tools/task0711/api.cjs');
(async()=>{
 const planDirectory=path.resolve(process.argv[2]),out=path.resolve(process.argv[3]||'test-results/review-evidence-retained.json');
 async function find(dir,name){const result=[];for(const e of await fs.readdir(dir,{withFileTypes:true})){if(e.isSymbolicLink())continue;const file=path.join(dir,e.name);if(e.isFile()&&name(e.name))result.push(file);else if(e.isDirectory()&&!['worktrees','workers','test-snapshots','bundles'].includes(e.name))result.push(...await find(file,name));}return result;}
 const records=await find(planDirectory,n=>n.startsWith('candidates-')&&n.endsWith('.json'));assert.equal(records.length,1);const candidates=JSON.parse(await fs.readFile(records[0],'utf8')),results=[];
 for(const a of candidates){assert(a.before&&a.after&&a.evidence);const collector=new api.ReviewGit(path.resolve('test-results/review-evidence-diffs'));
  const evidence=await collector.evidence(a.before,a.after,a.evidence.task,a.evidence.delivery,a.evidence.tests,'CLAUDE_CAPTURE');
  evidence.write_proof=api.executionWriteProof(a.before,a.after,a.task_id,a.worker_id,a.audit);
  const actual=await collector.snapshot(a.workspace.workspace_path),same=actual.digest===a.after.digest;
  assert(evidence.scope_proof.complete);assert.equal(evidence.scope_proof.outside_allowed.length,0);assert.equal(evidence.gaps.length,0,evidence.gaps.join('\n'));assert(evidence.write_proof.writes.every(w=>w.matches_snapshots));
  const prompt=api.reviewPrompt(evidence,api.makeBatches(evidence,120000,true)[0],1);assert(prompt.includes('HOST SCOPE PROOF'));assert(prompt.includes('AUTHENTICATED WRITE PROOF'));
  results.push({task_id:a.task_id,old_gaps:a.evidence.gaps,new_gaps:evidence.gaps,context_characters:evidence.context.reduce((n,f)=>n+(f.text?.length||0),0),before_files:a.before.files.length,after_files:a.after.files.length,changed_files:evidence.changes.map(c=>c.path),scope_proof_complete:evidence.scope_proof.complete,write_proof_matches:evidence.write_proof.writes.every(w=>w.matches_snapshots),current_candidate_matches_checkpoint:same,tests:evidence.tests.map(t=>({command:t.command,status:t.status,test_count:t.test_count,source_digest:t.source_digest})),next_step:same?'Explicit independent model re-review after gateway confirmation':'Candidate changed: block recovery and collect fresh authorized evidence'});
 }
 await fs.writeFile(out,JSON.stringify({passed:true,offline_only:true,model_requests:0,candidate_writes:0,results},null,2));console.log(JSON.stringify({passed:true,results}));
})().catch(e=>{console.error(e.stack);process.exitCode=1;});
