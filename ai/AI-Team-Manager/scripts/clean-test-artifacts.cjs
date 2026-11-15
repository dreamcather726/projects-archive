'use strict';
const fs = require('node:fs');
const path = require('node:path');
const root = path.resolve(__dirname, '..');
const tools = path.join(root, '.tools');
const apply = process.argv.includes('--apply');
if (process.argv.slice(2).some(arg => arg !== '--apply')) {
  console.error('Usage: node scripts/clean-test-artifacts.cjs [--apply]');
  process.exit(1);
}
const names = fs.existsSync(tools) ? fs.readdirSync(tools).filter(name => /^permissions-unit-[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/i.test(name)) : [];
let count = 0;
for (const name of names) {
  const target = path.resolve(tools, name);
  if (path.dirname(target) !== tools) throw new Error('Target outside test cache directory');
  const stat = fs.lstatSync(target);
  if (!stat.isDirectory() || stat.isSymbolicLink()) continue;
  if (apply) fs.rmSync(target, { recursive: true });
  count++;
}
console.log(`${apply ? 'Removed' : 'Would remove'} ${count} disposable permission-test directories.`);
if (!apply) console.log('Run with --apply after permission tests have stopped.');
