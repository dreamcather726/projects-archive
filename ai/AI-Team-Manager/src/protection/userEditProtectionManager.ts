import * as fs from 'node:fs/promises';
import * as path from 'node:path';
import { randomUUID } from 'node:crypto';
import { digest, git } from '../review/reviewGit';
import { plannedPath } from '../planning/adaptiveTaskPlanner';
import { ReviewStorage } from '../review/reviewStorage';

export interface EditorFileState { file: string; dirty: boolean; version: number; buffer_hash: string; open: boolean }
export interface ProtectionSnapshot { id:string; base_commit:string; head:string; files:{file:string;hash:string;reasons:string[]}[]; editors:EditorFileState[]; manual_locks:string[]; git_status:string; fingerprint:string; safe:boolean; captured_at:string }
interface DeliveryGrant { id:string; fingerprint:string; files:string[]; base:string; expires:number; external_editors_confirmed:boolean }
/** The main workspace is inspected, never committed, cleaned or overwritten by this manager. */
export class UserEditProtectionManager {
  private manual=new Set<string>();
  private grants=new Map<string,DeliveryGrant>();
  private serial:Promise<unknown>=Promise.resolve();
  constructor(readonly root:string,private editors:()=>Promise<EditorFileState[]>,private storage:ReviewStorage){}
  async load(){const files=await this.storage.read<string[]>(this.root,'manual-file-locks.json')||[];if(!Array.isArray(files))throw Error('手动文件锁记录损坏。');this.manual=new Set(files.map(plannedPath));}
  async lock(file:string,locked=true){const key=plannedPath(file);locked?this.manual.add(key):this.manual.delete(key);this.grants.clear();await this.storage.write(this.root,'manual-file-locks.json',[...this.manual]);}
  invalidate(){this.grants.clear();}
  async capture(base:string,files:string[]):Promise<ProtectionSnapshot>{
    const names=[...new Set(files.map(plannedPath))].sort(),root=await fs.realpath(this.root);
    const same=process.platform==='win32'?root.toLowerCase()===path.resolve(this.root).toLowerCase():root===path.resolve(this.root);
    if(!same||!names.length)throw Error('主工作区真实路径或目标文件范围无效。');
    const head=(await git(root,['rev-parse','HEAD'])).trim(),status=await git(root,['status','--porcelain=v1','-z','--untracked-files=all']);
    const dirty=new Set<string>(),parts=status.split('\0').filter(Boolean);
    for(let i=0;i<parts.length;i++){const entry=parts[i];try{dirty.add(plannedPath(entry.slice(3)));}catch{/* Protected/generated paths never become integration targets. */}if(/[RC]/.test(entry.slice(0,2))){try{dirty.add(plannedPath(parts[++i]));}catch{}}}
    const editors=await this.editors();if(!Array.isArray(editors)||editors.some(e=>typeof e.file!=='string'||typeof e.dirty!=='boolean'||typeof e.buffer_hash!=='string'||!Number.isInteger(e.version)))throw Error('无法可靠获取编辑器状态，集成保持停止。');
    const states=[];
    for(const file of names){const reasons:string[]=[];if(head!==base)reasons.push('BASE_COMMIT_CHANGED');if(dirty.has(file))reasons.push('UNCOMMITTED_OR_UNTRACKED');if(this.manual.has(file))reasons.push('MANUAL_LOCK');
      const editor=editors.filter(e=>plannedPath(e.file)===file);if(editor.some(e=>e.open))reasons.push('OPEN_IN_EDITOR');if(editor.some(e=>e.dirty))reasons.push('UNSAVED_BUFFER');
      const full=path.join(root,file);let hash='ABSENT',current=root;
      for(const part of file.split('/')){current=path.join(current,part);const st=await fs.lstat(current).catch((e:any)=>{if(e.code==='ENOENT')return undefined;throw e;});if(st?.isSymbolicLink()||st?.isFile()&&st.nlink>1){reasons.push('PATH_ALIAS');break;}}
      if(!reasons.includes('PATH_ALIAS')){const bytes=await fs.readFile(full).catch((e:any)=>{if(e.code==='ENOENT')return undefined;throw e;});if(bytes)hash=digest(bytes);}
      states.push({file,hash,reasons});
    }
    const data={base_commit:base,head,files:states,editors:editors.filter(e=>names.includes(plannedPath(e.file))).sort((a,b)=>a.file.localeCompare(b.file)),manual_locks:[...this.manual].sort(),git_status:status};
    return {id:randomUUID(),...data,fingerprint:digest(JSON.stringify(data)),safe:states.every(s=>!s.reasons.length),captured_at:new Date().toISOString()};
  }
  async authorize(base:string,files:string[],externalEditorsConfirmed:boolean){
    if(!externalEditorsConfirmed)throw Error('其他编辑器的未保存内容无法由VS Code检测；需用户确认后才能签发交付授权。');
    const snapshot=await this.capture(base,files);if(!snapshot.safe)throw Error('WAITING_INTEGRATION：目标文件正在编辑、未提交、被锁定或基准改变；候选结果保留。');
    const grant:DeliveryGrant={id:randomUUID(),fingerprint:snapshot.fingerprint,files:[...files],base,expires:Date.now()+60000,external_editors_confirmed:true};this.grants.set(grant.id,grant);return {...grant};
  }
  async withDelivery<T>(id:string,apply:(snapshot:ProtectionSnapshot)=>Promise<T>):Promise<T>{
    const run=this.serial.then(async()=>{const grant=this.grants.get(id);this.grants.delete(id);if(!grant||grant.expires<Date.now())throw Error('交付授权已失效或已使用，需重新确认。');const now=await this.capture(grant.base,grant.files);if(!now.safe||now.fingerprint!==grant.fingerprint)throw Error('交付前文件/编辑器/版本已改变，旧授权失效；未调用集成操作。');return apply(now);});
    this.serial=run.catch(()=>{});return run;
  }
}
