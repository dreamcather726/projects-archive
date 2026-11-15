import { createHash } from 'node:crypto';
import type { ReviewEvidence, ReviewTask, SnapshotScopeProof, SourceSnapshot } from './reviewTypes';
const key=(p:string)=>process.platform==='win32'?p.toLowerCase():p;
const hash=(s:string)=>createHash('sha256').update(s).digest('hex');
export function snapshotScopeProof(before:SourceSnapshot,after:SourceSnapshot,task:ReviewTask):SnapshotScopeProof{
  const old=new Map(before.files.map(f=>[f.path,f])),now=new Map(after.files.map(f=>[f.path,f]));
  const changes=[...new Set([...old.keys(),...now.keys()])].filter(p=>old.get(p)?.hash!==now.get(p)?.hash).map(p=>({path:p,before_hash:old.get(p)?.hash??null,after_hash:now.get(p)?.hash??null}));
  const allowed=task.allowed_files?.map(key),forbidden=task.forbidden_files||[];
  const binding_valid=[before,after].every(s=>s.digest===hash(JSON.stringify(s.files.map(f=>[f.path,f.hash]))));
  return {before_digest:before.digest,after_digest:after.digest,binding_valid,before_files:before.files.length,after_files:after.files.length,
    complete:binding_valid&&!before.gaps.length&&!after.gaps.length&&!changes.some(c=>old.get(c.path)?.omitted||now.get(c.path)?.omitted),unchanged_files:before.files.filter(f=>now.get(f.path)?.hash===f.hash).length,changes,
    outside_allowed:allowed?changes.filter(c=>!allowed.includes(key(c.path))).map(c=>c.path):[],
    protected_files:forbidden.map(p=>{const a=before.files.find(f=>key(f.path)===key(p))?.hash??null,b=after.files.find(f=>key(f.path)===key(p))?.hash??null;return {path:p,before_hash:a,after_hash:b,unchanged:a===b};})};
}
/** Summarize only authenticated gateway write results, and bind the chain to snapshots. */
export function executionWriteProof(before:SourceSnapshot,after:SourceSnapshot,task_id:string,worker_id:string,audit:unknown[]):NonNullable<ReviewEvidence['write_proof']>{
  const writes=new Map<string,{path:string;count:number;before_hash:string|null;after_hash:string;matches_snapshots:boolean}>();
  for(const raw of audit){const r=raw as any,d=r?.entry?.data;if(r?.task_id!==task_id||r?.worker_id!==worker_id||r?.entry?.kind!=='FILE_WRITE_RESULT'||typeof d?.file_path!=='string'||! /^(?:[a-f0-9]{64})$/.test(d.after_hash)||!(d.before_hash===null||/^[a-f0-9]{64}$/.test(d.before_hash)))continue;
    const previous=writes.get(d.file_path),initial=before.files.find(f=>f.path===d.file_path)?.hash??null;
    writes.set(d.file_path,{path:d.file_path,count:(previous?.count||0)+1,before_hash:previous?previous.before_hash:d.before_hash,after_hash:d.after_hash,matches_snapshots:(previous?.matches_snapshots??true)&&(previous?previous.after_hash:initial)===d.before_hash});
  }
  return {task_id,worker_id,writes:[...writes.values()].map(w=>({...w,matches_snapshots:w.matches_snapshots&&after.files.find(f=>f.path===w.path)?.hash===w.after_hash}))};
}
