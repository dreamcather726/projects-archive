import * as fs from 'node:fs/promises';
import * as path from 'node:path';
import { execFile } from 'node:child_process';
import { randomUUID } from 'node:crypto';
import { git } from '../review/reviewGit';
import { relativeFile, excludedPath } from '../review/reviewSafety';

export interface WorkerWorkspace { workspace_path: string; branch_name: string; base_commit: string; omitted_files: string[] }
export class WorkerWorkspaceManager {
  constructor(readonly project: string, readonly directory: string) {}
  async create(base: string, taskId: string): Promise<WorkerWorkspace> {
    if (!/^[a-f0-9]{40,64}$/.test(base) || !/^[A-Za-z0-9_-]{1,80}$/.test(taskId)) throw Error('工作区基准或任务编号无效。');
    const project = await fs.realpath(this.project);
    await fs.mkdir(this.directory, { recursive: true });
    const directory = await fs.realpath(this.directory), rel = path.relative(project, directory);
    if (!rel.startsWith('..' + path.sep) && rel !== '..' && !path.isAbsolute(rel)) throw Error('Worker工作区根目录必须位于用户项目目录之外。');
    const actual = (await git(project, ['rev-parse', '--show-toplevel'])).trim();
    if (path.resolve(actual).toLowerCase() !== project.toLowerCase()) throw Error('必须使用Git仓库根目录。');
    const tree = await git(project, ['ls-tree', '-r', '-z', '--full-tree', base]);
    const entries = tree.split('\0').filter(Boolean).map(row => { const tab = row.indexOf('\t'), [mode, type, hash] = row.slice(0, tab).split(' '); return { mode, type, hash, file: row.slice(tab + 1) }; });
    if (entries.length > 20000 || entries.some(e => !relativeFile(e.file) || /\\|[<>:"|?*]/.test(e.file) || e.file.split('/').some(p => /[. ]$/.test(p) || /^(?:con|prn|aux|nul|com[1-9]|lpt[1-9])(?:\.|$)/i.test(p)))) throw Error('Git树包含不安全路径或规模超出当前隔离采集上限。');
    if (process.platform === 'win32' && new Set(entries.map(e=>e.file.toLowerCase())).size !== entries.length) throw Error('Git树存在Windows大小写路径冲突，未创建可写工作区。');
    if (entries.some(e => !['100644','100755'].includes(e.mode) || e.type !== 'blob')) throw Error('含符号链接、子模块或特殊文件；未开放Worker写入。');
    const id = randomUUID(), workspace = path.join(directory, id), branch = 'atm/task07-' + id;
    await git(project, ['worktree', 'add', '--no-checkout', '-b', branch, workspace, base]);
    // Index only, then raw blobs. No checkout/smudge filters or project hooks are executed.
    await git(workspace, ['read-tree', base]);
    const omitted: string[] = []; let bytes = 0;
    for (const e of entries) {
      if (excludedPath(e.file)) { omitted.push(e.file); continue; }
      const buffer = await new Promise<Buffer>((resolve, reject) => execFile('git', ['-c','core.fsmonitor=false','-c','core.hooksPath=/dev/null','-C', project, 'cat-file', 'blob', e.hash], { windowsHide: true, timeout: 20000, maxBuffer: 32 * 1024 * 1024, encoding: 'buffer' }, (error, stdout) => error ? reject(Error('Git原始对象读取失败；候选工作区保留供检查。')) : resolve(stdout)));
      bytes += buffer.length; if (bytes > 256 * 1024 * 1024) throw Error('工作区对象超过256MiB；已建候选目录保留，未覆盖主工作区。');
      const target = path.join(workspace, e.file); await fs.mkdir(path.dirname(target), { recursive: true }); await fs.writeFile(target, buffer, { mode: e.mode === '100755' ? 0o755 : 0o644 });
    }
    return { workspace_path: await fs.realpath(workspace), branch_name: branch, base_commit: base, omitted_files: omitted };
  }
  async verify(workspace: WorkerWorkspace) {
    const root = await fs.realpath(workspace.workspace_path), directory = await fs.realpath(this.directory), rel = path.relative(directory, root);
    if (!rel || rel.startsWith('..') || path.isAbsolute(rel) || root !== workspace.workspace_path) throw Error('Worker真实目录已改变或不在专属根目录中。');
    if ((await git(root, ['branch','--show-current'])).trim() !== workspace.branch_name || (await git(root, ['rev-parse','HEAD'])).trim() !== workspace.base_commit) throw Error('Worker分支或基准提交已改变，旧执行授权失效。');
  }
}
