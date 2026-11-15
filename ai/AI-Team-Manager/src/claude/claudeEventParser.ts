import { StringDecoder } from "node:string_decoder";
import type { ClaudeEvent } from "./claudeTypes";
import type { Usage } from "../chat/chatTypes";
import { safeClaudeText, isClaudeApiDiagnostic } from "./claudeErrors";
function parseUsage(u: any): Usage | undefined {
  if (!u || typeof u !== 'object') return undefined;
  const n = (key: string) => typeof u[key] === 'number' && Number.isFinite(u[key]) && u[key] >= 0 ? u[key] : null;
  const input=n('input_tokens'), output=n('output_tokens'), cached=n('cache_read_input_tokens'), cacheWrite=n('cache_creation_input_tokens');
  return {input,ordinaryInput:input,output,cached,cacheWrite,reasoning:null,total:[input,output,cached,cacheWrite].every(v=>v!==null)?input!+output!+cached!+cacheWrite!:null};
}
function hasPositiveNumber(value: unknown): boolean {
  if (typeof value === 'number') return value > 0;
  return !!value && typeof value === 'object' && Object.values(value).some(hasPositiveNumber);
}
export const validClaudeSession = (id: unknown): id is string =>
  typeof id === "string" &&
  /^[a-f0-9]{8}-[a-f0-9]{4}-[a-f0-9]{4}-[a-f0-9]{4}-[a-f0-9]{12}$/i.test(id);
