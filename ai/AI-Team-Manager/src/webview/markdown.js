// Deliberately small Markdown renderer: all model text becomes text nodes.
// No raw HTML, external images, iframe, style, or executable links are accepted.
(() => {
  function inline(parent, text) {
    const pattern = /(`[^`]+`|\*\*[^*]+\*\*|\*[^*]+\*|\[[^\]]+\]\([^\s)]+\))/g;
    let cursor = 0;
    for (const match of text.matchAll(pattern)) {
      parent.append(document.createTextNode(text.slice(cursor, match.index)));
      const token = match[0];
      const link = /^\[([^\]]+)\]\(([^)]+)\)$/.exec(token);
      if (link) {
        const node = document.createElement(/^(https?:|mailto:)/i.test(link[2]) ? 'a' : 'span');
        node.textContent = link[1];
        if (node.tagName === 'A') { node.href = link[2]; node.rel = 'noopener noreferrer'; node.target = '_blank'; }
        parent.append(node); cursor = match.index + token.length; continue;
      }
      const node = document.createElement(token[0] === '`' ? 'code' : token.startsWith('**') ? 'strong' : 'em');
      node.textContent = token.slice(token.startsWith('**') ? 2 : 1, token.startsWith('**') ? -2 : -1);
      parent.append(node);cursor = match.index + token.length;
    }
    parent.append(document.createTextNode(text.slice(cursor)));
  }
  window.renderGPTMarkdown = (text, copy) => {
    const fragment = document.createDocumentFragment();
    const lines = String(text).split('\n');
    for (let i = 0; i < lines.length; i++) {
      const line = lines[i];
      if (line.startsWith('```')) {
        const language = line.slice(3).trim().slice(0,40);const code = [];
        while (++i < lines.length && !lines[i].startsWith('```')) code.push(lines[i]);
        const container = document.createElement('div');container.className = 'code-block';
        const header = document.createElement('div');header.className = 'code-header';
        const label = document.createElement('span');label.textContent = language || '代码';
        const button = document.createElement('button');button.textContent = '复制代码';button.type = 'button';
        button.addEventListener('click', () => {copy(code.join('\n'));button.textContent = '已复制';});
        const pre = document.createElement('pre');const content = document.createElement('code');content.dataset.language = language;
        const source = code.join('\n');
        // Small local lexer: text nodes only, including every unrecognised token.
        const lexer = /("(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*'|\b(?:const|let|function|return|class|import|from|def|if|else|for|while|true|false|null|None|True|False)\b|\b\d+(?:\.\d+)?\b)/g;
        let cursor = 0;
        for (const match of source.matchAll(lexer)) { content.append(document.createTextNode(source.slice(cursor, match.index))); const token = document.createElement('span'); token.className = /^["']/.test(match[0]) ? 'syntax-string' : /^\d/.test(match[0]) ? 'syntax-number' : 'syntax-keyword'; token.textContent = match[0]; content.append(token); cursor = match.index + match[0].length; }
        content.append(document.createTextNode(source.slice(cursor))); pre.append(content);
        header.append(label,button);container.append(header,pre);fragment.append(container);
      } else if (line.trim()) {
        if (line.includes('|') && /^\s*\|?\s*:?-{3,}/.test(lines[i+1] || '')) {
          const table = document.createElement('table'), head = document.createElement('thead'), body = document.createElement('tbody');
          const cells = s => s.trim().replace(/^\||\|$/g, '').split('|').map(s => s.trim());
          const row = (s, tag) => { const tr = document.createElement('tr'); for (const value of cells(s)) { const cell = document.createElement(tag); inline(cell, value); tr.append(cell); } return tr; };
          head.append(row(line, 'th')); i++;
          while (i+1 < lines.length && lines[i+1].includes('|') && lines[i+1].trim()) body.append(row(lines[++i], 'td'));
          table.append(head, body); const wrap = document.createElement('div'); wrap.className='markdown-table'; wrap.append(table); fragment.append(wrap); continue;
        }
        const heading = /^(#{1,6})\s+(.*)$/.exec(line);
        const list = /^\s*(?:[-*+]\s+|\d+\.\s+)(.*)$/.exec(line);
        const node = document.createElement(heading ? `h${heading[1].length}` : list ? 'li' : line.startsWith('> ') ? 'blockquote' : 'p');
        inline(node, heading ? heading[2] : list ? list[1] : line.startsWith('> ') ? line.slice(2) : line);
        if (list) { const tag = /^\s*\d+\./.test(line) ? 'OL' : 'UL'; let group = fragment.lastChild; if (group?.tagName !== tag) { group = document.createElement(tag.toLowerCase()); if(tag==='OL')group.start = parseInt(line,10); fragment.append(group); } group.append(node); continue; }
        fragment.append(node);
      }
    }
    return fragment;
  };
})();
