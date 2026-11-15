import type { Usage } from '../chat/chatTypes';
const count = (value: unknown): number | null => typeof value === 'number' && Number.isSafeInteger(value) && value >= 0 ? value : null;
export function parseUsage(raw: Record<string, unknown>): Usage {
  const input = count(raw.input_tokens);
  const cached = count(raw.cached_input_tokens);
  const cacheWrite = count(raw.cache_write_input_tokens);
  const output = count(raw.output_tokens);
  const reasoning = count(raw.reasoning_output_tokens);
  return { input, cached, cacheWrite, output, reasoning,
    ordinaryInput: input !== null && cached !== null && cached <= input ? input - cached : null,
    total: input !== null && output !== null ? input + output : null };
}
export function sumUsage(records: { usage?: Usage }[]): Usage {
  const keys: (keyof Usage)[] = ['input','cached','cacheWrite','output','reasoning','ordinaryInput','total'];
  return Object.fromEntries(keys.map(key => [key, records.length && records.every(r => r.usage?.[key] != null)
    ? records.reduce((sum, r) => sum + r.usage![key]!, 0) : null])) as unknown as Usage;
}
// Current Codex exec reports thread totals, including on resume. Persist raw snapshots
// and subtract the previous known snapshot; never add cached/reasoning twice.
export function usageDelta(current: Usage, previous: Usage | undefined, first: boolean): Usage {
  if(first) return {...current};
  const result = {} as Usage;
  for(const key of Object.keys(current) as (keyof Usage)[]) {
    const before = previous?.[key]; const now = current[key];
    result[key] = now !== null && before != null && now >= before ? now - before : null;
  }
  return result;
}
