(() => {
  const vscode = acquireVsCodeApi();
  const root = document.getElementById('root');
  function el(tag,text,cls) {const n=document.createElement(tag);if(text!==undefined)n.textContent=text;if(cls)n.className=cls;return n;}
  function row(label,value) {const n=el('div',undefined,'row');n.append(el('span',label,'muted'),el('span',value));root.append(n);}
  let chatUI;
  let pending;
  let lastProject;
  function button(text,type) {const b=el('button',text);b.type='button';b.addEventListener('click',()=>vscode.postMessage({type}));return b;}
  function initializeChat() {
    root.replaceChildren();
    const title=el('h2','GPT 总指挥');const subtitle=el('p','只读分析 · 复用现有 Codex 配置','muted');
    const status=el('div',undefined,'badge');const project=el('div',undefined,'project');
    const toolbar=el('div',undefined,'toolbar');
    const check=button('检测 CLI','check'),history=button('历史','history'),select=button('选择项目','project'),clear=button('清空记录','clear');toolbar.append(check,history,select,clear);
    const session=el('details');session.append(el('summary','会话信息'));const sessionText=el('p',undefined,'session-id');session.append(sessionText);
    const error=el('div',undefined,'error chat-error');error.setAttribute('role','alert');
    const messages=el('div',undefined,'messages');messages.setAttribute('role','log');messages.setAttribute('aria-label','GPT 聊天消息');
    const activities=el('details');activities.append(el('summary','执行活动'));const activityText=el('pre',undefined,'activity-text');activities.append(activityText);
    const stats=el('details');stats.open=true;stats.append(el('summary','资源统计'));const statsBody=el('div');stats.append(statsBody);
    const composer=el('div',undefined,'composer');const input=el('textarea');input.rows=4;input.placeholder='讨论需求或分析项目… Enter 发送，Shift+Enter 换行';input.setAttribute('aria-label','发送给 GPT 的消息');input.maxLength=100000;
    const saved=vscode.getState();input.value=saved?.draft || '';lastProject=saved?.project;
    input.addEventListener('input',()=>vscode.setState({draft:input.value,project:lastProject}));
    const actions=el('div',undefined,'toolbar');const send=el('button','发送','primary');send.type='button';
    const stop=button('停止','stop'),fresh=button('新建会话','new');actions.append(send,stop,fresh);composer.append(input,actions);
    const submit=()=> {if(send.disabled||!input.value.trim())return;pending=input.value;send.disabled=true;vscode.postMessage({type:'send',text:input.value});};
    send.addEventListener('click',submit);input.addEventListener('keydown',event=>{if(event.key==='Enter'&&!event.shiftKey&&!event.isComposing){event.preventDefault();submit();}});
    const diagnosis=el('details');diagnosis.append(el('summary','环境诊断'));const diagnosticBody=el('div');diagnosis.append(diagnosticBody);
    const openWorkspace=button('↗ 打开完整工作台','openWorkspace');openWorkspace.className='primary';
    root.append(openWorkspace,title,subtitle,status,project,toolbar,session,error,messages,activities,stats,composer,diagnosis);
    chatUI={status,project,sessionText,error,messages,activityText,statsBody,input,send,stop,fresh,history,select,clear,check,diagnosticBody};
  }
  function renderChat(data) {
    if(!chatUI)initializeChat();
    const u=chatUI,c=data.chat || {},conversation=c.conversation;
    if(lastProject&&lastProject!==c.projectPath){u.input.value='';pending=undefined;vscode.setState({draft:'',project:c.projectPath});}
    lastProject=c.projectPath;
    u.status.textContent=`${c.connection||'尚未检测'} · ${conversation?.status||'READY'}`;
    u.project.textContent=c.projectName||data.project;u.project.title=c.projectPath||'';
    const cliSource=c.cli?({'settings':'手动设置','path':'PATH','extension':'已安装的 Codex 扩展'}[c.cli.source]):'';
    u.sessionText.textContent=(conversation?.threadId||'尚未建立 Codex 会话；发送首条消息后保存 ID')+(c.cli?`\nCLI 来源：${cliSource}\n${c.cli.command}`:'');
    u.error.textContent=c.error||'';
    u.send.disabled=!!c.busy;u.stop.disabled=!c.busy;
    for(const b of [u.fresh,u.history,u.select,u.clear,u.check])b.disabled=!!c.busy;
    if(pending&&conversation?.messages.some(m=>m.role==='user'&&m.text===pending)){u.input.value='';vscode.setState({draft:'',project:lastProject});pending=undefined;}
    if(!c.busy)pending=undefined;
    const nearBottom=u.messages.scrollHeight-u.messages.scrollTop-u.messages.clientHeight<60;
    u.messages.replaceChildren();
    if(!conversation?.messages.length)u.messages.append(el('p','发送消息开始只读项目讨论。历史仅保存在插件本地存储中。','muted'));
    for(const message of conversation?.messages||[]) {
      const article=el('article',undefined,`message ${message.role}`);
      article.append(el('div',`${message.role==='user'?'你':'GPT'} · ${new Date(message.time).toLocaleTimeString()} · ${message.status}`,'message-meta'));
      const body=el('div',undefined,'message-body');
      if(message.role==='assistant')body.append(window.renderGPTMarkdown(message.text,text=>vscode.postMessage({type:'copy',text})));
      else body.append(el('p',message.text));
      article.append(body);u.messages.append(article);
    }
    if(c.busy)u.messages.append(el('p','正在接收 Codex 事件…','muted'));
    if(nearBottom)u.messages.scrollTop=u.messages.scrollHeight;
    u.activityText.textContent=(c.activities||[]).join('\n\n')||'暂无命令活动';
    u.statsBody.replaceChildren();
    const latest=conversation?.requests?.at(-1)?.usage;
    const statistic=(name,value)=>{const n=el('div',undefined,'row');n.append(el('span',name,'muted'),el('span',value===undefined||value===null?'--':String(value)));u.statsBody.append(n);};
    statistic('本轮输入（含缓存）',latest?.input);statistic('本轮普通输入',latest?.ordinaryInput);statistic('本轮缓存读取',latest?.cached);statistic('本轮缓存写入',latest?.cacheWrite);statistic('本轮输出',latest?.output);statistic('本轮推理 Token 数',latest?.reasoning);statistic('会话累计 Token',c.usage?.total);statistic('GPT 调用次数',conversation?.requests?.length);
    u.diagnosticBody.replaceChildren();
    if(!data.diagnostics.length)u.diagnosticBody.append(el('p',data.running?'正在检测…':'尚未检测，请运行 Diagnose Environment','muted'));
    for(const result of data.diagnostics){const n=el('div',undefined,'diagnostic');n.append(el('strong',`${result.ok?'✓':'✗'} ${result.name}`),el('p',result.detail,result.ok?'muted':'error'));u.diagnosticBody.append(n);}
  }
  window.addEventListener('message',({data}) => {
    if(document.body.dataset.section==='commander') {
      renderChat(data);
    } else if(document.body.dataset.section==='tasks') {
      root.replaceChildren();
      root.append(el('span','任务执行尚未接入','badge'));
      row('当前任务','未指定');row('待执行任务','未知');row('已完成任务','未知');
      row('任务执行进度','暂无实际数据');row('任务审核状态','未提交验收');
      root.append(el('p','当前任务没有通过 GPT 验收，不允许进入下一项任务。'));
    } else {
      root.replaceChildren();
      const n=el('div',undefined,'logs');n.setAttribute('role','log');
      data.logs.forEach(entry=>n.append(el('div',`${entry.time} [${entry.level}] ${entry.message}`,`log ${entry.level==='ERROR'?'error':''}`)));
      root.append(n);n.scrollTop=n.scrollHeight;
    }
  });
  vscode.postMessage({type:'ready'});
})();
