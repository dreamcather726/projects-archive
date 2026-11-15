import crossSpawn from 'cross-spawn';
import type { ChildProcess } from 'node:child_process';
import { CodexError } from '../services/codexErrors';
export type Launcher = (command: string, args: string[], cwd: string, env?: NodeJS.ProcessEnv) => ChildProcess;
export const launch: Launcher = (command, args, cwd, env) => crossSpawn(command, args, {
  cwd, env: { ...process.env, ...env }, windowsHide: true, detached: process.platform !== 'win32', stdio: ['pipe','pipe','pipe']
});
export const COMMANDER_INSTRUCTIONS = `你是 AI Team Manager 的 GPT 总指挥。
你的主要职责是理解用户需求，分析当前 VS Code 项目，了解项目架构和技术约束，帮助用户制定开发计划，将复杂需求拆分成可执行任务，判断任务依赖关系，为后续 Claude Agent 团队制定任务书，负责后续开发任务的监督、纠偏和验收。
当前阶段只允许需求分析、项目理解、技术讨论和任务规划。你不能直接修改项目文件，不得执行 Git Commit、Git Push 或任何修改性操作。你不能声称已经启动 Claude，不能声称完成实际开发。所有回答必须以工作区实际内容为依据，不得假装读取不存在的文件。后续阶段再开放执行和 Agent 调度权限。
只允许使用只读沙箱，不得申请提权或绕过权限。不要读取密钥、认证文件或 CC Switch 配置，不使用外部应用或 MCP 执行修改性操作。`;
export interface CodexRunOptions { instructions?: string; config?: string[]; env?: NodeJS.ProcessEnv }
export function buildArgs(workspace: string, threadId?: string, options?: CodexRunOptions): string[] {
  if (threadId && !/^[a-zA-Z0-9_-]{1,128}$/.test(threadId)) throw new CodexError('SESSION_RESUME_FAILED');
  const args = ['exec','--json','--sandbox','read-only','--cd',workspace,
    '-c','approval_policy="never"','-c',`developer_instructions=${JSON.stringify(options?.instructions || COMMANDER_INSTRUCTIONS)}`];
  for (const config of options?.config || []) args.push('-c', config);
  return [...args, ...(threadId ? ['resume',threadId] : []), '-'];
}
