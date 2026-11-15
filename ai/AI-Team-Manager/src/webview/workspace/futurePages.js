(() => {
  const {el,button}=window.ATMElements;
  const fmt=()=>window.ATMOutputFormatter;
  const pathKey=p=>typeof p==='string'?p.replace(/\\/g,'/').toLowerCase():'';
  function current(snapshot){const w=snapshot?.dispatch?.team?.workflow,chat=snapshot?.chat;return w&&w.plan.owner_id===chat?.conversation?.id&&pathKey(w.plan.project_path)===pathKey(chat?.projectPath)?snapshot.dispatch.team:undefined;}
  function fold(title,cls){const d=el('details',undefined,cls);d.append(el('summary',title));return d;}
  function selector(label,values){const s=el('select');s.setAttribute('aria-label',label);for(const [value,text] of values){const o=el('option',text);o.value=value;s.append(o);}return s;}
  class FuturePage {
    constructor(id,title,description,back,post){
      this.post=post;this.node=el('section',undefined,'future-page');this.node.dataset.viewId='aiTeamManager.'+id;
      const head=el('header',undefined,'page-heading');head.append(el('h1',title),button('返回 AI 协同开发','返回 AI 协同开发',back,'secondary-button'));
      this.node.append(head,el('p',description,'muted-text'));this.content=el('div',undefined,'page-content');this.node.append(this.content);
    }
  }
  class TaskCenter extends FuturePage {
    constructor(back,post,selectWorker){
      super('task-center','任务调度中心','查看真实任务进度；选择任务了解详情。',back,post);this.selectWorker=selectWorker;
      const filters=el('div',undefined,'page-filters');this.project=button('选择项目','任务中心选择项目',()=>post({type:'project',panelId:'gpt'}),'secondary-button');
      this.filter=selector('筛选任务状态',[['','全部状态'],...['RUNNING','RETRYING','WAITING_DEPENDENCY','WAITING_REVIEW','WAITING_APPROVAL','WAITING_INTEGRATION','COMPLETED','FAILED','PAUSED'].map(s=>[s,fmt().stateLabel(s)])]);
      this.search=el('input');this.search.type='search';this.search.placeholder='搜索任务或 Worker';this.search.setAttribute('aria-label','搜索任务');filters.append(this.project,this.filter,this.search);
      this.stats=el('p',undefined,'compact-stats');this.stats.setAttribute('aria-label','后台重试统计');this.rows=el('div',undefined,'task-table-wrap');
      this.progressView=fmt().progressView('任务中心整体任务');this.overview=el('div',undefined,'progress-overview');this.overview.setAttribute('aria-label','任务进度总览');
      this.dependencies=fold('依赖关系','task-dependencies');this.graph=el('div',undefined,'dependency-list');this.dependencies.append(this.graph);
      this.logs=fold('执行事件','task-event-log');const tools=el('div',undefined,'page-filters');
      this.workerFilter=selector('筛选事件 Worker',[['','全部 Worker']]);this.eventFilter=selector('筛选事件类型',[['','全部事件'],['RETRY','自动重试'],['GPT_REVIEW','独立审查'],['FAILED','失败'],['WAITING','等待'],['FILE_WRITE','文件写入']]);
      this.eventSearch=el('input');this.eventSearch.type='search';this.eventSearch.placeholder='搜索事件';this.eventSearch.setAttribute('aria-label','搜索执行事件');this.eventRows=el('div');
      tools.append(this.workerFilter,this.eventFilter,this.eventSearch,button('复制筛选结果','复制筛选后的执行事件',()=>post({type:'copy',panelId:'gpt',text:JSON.stringify(this.filteredEvents(),null,2)}),'secondary-button'));
      this.logs.append(tools,this.eventRows);this.content.append(filters,this.progressView.node,this.overview,this.stats,this.rows,this.dependencies,this.logs);
      this.drawer=el('aside',undefined,'task-drawer');this.drawer.hidden=true;this.drawer.tabIndex=-1;this.drawer.setAttribute('role','dialog');this.drawer.setAttribute('aria-modal','true');this.drawer.setAttribute('aria-label','任务详情');this.node.append(this.drawer);
      this.drawer.addEventListener('keydown',e=>{if(e.key==='Escape')this.closeDrawer();if(e.key==='Tab'){const controls=[...this.drawer.querySelectorAll('button:not(:disabled),summary,input,select')];const first=controls[0],last=controls.at(-1);if(e.shiftKey&&(document.activeElement===first||document.activeElement===this.drawer)){e.preventDefault();last?.focus();}else if(!e.shiftKey&&document.activeElement===last){e.preventDefault();first?.focus();}}});
      for(const input of [this.filter,this.search])input.addEventListener('input',()=>this.renderRows());
      for(const input of [this.workerFilter,this.eventFilter,this.eventSearch])input.addEventListener('input',()=>this.renderEvents());
    }
    update(snapshot){
      this.snapshot=snapshot;this.team=current(snapshot);const w=this.team?.workflow,identity=w?.plan.plan_id;
      if(identity!==this.identity){this.identity=identity;this.selected=undefined;this.drawer.hidden=true;this.content.inert=false;this.filter.value='';this.search.value='';this.dependencies.open=this.logs.open=false;}
      this.project.textContent=snapshot.chat?.projectName||'选择项目';
      const old=this.workerFilter.value;this.workerFilter.replaceChildren(el('option','全部 Worker'));this.workerFilter.firstChild.value='';
      for(const worker of w?.workers||[]){const o=el('option','Claude '+worker.worker_id.slice(0,8));o.value=worker.worker_id;this.workerFilter.append(o);}this.workerFilter.value=old;
      const s=w?.retry_stats;this.stats.textContent=s?`执行 ${s.running} · 重试中 ${s.retrying} · 已恢复 ${s.recovered} · 重试耗尽 ${s.exhausted} · 待处理 ${s.needs_attention}`:'重试统计：后端尚未提供';
      this.progressView.update(this.team?.progress);this.overview.replaceChildren();
      for(const [key,name] of [['waiting','等待执行'],['running','正在执行'],['testing','测试中'],['waiting_review','等待审查'],['review_passed','审查通过'],['waiting_integration','等待集成'],['waiting_delivery','等待用户交付'],['completed','已完成'],['blocked','失败或阻塞']])this.overview.append(el('span',name+' '+(this.team?.progress?.overview[key]??'—')));
      this.renderRows();this.renderDependencies();this.renderEvents();if(this.selected)this.openTask(this.selected,false);
    }
    status(row){return this.team.workflow.workers.find(w=>w.task_id===row.task.task_id)?.status==='RETRYING'?'RETRYING':row.status;}
    renderRows(){
      this.rows.replaceChildren();const w=this.team?.workflow;if(!w){this.rows.append(el('p','当前会话暂无实际任务队列','empty-state'));return;}
      const query=this.search.value.trim().toLowerCase(),rows=w.tasks.filter(r=>(!this.filter.value||this.status(r)===this.filter.value)&&JSON.stringify([r.task.task_name,r.task.task_description,r.task.task_id,r.task.assigned_worker,w.workers.find(a=>a.task_id===r.task.task_id)?.worker_id]).toLowerCase().includes(query));
      const table=el('table',undefined,'task-table'),head=el('thead'),tr=el('tr');for(const name of ['任务','Worker','状态','优先级','更新时间'])tr.append(el('th',name));head.append(tr);table.append(head);const body=el('tbody');
      for(const row of [...rows].sort((a,b)=>(a.task.priority??3)-(b.task.priority??3))){const tr=el('tr'),task=el('td');const b=button(row.task.task_name||row.task.task_id,'查看任务 '+row.task.task_id,()=>{this.opener=b;this.openTask(row.task.task_id);},'task-name');task.append(b,el('small',row.task.task_description));tr.append(task);
        const worker=w.workers.find(a=>a.task_id===row.task.task_id);tr.append(el('td',worker?'Claude '+worker.worker_id.slice(0,8):row.task.assigned_worker));const status=el('td');status.append(fmt().badge(this.status(row)));tr.append(status,el('td',String(row.task.priority??'未知')));
        const time=row.events?.at(-1)?.time||worker?.updated_at;tr.append(el('td',time?new Date(time).toLocaleString('zh-CN',{hour12:false}):'未提供'));body.append(tr);
      }table.append(body);this.rows.append(table);if(!rows.length)this.rows.append(el('p','没有符合筛选条件的任务','empty-state'));
    }
    openTask(id,focus=true){
      const w=this.team?.workflow,row=w?.tasks.find(r=>r.task.task_id===id);if(!row){this.closeDrawer();return;}
      const progress=this.team.progress?.tasks.find(t=>t.task_id===id),drawerKey=JSON.stringify([id,row,w.workers.find(a=>a.task_id===id),w.record.reviews[id],w.candidates.find(c=>c.task_id===id),progress]);
      if(!focus&&!this.drawer.hidden&&this.drawerKey===drawerKey)return;this.drawerKey=drawerKey;this.selected=id;this.drawer.hidden=false;this.content.inert=true;
      const opened=[...this.drawer.querySelectorAll('details')].map((d,i)=>d.open?i:-1);this.drawer.replaceChildren(button('关闭','关闭任务详情',()=>this.closeDrawer(),'secondary-button'),el('h2',row.task.task_name||id),fmt().badge(this.status(row)),el('p',row.task.task_description));
      if(row.reason)this.drawer.append(el('p',row.reason,'commander-risk'));
      const view=fmt().progressView('任务 '+id);view.update(progress);this.drawer.append(view.node,fmt().progressTimeline(progress));
      const worker=w.workers.find(a=>a.task_id===id);if(worker)this.drawer.append(button('打开 Worker','打开任务 Worker',()=>this.selectWorker(worker.worker_id),'primary-button'));
      this.drawer.append(el('p','任务编号：'+id),el('p','上游依赖：'+(row.task.dependencies.join('、')||'无')));
      const scope=fold('文件范围与验收','task-scope');scope.append(el('p','允许修改：'+row.task.allowed_files.join('、')),el('p','禁止修改：'+row.task.forbidden_files.join('、')));for(const c of row.task.acceptance_criteria)scope.append(el('p','○ '+c+' · 逐项结果未提供'));this.drawer.append(scope);
      const reports=w.record.reviews[id]||[];for(const review of reports){const d=fold('审查 · '+fmt().stateLabel(review.verdict));d.append(el('p',review.summary),el('small','审查编号：'+review.review_id));for(const reason of review.block?.reasons||review.missing_evidence||[])d.append(el('p',reason));if(review.block)d.append(el('p',review.block.next_action));this.drawer.append(d);}
      const candidate=w.candidates.find(c=>c.task_id===id);for(const t of candidate?.tests||[])this.drawer.append(el('p',`${fmt().stateLabel(t.status)} · ${t.test_count??'未知'} 项 · ${t.command}`));
      for(const [i,d] of [...this.drawer.querySelectorAll('details')].entries())d.open=opened.includes(i);
      if(focus)this.drawer.focus();
    }
    closeDrawer(){this.drawer.hidden=true;this.content.inert=false;this.selected=undefined;if(this.opener?.isConnected)this.opener.focus();else this.search.focus();}
    renderDependencies(){
      this.graph.replaceChildren();const rows=this.team?.workflow.tasks||[];
      for(const row of rows){const edge=el('div',undefined,'dependency-row');for(const id of row.task.dependencies){edge.append(el('span',id,'dependency-node'),el('span','→','dependency-arrow'));}if(!row.task.dependencies.length)edge.append(el('small','独立任务'));edge.append(el('span',row.task.task_id,'dependency-node'),fmt().badge(row.status));this.graph.append(edge);}
      if(!rows.length)this.graph.append(el('p','尚无任务依赖'));
    }
    filteredEvents(){return (this.team?.workflow.record.supervision||[]).filter(e=>(!this.workerFilter.value||e.worker_id===this.workerFilter.value)&&(!this.eventFilter.value||e.kind.includes(this.eventFilter.value))&&JSON.stringify([e.kind,e.reason,e.task_id]).toLowerCase().includes(this.eventSearch.value.trim().toLowerCase()));}
    renderEvents(){
      this.eventRows.replaceChildren();const groups=new Map();for(const e of this.filteredEvents()){const key=e.id||JSON.stringify([e.task_id,e.worker_id,e.kind,e.time,e.reason]);const g=groups.get(key);if(g)g.count++;else groups.set(key,{event:e,count:1});}
      for(const {event:e,count} of groups.values()){const d=fold(`${e.task_id} · ${e.reason}${count>1?'（重复记录 '+count+' 条）':''}`,'task-event');d.append(el('small',e.time||'时间未提供'),el('p','Worker：'+(e.worker_id||'未提供')),button('复制事件','复制执行事件 '+e.id,()=>this.post({type:'copy',panelId:'gpt',text:JSON.stringify(e,null,2)}),'secondary-button'));this.eventRows.append(d);}
      if(!groups.size)this.eventRows.append(el('p','当前筛选下没有执行事件','empty-state'));
    }
  }
  class ReviewIntegration extends FuturePage {
    constructor(back,post,selectWorker){super('review-integration','代码审查与集成','审查证据与安全交付；通过候选不代表主项目已交付。',back,post);this.selectWorker=selectWorker;}
    update(snapshot){this.content.replaceChildren();const team=current(snapshot),w=team?.workflow;if(!w){this.content.append(el('p','尚无当前团队的审查记录'));return;}
      for(const row of w.tasks){const report=w.record.reviews[row.task.task_id]?.at(-1),d=fold((row.task.task_name||row.task.task_id)+' · '+(report?fmt().stateLabel(report.verdict):'尚未审查'),'review-task');if(report){d.append(el('p',report.summary),el('small','审查编号：'+report.review_id));for(const issue of report.issues||[])d.append(el('p',issue.problem_description));for(const gap of report.missing_evidence||[])d.append(el('p',gap));}
        const worker=w.workers.find(a=>a.task_id===row.task.task_id);if(worker)d.append(button('查看 Worker 文件与记录','查看任务文件 '+row.task.task_id,()=>this.selectWorker(worker.worker_id),'secondary-button'));
        for(const r of w.record.repairs?.filter(r=>r.task_id===row.task.task_id)||[])d.append(el('p','第 '+r.round+' 轮修复 · '+fmt().stateLabel(r.status)+' · '+r.description));this.content.append(d);}
      const i=team.integration;this.content.append(el('h2','集成与交付'),el('p',i?fmt().stateLabel(i.status):'等待候选审查通过'));
      if(i?.evidence)this.content.append(button('查看集成补丁','查看实际集成补丁',()=>this.post({type:'teamPreview',projectPath:w.plan.project_path,planId:w.plan.plan_id}),'secondary-button'));
      if(i?.status==='READY')this.content.append(button('授权安全交付','授权安全交付',()=>this.post({type:'teamDeliver',projectPath:w.plan.project_path,planId:w.plan.plan_id}),'primary-button'),el('small','必须先查看当前补丁，再经原生授权；不会自动交付。'));
    }
  }
  class Projects extends FuturePage {
    constructor(back,post){super('projects','项目管理','使用现有工作区和计划历史，不复制业务会话。',back,post);}
    update(snapshot){this.content.replaceChildren();for(const p of snapshot.projects||[])this.content.append(el('h2',p.name),el('p',p.path||'路径未提供','muted-text'));this.content.append(button('选择工作区项目','项目管理选择项目',()=>this.post({type:'project',panelId:'gpt'}),'secondary-button'));const team=current(snapshot);if(team)this.content.append(el('p','当前计划：'+team.workflow.plan.goal),el('p','项目状态：'+(team.busy?'执行中':'等待处理')));const plans=(snapshot.dispatch?.planning?.plans||[]).filter(p=>pathKey(p.project_path)===pathKey(snapshot.chat?.projectPath)&&p.owner_id===snapshot.chat?.conversation?.id);const history=fold('计划历史');for(const p of plans)history.append(el('p',p.goal+' · '+p.created_at));if(!plans.length)history.append(el('p','当前会话没有已提供的计划历史'));this.content.append(history);}
  }
  class Settings extends FuturePage {
    constructor(back,post){super('settings','系统设置','显示当前配置；修改仍通过现有 VS Code 设置和授权入口。',back,post);}
    update(snapshot){this.content.replaceChildren();const s=snapshot.dispatch?.settings||{},dl=el('dl',undefined,'settings-list');for(const [key,label] of [['teamMaxConcurrent','Worker 最大并发'],['teamMaxRepairRounds','最大修复轮数'],['claudeRetryMaxRetries','自动重试次数'],['claudeRetryBaseDelayMs','重试基础等待（毫秒）'],['claudeExecutable','Claude CLI'],['codexExecutable','Codex CLI']])dl.append(el('dt',label),el('dd',s[key]===undefined?'后端未提供':String(s[key])));dl.append(el('dt','GPT Token 预算'),el('dd',String(current(snapshot)?.usage?.budget??'后端未提供')));this.content.append(dl,button('打开扩展设置','打开现有扩展设置',()=>this.post({type:'settings'}),'primary-button'));const b=button('用量与预算','配置 GPT 用量与预算',()=>this.post({type:'teamUsageConfigure',projectPath:snapshot.chat?.projectPath}),'secondary-button');b.disabled=!snapshot.chat?.projectPath;this.content.append(b);}
  }
  window.ATMFuturePages={create(back,post=()=>{},selectWorker=()=>{}){return new Map([['task-center',new TaskCenter(back,post,selectWorker)],['review-integration',new ReviewIntegration(back,post,selectWorker)],['projects',new Projects(back,post)],['settings',new Settings(back,post)]]);}};
})();
