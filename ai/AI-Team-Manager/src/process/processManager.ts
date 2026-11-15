import { execFile, type ChildProcess } from 'node:child_process';
export async function terminateTree(child: ChildProcess): Promise<void> {
  if (!child.pid) return;
  if (process.platform === 'win32') {
    await new Promise<void>((resolve, reject) => {
      execFile(`${process.env.SystemRoot || 'C:\\Windows'}\\System32\\taskkill.exe`, ['/pid',String(child.pid),'/T','/F'], {windowsHide:true,timeout:10000}, error => {
        if (error && child.exitCode === null && child.signalCode === null) reject(new Error('无法确认进程树已终止，请在系统任务管理器检查该 Codex 进程。'));
        else resolve();
      });
    });
  } else {
    try { process.kill(-child.pid,'SIGKILL'); } catch (error: any) { if (error.code !== 'ESRCH') throw error; }
  }
}
