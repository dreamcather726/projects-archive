const path=require('node:path');
Object.assign(process.env,{ATM_HOST_CODEX_REAL:'1',ATM_HOST_TEST_MODULE:path.resolve('test/task0711-package-host.cjs'),ATM_HOST_LABEL:'task0711-final-package',ATM_HOST_TIMEOUT_MS:'120000',ATM_HOST_VSIX:path.resolve('test-results/ai-team-manager-0.8.0.vsix')});require('./run-host.cjs');
