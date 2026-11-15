import { access, stat } from 'node:fs/promises';
import { constants } from 'node:fs';
import * as path from 'node:path';
import { CodexError } from '../services/codexErrors';

export interface ResolvedCodex { command: string; source: 'settings' | 'path' | 'extension' }
export interface ResolverOptions {
  env?: NodeJS.ProcessEnv;
  platform?: NodeJS.Platform;
  arch?: string;
  extensionDirectory: () => string | undefined;
}
async function executableFile(filename: string): Promise<boolean> {
  try {return (await stat(filename)).isFile() && await access(filename,constants.X_OK).then(() => true);}
  catch {return false;}
}
export async function resolveCodexExecutable(configured: string, options: ResolverOptions): Promise<ResolvedCodex> {
  const command = configured.trim() || 'codex';
  // Explicit settings are authoritative: an invalid path must not silently pick another CLI.
  if(command !== 'codex') return {command,source:'settings'};
  const platform = options.platform || process.platform;
  const env = options.env || process.env;
  const envValue = (key: string) => Object.entries(env).find(([name]) => name.toLowerCase() === key.toLowerCase())?.[1] || '';
  const delimiter = platform === 'win32' ? ';' : ':';
  const extensions = platform === 'win32' ? (envValue('PATHEXT') || '.COM;.EXE;.BAT;.CMD').split(';').map(e => e.toLowerCase()) : [''];
  for(const entry of envValue('PATH').split(delimiter)) {
    const directory = entry.trim().replace(/^"(.*)"$/,'$1');
    // Do not auto-run a workspace-local binary from an empty/relative PATH entry.
    if(!directory || !path.isAbsolute(directory)) continue;
    for(const suffix of extensions) {
      const candidate = path.join(directory,`codex${suffix}`);
      if(await executableFile(candidate)) return {command:candidate,source:'path'};
    }
  }
  // Query the VS Code extension registry on every check; never pin a versioned directory.
  const extension = options.extensionDirectory();
  const arch = options.arch || process.arch;
  const cpu = arch === 'x64' ? 'x86_64' : arch === 'arm64' ? 'aarch64' : undefined;
  const os = platform === 'win32' ? 'windows' : platform === 'darwin' ? 'macos' : platform === 'linux' ? 'linux' : undefined;
  if(extension && cpu && os) {
    const candidate = path.join(extension,'bin',`${os}-${cpu}`,platform === 'win32' ? 'codex.exe' : 'codex');
    if(await executableFile(candidate)) return {command:candidate,source:'extension'};
  }
  throw new CodexError('CODEX_NOT_FOUND');
}
