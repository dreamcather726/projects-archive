export type LayoutMode = "single" | "dual" | "triple" | "quad" | "auto";
export interface WorkspacePanelSlot {
  id: string;
  kind: "gpt" | "claude-placeholder" | "team-worker";
  title: string;
  collapsed: boolean;
}
export interface WorkspaceLayout {
  version: 1;
  mode: LayoutMode;
  navWidth: number;
  inspectorWidth: number;
  logHeight: number;
  split: number;
  rowSplit: number;
  inspectorCollapsed: boolean;
  logsCollapsed: boolean;
  activePanel: string;
  maximized: string | null;
  panels: WorkspacePanelSlot[];
  navigation: { route: string; hiddenTabs: string[] };
}
export function defaultLayout(): WorkspaceLayout {
  return {
    version: 1,
    mode: "single",
    navigation: { route: "collaboration", hiddenTabs: [] },
    navWidth: 220,
    inspectorWidth: 420,
    logHeight: 180,
    split: 58,
    rowSplit: 50,
    inspectorCollapsed: false,
    logsCollapsed: true,
    activePanel: "gpt",
    maximized: null,
    panels: [
      { id: "gpt", kind: "gpt", title: "GPT 总指挥", collapsed: false },
      {
        id: "claude-01",
        kind: "claude-placeholder",
        title: "Claude Worker 01",
        collapsed: false,
      },
    ],
  };
}
export function validateLayout(value: unknown): WorkspaceLayout {
  const defaults = defaultLayout();
  if (!value || typeof value !== "object") return defaults;
  const raw = value as Record<string, unknown>;
  const clamp = (
    name: string,
    min: number,
    max: number,
    fallback: number,
  ): number =>
    typeof raw[name] === "number" && Number.isFinite(raw[name])
      ? Math.max(min, Math.min(max, raw[name] as number))
      : fallback;
  const panels: WorkspacePanelSlot[] = [];
  if (Array.isArray(raw.panels))
    for (const p of raw.panels.slice(0, 1012)) {
      if (
        !p ||
        typeof p.id !== "string" ||
        !/^[a-z0-9-]{1,60}$/.test(p.id) ||
        panels.some((x) => x.id === p.id)
      )
        continue;
      if (
        p.kind === "gpt" &&
        (p.id !== "gpt" || panels.some((x) => x.kind === "gpt"))
      )
        continue;
      if (p.kind !== "gpt" && p.kind !== "claude-placeholder" && p.kind !== "team-worker") continue;
      if(p.kind==='team-worker'&&!/^[a-f0-9-]{36}$/.test(p.id))continue;
      if (p.id === "gpt" && p.kind !== "gpt") continue;
      panels.push({
        id: p.id,
        kind: p.kind,
        title:
          p.kind === 'team-worker' ? 'Claude '+p.id.slice(0,8) : p.kind === "gpt"
            ? "GPT 总指挥"
            : p.id === "claude-01"
              ? "Claude Worker 01"
              : "预留对话窗口",
        collapsed: p.collapsed === true,
      });
    }
  const validPanels = panels.length ? panels : defaults.panels;
  const has = (id: unknown): id is string =>
    typeof id === "string" && validPanels.some((p) => p.id === id);
  const navigation = raw.navigation as Record<string, unknown> | undefined;
  const pages = ["collaboration", "task-center", "review-integration", "projects", "settings"];
  const candidate = typeof navigation?.route === "string" ? navigation.route :
    has(raw.activePanel) && raw.activePanel !== "gpt" ? `collaboration/worker/${raw.activePanel}` : "collaboration";
  const worker = candidate.startsWith("collaboration/worker/") ? candidate.slice(21) : undefined;
  const route = pages.includes(candidate) || (worker !== "gpt" && has(worker)) ? candidate : "collaboration";
  const sessionId = route === "collaboration" ? "gpt" : route.startsWith("collaboration/worker/") ? worker : null;
  const hiddenTabs = Array.isArray(navigation?.hiddenTabs)
    ? [...new Set(navigation.hiddenTabs.filter(has))].filter(id => id !== sessionId) : [];
  return {
    version: 1,
    navigation: { route, hiddenTabs },
    mode: ["single", "dual", "triple", "quad", "auto"].includes(
      String(raw.mode),
    )
      ? (raw.mode as LayoutMode)
      : defaults.mode,
    navWidth: clamp("navWidth", 180, 300, 220),
    inspectorWidth: clamp("inspectorWidth", 250, 520, 420),
    logHeight: clamp("logHeight", 100, 420, 180),
    split: clamp("split", 25, 75, 58),
    rowSplit: clamp("rowSplit", 25, 75, 50),
    inspectorCollapsed: raw.inspectorCollapsed === true,
    logsCollapsed: raw.logsCollapsed === true,
    panels: validPanels,
    activePanel: has(raw.activePanel) ? raw.activePanel : validPanels[0].id,
    maximized: has(raw.maximized) ? raw.maximized : null,
  };
}
