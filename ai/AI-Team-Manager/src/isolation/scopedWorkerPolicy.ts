import { PermissionPolicy } from '../permissions/permissionPolicy';
import type { PermissionDecision } from '../permissions/permissionTypes';
import { FileLockManager } from './fileLockManager';
import { FileOwnershipManager } from './fileOwnershipManager';

/** Used by the existing PreToolUse/PermissionRequest gateway. No shell or code execution tool is enabled. */
export class ScopedWorkerPolicy extends PermissionPolicy {
  constructor(root: string, private ownership: FileOwnershipManager, private locks: FileLockManager,
    private taskId: string, private workerId: string, private leaseId: string,
    private registered: () => Promise<void>, private mcpTools: string[] = []) {
    super(root, { allowedFiles: ownership.task(taskId).allowed_files, safeCommands: [] });
  }
  async check(tool: string, input: any): Promise<PermissionDecision> {
    const deny = (reason: string): PermissionDecision => ({ behavior: 'deny', operation: tool, target: '', reason });
    if (this.mcpTools.includes(tool)) return { behavior: 'allow', operation: 'ISOLATED_TOOL', target: tool, reason: '插件提供的受限工具；其入口还会独立验证当前Worker和精确测试命令。' };
    if (!['Read','Edit','Write','Glob','Grep'].includes(tool)) return deny('隔离Worker禁止任意终端、脚本、网络、Agent或外部工具；测试必须使用隔离执行入口。');
    const base = await super.check(tool, input);
    if (base.behavior === 'deny') return base;
    try {
      if (tool === 'Edit' || tool === 'Write') {
        await this.registered();
        if (!this.ownership.mayWrite(this.taskId, base.target)) return deny('当前任务不拥有此逻辑文件，禁止扩大范围。');
        await this.locks.assertHeld(this.leaseId, this.taskId, this.workerId, base.target);
      } else if (tool === 'Read' || tool === 'Grep') {
        if (!this.ownership.mayRead(this.taskId, base.target)) return deny('文件不在本任务允许读取的精确范围。');
      } else {
        // Only explicit owned/read-only files can be enumerated. A broad glob would expose unrelated names.
        if (typeof input.pattern !== 'string' || !this.ownership.mayRead(this.taskId, input.pattern)) return deny('Glob仅允许枚举本任务已授权的精确文件；目录遍历和通配搜索未开放。');
      }
      return base;
    } catch (e) { return deny(e instanceof Error ? e.message : '所有权或文件锁不可验证。'); }
  }
}
