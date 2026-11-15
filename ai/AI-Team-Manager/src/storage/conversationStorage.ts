import * as fs from 'node:fs/promises';
import * as path from 'node:path';
import { createHash, randomUUID } from 'node:crypto';
import type { ConversationFile } from '../chat/chatTypes';
import { CodexError } from '../services/codexErrors';
export class ConversationStorage {
  private queue: Promise<void> = Promise.resolve();
  private filename: string;
  constructor(directory: string, public readonly projectPath: string) {
    const key = createHash('sha256').update(process.platform === 'win32' ? projectPath.toLowerCase() : projectPath).digest('hex');
    this.filename = path.join(directory, 'conversations', `${key}.json`);
  }
  async load(): Promise<ConversationFile | undefined> {
    try {
      const parsed = JSON.parse(await fs.readFile(this.filename,'utf8'));
      if (parsed.version !== 1 || parsed.projectPath !== this.projectPath || !Array.isArray(parsed.conversations) || typeof parsed.activeId !== 'string') throw new Error();
      for (const c of parsed.conversations) {
        if (c.projectPath !== this.projectPath || typeof c.id !== 'string' || !Array.isArray(c.messages) || !Array.isArray(c.requests)
          || c.messages.some((m: any) => typeof m.text !== 'string' || !['user','assistant'].includes(m.role))
          || (c.threadId && !/^[\w-]{1,128}$/.test(c.threadId))) throw new Error();
      }
      return parsed;
    } catch (error: any) {if(error.code === 'ENOENT') return undefined; throw new CodexError('STORAGE_ERROR');}
  }
  save(data: ConversationFile): Promise<void> {
    const serialized = JSON.stringify(data);
    const operation = this.queue.catch(() => {}).then(async () => {
      const temp = `${this.filename}.${randomUUID()}.tmp`;
      try {
        await fs.mkdir(path.dirname(this.filename),{recursive:true});
        await fs.writeFile(temp,serialized,{mode:0o600});
        await fs.rename(temp,this.filename);
      } catch {await fs.unlink(temp).catch(() => {});throw new CodexError('STORAGE_ERROR');}
    });
    this.queue = operation;
    return operation;
  }
}
