// Front-end route registry. This module has no VS Code bridge or execution services.
(() => {
  const prefix = "collaboration/worker/";
  const definitions = [
    ["collaboration", "AI 协同开发", "◈"],
    ["task-center", "任务调度中心", "☷"],
    ["review-integration", "代码审查与集成", "◇"],
    ["projects", "项目管理", "▣"],
    ["settings", "系统设置", "⚙"],
  ];
  class WorkspaceRouter {
    constructor() {
      this.pages = new Map();
      for (const [id, title, icon] of definitions) this.register({ id, title, icon });
    }
    register(page) {
      if (!/^[a-z-]+$/.test(page.id) || this.pages.has(page.id)) throw new Error("Invalid or duplicate page ID");
      this.pages.set(page.id, Object.freeze({ ...page }));
    }
    resolve(route, panels) {
      if (this.pages.has(route)) return { page: this.pages.get(route), sessionId: route === "collaboration" ? "gpt" : null };
      const id = route?.startsWith(prefix) ? route.slice(prefix.length) : "";
      if (id !== "gpt" && panels.some(p => p.id === id))
        return { page: this.pages.get("collaboration"), sessionId: id };
      return { page: this.pages.get("collaboration"), sessionId: "gpt" };
    }
    static forSession(id) { return id === "gpt" ? "collaboration" : prefix + id; }
    static normalize(value, panels, active = "gpt") {
      const router = new WorkspaceRouter();
      const candidate = typeof value?.route === "string" ? value.route : WorkspaceRouter.forSession(active);
      const resolved = router.resolve(candidate, panels);
      const route = resolved.sessionId ? WorkspaceRouter.forSession(resolved.sessionId) : resolved.page.id;
      const hiddenTabs = Array.isArray(value?.hiddenTabs)
        ? [...new Set(value.hiddenTabs.filter(id => panels.some(p => p.id === id)))].filter(id => id !== resolved.sessionId) : [];
      return { route, hiddenTabs };
    }
  }
  window.ATMWorkspaceRouter = WorkspaceRouter;
})();