export class ClaudeEventParser {
  private decoder = new StringDecoder("utf8");
  private pending = "";
  private dropping = false;
  private current = "";
  private texts = new Map<number, string>();
  private tools = new Set<string>();
  private session?: string;
  private lastText = "";
  constructor(private emit: (event: ClaudeEvent) => void) {}
  push(chunk: Buffer | string): void {
    this.consume(typeof chunk === "string" ? chunk : this.decoder.write(chunk));
  }
  end(): void {
    this.consume(this.decoder.end());
    if (this.pending.trim() && !this.dropping) this.line(this.pending);
    this.pending = "";
  }
  private consume(text: string): void {
    for (const segment of text.split(/(?<=\n)/)) {
      const ends = segment.endsWith("\n");
      if (!this.dropping) this.pending += segment;
      if (this.pending.length > 2 * 1024 * 1024) {
        this.pending = "";
        this.dropping = true;
        this.emit({ type: "parseError" });
      }
      if (ends) {
        if (!this.dropping) this.line(this.pending);
        this.pending = "";
        this.dropping = false;
      }
    }
  }
  private text(
    id: string,
    text: string,
    completed: boolean,
    delta = false,
  ): void {
    if (text) {
      this.lastText = text;
      this.emit({
        type: "text",
        id,
        text: safeClaudeText(text),
        completed,
        delta,
      });
    }
  }
  private tool(block: any): void {
    if (typeof block.id !== "string" || typeof block.name !== "string") { this.emit({type:'parseError'}); return; }
    // Keep event metadata only; never copy raw file content, command stdin or arbitrary tool input into logs.
    const input = block.input || {},
      detail =
        typeof input.file_path === "string"
          ? input.file_path
          : typeof input.command === "string"
            ? input.command
            : undefined;
    if (this.tools.has(block.id) && !detail) return;
    this.tools.add(block.id);
    this.emit({
      type: "tool",
      id: block.id.slice(0, 200),
      name: block.name.slice(0, 80),
      phase: "running",
      detail: detail ? safeClaudeText(detail).slice(0, 500) : undefined,
    });
  }
  private line(line: string): void {
    if (!line.trim()) return;
    let e: any;
    try {
      e = JSON.parse(line);
      if (!e || typeof e !== "object") throw new Error();
    } catch {
      this.emit({ type: "parseError" });
      return;
    }
    if (hasPositiveNumber(e.usage) || hasPositiveNumber(e.modelUsage) || hasPositiveNumber(e.total_cost_usd)) this.emit({type:'execution',kind:'UNKNOWN'});
    if (e.parent_tool_use_id) { this.emit({type:'execution',kind:'CHILD_ACTIVITY'}); return; } // Do not expose child content, but never replay it.
    if (e.session_id !== undefined) {
      if (!validClaudeSession(e.session_id)) {
        this.emit({ type: "parseError" });
        return;
      }
      if (this.session && this.session !== e.session_id) {
        this.emit({ type: "parseError" });
        return;
      }
      if (!this.session) {
        this.session = e.session_id;
        this.emit({ type: "session", id: e.session_id });
      }
    }
    if (e.type === "system" && e.subtype === "init")
      this.emit({ type: "status", message: "Claude CLI 已启动" });
    else if (e.type === "stream_event") {
      const event = e.event;
      if (!event || typeof event.type !== "string") {
        this.emit({ type: "parseError" });
        return;
      }
      if (event.type === "message_start") {
        this.emit({type:'execution',kind:'UNKNOWN'});
        this.current = String(event.message?.id || "").slice(0, 200);
        const usage = parseUsage(event.message?.usage);
        if (usage) this.emit({type:'usage',usage});
        this.texts.clear();
        this.emit({ type: "status", message: "正在接收 Claude 输出" });
      } else if (event.type === "content_block_start") {
        const b = event.content_block;
        if (b?.type === "text") {
          this.texts.set(event.index, typeof b.text === "string" ? b.text : "");
          if (b.text) this.text(this.current || 'stream',b.text,false);
        } else if (b?.type === "tool_use") this.tool(b);
        else if (b?.type === "thinking") {
          this.emit({type:'execution',kind:'THINKING'});
          this.emit({type:'status',message:'Claude 正在思考（状态事件，不展示原始推理内容）'});
        } else this.emit({type:'execution',kind:'UNKNOWN'});
      } else if (event.type === "content_block_delta") {
        const delta = event.delta;
        if (
          delta?.type === "text_delta" &&
          typeof delta.text === "string" &&
          this.current
        ) {
          this.texts.set(
            event.index,
            (this.texts.get(event.index) || "") + delta.text,
          );
          this.text(
            this.current,
            [...this.texts.entries()]
              .sort((a, b) => a[0] - b[0])
              .map(([, v]) => v)
              .join(""),
            false,
            true,
          );
        } else if (delta?.type === "thinking_delta") {
          this.emit({type:'execution',kind:'THINKING'});
          this.emit({ type: "status", message: "Claude 正在思考" });
        } else this.emit({type:'execution',kind:'UNKNOWN'});
      } else if (event.type === 'message_delta') {
        const usage=parseUsage(event.usage);if(usage)this.emit({type:'usage',usage});
      } else if (event.type === "message_stop" && this.current) {
        const text = [...this.texts.entries()]
          .sort((a, b) => a[0] - b[0])
          .map(([, v]) => v)
          .join("");
        this.text(this.current, text, true);
      } else if (event.type !== 'content_block_stop') this.emit({type:'execution',kind:'UNKNOWN'});
    } else if (e.type === "assistant" && Array.isArray(e.message?.content)) {
      const id = String(e.message.id || this.current || "assistant").slice(
        0,
        200,
      );
      const text = e.message.content.filter((b:any)=>b?.type==='text'&&typeof b.text==='string').map((b:any)=>b.text).join('\n');
      const usage=parseUsage(e.message.usage);if(usage)this.emit({type:'usage',usage});
      if (e.error && isClaudeApiDiagnostic(text)) {
        this.emit({type:'result',ok:false,denied:false,error:text});
      } else this.text(id,text,true);
      for (const block of e.message.content) {
        if (block?.type === 'tool_use') this.tool(block);
        else if (block?.type === 'thinking' || block?.type === 'redacted_thinking') this.emit({type:'execution',kind:'THINKING'});
        else if (block?.type !== 'text') this.emit({type:'execution',kind:'UNKNOWN'});
      }
      if (e.error)
        this.emit({
          type: "status",
          message: "Claude 返回 API 错误，等待最终错误分类",
        });
    } else if (e.type === "user" && Array.isArray(e.message?.content)) {
      for (const block of e.message.content)
        if (
          block?.type === "tool_result" &&
          typeof block.tool_use_id === "string"
        )
          this.emit({
            type: "tool",
            id: block.tool_use_id.slice(0, 200),
            name: "工具",
            phase: block.is_error ? "failed" : "completed",
            detail: block.is_error
              ? "工具返回失败；可能需要额外权限。"
              : undefined,
          });
    } else if (e.type === "result") {
      if (
        typeof e.result === "string" &&
        e.result.trim() !== this.lastText.trim() &&
        !(e.is_error === true && isClaudeApiDiagnostic(e.result))
      )
        this.text("result", e.result, true);
      const usage = parseUsage(e.usage);
      const denied =
        Array.isArray(e.permission_denials) && e.permission_denials.length > 0;
      this.emit({
        type: "result",
        ok: e.is_error === false && e.subtype === "success",
        usage,
        denied,
        error: e.is_error
          ? [e.subtype, e.result, ...(Array.isArray(e.errors) ? e.errors : [])]
              .filter((v) => typeof v === "string")
              .join("\n")
              .slice(0, 16000)
          : undefined,
      });
    } else if (e.type === "tool_progress") {
      this.emit({type:'execution',kind:'UNKNOWN'});
      this.emit({
        type: "status",
        message: `工具 ${safeClaudeText(String(e.tool_name || "执行中")).slice(0, 80)} 仍在执行`,
      });
    } else if (e.type === "error")
      this.emit({
        type: "result",
        ok: false,
        denied: false,
        error:
          typeof e.message === "string"
            ? e.message
            : typeof e.error?.message === "string"
              ? e.error.message
              : "Claude error",
      });
    else this.emit({type:'execution',kind:'UNKNOWN'});
  }
}
