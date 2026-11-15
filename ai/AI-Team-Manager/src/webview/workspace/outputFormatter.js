// One local, text-node-only formatter for GPT, Claude, and captured tool events.
(() => {
  const { el, button } = window.ATMElements;
  const labels = { goal:'开发目标', approach:'技术方案', technical_approach:'技术方案', files:'文件权限', allowed_files:'允许文件', forbidden_files:'禁止文件', steps:'任务步骤（仅规划）', commands:'测试命令', risks:'风险提示', tasks:'Worker 任务规划', acceptance_criteria:'验收标准（未验证）' };
  function parse(text) {
    const value = String(text).trim(), fenced = /^```(?:json)?\s*\n([\s\S]*?)\n```$/i.exec(value);
    const raw = fenced ? fenced[1].trim() : value;
    if (!/^[{[]/.test(raw)) return;
    try { const json = JSON.parse(raw); if (json && typeof json==='object') return json; } catch {}
  }
  function tree(value, depth=0) {
    if (!value || typeof value!=='object') return el('span', JSON.stringify(value));
    // Large arrays are complete in raw mode; expand chunks on demand in formatted mode.
    const node = el('div', undefined, 'json-tree'), entries = Object.entries(value);
    let index=0;
    const more = button('加载更多字段', '加载更多 JSON 字段', append, 'text-button');
    function append() {
      more.remove();
      for (const [key, child] of entries.slice(index,index+40)) {
        const row = el('details', undefined, 'json-field'); row.open = depth < 1;
        row.append(el('summary', Array.isArray(value) ? '['+key+']' : key));
        if(depth>16)row.append(el('pre',JSON.stringify(child,null,2))); else row.append(tree(child,depth+1));
        node.append(row);
      }
      index+=40; if(index<entries.length)node.append(more);
    }
    append(); return node;
  }
  function render(text, copy, mode='formatted', context={}) {
    const node=el('div',undefined,'formatted-output');
    if(mode==='debug')node.append(el('pre',JSON.stringify(context,null,2),'output-debug'));
    if(mode!=='formatted'){node.append(el('pre',String(text),'raw-output'));return node;}
    const json=parse(text);
    if(json){
      node.classList.add('json-output');
      node.append(button('复制完整 JSON','复制完整 JSON',()=>copy(String(text)),'secondary-button'));
      for(const [key,value] of Object.entries(json)){
        const card=el('section',undefined,'output-card');card.dataset.field=key;
        const header=el('header');header.append(el('strong',labels[key]||key),button('复制字段','复制字段 '+key,()=>copy(JSON.stringify(value,null,2))));
        card.append(header,tree(value));node.append(card);
      }
      const raw=el('details');raw.append(el('summary','查看原始 JSON'),el('pre',String(text),'raw-output'));node.append(raw);
    }else node.append(window.renderGPTMarkdown(text,copy));
    return node;
  }
  const retryKinds={RETRY_SCHEDULED:'等待自动重试',RETRY_STARTED:'正在重新连接',RETRY_SUCCEEDED:'已恢复连接',RETRY_EXHAUSTED:'自动重试次数已用尽',RETRY_CANCELLED:'自动重试已取消',RETRY_BLOCKED_SIDE_EFFECT:'安全检查阻止重试',RETRY_DEFERRED:'等待手动恢复'};
  const retryReasons={HTTP_502:'HTTP 502：网关错误',HTTP_503:'HTTP 503：服务暂不可用',HTTP_504:'HTTP 504：网关超时',HTTP_429:'HTTP 429：请求限流',NO_AVAILABLE_CHANNEL:'模型通道暂不可用',ECONNRESET:'连接被重置',ETIMEDOUT:'连接超时',GATEWAY_UNAVAILABLE:'临时网关连接失败'};
  function retrySummary(r){return (retryKinds[r.kind]||'重试状态')+' · '+(retryReasons[r.reason]||'上游连接异常')+' · 第 '+r.retry+' / '+r.maxRetries+' 次'+(r.delayMs?' · 计划等待 '+Math.ceil(r.delayMs/1000)+' 秒':'');}
  const phases={started:'执行中',running:'执行中',completed:'执行完成',failed:'执行失败',pending:'等待执行',cancelled:'已取消',denied:'权限阻止'};
  function tool(entry,copy,mode='formatted'){
    const event=entry.event||entry, card=el('details',undefined,'tool-event');
    // Permission is a decision, never evidence that the operation ran successfully.
    const phase=event.type==='retry'?(retryKinds[event.kind]||'重试状态'):event.denied||event.decision==='DENY'?'权限阻止':event.decision==='ALLOW'?'权限已批准 · 执行结果待确认':event.type==='result'?(event.ok?'请求完成':'请求失败'):event.type==='text'?'输出已接收':event.type==='status'?'状态通知':phases[event.phase]||event.status||'状态未提供';
    card.dataset.phase=event.phase||event.decision||event.status||'unknown';
    card.append(el('summary',`${entry.time||event.time||'时间未提供'} · ${event.name||event.tool||event.type} · ${phase}`));
    if(mode==='debug')card.append(el('pre',JSON.stringify({project_id:entry.project_id,task_id:entry.task_id,worker_id:entry.worker_id,session_id:entry.session_id,time:entry.time},null,2)));
    if(mode==='formatted')card.append(el('p',event.type==='retry'?retrySummary(event):event.detail||event.reason||event.error||event.message||'无结果摘要'));
    if(mode==='formatted'){
      const raw=el('details',undefined,'event-raw');raw.append(el('summary','原始诊断'),el('pre',JSON.stringify(event,null,2),'raw-output'));card.append(raw);
    }else card.append(el('pre',JSON.stringify(event,null,2),'raw-output'));
    card.append(button('复制事件','复制原始事件',()=>copy(JSON.stringify(entry,null,2)),'secondary-button'));return card;
  }
  const stateLabels={IDLE:'空闲',QUEUED:'排队中',PENDING:'待执行',READY:'就绪',RUNNING:'执行中',RETRYING:'自动重试中',REVIEWING:'独立审查中',REPAIRING:'定向修复中',WAITING_DEPENDENCY:'等待上游审查',WAITING_FILE_LOCK:'等待文件锁',WAITING_APPROVAL:'等待人工确认',WAITING_REVIEW:'等待补证复审',WAITING_INTEGRATION:'等待安全集成',COMPLETED:'已完成',FAILED:'失败',PAUSED:'已暂停',PAUSING:'正在停止',CANCELLED:'已取消',INTERRUPTED:'已中断',PASS:'通过',PASSED:'通过',BLOCKED:'受阻',REQUEST_CHANGES:'需要修复',NEEDS_HUMAN_REVIEW:'需要人工确认',UNVERIFIED:'未验证',DELIVERED:'已交付',PREPARING:'准备中',CONFLICT:'存在冲突'};
  const stateLabel=s=>stateLabels[s]||s||'未知';
  const stateTone=s=>/CANCELLED/.test(s||'')?'neutral':/FAILED|BLOCKED|EXHAUSTED|CONFLICT|REQUEST_CHANGES/.test(s||'')?'danger':/PASS|SUCCEEDED|COMPLETED|DELIVERED/.test(s||'')?'success':/WAIT|RETRY|PAUSED|PENDING|QUEUED/.test(s||'')?'warning':'neutral';
  function badge(s){const n=el('span',stateLabel(s),'state-badge');n.dataset.tone=stateTone(s);return n;}
  function retryBar(r,worker,copy){
    const node=el('details',undefined,'retry-status');node.dataset.tone=stateTone(r.kind);node.dataset.retryKind=r.kind;
    const header=el('summary'),title=el('strong',retryKinds[r.kind]||'重试状态'),info=el('span',retryReasons[r.reason]||'上游连接异常'),count=el('span',`第 ${r.retry} / ${r.maxRetries} 次`),timer=el('span',undefined,'retry-countdown');
    header.append(el('span',worker),title,info,count,timer,el('span',r.sessionId?'原会话已保留':'原会话尚未建立'));node.append(header);
    node.append(el('p',`${worker} · ${r.sessionId?'原会话已保留':'尚未建立会话，仍使用同一请求记录'}`));
    if(r.sessionId)node.append(el('p','会话编号：'+r.sessionId,'muted-text'));
    if(r.kind==='RETRY_BLOCKED_SIDE_EFFECT')node.append(el('p','发现执行活动、文件变化或无法排除副作用。已停止自动重试，候选保留。'));
    if(r.kind==='RETRY_EXHAUSTED')node.append(el('p','达到自动重试上限。原会话、候选文件和任务记录保留，需人工处理。'));
    if(r.kind==='RETRY_SUCCEEDED')node.append(el('p','连接已恢复；任务继续执行，仍须经过测试和独立审查。'));
    if(r.kind==='RETRY_CANCELLED')node.append(el('p','等待已取消，不会自动发起下一次请求。'));
    node.append(button('复制诊断','复制重试诊断信息',()=>copy(JSON.stringify({worker,...r},null,2)),'secondary-button'));
    const update=()=>{const seconds=Math.max(0,Math.ceil((Date.parse(r.nextRetryAt)-Date.now())/1000));timer.textContent=r.kind==='RETRY_SCHEDULED'&&Number.isFinite(seconds)?(seconds?`${seconds} 秒后重试`:'正在校验重试条件'):'';};update();
    return {node,tick:update};
  }
  function progressView(label){
    const node=el('section',undefined,'verification-progress'),heading=el('div',undefined,'progress-heading'),title=el('strong'),phase=el('span'),bar=el('div',undefined,'verification-track'),fill=el('span'),counts=el('small');
    node.setAttribute('aria-label',label);bar.setAttribute('role','progressbar');bar.setAttribute('aria-label',label+'验证进度');bar.append(fill);heading.append(title,phase);node.append(heading,bar,counts);
    let frame;
    return {node,update(p){
      title.textContent=p?.name||'当前任务';phase.textContent=p?.phase||'阶段未知';node.dataset.state=p?.state||'UNKNOWN';node.dataset.tone=['BLOCKED','FAILED','WAITING_REVIEW'].includes(p?.state)?'danger':stateTone(p?.state);
      const known=Number.isFinite(p?.percent)&&Number.isInteger(p?.total)&&p.total>0;
      if(known){bar.setAttribute('aria-valuemin','0');bar.setAttribute('aria-valuemax','100');bar.setAttribute('aria-valuenow',String(p.percent));bar.setAttribute('aria-valuetext',`已验证 ${p.completed} / ${p.total} 个阶段，${p.percent}%`);counts.textContent=`已验证 ${p.completed} / ${p.total} 个阶段 · 剩余 ${p.total-p.completed}`;}
      else {bar.removeAttribute('aria-valuenow');bar.setAttribute('aria-valuetext','进度未知；'+phase.textContent);counts.textContent='步骤总量未知 · 等待可靠验证记录';}
      if(p?.updated_at)counts.textContent+=' · 更新 '+new Date(p.updated_at).toLocaleTimeString('zh-CN',{hour12:false});
      node.dataset.indeterminate=String(!known);if(frame)cancelAnimationFrame(frame);
      const apply=()=>{fill.style.width=known?p.percent+'%':'';};
      if(typeof requestAnimationFrame==='function')frame=requestAnimationFrame(apply);else apply();
    }};
  }
  function progressTimeline(p){
    const details=el('details',undefined,'progress-timeline');details.append(el('summary','步骤时间线'));
    const list=el('ol'),date=t=>t?new Date(t).toLocaleString('zh-CN',{hour12:false}):'未记录';
    for(const s of p?.steps||[]){const li=el('li');li.dataset.tone=stateTone(s.state);li.append(el('strong',s.name),badge(s.state),el('small','开始：'+date(s.started_at)+' · 完成：'+date(s.finished_at)));if(s.reason)li.append(el('p',s.reason,'commander-risk'));list.append(li);}details.append(list);
    if(p?.timeline?.length){const history=el('details');history.append(el('summary','执行状态记录'));for(const s of p.timeline){history.append(el('p',s.name),el('small','开始：'+date(s.started_at)+' · 结束：'+date(s.finished_at)));if(s.reason)history.append(el('p',s.reason));}details.append(history);}
    return details;
  }
  window.ATMOutputFormatter={parse,render,tool,retrySummary,retryBar,stateLabel,stateTone,badge,progressView,progressTimeline};
})();
