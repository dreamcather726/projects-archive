import * as path from "node:path";
import { safeClaudeText } from "../claude/claudeErrors";
// Preserve newlines so evidence locations remain meaningful after redaction.
export function safeReviewText(text: string): string {
  const mask = (value: string) => value.replace(/[^\r\n]/g, "█");
  return safeClaudeText(text)
    .replace(
      /-----BEGIN [^-\r\n]*PRIVATE KEY-----[\s\S]*?-----END [^-\r\n]*PRIVATE KEY-----/g,
      mask,
    )
    .replace(
      /((?:["']?(?:password|passwd|secret|api[_-]?key|access[_-]?token|refresh[_-]?token|client[_-]?secret)["']?)\s*[:=]\s*)(["'])([^\r\n]*?)\2/gi,
      (_m, p, q) => `${p}${q}[REDACTED SECRET]${q}`,
    )
    .replace(/\bAKIA[A-Z0-9]{16}\b/g, "[REDACTED AWS KEY]");
}
const generated = new Set([
  "node_modules",
  ".tools",
  "dist",
  "build",
  "out",
  "coverage",
  ".next",
  ".venv",
  "venv",
  "__pycache__",
  ".git",
]);
export function excludedPath(
  file: string,
): "generated" | "sensitive" | undefined {
  const parts = file.replace(/\\/g, "/").split("/"),
    base = parts.at(-1)!.toLowerCase();
  if (
    parts.some((p) =>
      [
        ".ssh",
        ".aws",
        ".codex",
        ".claude",
        ".azure",
        ".config",
        "cc-switch",
        "cc_switch",
      ].includes(p.toLowerCase()),
    ) ||
    /^(?:\.env(?:\..*)?|\.npmrc|\.netrc|credentials?(?:\..*)?|auth\.json|secrets?(?:\..*)?|id_rsa|id_ed25519)$/.test(
      base,
    ) ||
    /\.(?:pem|p12|pfx|key)$/.test(base)
  )
    return "sensitive";
  if (
    parts.some((p) => generated.has(p.toLowerCase())) ||
    /\.(?:vsix|zip|exe|dll|png|jpg|jpeg|gif|woff2?|mp4|wav|bin|onnx|safetensors|map)$/.test(
      base,
    )
  )
    return "generated";
}
export function relativeFile(file: string): boolean {
  return (
    !!file &&
    !/[\x00-\x1f\x7f:]/.test(file) &&
    !file.includes("\\") &&
    !path.posix.isAbsolute(file) &&
    !file.split("/").some((p) => p === ".." || p === ".") &&
    !/^[A-Za-z]:/.test(file)
  );
}
