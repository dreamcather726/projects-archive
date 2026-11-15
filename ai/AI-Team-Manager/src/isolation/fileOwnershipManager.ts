import { plannedPath } from '../planning/adaptiveTaskPlanner';
import type { DevelopmentPlan, PlannedTask } from '../planning/planningTypes';
export class FileOwnershipManager {
  constructor(private plan: DevelopmentPlan) {}
  task(taskId: string): PlannedTask { const task = this.plan.tasks.find(t => t.task_id === taskId); if (!task) throw Error('任务不属于当前已验证计划。'); return task; }
  mayRead(taskId: string, file: string) { const task = this.task(taskId), name = plannedPath(file); return [...task.allowed_files, ...task.read_only_files].some(f => plannedPath(f) === name); }
  mayWrite(taskId: string, file: string) {
    const task = this.task(taskId), name = plannedPath(file), shared = this.plan.shared_file_owners.find(f => plannedPath(f.file) === name);
    return (!shared || shared.task_id === taskId) && task.allowed_files.some(f => plannedPath(f) === name) && !task.forbidden_files.some(f => plannedPath(f) === name);
  }
}
