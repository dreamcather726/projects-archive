(() => {
  const el = (tag, text, cls) => {
    const n = document.createElement(tag);
    if (text !== undefined) n.textContent = text;
    if (cls) n.className = cls;
    return n;
  };
  const button = (text, title, fn, cls = "icon-button") => {
    const b = el("button", text, cls);
    b.type = "button";
    b.title = title;
    b.setAttribute("aria-label", title);
    b.addEventListener("click", fn);
    return b;
  };
  class ChatPanel {
    constructor(slot, post, actions, draft) {
      this.slot = slot;
      this.post = post;
      this.actions = actions;
      this.isClaude = slot.id === "claude-01";
      this.hiddenMessages = new Set();
      this.rendered = new Map();
      this.node = el(
        "section",
        undefined,
        `chat-panel ${slot.kind === "gpt" ? "gpt-panel" : "claude-panel"}`,
      );
      this.node.dataset.panelId = slot.id;
      this.header = el("header", undefined, "chat-header");
      const avatar = el(
        "span",
        slot.kind === "gpt" ? "◎" : "✳",
        "panel-avatar",
      );
      this.title = el("h2", slot.title);
      this.status = el("span", "未接入", "status-pill muted");
      const tools = el("div", undefined, "panel-tools");
      this.collapse = button("−", "折叠对话面板", () =>
        actions.collapse(slot.id),
      );
      this.maximize = button("↗", "最大化 / 恢复对话", () =>
        actions.maximize(slot.id),
      );
      tools.append(
        this.collapse,
        this.maximize,
        button("×", "移除此窗口（保留历史）", () => actions.remove(slot.id)),
      );
      this.header.append(avatar, this.title, this.status, tools);
      this.content = el("div", undefined, "chat-content");
      this.meta = el("div", undefined, "chat-meta");
      this.error = el("div", undefined, "chat-error");
      this.error.setAttribute("role", "alert");
      this.messages = el("div", undefined, "chat-messages");
      this.messages.setAttribute("role", "log");
      this.messages.setAttribute("aria-label", `${slot.title} 独立聊天记录`);
      this.empty = el("div", undefined, "chat-empty");
      this.session = el("details", undefined, "session-details");
      this.session.append(el("summary", "会话与 CLI"));
      this.sessionText = el("p");
      this.session.append(this.sessionText);
      this.composer = el("div", undefined, "chat-composer");
      this.input = el("textarea");
      this.input.rows = 2;
      this.input.maxLength = 100000;
      this.input.setAttribute("aria-label", `向 ${slot.title} 发送消息`);
      this.input.placeholder =
        slot.kind === "gpt"
          ? "描述需求，与 GPT 总指挥讨论…"
          : "Claude 尚未接入，此处暂不可发送";
      this.input.value = draft || "";
      this.input.addEventListener("input", () =>
        actions.draft(slot.id, this.input.value),
      );
      this.send = button(
        "发送 ↗",
        "发送消息",
        () => this.submit(),
        "primary-button",
      );
      this.stop = button("■", "停止当前响应", () =>
        post({ type: "stop", panelId: slot.id }),
      );
      const composeTools = el("div", undefined, "compose-tools");
      composeTools.append(
        el("span", "Enter 发送 · Shift+Enter 换行", "hint"),
        this.stop,
        this.send,
      );
      this.composer.append(this.input, composeTools);
      this.outputMode = el('select'); this.outputMode.setAttribute('aria-label', '输出显示模式 '+slot.title);
      for(const [value,label] of [['formatted','格式化'],['raw','原始'],['debug','调试']]){const option=el('option',label);option.value=value;this.outputMode.append(option);}
      this.outputMode.addEventListener('change',()=>{for(const entry of this.rendered.values())entry.text=undefined;if(this.lastChat)this.update(this.slot,this.lastChat,this.lastLayout);});
      tools.prepend(this.outputMode);
      if (this.isClaude) {
        this.claudeTools = el("div", undefined, "claude-actions");
        for (const [label, type] of [
          ["检测 CLI", "check"],
          ["新建", "new"],
          ["历史", "history"],
          ["选择项目", "project"],
          ['任务权限', 'permissions'],
        ])
          this.claudeTools.append(
            button(
              label,
              `Claude: ${label}`,
              () => post({ type, panelId: slot.id }),
              "secondary-button",
            ),
          );
        this.claudeTools.append(
          button(
            "清空显示",
            "清空 Claude 当前展示；保留 Session 与历史",
            () => {
              for (const m of this.lastChat?.conversation?.messages || [])
                this.hiddenMessages.add(m.id);
              this.messages.replaceChildren();
              this.rendered.clear();
              this.update(this.slot, this.lastChat, this.lastLayout);
            },
            "secondary-button",
          ),
        );
        this.activityView = el(
          "details",
          undefined,
          "session-details claude-activities",
        );
        this.activityView.append(el("summary", "实时状态与工具活动"));
        this.activityView.open = true;
        this.activityText = el("p");
        this.activityView.append(this.activityText);
      }
      this.input.addEventListener("keydown", (event) => {
        if (event.key === "Enter" && !event.shiftKey && !event.isComposing) {
          event.preventDefault();
          this.submit();
        }
      });
      this.content.append(
        this.meta,
        this.error,
        this.messages,
        this.session,
        this.composer,
      );
      if (this.isClaude) {
        this.meta.after(this.claudeTools);
        this.session.before(this.activityView);
      }
      this.node.append(this.header, this.content);
      if (slot.kind === "gpt") {
        const intro = el("p", "需求分析 · 任务规划 · 执行调度 · 独立审查", "commander-role");
        this.header.after(intro);
        this.gptTools = el("div", undefined, "gpt-actions");
        for (const [label, type] of [["历史会话", "history"], ["新会话", "new"], ["选择项目", "project"], ["检测 CLI", "check"]])
          this.gptTools.append(button(label, "GPT: " + label, () => post({ type, panelId: "gpt" }), "secondary-button"));
        this.gptTools.append(button('用量与预算','GPT: 用量与预算',()=>post({type:'teamUsageConfigure',projectPath:this.lastChat?.projectPath}),'secondary-button'));
        this.meta.after(this.gptTools);
      }
      this.node.addEventListener("pointerdown", () =>
        actions.activate(slot.id),
      );
      if (slot.kind !== "gpt") {
        this.input.disabled = true;
        this.send.disabled = true;
        this.stop.disabled = true;
        this.meta.textContent = "专业角色：未分配  /  当前任务：未分配";
        this.empty.append(
          el("div", "✳", "empty-orbit"),
          el(
            "h3",
            slot.id === "claude-01"
              ? "Claude Worker 01 尚未接入"
              : "为下一位协作者留一个位置",
          ),
          el(
            "p",
            "此窗口仅为布局预留。没有启动 Agent，也没有生成会话或聊天记录。",
          ),
          el("span", "SESSION ID  —", "empty-session"),
        );
        this.messages.append(this.empty);
        this.sessionText.textContent = "未建立会话";
      }
    }
    setCommanderContext(node) {
      if (this.slot.kind === "gpt" && node.parentElement !== this.content) this.session.before(node);
    }
    submit() {
      if (
        (this.slot.kind !== "gpt" && !this.isClaude) ||
        this.send.disabled ||
        !this.input.value.trim()
      )
        return;
      this.pending = this.input.value;
      this.pendingExisting = new Set(
        (this.lastChat?.conversation?.messages || []).map((m) => m.id),
      );
      this.send.disabled = true;
      this.post({
        type: "send",
        panelId: this.slot.id,
        text: this.input.value,
      });
    }
    update(slot, chat, layout, dispatch = this.lastDispatch) {
      this.lastDispatch = dispatch;
      this.slot = slot;
      this.node.classList.toggle("collapsed", slot.collapsed);
      this.node.classList.toggle("active", layout.activePanel === slot.id);
      this.node.classList.toggle("maximized", layout.maximized === slot.id);
      this.collapse.textContent = slot.collapsed ? "+" : "−";
      this.collapse.setAttribute("aria-expanded", String(!slot.collapsed));
      if (slot.kind !== "gpt" && !(this.isClaude && chat.connectedAdapter))
        return;
      if (this.isClaude && !this.adapterReady) {
        this.adapterReady = true;
        this.messages.replaceChildren();
        this.rendered.clear();
      }
      this.lastChat = chat;
      this.lastLayout = layout;
      if (
        this.conversationId &&
        this.conversationId !== chat.conversation?.id
      ) {
        this.input.value = "";
        this.pending = undefined;
        this.actions.draft(slot.id, "");
      }
      if (this.isClaude) {
        this.input.disabled = false;
        this.input.placeholder =
          "向 Claude Worker 01 发送任务…（可修改所选项目）";
      }
      if (this.project && this.project !== chat.projectPath) {
        this.input.value = "";
        this.actions.draft(slot.id, "");
        this.pending = undefined;
      }
      this.project = chat.projectPath;
      const conversation = chat.conversation;
      this.status.textContent = chat.busy && chat.retry?.kind==='RETRY_SCHEDULED'
        ? '自动重试中' : chat.busy
        ? "响应中"
        : conversation?.status === "CANCELLED"
          ? "已停止"
          : chat.connection || "尚未检测";
      this.status.className = `status-pill ${chat.busy || this.status.textContent.includes("已连接") ? "online" : "muted"}`;
      if (slot.kind === "gpt") {
        const connection = window.ATMGPTCommanderPanel.connection(chat);
        this.status.textContent = connection.text;
        this.status.dataset.connection = connection.kind;
        this.status.title = connection.detail;
        this.status.className = "status-pill connection-" + connection.kind;
        for (const action of this.gptTools.querySelectorAll("button")) action.disabled = !!chat.busy;
      }
      this.meta.textContent = `${this.isClaude ? "文件执行" : "分析 / 受控派发"} · ${chat.projectName || "未选择项目"} · ${conversation?.threadId ? "多轮会话已保存" : "等待首条消息"}`;
      this.error.textContent = chat.error || "";
      this.send.disabled = !!chat.busy;
      this.stop.disabled = !chat.busy;
      if (
        this.pending &&
        conversation?.messages.some(
          (m) =>
            m.role === "user" &&
            m.text === this.pending &&
            !this.pendingExisting?.has(m.id),
        )
      ) {
        this.input.value = "";
        this.actions.draft(slot.id, "");
        this.pending = undefined;
      }
      if (!chat.busy) this.pending = undefined;
      if (this.isClaude) {
        for (const b of this.claudeTools.querySelectorAll("button"))
          b.disabled = !!chat.busy;
        this.activityText.textContent = [
          dispatch?.session_id && dispatch.session_id === conversation?.threadId
            ? `调度：${dispatch.state} · GPT审查：${dispatch.rounds?.at(-1)?.review?.verdict || "尚未验收"}`
            : chat.statusEvent || "未启动任务",
          ...(chat.activities || []).map(
            (a) => `${a.name} · ${a.phase}${a.detail ? ` · ${a.detail}` : ""}`,
          ),
        ].join("\n");
      }
      if (this.conversationId !== conversation?.id) this.hiddenMessages.clear();
      const messages = (conversation?.messages || []).filter(
        (m) => !this.hiddenMessages.has(m.id),
      );
      const nearBottom =
        this.messages.scrollHeight -
          this.messages.scrollTop -
          this.messages.clientHeight <
        60;
      if (this.conversationId !== conversation?.id) {
        this.messages.replaceChildren();
        this.rendered.clear();
        this.conversationId = conversation?.id;
      }
      if (!messages.length) {
        if (!this.messages.querySelector(".chat-empty")) {
          this.empty.replaceChildren(
            el("div", this.isClaude ? "✳" : "◎", "empty-orbit"),
            el(
              "h3",
              this.isClaude
                ? this.hiddenMessages.size
                  ? "当前对话展示已清空"
                  : "Claude Worker 01 已接入"
                : "让需求，从一次对话开始",
            ),
            el(
              "p",
              this.isClaude
                ? "使用本机已有 Claude 授权。发送任务后启动 CLI，可读取与修改当前项目；停止不会回滚已发生的修改。"
                : "讨论方案，或让GPT通过真实工具派发Claude开发。文件与命令受任务审批和执行前权限检查约束。",
            ),
            el(
              "div",
              this.isClaude
                ? "独立 Session  /  真实流式输出  /  按需执行"
                : "需求分析  /  技术决策  /  任务规划",
              "empty-topics",
            ),
          );
          this.messages.replaceChildren(this.empty);
        }
      } else {
        this.empty.remove();
        for (const message of messages) {
          let entry = this.rendered.get(message.id);
          if (!entry) {
            const article = el(
              "article",
              undefined,
              `chat-message ${message.role}`,
            );
            const info = el("div", undefined, "message-meta"),
              body = el("div", undefined, "message-body");
            article.append(info, body);
            this.messages.append(article);
            entry = { article, info, body };
            this.rendered.set(message.id, entry);
          }
          entry.article.dataset.messageType = message.role === "user" ? "user" : this.isClaude ? "claude" : "gpt";
          entry.info.textContent = `${message.role === "user" ? "你" : this.slot.title}  ·  ${new Date(message.time).toLocaleTimeString()}  ·  ${message.status}`;
          if (entry.text !== message.text) {
            entry.body.replaceChildren();
            if (message.role === "assistant")
              entry.body.append(
                (window.ATMOutputFormatter?.render || window.renderGPTMarkdown)(message.text, (text) =>
                  this.post({ type: "copy", panelId: this.slot.id, text }),
                  this.outputMode.value,{session_id:conversation.threadId,message_id:message.id,status:message.status,time:message.time},
                ),
              );
            else entry.body.textContent = message.text;
            entry.text = message.text;
          }
        }
      }
      if (nearBottom) this.messages.scrollTop = this.messages.scrollHeight;
      const usage = conversation?.requests?.at(-1)?.usage;
      const number = (n) => (n == null ? "--" : n.toLocaleString());
      this.sessionText.textContent = `Session ID: ${conversation?.threadId || "--"}\nCLI: ${chat.cli?.command || "尚未检测"}\n本轮输入 ${number(usage?.input)} · 缓存 ${number(usage?.cached)} · 输出 ${number(usage?.output)}\n缓存写入 ${number(usage?.cacheWrite)} · 推理 Token ${number(usage?.reasoning)}\n会话累计 ${number(chat.usage?.total)} · 调用 ${conversation?.requests?.length ?? 0} 次`;
      if (this.isClaude)
        this.sessionText.textContent = `Session ID: ${conversation?.threadId || "--"}\nCLI: ${chat.cli?.command || "尚未检测"}\n版本: ${chat.cli?.version || "--"}\nCLI usage：普通输入 ${number(usage?.input)} · 缓存读取 ${number(usage?.cached)} · 缓存写入 ${number(usage?.cacheWrite)} · 输出 ${number(usage?.output)}\n本轮合计 ${number(usage?.total)} · 调用 ${conversation?.requests?.length ?? 0} 次`;
    }
  }
  window.ATMChatPanel = ChatPanel;
  window.ATMElements = { el, button };
})();
