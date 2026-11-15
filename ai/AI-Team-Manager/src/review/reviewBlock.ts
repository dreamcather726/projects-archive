import type { ReviewReport } from './reviewTypes';
import { safeReviewText } from './reviewSafety';

export function reviewBlock(report:ReviewReport){
  if(report.verdict==='PASS'||report.verdict==='REQUEST_CHANGES')return undefined;
  const errors=report.batches.flatMap(b=>b.error?[b.error]:[]);
  const kind=report.verdict==='NEEDS_HUMAN_REVIEW'?'HUMAN_REVIEW':errors.length?'REVIEW_SERVICE_ERROR':'MISSING_EVIDENCE';
  const reasons=[...new Set([...report.missing_evidence,...errors,...report.dimensions.filter(d=>d.status==='INSUFFICIENT_EVIDENCE'||d.status==='NEEDS_HUMAN_REVIEW').map(d=>d.summary)])].filter(Boolean);
  if(!reasons.length)reasons.push(...report.batches.map(b=>b.summary).filter(Boolean));
  return {kind,reasons:reasons.slice(0,8).map(s=>safeReviewText(s).slice(0,700)),reason_count:reasons.length,next_action:kind==='HUMAN_REVIEW'?'查看完整报告并人工确认；不能自动通过。':kind==='REVIEW_SERVICE_ERROR'?'处理审查服务错误后显式重新审查；保留候选，不重复开发。':'补齐必要证据后显式重新审查；版本未变时复用有效测试。'};
}
