import { ConversationStorage } from "../storage/conversationStorage";
import type { ConversationFile } from "../chat/chatTypes";
// Windows file watchers can briefly lock the destination during an atomic rename.
// Serialize the whole retry, so an older snapshot can never overwrite a newer one.
export class ClaudeStorage extends ConversationStorage {
  private saves: Promise<void> = Promise.resolve();
  override save(data: ConversationFile): Promise<void> {
    const snapshot = structuredClone(data);
    const operation = this.saves
      .catch(() => {})
      .then(async () => {
        for (let attempt = 0; ; attempt++) {
          try {
            await super.save(snapshot);
            return;
          } catch (error) {
            if (attempt >= 3) throw error;
            await new Promise((resolve) =>
              setTimeout(resolve, 50 * (attempt + 1)),
            );
          }
        }
      });
    this.saves = operation;
    return operation;
  }
}
