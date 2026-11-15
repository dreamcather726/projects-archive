import * as fs from 'node:fs/promises';
import * as path from 'node:path';
import { createHash } from 'node:crypto';
import { safeReviewText, relativeFile, excludedPath } from '../review/reviewSafety';
import type { PermissionDecision, TaskPermissionScope } from './permissionTypes';
// Exact commands only. Scripts still execute project code: configuring these is explicit user authorization.
export const commandCatalog = ['node --version', 'npm --version', 'git status --short', 'git diff --stat', 'node --test', 'npm test', 'npm run compile', 'npm run build', 'npm run lint', 'npm run typecheck'] as const;
export const defaultScope = (): TaskPermissionScope => ({ allowedFiles: [], safeCommands: ['node --version', 'npm --version', 'git status --short', 'git diff --stat'] });
const protectedPath = (f: string) => excludedPath(f)==='sensitive' || /(?:^|\/)(?:\.git|\.claude|\.codex|\.ssh|\.aws|\.?cc[-_ ]?switch)(?:\/|$)|(?:^|\/)(?:\.env(?:\.[^/]*)?|\.npmrc|\.netrc|credentials[^/]*|auth\.json|.*\.(?:pem|key|p12|pfx))$/i.test(f);
const unsafeSegment = (f: string) => f.split('/').some(part => /[. ]$|[\x00-\x1f<>:"|?*]/.test(part) || /^(?:CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\.|$)/i.test(part));
const configuration = (f: string) => /\.(?:jsonc?|ya?ml|toml|ini|cfg|conf|xml|props|targets|sln|vcxproj|csproj|cmake)$|(?:^|\/)(?:[^/]*config[^/]*\.[cm]?[jt]s|\.gitignore|\.gitattributes|\.gitmodules|\.editorconfig|CLAUDE\.md|AGENTS\.md|CMakeLists\.txt|Makefile|Dockerfile|Cargo\.lock|requirements[^/]*\.txt)|^\.vscode\//i.test(f);
const ordinarySource = (f: string) => /\.(?:c|h|cc|cpp|hpp|cxx|hxx|ts|tsx|js|jsx|mjs|cjs|py|go|rs|java|kt|swift|rb|php|vue|svelte|html|css|scss|less|sql|md|txt)$/i.test(f);
const key = (s: string) => process.platform === 'win32' ? s.toLowerCase() : s;
export function validateScope(scope: TaskPermissionScope): TaskPermissionScope {
  if (!scope || !Array.isArray(scope.allowedFiles) || !Array.isArray(scope.safeCommands) || scope.allowedFiles.length > 200 || scope.safeCommands.length > commandCatalog.length) throw new Error('任务授权范围无效。');
  const files = [...new Set(scope.allowedFiles)];
  if (files.some(f => typeof f !== 'string' || !relativeFile(f) || f !== path.posix.normalize(f) || unsafeSegment(f) || /[*?\[\]]/.test(f) || protectedPath(f))) throw new Error('允许文件必须是精确相对路径；不能使用通配符、父目录、设备名、配置凭据或受保护路径。');
  if (scope.safeCommands.some(c => !(commandCatalog as readonly string[]).includes(c))) throw new Error('白名单只支持列出的精确测试/只读命令，不接受任意Shell表达式。');
  return { allowedFiles: files, safeCommands: [...new Set(scope.safeCommands)] };
}
export class PermissionPolicy {
  readonly scope: TaskPermissionScope;
  constructor(public readonly root: string, scope: TaskPermissionScope,
    private repair?: { deleteFiles: string[] }) { this.scope = validateScope(scope); }
  private async target(value: unknown, directory = false): Promise<string> {
    if (typeof value !== 'string' || !value.trim() || value.includes('\0') || value.startsWith('\\\\') || value.startsWith('//') || /(?:^|[\\/])\.\.(?:[\\/]|$)/.test(value)) throw new Error('目标路径为空、网络路径或含父目录。');
    const full = path.resolve(this.root, value), rel = path.relative(this.root, full).replace(/\\/g, '/');
    if (path.isAbsolute(rel) || rel === '..' || rel.startsWith('../') || (!directory && !rel) || unsafeSegment(rel) || protectedPath(rel)) throw new Error('目标位于工作区外、使用不安全别名或属于受保护路径。');
    // Reject junctions, symlinks, and hard-linked write targets; do not follow an alias outside the root.
    const parts = rel.split('/').filter(Boolean); let current = this.root;
    for (const part of parts) {
      current = path.join(current, part); const stat = await fs.lstat(current).catch((e: any) => { if (e.code === 'ENOENT') return undefined; throw e; });
      if (stat?.isSymbolicLink()) throw new Error('符号链接或目录联接不属于可自动授权的路径。');
      if (stat && !stat.isDirectory() && !stat.isFile()) throw new Error('目标不是普通文件。');
      if (stat) {
        const canonical = path.relative(this.root,await fs.realpath(current)).replace(/\\/g,'/');
        if (canonical.startsWith('../') || path.isAbsolute(canonical) || protectedPath(canonical) || key(canonical) !== key(path.relative(this.root,current).replace(/\\/g,'/'))) throw new Error('规范路径与请求路径不一致或指向受保护文件。');
      }
    }
    return rel;
  }
  async check(tool: string, input: any): Promise<PermissionDecision> {
    const deny = (reason: string, target = '', operation = tool): PermissionDecision => ({ behavior: 'deny', reason, target: safeReviewText(target).slice(0,500), operation });
    try {
      if (!input || typeof input !== 'object' || Array.isArray(input)) return deny('工具输入格式无效。');
      if (['Read','Glob','Grep'].includes(tool)) {
        const rel = await this.target(tool === 'Read' ? input.file_path : input.path || this.root, tool !== 'Read');
        const readStat = await fs.lstat(path.join(this.root,rel)).catch(() => undefined);
        if (readStat?.isFile() && readStat.nlink > 1) return deny('硬链接文件无法确认独立的授权来源，拒绝读取。',rel,'READ');
        // Globs/searches must not traverse dot/config paths. Explicit matching content only in ordinary project sources.
        if (tool !== 'Read' && (typeof input.pattern !== 'string' || input.pattern.length > 2000 || /\.git|\.env|\.claude|\.codex|\.ssh|\.aws|cc[-_ ]?switch|\.\.\//i.test(input.pattern))) return deny('搜索表达式包含受保护路径或无效输入。', rel);
        if (tool === 'Glob' && (input.pattern.includes('..') || path.isAbsolute(input.pattern) || input.pattern.includes(':') || input.pattern.startsWith('/'))) return deny('Glob只能匹配工作区内的相对路径。', rel);
        // A directory-wide search could follow aliases or read secrets; require explicit ordinary-file targets for Grep.
        if (tool === 'Grep') {
          const stat = await fs.lstat(path.join(this.root, rel)).catch(() => undefined);
          if (!stat?.isFile()) return deny('Grep仅允许指定普通文件；请先用Glob列出文件，再逐个读取。', rel);
        }
        return { behavior: 'allow', reason: '工作区内普通文件只读操作。', target: rel || '.', operation: 'READ' };
      }
      if (tool === 'Edit' || tool === 'Write') {
        const rel = await this.target(input.file_path);
        if (!this.scope.allowedFiles.some(f => key(f) === key(rel))) return deny('该文件不在本次任务允许修改的精确文件列表中；不能用一次审批扩大范围。', rel, 'WRITE');
        const stat = await fs.lstat(path.join(this.root, rel)).catch(() => undefined);
        if (stat && (!stat.isFile() || stat.nlink > 1)) return deny('目录、非普通文件或硬链接目标禁止修改。', rel, 'WRITE');
        const replacement = tool === 'Write' ? input.content : input.new_string;
        if (typeof replacement !== 'string') return deny('缺少写入内容，无法审查操作。', rel, 'WRITE');
        const empty = !replacement.trim() && (tool === 'Write' || typeof input.old_string === 'string' && stat?.size === Buffer.byteLength(input.old_string,'utf8'));
        const config = configuration(rel) || !ordinarySource(rel);
        return { behavior: config || empty ? 'ask' : 'allow', target: rel, operation: config ? 'CONFIG_WRITE' : empty ? 'TRUNCATE' : 'WRITE',
          reason: config ? '工程配置或无法确定类型的文件变更需要人工审批。' : empty ? '清空文件需要人工审批。' : '文件位于本次任务授权范围。',
          preview: safeReviewText(replacement).slice(0,1200) };
      }
      if (tool === 'Bash') {
        const command = input.command;
        if(!validCommandInput(input))return deny('终端参数包含未知选项、工作目录覆盖或无效超时；默认拒绝。','','COMMAND');
        if (this.repair && !(typeof command === 'string' && /^rm /.test(command))) return deny('定向修复不授权终端命令；编译/测试由用户在修复面板另行选择。', '', 'COMMAND');
        if (typeof command !== 'string' || command.length > 2000 || input.run_in_background || input.dangerouslyDisableSandbox || /[\r\n;&|<>`$%!\x00]/.test(command)) return deny('复合、重定向、后台、绕过沙箱或含替换符的命令默认拒绝。', '', 'COMMAND');
        if ((commandCatalog as readonly string[]).includes(command)) return { behavior: this.scope.safeCommands.includes(command) ? 'allow' : 'ask', operation: 'TEST_COMMAND', target: command,
          reason: this.scope.safeCommands.includes(command) ? '匹配用户配置的精确命令白名单。' : '此测试/构建命令尚未加入本次任务白名单；它会执行项目脚本。' };
        const match = command.match(/^rm (?:-- )?(?:"([a-zA-Z0-9_./ -]+)"|([a-zA-Z0-9_./-]+))$/);
        if (match) {
          const rel = await this.target(match[1] || match[2]);
          if (this.repair && !this.repair.deleteFiles.some(f => key(f) === key(rel))) return deny('该文件未在修复方案中单独批准删除。', rel, 'DELETE');
          if (!this.scope.allowedFiles.some(f => key(f) === key(rel))) return deny('删除目标不在本次任务允许文件范围。', rel, 'DELETE');
          const stat = await fs.lstat(path.join(this.root, rel));
          if (!stat.isFile() || stat.nlink > 1 || (match[1] || match[2]).startsWith('-')) return deny('只支持删除授权范围内的单个普通文件；目录及选项参数默认拒绝。', rel, 'DELETE');
          return { behavior: 'ask', operation: 'DELETE', target: rel, reason: '删除单个文件必须人工审批；不支持递归、强制或目录删除。' };
        }
        return deny('未知或危险终端命令默认拒绝；不能通过人工审批放行任意Shell代码。', command, 'COMMAND');
      }
      return deny('本任务未启用该工具，默认拒绝。');
    } catch (e) { return deny(e instanceof Error ? e.message : '无法验证路径或操作，默认拒绝。'); }
  }
}
export const operationFingerprint = (tool: string, input: unknown) => createHash('sha256').update(JSON.stringify([tool,input])).digest('hex');
export const validCommandInput=(input:any)=>input && typeof input==='object'&&!Array.isArray(input)&&
  Object.keys(input).every(k=>['command','description','timeout','run_in_background','dangerouslyDisableSandbox'].includes(k))&&
  (input.timeout===undefined||Number.isFinite(input.timeout)&&input.timeout>0&&input.timeout<=120000);
