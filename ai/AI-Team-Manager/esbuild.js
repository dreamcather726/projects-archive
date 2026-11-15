require('esbuild').build({entryPoints:['src/extension.ts'],bundle:true,platform:'node',format:'cjs',target:'node20',external:['vscode'],outfile:'dist/extension.js'}).catch(()=>process.exit(1));
