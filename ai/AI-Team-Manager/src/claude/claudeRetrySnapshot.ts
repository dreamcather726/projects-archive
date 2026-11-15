import * as fs from 'node:fs/promises';
import * as path from 'node:path';
import { createHash } from 'node:crypto';
import { excludedPath } from '../review/reviewSafety';

/** Local-only comparison: no contents/paths leave this module. Unreadable or incomplete scans fail closed. */
export async function retryWorkspaceFingerprint(root: string, signal?: AbortSignal): Promise<string> {
  const hash = createHash('sha256');
  const started = Date.now();
  const check = () => { if (signal?.aborted || Date.now()-started>10000) throw Error('Retry snapshot cancelled or incomplete'); };
  let visits = 0, bytes = 0;
  const walk = async (relative: string): Promise<void> => {
    check();
    const directory=path.join(root,relative), beforeDirectory=await fs.lstat(directory);
    if(!beforeDirectory.isDirectory()||beforeDirectory.isSymbolicLink())throw Error('Retry snapshot directory changed');
    const names = (await fs.readdir(directory)).sort();
    for (const name of names) {
      check();
      if (++visits > 50000) throw Error('Retry snapshot incomplete');
      const rel = relative ? `${relative}/${name}` : name, full = path.join(root,rel);
      const before = await fs.lstat(full);
      // Sensitive/generated file contents are never read. Still enumerate and bind their metadata;
      // a skipped subtree could otherwise conceal an unreported write during a failed request.
      const metadataOnly = !!excludedPath(rel);
      if (before.isSymbolicLink() || !before.isDirectory() && !before.isFile()) throw Error('Retry snapshot unsupported entry');
      hash.update(JSON.stringify([rel,before.mode,before.ino,before.size,before.mtimeMs,before.ctimeMs]));
      if (before.isDirectory()) await walk(rel);
      else if (!metadataOnly) {
        bytes += before.size;
        if (before.size > 16*1024*1024 || bytes > 64*1024*1024) throw Error('Retry snapshot too large');
        hash.update(JSON.stringify([before.size,before.mtimeMs,before.ctimeMs]));
        hash.update(await fs.readFile(full));
      }
      const after = await fs.lstat(full);
      if (before.ino !== after.ino || before.size !== after.size || before.mtimeMs !== after.mtimeMs || before.ctimeMs !== after.ctimeMs) throw Error('Retry snapshot changed during capture');
    }
    const afterDirectory=await fs.lstat(directory);
    if(beforeDirectory.ino!==afterDirectory.ino||beforeDirectory.mtimeMs!==afterDirectory.mtimeMs||beforeDirectory.ctimeMs!==afterDirectory.ctimeMs)throw Error('Retry snapshot enumeration changed');
    check();
  };
  await walk('');
  return hash.digest('hex');
}
