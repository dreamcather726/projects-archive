(() => {
  const {el, button} = window.ATMElements;
  class RepairPanel {
    constructor(post) {
      this.post = post; this.selected = new Set(); this.reportId = ''; this.key = '';
      this.node = el('section', undefined, 'repair-panel'); this.node.setAttribute('aria-label','定向修复');
      this.node.append(el('h3','GPT → Claude 定向修复'));
      this.status = el('p'); this.issues = el('details'); this.issues.open = true;
      this.issues.append(el('summary','选择审查问题'));
      this.filter = el('select'); this.filter.setAttribute('aria-label','修复问题严重程度');
      for (const s of ['ALL','CRITICAL','HIGH','MEDIUM','LOW','INFO']) { const o=el('option',s);o.value=s;this.filter.append(o); }
      this.filter.addEventListener('change',() => this.renderIssues());
      this.issueList=el('div'); this.issues.append(this.filter,this.issueList);
      this.requirements=el('textarea');this.requirements.placeholder='补充修复要求（可选）';this.requirements.maxLength=16000;this.requirements.setAttribute('aria-label','补充修复要求');
      this.manage=el('div',undefined,'review-actions');
      for(const [label,disposition] of [['忽略选中','IGNORED'],['人工处理','HUMAN'],['恢复问题','OPEN']]) this.manage.append(button(label,label,()=>post({type:'repairDisposition',reviewId:this.reportId,issueIds:[...this.selected],disposition}),'secondary-button'));
      this.generate=button('生成修复方案','生成修复方案',()=>post({type:'repairGenerate',reviewId:this.reportId,issueIds:[...this.selected],requirements:this.requirements.value}),'secondary-button');
      this.plan=el('div');this.history=el('select');this.history.setAttribute('aria-label','修复历史');this.history.addEventListener('change',()=>{if(this.history.value)post({type:'repairSelect',repairId:this.history.value});});
      this.node.append(this.status,this.issues,this.manage,this.requirements,this.generate,this.plan,this.history);
    }
    renderIssues() {
      this.issueList.replaceChildren();
      for(const i of this.report?.issues || []) {
        if(this.filter.value !== 'ALL' && i.severity !== this.filter.value)continue;
        const card=el('article',undefined,'review-issue'), label=el('label'),input=el('input');input.type='checkbox';input.checked=this.selected.has(i.issue_id);
        input.disabled=!!this.state?.busy;
        input.addEventListener('change',()=>{if(input.checked)this.selected.add(i.issue_id);else this.selected.delete(i.issue_id);this.updateSelection();});
        const disposition=(this.state?.dispositions || []).find(d=>d.review_id===this.reportId && d.issue_id===i.issue_id)?.disposition || 'OPEN';
        label.append(input,el('span',`${i.issue_id} · ${i.severity} · ${disposition}`));card.append(label,el('p',i.problem_description));
        const locate=button(`${i.file_path}:${i.line_number || '?'}`,`修复定位 ${i.issue_id}`,()=>this.post({type:'reviewOpenIssue',reviewId:this.reportId,issueId:i.issue_id}),'text-button');locate.disabled=i.evidence_status!=='VERIFIED';card.append(locate);this.issueList.append(card);
      }
      this.updateSelection();
    }
    updateSelection() {
      const eligible=[...this.selected].every(id=>this.report?.issues.some(i=>i.issue_id===id && i.evidence_status==='VERIFIED' && i.basis==='after'));
      this.generate.disabled=!!this.state?.busy || !this.selected.size || !eligible;
      this.generate.title=eligible?'生成修复方案':'缺少当前代码证据的问题请标记人工处理';
      for(const b of this.manage.querySelectorAll('button'))b.disabled=!!this.state?.busy || !this.selected.size;
    }
    update(state={}, review={}) {
      this.state=state;this.report=review.report;
      if(this.reportId!==this.report?.review_id){this.reportId=this.report?.review_id || '';this.selected.clear();}
      this.status.textContent=state.error || (state.busy?state.progress || '修复操作中':'先预览并批准；每次只执行一轮修复。');
      this.renderIssues();this.requirements.disabled=!!state.busy;
      const r=state.record,key=JSON.stringify([r,state.history,state.busy]);if(key===this.key)return;this.key=key;
      const empty=el('option','选择修复历史');empty.value='';this.history.replaceChildren(empty);
      for(const h of state.history || []) {const o=el('option',`${h.state} · ${h.title}`);o.value=h.repair_id;o.selected=h.repair_id===r?.repair_id;this.history.append(o);}
      this.plan.replaceChildren();if(!r)return;
      this.plan.append(el('strong',`${r.state} · ${r.plan?.title || '方案生成失败'}`),el('small',`review ${r.review_id} / repair ${r.repair_id}`));
      if(r.error)this.plan.append(el('p',r.error,'review-warning'));
      const p=r.plan;
      if(p){const preview=el('details');preview.open=['DRAFT','DEFERRED'].includes(r.state);preview.append(el('summary','修复方案预览'));
        for(const [label,value] of [['对应问题',p.issue_ids],['严重程度',r.original_issues.map(i=>`${i.issue_id}: ${i.severity}`)],['修改目标',p.problem_description],['根因',p.root_cause],['允许文件',p.allowed_files],['禁止文件（其他路径亦禁止）',p.forbidden_files],['修复方向',p.repair_strategy],['验收标准',p.acceptance_criteria],['测试要求',p.test_requirements],['风险',p.risk_level],['删除文件',p.delete_files],['公共接口变更',p.public_interface_changes],['缺少上下文',p.missing_context]])preview.append(el('strong',label),el('p',Array.isArray(value)?value.join('\n') || '无':value));this.plan.append(preview);}
      const actions=el('div',undefined,'review-actions');
      for(const [label,type,enabled] of [['批准并发送','repairApprove',p && !r.sent_once && ['DRAFT','DEFERRED'].includes(r.state) && !p.missing_context.length],['修改修复要求','repairEdit',p && !r.sent_once && ['DRAFT','DEFERRED'].includes(r.state)],['暂不处理','repairDefer',!r.sent_once && r.state==='DRAFT'],['拒绝修复','repairReject',!r.sent_once && ['DRAFT','DEFERRED'].includes(r.state)],['编译 / 测试','repairCheck',['EXECUTED','FAILED'].includes(r.state)&&r.sent_once],['GPT复审','repairVerify',['EXECUTED','FAILED'].includes(r.state)&&r.sent_once],['停止','repairStop',state.busy],['查看完整报告','repairReport',true]]) {
        const b=button(label,label,()=>this.post({type,repairId:r.repair_id,planHash:r.plan_hash}),'secondary-button');b.disabled=type==='repairStop'?!state.busy:!!state.busy || !enabled;actions.append(b);
      }this.plan.append(actions);
      this.plan.append(el('p',`GPT Token：输入 ${r.gpt_usage?.input ?? '未知'} / 输出 ${r.gpt_usage?.output ?? '未知'} / 总计 ${r.gpt_usage?.total ?? '未知'}`));
      for(const a of r.approvals || [])this.plan.append(el('small',`${a.time} · ${a.actor} · ${a.decision} · ${a.risks.join(', ')}`));
      for(const c of r.checks || [])this.plan.append(el('p',`${c.command}: ${c.status} (exit ${c.exit_code}, tests ${c.test_count ?? '—'})`));
      for(const c of r.verification || [])this.plan.append(el('strong',`${c.issue_id}: ${c.status}`),el('p',c.explanation),el('pre',c.evidence));
      if(r.evidence){const diff=el('details');diff.append(el('summary','实际代码 Diff'));for(const c of r.evidence.changes || [])diff.append(el('strong',`${c.kind} ${c.path}`),el('pre',c.diff));this.plan.append(diff);}
    }
  }
  window.ATMRepairPanel=RepairPanel;
})();
