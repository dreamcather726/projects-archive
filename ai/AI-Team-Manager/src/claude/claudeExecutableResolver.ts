import { access, stat } from "node:fs/promises";
import { constants } from "node:fs";
import * as path from "node:path";
import { ClaudeError } from "./claudeErrors";
export interface ResolvedClaude {
  command: string;
  source: "settings" | "path" | "extension";
  version?: string;
}
export interface ClaudeResolverOptions {
  env?: NodeJS.ProcessEnv;
  platform?: NodeJS.Platform;
  extensionDirectory: () => string | undefined;
}
async function executable(file: string): Promise<boolean> {
  try {
    return (
      (await stat(file)).isFile() &&
      (await access(file, constants.X_OK).then(() => true))
    );
  } catch {
    return false;
  }
}
export async function resolveClaudeExecutable(
  configured: string,
  options: ClaudeResolverOptions,
): Promise<ResolvedClaude> {
  const command = configured.trim() || "claude";
  if (command !== "claude") return { command, source: "settings" };
  const platform = options.platform || process.platform,
    env = options.env || process.env;
  const value = (key: string) =>
    Object.entries(env).find(
      ([name]) => name.toLowerCase() === key.toLowerCase(),
    )?.[1] || "";
  const suffixes =
    platform === "win32"
      ? (value("PATHEXT") || ".COM;.EXE;.BAT;.CMD")
          .split(";")
          .map((s) => s.toLowerCase())
      : [""];
  for (const entry of value("PATH").split(platform === "win32" ? ";" : ":")) {
    const directory = entry.trim().replace(/^"(.*)"$/, "$1");
    if (!directory || !path.isAbsolute(directory)) continue;
    for (const suffix of suffixes) {
      const candidate = path.join(directory, `claude${suffix}`);
      if (await executable(candidate))
        return { command: candidate, source: "path" };
    }
  }
  const extension = options.extensionDirectory();
  if (extension) {
    const candidate = path.join(
      extension,
      "resources",
      "native-binary",
      platform === "win32" ? "claude.exe" : "claude",
    );
    if (await executable(candidate))
      return { command: candidate, source: "extension" };
  }
  throw new ClaudeError("CLAUDE_NOT_FOUND");
}
