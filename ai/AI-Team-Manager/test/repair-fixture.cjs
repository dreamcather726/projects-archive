const fs=require('node:fs/promises'),path=require('node:path'),{execFileSync}=require('node:child_process');
const samples=require('./fixtures/review/cases.cjs');
exports.sample={...samples[1],task:samples[0].task,test:samples[0].test};
exports.create=async root=>{
  await require('./review-fixture.cjs').create(root,exports.sample);
  await fs.writeFile(path.join(root,'package.json'),JSON.stringify({name:'atm-repair-fixture',private:true,version:'1.0.0',scripts:{compile:'node --check worker.cjs',test:'node --test'}},null,2)+'\n');
  const git=args=>execFileSync('git',['-C',root,...args],{windowsHide:true,stdio:'pipe'});
  git(['add','--','package.json']);git(['-c','user.name=ATM Test','-c','user.email=atm-test@example.invalid','commit','--quiet','-m','controlled compile and test commands']);
};
