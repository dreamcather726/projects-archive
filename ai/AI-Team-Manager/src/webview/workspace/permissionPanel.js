(() => {
  const {el,button}=window.ATMElements;
  class PermissionPanel {
    constructor(post) {
      this.post=post;this.key='';this.node=el('section',undefined,'permission-panel');
      this.node.setAttribute('aria-label','Claude任务权限');
      this.node.append(el('h3','Claude任务权限'));
      this.mode=el('p');this.scope=el('p');this.error=el('p',undefined,'review-warning');
      this.configure=button('设置任务授权范围','设置Claude任务授权范围',()=>post({type:'permissions',panelId:'claude-01'}),'secondary-button');
      this.pending=el('div');this.pending.setAttribute('aria-live','polite');
      this.history=el('details');this.history.append(el('summary','授权记录与拒绝原因'));
      this.logs=el('div',undefined,'permission-logs');this.history.append(this.logs);
      this.node.append(this.mode,this.scope,this.error,this.configure,this.pending,this.history);
    }
    update(state={},busy=false) {
      this.configure.disabled=!!busy;
      const key=JSON.stringify(state);if(this.key===key)return;this.key=key;
      this.mode.textContent=`权限模式：${state.mode || 'manual（等待初始化）'}${state.active?' · 任务执行中':''}`;
      this.scope.textContent=`允许修改：${state.scope?.allowedFiles?.join(', ') || '无（只读）'}\n精确命令白名单：${state.scope?.safeCommands?.join(', ') || '无'}`;
      this.error.textContent=state.error || '';
      this.pending.replaceChildren();
      for(const p of state.pending || []) {
        const card=el('article',undefined,'permission-request');
        card.append(el('strong',`待审批：${p.tool} · ${p.operation}`),el('p',p.target),el('p',p.reason));
        if(p.preview)card.append(el('pre',p.preview));
        const actions=el('div',undefined,'review-actions');
        for(const [label,choice]of[['允许一次','ONCE'],['允许本次任务','TASK'],['拒绝','DENY']])actions.append(button(label,`${label} ${p.id}`,()=>this.post({type:'permissionAnswer',panelId:'claude-01',id:p.id,choice}),'secondary-button'));
        card.append(actions);this.pending.append(card);
      }
      this.logs.replaceChildren();
      for(const l of [...state.logs || []].reverse())this.logs.append(el('p',`${l.time} · ${l.decision} · ${l.tool}/${l.operation} · ${l.target}\n${l.source}：${l.reason}`,l.decision==='DENY'?'permission-denied':''));
      if(state.note)this.logs.append(el('p',state.note,'muted-text'));
      if(state.auditPath)this.logs.append(el('small',`完整授权日志：${state.auditPath}`));
    }
  }
  window.ATMPermissionPanel=PermissionPanel;
})();
