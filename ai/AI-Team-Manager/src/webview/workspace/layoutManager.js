(() => {
  const defaults = () => ({
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
  });
  class LayoutManager {
    constructor(shell, saved, changed) {
      this.shell = shell;
      this.state = saved?.version === 1 ? saved : defaults();
      this.state.navigation = window.ATMWorkspaceRouter.normalize(this.state.navigation, this.state.panels, this.state.activePanel);
      this.changed = changed;
      this.apply();
    }
    persist() {
      this.apply();
      this.changed(this.state);
    }
    apply() {
      const s = this.state,
        n = this.shell;
      n.dataset.mode = s.mode;
      n.dataset.maximized = s.maximized || "";
      n.dataset.inspectorCollapsed = String(s.inspectorCollapsed);
      n.dataset.logsCollapsed = String(s.logsCollapsed);
      if (s.inspectorCollapsed) n.classList.remove("inspector-open");
      n.style.setProperty("--row-ratio", String(s.rowSplit / 100));
      for (const [key, value] of Object.entries({
        "--nav-width": `${s.navWidth}px`,
        "--inspector-width": `${s.inspectorWidth}px`,
        "--log-height": `${s.logsCollapsed ? 42 : s.logHeight}px`,
        "--split": `${s.split}%`,
        "--row-split": `${s.rowSplit}%`,
      }))
        n.style.setProperty(key, value);
    }
    setMode(mode) {
      this.state.mode = mode;
      this.state.maximized = null;
      const count = { single: 1, dual: 2, triple: 3, quad: 4 }[mode] || 0;
      while (this.state.panels.length < count) this.add(false);
      this.persist();
    }
    add(save = true) {
      if (this.state.panels.length >= 12) return;
      const id = `reserved-${Date.now().toString(36)}-${Math.random().toString(36).slice(2, 6)}`;
      this.state.panels.push({
        id,
        kind: "claude-placeholder",
        title: "预留对话窗口",
        collapsed: false,
      });
      if (save) {
        this.state.mode = "auto";
        this.state.activePanel = id;
        this.state.maximized = null;
        this.persist();
      }
    }
    remove(id) {
      this.state.panels = this.state.panels.filter((p) => p.id !== id);
      if (!this.state.panels.length)
        this.state.panels = defaults().panels.slice(0, 1);
      if (this.state.activePanel === id)
        this.state.activePanel = this.state.panels[0].id;
      if (this.state.maximized === id) this.state.maximized = null;
      this.persist();
    }
    focus(id) {
      if (id === "gpt" && !this.state.panels.some((p) => p.id === "gpt"))
        this.state.panels.unshift(defaults().panels[0]);
      this.state.activePanel = id;
      const panel = this.state.panels.find((p) => p.id === id);
      if (panel) panel.collapsed = false;
      this.persist();
    }
    toggleCollapse(id) {
      const p = this.state.panels.find((p) => p.id === id);
      if (p) {
        p.collapsed = !p.collapsed;
        this.persist();
      }
    }
    maximize(id) {
      this.state.maximized = this.state.maximized === id ? null : id;
      this.state.activePanel = id;
      const p = this.state.panels.find((p) => p.id === id);
      if (p) p.collapsed = false;
      this.persist();
    }
    reset() {
      this.state = defaults();
      this.persist();
    }
    bindResize(handle, key, measure, min, max, direction = 1) {
      handle.tabIndex = 0;
      handle.setAttribute("role", "separator");
      handle.setAttribute("aria-label", handle.title || "调整面板尺寸");
      handle.setAttribute("aria-valuemin", String(min));
      handle.setAttribute("aria-valuemax", String(max));
      handle.setAttribute("aria-valuenow", String(this.state[key]));
      const vertical = ["logHeight", "rowSplit"].includes(key);
      handle.setAttribute(
        "aria-orientation",
        vertical ? "horizontal" : "vertical",
      );
      handle.addEventListener("pointerdown", (event) => {
        event.preventDefault();
        handle.setPointerCapture?.(event.pointerId);
        document.body.classList.add("resizing");
        const origin = vertical ? event.clientY : event.clientX,
          initial = this.state[key],
          scale = measure();
        const move = (e) => {
          this.state[key] = Math.max(
            min,
            Math.min(
              max,
              initial +
                ((vertical ? e.clientY : e.clientX) - origin) *
                  direction *
                  scale,
            ),
          );
          this.apply();
        };
        const done = () => {
          document.removeEventListener("pointermove", move);
          document.removeEventListener("pointerup", done);
          document.removeEventListener("pointercancel", done);
          document.body.classList.remove("resizing");
          handle.setAttribute("aria-valuenow", String(this.state[key]));
          this.persist();
        };
        document.addEventListener("pointermove", move);
        document.addEventListener("pointerup", done, { once: true });
        document.addEventListener("pointercancel", done, { once: true });
      });
      handle.addEventListener("keydown", (event) => {
        if (
          !["ArrowLeft", "ArrowRight", "ArrowUp", "ArrowDown"].includes(
            event.key,
          )
        )
          return;
        event.preventDefault();
        const step = key === "split" || key === "rowSplit" ? 2 : 10;
        this.state[key] = Math.max(
          min,
          Math.min(
            max,
            this.state[key] +
              (["ArrowRight", "ArrowDown"].includes(event.key) ? step : -step) *
                direction,
          ),
        );
        handle.setAttribute("aria-valuenow", String(this.state[key]));
        this.persist();
      });
    }
  }
  window.ATMLayoutManager = LayoutManager;
})();
