import type { TokenLedger } from './tokenLedger';
export interface UsageConfigurationUI {
  inputBudget(state:ReturnType<TokenLedger['snapshot']>):Promise<string|undefined>;
  confirmUnknown(count:number):Promise<boolean>;
}
export function validUsageBudget(value:string,known:number):boolean {
  const text=value.trim(),n=Number(text);
  return /^\d+$/.test(text)&&Number.isSafeInteger(n)&&n>=1000&&n<=100000000&&n>known;
}
/** Only an explicit UI confirmation can acknowledge unknown historical usage. */
export async function configureGPTUsage(ledger:TokenLedger,ui:UsageConfigurationUI):Promise<boolean> {
  const state=ledger.snapshot();
  if(state.running)throw Error('请先停止或完成当前 GPT 请求，再调整用量与预算。');
  const input=await ui.inputBudget(state);
  if(input===undefined)return false;
  if(!validUsageBudget(input,state.known_total))throw Error('预算须为 1000–100000000 的整数，且大于已知累计用量。');
  const acknowledged=state.unknown_requests>0?await ui.confirmUnknown(state.unknown_requests):false;
  if(state.unknown_requests>0&&!acknowledged)return false;
  // Recheck after user interaction; a new request must not be changed mid-flight.
  if(JSON.stringify(ledger.snapshot())!==JSON.stringify(state))throw Error('GPT 用量状态已变化，请重新打开用量与预算确认；本次未修改账本。');
  await ledger.configure(Number(input.trim()),acknowledged);
  return true;
}
