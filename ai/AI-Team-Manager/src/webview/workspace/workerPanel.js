(() => {
  const {el,button}=window.ATMElements;
  const status={IDLE:'空闲',QUEUED:'等待执行',RUNNING:'执行中',RETRYING:'自动重试中',REVIEWING:'等待审查',REPAIRING:'修复中',WAITING_DEPENDENCY:'等待上游审查',WAITING_FILE_LOCK:'等待文件锁',WAITING_APPROVAL:'等待审批',WAITING_REVIEW:'审查受阻 · 等待复审',WAITING_INTEGRATION:'待安全集成',COMPLETED:'已完成',FAILED:'执行失败',PAUSED:'已暂停',PAUSING:'正在安全停止',CANCELLED:'已取消',INTERRUPTED:'已中断'};
  class WorkerPanel {
    constructor(post,select){
      this.post=post;this.select=select;this.states=new Map();this.rendered=new Map();
      this.node=el('section',undefined,'worker-workbench');this.node.id='team-worker-session';
      this.list=el('aside',undefined,'worker-list');this.list.setAttribute('aria-label','项目 Worker 列表');
      this.main=el('section',undefined,'worker-main');this.heading=el('h2');
      this.tabs=el('div',undefined,'worker-subtabs');this.tabs.setAttribute('role','tablist');
      for(const [id,label] of [['chat','对话'],['files','文件变更'],['logs','执行日志'],['tests','测试结果'],['task','任务信息']]){
        const b=button(label,label,()=>{this.save();this.state.tab=id;this.key=undefined;this.update(this.team,this.id);},'secondary-button');b.dataset.tab=id;b.setAttribute('role','tab');this.tabs.append(b);
      }
      this.tools=el('div',undefined,'worker-tools');this.mode=el('select');this.mode.setAttribute('aria-label','Worker 输出显示模式');
      for(const [value,label] of [['formatted','格式化'],['raw','原始'],['debug','调试']]){const o=el('option',label);o.value=value;this.mode.append(o);}
      this.search=el('input');this.search.type='search';this.search.placeholder='搜索当前页日志';this.search.setAttribute('aria-label','搜索 Worker 日志');
      this.auto=button('暂停滚动','暂停或恢复 Worker 自动滚动',()=>{this.state.paused=!this.state.paused;this.auto.setAttribute('aria-pressed',String(this.state.paused));});
      this.tools.append(this.mode,this.search,this.auto,button('复制日志','复制当前页 Worker 日志',()=>this.copy(JSON.stringify(this.detail?.events.events||[],null,2))),button('清空显示','清空 Worker 日志显示，保留原始记录',()=>{this.state.cleared=this.detail?.event_total||0;this.key=undefined;this.update(this.team,this.id);}));
      for(const control of [this.mode,this.search])control.addEventListener('input',()=>{this.save();this.state.mode=this.mode.value;this.state.query=this.search.value;this.key=undefined;this.update(this.team,this.id);});
      this.body=el('div',undefined,'worker-body');this.body.setAttribute('aria-label','当前 Worker 独立内容');
      this.pager=el('div',undefined,'worker-pagination');
      this.input=el('textarea');this.input.rows=2;this.input.maxLength=12000;this.input.setAttribute('aria-label','当前 Worker 补充指令');
      this.input.addEventListener('input',()=>{if(this.state)this.state.draft=this.input.value;});
      this.send=button('保存补充指令','保存补充指令，恢复时经过原权限网关',()=>{if(this.send.disabled||!this.input.value.trim())return;this.action('teamInstruct',{text:this.input.value});},'primary-button');
      this.controls=el('div',undefined,'worker-controls');
      this.detailToggle=button('详情','展开或收起 Worker 详情',()=>{this.showDetails(!this.state.details);if(this.state.details)this.side.focus();},'secondary-button');
      this.controls.append(this.detailToggle);
      this.controlButtons=new Map();
      for(const [action,label] of [['pause','暂停'],['resume','恢复'],['cancel','停止']]){const b=button(label,label+'当前 Worker',()=>this.action('teamControl',{action}),'secondary-button');this.controlButtons.set(action,b);this.controls.append(b);}
      const header=el('header',undefined,'worker-heading');header.append(this.heading,this.controls);
      this.retrySlot=el('div',undefined,'worker-retry');
      this.progressView=window.ATMOutputFormatter.progressView('当前 Worker');
      this.supplement=el('details',undefined,'worker-supplement');this.supplement.append(el('summary','补充指令'),this.input,this.send,el('small','仅暂停或等待审批时可保存；保持原任务权限。'));
      this.main.append(header,this.progressView.node,this.retrySlot,this.tabs,this.tools,this.body,this.pager,this.supplement);
      this.side=el('aside',undefined,'worker-details');this.side.setAttribute('aria-label','Worker 执行详情');this.side.hidden=true;this.side.tabIndex=-1;
      this.side.addEventListener('keydown',e=>{if(e.key==='Escape'){this.showDetails(false);this.detailToggle.focus();}});
      const splitter=(name,target,side)=>{const handle=el('div',undefined,'worker-splitter');handle.tabIndex=0;handle.setAttribute('role','separator');handle.setAttribute('aria-label',name);handle.setAttribute('aria-orientation','vertical');
        const resize=(value)=>{const width=Math.max(side==='left'?130:200,Math.min(side==='left'?300:520,value));this.node.style.setProperty(target,width+'px');handle.setAttribute('aria-valuenow',String(width));};
        handle.addEventListener('pointerdown',event=>{event.preventDefault();const initial=side==='left'?this.list.getBoundingClientRect().width:this.side.getBoundingClientRect().width,start=event.clientX;handle.setPointerCapture(event.pointerId);const move=e=>resize(initial+(e.clientX-start)*(side==='left'?1:-1)),up=()=>{handle.removeEventListener('pointermove',move);handle.removeEventListener('pointerup',up);handle.removeEventListener('pointercancel',up);};handle.addEventListener('pointermove',move);handle.addEventListener('pointerup',up);handle.addEventListener('pointercancel',up);});
        handle.addEventListener('keydown',event=>{if(['ArrowLeft','ArrowRight'].includes(event.key)){event.preventDefault();const current=side==='left'?this.list.getBoundingClientRect().width:this.side.getBoundingClientRect().width;resize(current+(event.key==='ArrowRight'?10:-10)*(side==='left'?1:-1));}});return handle;};
      this.node.append(this.list,splitter('调整 Worker 列表宽度','--worker-list-width','left'),this.main,this.side);
    }
    showDetails(open){if(this.state)this.state.details=open;this.side.hidden=!open;this.detailToggle.setAttribute('aria-expanded',String(open));}
    tick(){if(!this.node.hidden)this.retryView?.tick();}
    copy(text){this.post({type:'copy',panelId:'gpt',text});}
    action(type,extra={}){this.post({type,projectPath:this.team.workflow.plan.project_path,planId:this.team.workflow.plan.plan_id,workerId:this.id,...extra});}
    save(){if(this.state){this.state.scroll=this.body.scrollTop;this.state.draft=this.input.value;}}
    selection(id){const state=this.states.get(id);return state?{messageStart:state.messageStart,logStart:state.logStart}:{};}
    update(team,id){
      if(!team?.workflow)return;
      const identity=team.workflow.plan.project_id+':'+team.workflow.plan.plan_id;
      if(this.identity!==identity){this.states.clear();this.id=undefined;this.identity=identity;}
      const switched=this.id!==id;
      if(switched){this.save();this.id=id;this.key=undefined;this.state=this.states.get(id)||{tab:'chat',mode:'formatted',query:'',draft:'',scroll:0,cleared:0};this.states.set(id,this.state);this.input.value=this.state.draft;this.mode.value=this.state.mode;this.search.value=this.state.query;}
      this.team=team;this.list.replaceChildren(el('h3','Claude Workers'));
      for(const worker of team.workflow.workers){
        const row=team.workflow.tasks.find(t=>t.task.task_id===worker.task_id);
        const b=button('', '打开 Worker '+worker.worker_id,()=>this.select(worker.worker_id),'worker-list-item');b.classList.toggle('selected',worker.worker_id===id);
        b.append(el('strong','✳ '+worker.worker_id.slice(0,8)),el('small',row?.task.task_description||worker.task_id),el('span',(status[worker.status==='RETRYING'?'RETRYING':row?.status||worker.status]||worker.status)+(worker.unread?' · 未读 '+worker.unread:'')));this.list.append(b);
      }
      this.list.append(button('新增任务 / Worker','回到 GPT 规划新增 Worker，禁止无任务启动',()=>this.select('gpt'),'text-button'));
      const d=team.selected_worker===id?team.detail:undefined;this.detail=d;
      if(!d){this.progressView.update();this.key=undefined;this.viewKey=undefined;this.rendered.clear();this.heading.textContent='正在加载所选 Worker';this.body.replaceChildren();this.side.replaceChildren();this.pager.replaceChildren();this.retrySlot.replaceChildren();this.retryView=undefined;this.side.hidden=true;this.input.disabled=this.send.disabled=true;for(const b of this.controlButtons.values())b.disabled=true;return;}
      const progress=team.progress?.tasks.find(p=>p.task_id===d.worker.task_id&&p.worker_id===id);
      this.progressView.update(progress);const key=JSON.stringify([d,progress,team.workflow.tasks.find(t=>t.task.task_id===d.worker.task_id),this.state.tab,this.state.mode,this.state.query,this.state.cleared]);if(key===this.key)return;this.key=key;
      const previousScroll=this.body.scrollTop,nearBottom=this.body.scrollHeight-this.body.scrollTop-this.body.clientHeight<70;
      const row=team.workflow.tasks.find(t=>t.task.task_id===d.worker.task_id);
      this.heading.textContent='Claude '+id.slice(0,8);
      this.heading.title=row?.task.task_name||row?.task.task_description||d.worker.task_id;
      this.retrySlot.replaceChildren();
      this.retrySlot.hidden=!!progress&&!d.worker.retry;
      this.retryView=d.worker.retry?window.ATMOutputFormatter.retryBar(d.worker.retry,this.heading.textContent,text=>this.copy(text)):undefined;
      if(this.retryView)this.retrySlot.append(this.retryView.node);
      else this.retrySlot.append(window.ATMOutputFormatter.badge(row?.status||d.worker.status),el('span',row?.task.task_name||row?.task.task_description||d.worker.task_id,'worker-task-label'));
      this.showDetails(!!this.state.details);
      for(const b of this.tabs.children)b.setAttribute('aria-selected',String(b.dataset.tab===this.state.tab));
      const viewKey=[identity,id,this.state.tab,this.mode.value,d.message_start].join(':');
      if(this.viewKey!==viewKey||this.state.tab!=='chat'){this.body.replaceChildren();this.rendered.clear();}this.viewKey=viewKey;
      this.pager.replaceChildren();this.tools.hidden=this.state.tab!=='logs'&&this.state.tab!=='chat';
      const format=(text,context)=>{try{return window.ATMOutputFormatter.render(text,text=>this.copy(text),this.mode.value,context);}catch{return el('pre',text,'raw-output');}};
      if(this.state.tab==='chat'){
        const messages=d.chat.conversation?.messages||[],ids=new Set(messages.map(m=>m.id));
        for(const [id,entry] of this.rendered)if(!ids.has(id)){entry.article.remove();this.rendered.delete(id);}
        for(const message of messages){let entry=this.rendered.get(message.id);if(!entry){const article=el('article',undefined,'chat-message '+message.role),meta=el('div',undefined,'message-meta'),body=el('div');article.append(meta,body);this.body.append(article);entry={article,meta,body};this.rendered.set(message.id,entry);}
          entry.meta.textContent=(message.role==='user'?'任务指令':'Claude')+' · '+message.status;
          if(entry.text!==message.text){entry.body.replaceChildren(format(message.text,{project_id:d.worker.project_id,worker_id:id,task_id:d.worker.task_id,session_id:d.worker.session_id,time:message.time}));entry.text=message.text;}
        }
        this.state.messageStart=d.message_start;
        this.page(d.message_start,d.message_total,'messageStart');
      }else if(this.state.tab==='logs'){
        for(const event of d.events.events.filter(e=>e.sequence>=this.state.cleared&&JSON.stringify(e).toLowerCase().includes(this.search.value.toLowerCase())))this.body.append(window.ATMOutputFormatter.tool(event,text=>this.copy(text),this.mode.value));
        this.state.logStart=d.events.events[0]?.sequence||0;this.page(this.state.logStart,d.event_total,'logStart');
      }else if(this.state.tab==='files'){
        for(const file of d.candidate?.files||[])this.body.append(button(file.kind+' · '+file.path,'查看实际 Diff '+file.path,()=>this.action('teamFile',{file:file.path}),'worker-file'));
        this.body.append(el('p','工作区：'+d.worker.workspace_path),el('p','写权限：'+d.worker.allowed_files.join('、')),el('p','文件锁由后台在执行期间持有；具体阻止原因见执行日志。'));
      }else if(this.state.tab==='tests'){
        for(const test of d.candidate?.tests||[]){const card=el('details',undefined,'tool-event');card.append(el('summary',test.status+' · '+test.command+' · '+(test.test_count??'未知')+' tests'),el('pre',test.output||''));this.body.append(card);}
      }else {
        this.body.append(el('h3',d.task?.task_name||d.task?.task_id||'当前任务'),el('p',d.task?.task_description||'未提供'));
        for(const [label,values] of [['允许修改',d.task?.allowed_files],['只读上下文',d.task?.read_only_files],['验收标准',d.task?.acceptance_criteria]]){this.body.append(el('h4',label));const list=el('ul');for(const value of values||[])list.append(el('li',value));this.body.append(list);}
        const raw=el('details');raw.append(el('summary','完整任务数据'),format(JSON.stringify(d.task,null,2),{worker_id:id,session_id:d.worker.session_id}));this.body.append(raw);
      }
      if(!this.body.childElementCount)this.body.append(el('p','尚无该类实际记录','muted-text'));
      this.side.replaceChildren(el('h3','执行详情'),el('p','Task ID: '+d.worker.task_id),el('p','Session ID: '+(d.worker.session_id||'尚未建立')),el('p','实际阶段：'+(status[row?.status]||row?.status||d.worker.status)));
      const retry=d.worker.retry;
      this.side.prepend(button('关闭详情','关闭 Worker 详情',()=>{this.showDetails(false);this.detailToggle.focus();},'secondary-button'));
      if(row?.reason)this.side.append(el('p',row.reason,'commander-risk'));
      this.side.append(el('small','状态更新：'+(progress?.updated_at?new Date(progress.updated_at).toLocaleString('zh-CN',{hour12:false}):'未记录')),window.ATMOutputFormatter.progressTimeline(progress));
      const steps=el('ol');for(const e of row?.events||[])steps.append(el('li',(e.time||'')+' · '+(status[e.to]||e.to)+' '+(e.reason||'')));this.side.append(el('h3','执行步骤'),steps);
      this.side.append(el('p','任务来源：GPT 计划 '+team.workflow.plan.plan_id),el('p','任务依赖：'+(d.task?.dependencies?.join('、')||'独立任务')),el('h3','GPT 审查反馈'));
      for(const r of d.reviews||[]){this.side.append(el('p',r.review_status+' · '+r.summary),el('small','Review ID: '+r.review_id));if(r.block){this.side.append(el('p',r.block.kind));for(const reason of r.block.reasons)this.side.append(el('p',reason,'commander-risk'));this.side.append(el('p',r.block.next_action));}}
      this.side.append(el('h3','定向修复'));for(const r of d.repairs||[])this.side.append(el('p','第 '+r.round+' 轮 · '+r.status+' · '+r.description));
      this.side.append(el('h3','文件变更'));for(const file of d.candidate?.files||[])this.side.append(button(file.kind+' '+file.path,'查看实际 Diff '+file.path,()=>this.action('teamFile',{file:file.path}),'worker-file'));
      this.side.append(el('h3','最近真实日志'));for(const event of d.events.events.slice(-8))this.side.append(window.ATMOutputFormatter.tool(event,text=>this.copy(text),this.mode.value));
      this.side.append(el('p','暂停会停止当前 CLI 并保留 Session 和候选；恢复需重新检查权限。','muted-text'));
      for(const [action,enabled] of [['pause',['RUNNING','REVIEWING','REPAIRING','READY','WAITING_DEPENDENCY','WAITING_FILE_LOCK'].includes(row?.status)],['resume',['PAUSED','WAITING_APPROVAL','WAITING_REVIEW','FAILED'].includes(row?.status)],['cancel',!['COMPLETED','CANCELLED','FAILED'].includes(row?.status)]])this.controlButtons.get(action).disabled=!enabled;
      const resume=this.controlButtons.get('resume');resume.textContent=row?.status==='WAITING_REVIEW'?'重新审查':'恢复';
      if(team.error||d.candidate?.error)this.side.append(el('p',team.error||d.candidate.error,'commander-risk'));
      this.input.disabled=this.send.disabled=!['PAUSED','WAITING_APPROVAL'].includes(row?.status);this.input.placeholder=this.input.disabled?'先暂停当前任务，再补充原范围内指令':'补充说明不扩大原任务权限；保存后点击恢复';
      this.body.scrollTop=switched?this.state.scroll:this.state.paused||!nearBottom?previousScroll:this.state.tab==='chat'||this.state.tab==='logs'?this.body.scrollHeight:this.state.scroll;
    }
    page(start,total,field){
      this.pager.hidden=total<=100;
      this.pager.append(el('span',`${total?start+1:0}–${Math.min(start+100,total)} / ${total}`));
      for(const [label,cursor,disabled] of [['上一页',Math.max(0,start-100),start===0],['下一页',start+100,start+100>=total],['最新',Math.max(0,total-100),start+100>=total]]){const b=button(label,label+' Worker 记录',()=>this.action('teamSelect',{[field]:cursor}),'secondary-button');b.disabled=disabled;this.pager.append(b);}
    }
  }
  window.ATMWorkerPanel=WorkerPanel;
})();
