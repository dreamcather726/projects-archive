(() => {
  const {el,button}=window.ATMElements;
  class DispatchPanel{
    constructor(post){
      this.node=el('section',undefined,'dispatch-panel');this.node.setAttribute('aria-label','GPT主动调度');
      this.title=el('h3','GPT → Claude 调度');this.mode=el('p');this.status=el('p');this.scope=el('p');this.error=el('p',undefined,'review-warning');
      this.configure=button('审批模式 / 轮数','配置调度审批模式和最大轮数',()=>post({type:'dispatchConfigure'}),'secondary-button');
      this.stop=button('停止任务','停止真实Claude任务、审批、测试和审查',()=>post({type:'dispatchStop'}),'secondary-button');
      this.report=button('完整记录','查看任务、计划、审批、Diff、测试和审查记录',()=>post({type:'dispatchReport'}),'secondary-button');
      this.plan=button('开发报告','查看自适应任务规划报告',()=>post({type:'dispatchPlanReport'}),'secondary-button');
      this.planning=el('p');
      this.pending=el('div');this.details=el('details');this.details.append(el('summary','调度与审批记录'));this.events=el('div',undefined,'permission-logs');this.details.append(this.events);
      const actions=el('div',undefined,'review-actions');actions.append(this.configure,this.stop,this.report,this.plan);this.node.append(this.title,this.mode,this.status,this.scope,this.error,this.planning,actions,this.pending,this.details);
    }
    update(s={}){
      this.configure.disabled=!!s.busy;this.stop.disabled=!s.busy;this.report.disabled=!s.task;
      const plan=s.planning?.plans?.[0];this.plan.disabled=!plan;
      this.planning.textContent=plan?`开发计划：${plan.goal}\n${plan.task_type} · ${plan.worker_count}个Worker槽位 · ${plan.tasks.length}项任务\n规划已验证，尚未执行。`:'';
      this.mode.textContent=`${s.mode||'GPT_HYBRID'} · 最多 ${s.max_rounds||2} 轮（含首轮）`;
      if(s.gpt_usage?.calls)this.mode.textContent+=`\n独立审批/审查 GPT Token：已知 ${s.gpt_usage.known_total} · 未知 ${s.gpt_usage.unknown_calls} 次（主对话另计）`;
      const r=s.task,last=r?.rounds?.at(-1);
      this.status.textContent=r?`${r.state} · 第 ${r.rounds.length} 轮 · ${r.input.description}`:'在GPT窗口输入开发需求即可派发；无需复制到Claude。';
      this.scope.textContent=r?`允许：${r.input.allowed_files.join(', ')}\n禁止：${r.input.forbidden_files.join(', ')||'所有其他文件'}\nSession：${r.session_id||'尚未创建'}\n审查：${last?.review?.verdict||'尚未验收'}`:'';
      this.error.textContent=s.error||r?.error||'';this.pending.replaceChildren();
      if(s.pending){const p=s.pending;this.pending.append(el('strong','等待VS Code原生任务审批'),el('p',p.reason),el('pre',JSON.stringify(p.input,null,2)));}
      this.events.replaceChildren();for(const e of [...r?.events||[]].reverse())this.events.append(el('p',`${e.time} · ${e.kind}\n${JSON.stringify(e.data).slice(0,1000)}`));
      if(s.note)this.events.append(el('small',s.note));
    }
  }
  window.ATMDispatchPanel=DispatchPanel;
})();
