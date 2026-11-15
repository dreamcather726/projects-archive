export interface WorkspaceLog {
  id: number;
  time: string;
  level: string;
  source: string;
  message: string;
}
export class WorkspaceTelemetry {
  private previousRequest = "";
  private previousError = "";
  private activities = new Set<string>();
  constructor(
    private add: (message: string, level?: string, source?: string) => void,
  ) {}
  observe(chat: any): void {
    const request = chat?.conversation?.requests?.at(-1);
    const key = request
      ? `${chat.projectPath}:${request.id}:${request.status}`
      : "";
    if (key && key !== this.previousRequest) {
      this.previousRequest = key;
      this.add(
        `GPT 请求 ${request.status}${request.usage?.total != null ? ` · 本轮 ${request.usage.total} Tokens` : ""}`,
        request.status === "FAILED" ? "ERROR" : "INFO",
        "GPT",
      );
    }
    const error = chat?.error || "";
    if (error && error !== this.previousError) this.add(error, "ERROR", "GPT");
    this.previousError = error;
    for (const activity of chat?.activities || []) {
      const id = `${request?.id}:${activity}`;
      if (!this.activities.has(id)) {
        this.activities.add(id);
        this.add(activity, "INFO", "Tool");
      }
    }
    if (this.activities.size > 500) this.activities.clear();
  }
}
