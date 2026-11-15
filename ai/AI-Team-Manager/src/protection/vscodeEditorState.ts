import * as vscode from 'vscode';
import * as path from 'node:path';
import { digest } from '../review/reviewGit';
import type { EditorFileState } from './userEditProtectionManager';
/** Include background text documents and visible editors, not only the currently focused tab. */
export async function editorFileState(root:string):Promise<EditorFileState[]>{
  return vscode.workspace.textDocuments.filter(d=>d.uri.scheme==='file').flatMap(d=>{const file=path.relative(root,d.uri.fsPath).replace(/\\/g,'/');if(!file||file.startsWith('../')||path.isAbsolute(file))return [];return [{file,dirty:d.isDirty,version:d.version,buffer_hash:digest(d.getText()),open:!d.isClosed}];});
}
