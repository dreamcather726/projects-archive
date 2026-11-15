import * as path from 'node:path';
import type { FileChange, ReviewTask, SourceSnapshot, TestEvidence } from './reviewTypes';

/** Required local dependencies/tests take precedence over optional project background. */
export function selectReviewContext(after:SourceSnapshot,changes:FileChange[],task:ReviewTask,tests:TestEvidence[],limit=64000){
  const files=new Map(after.files.map(f=>[f.path,f])),changed=new Set(changes.map(c=>c.path));
  const required=new Set(task.context_files||[]),queue=changes.filter(c=>c.after).map(c=>c.path);
  if(files.has('AGENTS.md'))required.add('AGENTS.md');
  const testFile=/(?:^|\/)(?:tests?|__tests__)\/|\.(?:test|spec)\.[^/]+$|(?:^|\/)test_[^/]+\.py$|_test\.[^/]+$/i;
  for(const t of tests){for(const m of (t.command||'').matchAll(/(?:^|\s)([A-Za-z0-9_][A-Za-z0-9_./-]*\.(?:py|cjs|mjs|js|ts))(?=\s|$)/g))if(files.has(m[1]))required.add(m[1]);
    if(/^(?:node --test|npm test)$/.test(t.command))for(const f of after.files)if(testFile.test(f.path))required.add(f.path);
  }
  // Include tests that directly import changed code, even when execution records are missing.
  // Those missing records still remain a separate blocking gap in the collector.
  for(const f of after.files){if(!testFile.test(f.path)||!f.text)continue;
    const js=[...f.text.matchAll(/(?:from\s*|require\s*\(\s*|import\s*)["'](\.[^"']+)["']/g)].map(m=>path.posix.normalize(path.posix.join(path.posix.dirname(f.path),m[1])));
    const py=f.path.endsWith('.py')?[...f.text.matchAll(/^\s*(?:from\s+([.\w]+)\s+import|import\s+([\w.]+))/gm)].map(m=>(m[1]||m[2]).replace(/\./g,'/')+'.py'):[];
    if([...js,...py].some(p=>[p,p+'.ts',p+'.js',p+'.cjs',p+'/index.ts',p+'/index.js'].some(candidate=>changed.has(candidate))))required.add(f.path);
  }
  queue.push(...required);const visited=new Set<string>();
  while(queue.length){const name=queue.shift()!;if(visited.has(name))continue;visited.add(name);const source=files.get(name)?.text||'';
    const imports=[...source.matchAll(/(?:from\s*|require\s*\(\s*|import\s*|#include\s*)["'](\.[^"']+)["']/g)].map(m=>path.posix.normalize(path.posix.join(path.posix.dirname(name),m[1])));
    if(name.endsWith('.py'))for(const m of source.matchAll(/^\s*(?:from\s+([.\w]+)\s+import|import\s+([\w.]+))/gm)){const module=m[1]||m[2],base=module.startsWith('.')?path.posix.dirname(name):'';imports.push(path.posix.join(base,module.replace(/^\./,'').replace(/\./g,'/')));}
    for(const base of imports)for(const suffix of ['', '.ts','.js','.cjs','.mjs','.tsx','.h','.py','/index.ts','/index.js','/__init__.py']){const candidate=base+suffix;if(files.has(candidate)&&!changed.has(candidate)&&!required.has(candidate)){required.add(candidate);queue.push(candidate);}}
  }
  const context:typeof after.files=[],gaps:string[]=[],warnings:string[]=[];let size=0,skipped=0;
  for(const name of [...required].sort()){if(changed.has(name))continue;const f=files.get(name);if(!f||f.text===undefined||f.redacted){gaps.push(`必要审查上下文不可完整读取：${name}`);continue;}if(size+f.text.length>limit){gaps.push(`必要审查上下文超过 ${limit} 字符：${name}`);continue;}context.push(f);size+=f.text.length;}
  for(const f of after.files){if(changed.has(f.path)||required.has(f.path)||f.text===undefined||! /^(?:README(?:\.[^/]*)?|package\.json|tsconfig\.json|pyproject\.toml|CMakeLists\.txt|AGENTS\.md)$/i.test(f.path))continue;if(f.redacted||size+f.text.length>limit){skipped++;continue;}context.push(f);size+=f.text.length;}
  if(skipped)warnings.push(`省略 ${skipped} 个可选项目背景文件；必要代码与测试的缺口单独记录。`);
  return {context,gaps,warnings};
}
