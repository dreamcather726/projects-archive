(() => {
  const { el, button } = window.ATMElements;
  class ReviewPanel {
    constructor(post) {
      this.post = post;
      this.node = el("section", undefined, "code-review-panel");
      this.node.setAttribute("aria-label", "GPT 独立代码审查");
      this.node.append(el("h3", "GPT 独立代码审查"));
      this.actions = el("div", undefined, "review-actions");
      for (const [label, type] of [
        ["审查结果", "reviewStart"],
        ["执行测试", "reviewTests"],
        ["历史", "reviewHistory"],
        ["停止审查", "reviewStop"],
      ]) {
        const b = button(
          label,
          label,
          () => post({ type }),
          "secondary-button",
        );
        b.dataset.reviewAction = type;
        this.actions.append(b);
      }
      this.status = el("p");
      this.summary = el("div");
      this.stats = el("p", undefined, "muted-text");
      this.details = el("details");
      this.details.append(el("summary", "问题详情与六维审查"));
      this.body = el("div", undefined, "review-details");
      this.details.append(this.body);
      this.history = el("select");
      this.history.setAttribute("aria-label", "审查历史");
      this.history.addEventListener("change", () =>
        post({ type: "reviewSelect", reviewId: this.history.value }),
      );
      this.node.append(
        this.actions,
        this.status,
        this.summary,
        this.stats,
        this.details,
        this.history,
      );
      this.key = "";
    }
    update(state = {}) {
      this.status.textContent =
        state.error ||
        (state.busy
          ? state.progress || "采集审查证据中"
          : "仅审查，不自动修改或纠偏");
      for (const b of this.actions.querySelectorAll("button"))
        b.disabled =
          b.dataset.reviewAction === "reviewStop"
            ? !(state.canStop ?? state.busy)
            : !!state.busy;
      const report = state.report,
        key = JSON.stringify([report?.review_id, state.history]);
      if (this.key === key) return;
      this.key = key;
      this.summary.replaceChildren();
      this.body.replaceChildren();
      this.history.replaceChildren(el("option", "选择审查历史"));
      for (const h of state.history || []) {
        const option = el(
          "option",
          `${h.verdict} · ${h.created_at} · ${h.task}`,
        );
        option.value = h.review_id;
        option.selected = h.review_id === report?.review_id;
        this.history.append(option);
      }
      if (!report) {
        this.summary.append(
          el("p", "尚未运行代码审查。先执行测试，再审查 Claude 当前结果。"),
        );
        this.stats.textContent = "";
        return;
      }
      this.summary.append(
        el(
          "strong",
          report.verdict,
          `review-verdict verdict-${report.verdict.toLowerCase()}`,
        ),
        el("p", report.summary),
        el("small", report.created_at),
      );
      this.stats.textContent = ["CRITICAL", "HIGH", "MEDIUM", "LOW", "INFO"]
        .map(
          (s) =>
            `${s} ${(report.issues || []).filter((i) => i.severity === s).length}`,
        )
        .join(" · ");
      for (const text of [
        ...(report.missing_evidence || []),
        ...(report.warnings || []),
      ])
        this.body.append(el("p", text, "review-warning"));
      for (const d of report.dimensions || [])
        this.body.append(
          el("strong", `${d.category} · ${d.status}`),
          el("p", d.summary),
        );
      for (const issue of report.issues || []) {
        const card = el("article", undefined, "review-issue");
        card.append(
          el(
            "strong",
            `${issue.issue_id} · ${issue.severity} · ${issue.category}`,
          ),
        );
        const locate = button(
          `${issue.file_path}:${issue.line_number || "?"} (${issue.basis})`,
          `定位 ${issue.issue_id}`,
          () =>
            this.post({
              type: "reviewOpenIssue",
              reviewId: report.review_id,
              issueId: issue.issue_id,
            }),
          "text-button",
        );
        locate.disabled =
          !issue.line_number || issue.evidence_status !== "VERIFIED";
        card.append(
          locate,
          el("p", issue.problem_description),
          el("pre", issue.evidence),
          el("p", `影响：${issue.impact}`),
          el("p", `建议：${issue.suggested_fix}`),
          el(
            "small",
            `置信度 ${issue.confidence} · 证据 ${issue.evidence_status}`,
          ),
        );
        this.body.append(card);
      }
    }
  }
  window.ATMReviewPanel = ReviewPanel;
})();
