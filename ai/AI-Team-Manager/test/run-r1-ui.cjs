const { buildSync } = require('esbuild');
const { spawnSync } = require('node:child_process');
buildSync({ entryPoints: ['test/task0710-api.ts'], bundle: true, platform: 'node', format: 'cjs', outfile: '.tools/r1-ui/api.cjs' });
for (const args of [['--test', 'test/r1-ui.test.cjs'], ['test/r1-ui-browser.cjs']]) {
  const result = spawnSync(process.execPath, args, { stdio: 'inherit' });
  if (result.status !== 0) { process.exitCode = result.status ?? 1; break; }
}
