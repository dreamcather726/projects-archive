import * as fs from 'node:fs/promises';
import * as path from 'node:path';
import { execFile } from 'node:child_process';
import { randomUUID } from 'node:crypto';
import { ReviewGit, digest } from '../review/reviewGit';
import { safeReviewText, relativeFile } from '../review/reviewSafety';
import type { SourceSnapshot } from '../review/reviewTypes';
import { plannedPath } from '../planning/adaptiveTaskPlanner';

export interface IsolatedTestResult { id: string; command: string; started_at: string; finished_at: string; exit_code: number; output: string; snapshot_digest: string; isolation: 'WSL_NAMESPACES_READONLY'; truncated: boolean; test_count?: number }
// Trusted bootstrap executes outside the chroot; project code never runs before the isolation boundary.
const bootstrap = String.raw`
import os,sys,json,base64,tempfile,subprocess,shutil,ctypes
p=json.loads(base64.b64decode(sys.argv[1]))
assert os.uname().machine=='x86_64', 'unverified architecture'
root=tempfile.mkdtemp(prefix='atm-ns-')
def call(*a): subprocess.run(a,check=True,stdout=subprocess.DEVNULL)
def readonly_tree(target,recursive=True):
 # mount_setattr(AT_RECURSIVE) also protects nested WSL system-library mounts.
 class Attr(ctypes.Structure): _fields_=[('set',ctypes.c_uint64),('clear',ctypes.c_uint64),('propagation',ctypes.c_uint64),('userns',ctypes.c_uint64)]
 libc=ctypes.CDLL(None,use_errno=True);a=Attr(1|2|4,0,0,0)
 if libc.syscall(442,-100,target.encode(),0x8000 if recursive else 0,ctypes.byref(a),ctypes.sizeof(a)) != 0: raise OSError(ctypes.get_errno(),'readonly mount failed')
call('/usr/bin/mount','--make-rprivate','/')
call('/usr/bin/mount','-t','tmpfs','-o','size=256m,nosuid,nodev','tmpfs',root)
for d in ['usr','bin','lib','lib64']:
 if os.path.exists('/'+d):
  os.makedirs(root+'/'+d,exist_ok=True);call('/usr/bin/mount','--rbind','/'+d,root+'/'+d);readonly_tree(root+'/'+d)
for d in ['work','tmp','proc','dev','etc']: os.makedirs(root+'/'+d,exist_ok=True)
call('/usr/bin/mount','--bind',p['snapshot'],root+'/work');call('/usr/bin/mount','-o','remount,bind,ro,nosuid,nodev,noexec',root+'/work')
call('/usr/bin/mount','-t','tmpfs','-o','size=64m,nosuid,nodev','tmpfs',root+'/tmp')
call('/usr/bin/mount','-t','proc','-o','nosuid,nodev,noexec','proc',root+'/proc')
for f in ['null','urandom','random']:
 open(root+'/dev/'+f,'w').close();call('/usr/bin/mount','--bind','/dev/'+f,root+'/dev/'+f)
if os.path.isfile('/etc/ld.so.cache'): shutil.copyfile('/etc/ld.so.cache',root+'/etc/ld.so.cache')
readonly_tree(root,False)
os.chroot(root);os.chdir('/work')
os.execv('/usr/bin/setpriv',['setpriv','--bounding-set=-all','--inh-caps=-all','--ambient-caps=-all','--no-new-privs','/usr/bin/env','-i','PATH=/usr/bin:/bin','HOME=/tmp','TMPDIR=/tmp','PYTHONDONTWRITEBYTECODE=1','/usr/bin/python3','-I','-c',p['runner'],json.dumps(p['argv'])])
`;
const runner = String.raw`
import os,sys,json,subprocess,resource
resource.setrlimit(resource.RLIMIT_CPU,(30,30));resource.setrlimit(resource.RLIMIT_AS,(536870912,536870912));resource.setrlimit(resource.RLIMIT_FSIZE,(16777216,16777216));resource.setrlimit(resource.RLIMIT_NOFILE,(64,64));resource.setrlimit(resource.RLIMIT_CORE,(0,0));resource.setrlimit(resource.RLIMIT_NPROC,(64,64))
s=open('/proc/self/status').read()
assert 'CapEff:\t0000000000000000' in s and 'CapBnd:\t0000000000000000' in s and 'NoNewPrivs:\t1' in s, 'capability boundary failed'
assert not os.path.exists('/mnt') and not os.path.exists('/init') and not os.path.exists('/work/.git'), 'host path exposed'
# Stream output directly; the Windows host caps capture and terminates the namespace on overflow.
r=subprocess.run(json.loads(sys.argv[1]),cwd='/work',stdin=subprocess.DEVNULL,close_fds=True)
sys.exit(r.returncode if r.returncode>=0 else 128-r.returncode)
`;
export class IsolatedTestRunner {
  private active = new Set<Promise<IsolatedTestResult>>();
  constructor(private directory: string, private distro = 'Ubuntu-22.04') {
    if (!/^[A-Za-z0-9_.-]{1,80}$/.test(distro)) throw Error('WSL发行版标识无效。');
  }
  static argv(command: string): string[] {
    const match = /^python(?:3)? -B ([A-Za-z0-9_][A-Za-z0-9_./-]*\.py)$/.exec(command);
    if (!match || match[1].split('/').some(s => s === '..' || s === '.')) throw Error('隔离执行器目前仅支持精确的 python -B 文件.py；其他运行时未通过隔离验证，禁止回退到宿主终端。');
    return ['/usr/bin/python3','-B',match[1]];
  }
  private wsl(args: string[], maxBuffer = 1024 * 1024, timeout = 60000): Promise<{code: number; output: string; truncated: boolean}> {
    return new Promise((resolve,reject) => execFile('wsl.exe',['-d',this.distro,'--exec',...args],{windowsHide:true,maxBuffer,timeout,encoding:'utf8'},(e,out,err) => {
      if (e && (typeof e.code !== 'number' || e.killed)) return reject(Error('WSL隔离启动、输出限制或超时失败；未回退到宿主执行。'));
      resolve({code:e ? Number(e.code) : 0,output:safeReviewText(out + (err ? '\n[stderr]\n'+err : '')),truncated:false});
    }));
  }
  run(root: string, command: string, authorized: string[], snapshot?: SourceSnapshot, readable?: string[]): Promise<IsolatedTestResult> {
    const run = this.execute(root,command,authorized,snapshot,readable); this.active.add(run); void run.finally(() => this.active.delete(run)).catch(() => {}); return run;
  }
  private async execute(root: string, command: string, authorized: string[], source?: SourceSnapshot, readable?: string[]): Promise<IsolatedTestResult> {
    if (!authorized.includes(command)) throw Error('测试命令不属于当前任务的精确授权。');
    const argv = IsolatedTestRunner.argv(command), id = randomUUID(), directory = path.join(this.directory,id);
    await fs.mkdir(directory,{recursive:true});
    const targetRel=path.relative(await fs.realpath(root),await fs.realpath(directory));
    if (!targetRel.startsWith('..'+path.sep) && targetRel!=='..' && !path.isAbsolute(targetRel)) throw Error('测试快照必须位于候选源码目录之外。');
    const permitted=readable?new Set(readable.map(plannedPath)):undefined;
    const snapshot = source || await new ReviewGit(this.directory).snapshot(root);
    if (await fs.realpath(root) !== await fs.realpath(snapshot.root) || snapshot.gaps.length) throw Error('源文件快照证据不完整或不属于本工作区，未执行隔离测试。');
    if (!snapshot.files.some(f => f.path === argv[2])) throw Error('测试文件不在真实代码快照中。');
    for (const f of snapshot.files) {
      if (f.hash === 'EXCLUDED') continue;
      if (permitted && !permitted.has(plannedPath(f.path))) continue;
      if (!relativeFile(f.path) || f.text === undefined || f.redacted) throw Error('代码快照存在省略或敏感内容；未执行不完整或脱敏后的伪测试。');
      const full=path.join(root,f.path),canonical=await fs.realpath(full),rel=path.relative(await fs.realpath(root),canonical),stat=await fs.lstat(full);
      if (rel.startsWith('..') || path.isAbsolute(rel) || stat.isSymbolicLink() || !stat.isFile() || stat.nlink>1) throw Error('测试源码出现别名、硬链接或越界路径，未执行。');
      const bytes=await fs.readFile(full);
      if (digest(bytes)!==f.hash) throw Error('测试前代码已改变，旧快照失效。');
      const target=path.join(directory,f.path); await fs.mkdir(path.dirname(target),{recursive:true}); await fs.writeFile(target,bytes);
    }
    if (permitted && !permitted.has(plannedPath(argv[2]))) throw Error('测试入口不在本任务授权读取范围。');
    const converted=await this.wsl(['/usr/bin/wslpath','-u',directory],16000,10000), linuxPath=converted.output.trim();
    if (converted.code !== 0 || !/^\/mnt\/[a-z]\//.test(linuxPath) || /[\r\n\0]/.test(linuxPath)) throw Error('隔离快照路径转换失败。');
    const payload=Buffer.from(JSON.stringify({snapshot:linuxPath,argv,runner})).toString('base64'), started_at=new Date().toISOString();
    // No interpolated shell commands. A new user/mount/network/PID/IPC namespace and chroot per test.
    const result=await this.wsl(['/usr/bin/timeout','--signal=KILL','45','/usr/bin/unshare','--user','--map-root-user','--mount','--net','--ipc','--pid','--fork','--kill-child','/usr/bin/python3','-I','-c',bootstrap,payload]);
    const count = /\bRan (\d+) tests? in /.exec(result.output);
    return {id,command,started_at,finished_at:new Date().toISOString(),exit_code:result.code,output:result.output,snapshot_digest:digest(JSON.stringify(snapshot.files.map(f=>[f.path,f.hash]))),isolation:'WSL_NAMESPACES_READONLY',truncated:result.truncated,test_count:count?Number(count[1]):undefined};
  }
  async drain() { await Promise.allSettled([...this.active]); }
}
