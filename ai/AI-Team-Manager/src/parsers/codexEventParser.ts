import { StringDecoder } from 'node:string_decoder';
import type { CodexEvent } from '../chat/chatTypes';
import { parseUsage } from '../services/codexUsageService';
import { redact } from '../services/codexErrors';
export class CodexEventParser {
  private decoder = new StringDecoder('utf8');
  private pending = '';
  private dropping = false;
  constructor(private emit: (event: CodexEvent) => void) {}
  push(chunk: Buffer | string): void { this.consume(typeof chunk === 'string' ? chunk : this.decoder.write(chunk)); }
  end(): void { this.consume(this.decoder.end()); if (this.pending.trim() && !this.dropping) this.line(this.pending); this.pending = ''; }
  private consume(text: string): void {
    for (const segment of text.split(/(?<=\n)/)) {
      const ends = segment.endsWith('\n');
      if (!this.dropping) this.pending += segment;
      if (this.pending.length > 2 * 1024 * 1024) { this.pending = ''; this.dropping = true; this.emit({type:'parseError'}); }
      if (ends) { if (!this.dropping) this.line(this.pending); this.pending = ''; this.dropping = false; }
    }
  }
  private line(line: string): void {
    if (!line.trim()) return;
    let e: any;
    try { e = JSON.parse(line); if (!e || typeof e !== 'object') throw new Error(); }
    catch { this.emit({type:'parseError'}); return; }
    const emit = this.emit;
    if (e.type === 'thread.started' && typeof e.thread_id === 'string') {
      if (/^[a-zA-Z0-9_-]{1,128}$/.test(e.thread_id)) emit({type:'thread',id:e.thread_id});
      else emit({type:'parseError'});
    } else if (e.type === 'turn.started') emit({type:'turn',state:'started'});
    else if (e.type === 'turn.completed') {
      if (e.usage && typeof e.usage === 'object') emit({type:'usage',usage:parseUsage(e.usage)});
      emit({type:'turn',state:'completed'});
    } else if (e.type === 'turn.failed') emit({type:'turn',state:'failed',error:typeof e.error?.message === 'string' ? e.error.message : ''});
    else if (e.type === 'error') emit({type:'error',message:typeof e.message === 'string' ? e.message : ''});
    else if (['item.started','item.updated','item.completed'].includes(e.type) && e.item && typeof e.item === 'object') {
      const item = e.item;
      const id = typeof item.id === 'string' ? item.id.slice(0,200) : 'unknown';
      const completed = e.type === 'item.completed';
      if (item.type === 'agent_message' && typeof item.text === 'string') emit({type:'message',id,text:redact(item.text),completed});
      else if (item.type === 'command_execution') emit({type:'activity',kind:'command',id,text:redact(String(item.command || '')).slice(0,400),output:typeof item.aggregated_output === 'string' ? redact(item.aggregated_output).slice(0,1000) : undefined,exitCode:typeof item.exit_code === 'number' ? item.exit_code : undefined,completed});
      else if (item.type === 'file_change') emit({type:'activity',kind:'file',id,text:'检测到文件变更事件，违反本阶段只读约束。',completed});
      else if(item.type==='mcp_tool_call')emit({type:'activity',kind:'mcp',id,text:redact(`${item.server}/${item.tool} · ${item.status}`),output:item.error?'工具调用失败（详见返回结果）':undefined,completed});
      // Reasoning items and unknown payloads never leave the parser.
    } else emit({type:'unknown'});
  }
}
