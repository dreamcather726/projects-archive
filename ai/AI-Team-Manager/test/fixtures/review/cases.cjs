// Intentional defective examples for real code-review acceptance; never application code.
const stub = "module.exports = { run() { throw new Error('TODO'); } };\n";
module.exports = [
  {
    name: "correct",
    task: "实现 run(value)：有限实数夹到 0..100（包含端点），负数返回0，大于100返回100，合法区间保持原值；NaN和无穷大抛TypeError。只修改 worker.cjs，不修改测试。",
    before: stub,
    after: `function run(value) {
  if (!Number.isFinite(value)) throw new TypeError('finite number required');
  return Math.max(0, Math.min(100, value));
}
module.exports = { run };
`,
    test: `const {run}=require('./worker.cjs');
test('finite clamping and boundaries',()=>{for(const [a,b] of [[-5,0],[0,0],[0.25,0.25],[50,50],[100,100],[150,100]])assert.equal(run(a),b);for(const n of [NaN,Infinity,-Infinity])assert.throws(()=>run(n),TypeError);});`,
  },
  {
    name: "logic",
    task: "实现 run(value)：有限实数夹到0..100；负数必须返回0，合法区间保持原值，大于100返回100。非有限数抛TypeError。只修改 worker.cjs。",
    before: stub,
    after: `function run(value) {
  if (!Number.isFinite(value)) throw new TypeError('finite number required');
  if (value < 0) return Math.abs(value);
  return Math.min(100, value);
}
module.exports = { run };
`,
    test: `const {run}=require('./worker.cjs');test('negative speed clamps to zero',()=>assert.equal(run(-5),0));`,
  },
  {
    name: "resource-leak",
    task: "实现 run(file)：同步读取一个非空文件的第一个字节并返回其数值。每次打开的文件描述符必须在成功及异常路径上关闭。只修改 worker.cjs，不修改测试。",
    before: stub,
    after: `const fs = require('node:fs');
function run(file) {
  const fd = fs.openSync(file, 'r');
  const buffer = Buffer.alloc(1);
  fs.readSync(fd, buffer, 0, 1, 0);
  return buffer[0];
}
module.exports = { run };
`,
    test: `const fs=require('node:fs'),path=require('node:path'),{run}=require('./worker.cjs');
test('file descriptors are closed after read',()=>{const open=fs.openSync,close=fs.closeSync,live=new Set();fs.openSync=(...a)=>{const fd=open(...a);live.add(fd);return fd;};fs.closeSync=fd=>{live.delete(fd);return close(fd);};try{for(let n=0;n<3;n++)assert.equal(run(path.join(__dirname,'input.txt')),65);assert.equal(live.size,0,'opened descriptors must be closed');}finally{fs.openSync=open;fs.closeSync=close;for(const fd of live)close(fd);}});`,
  },
  {
    name: "out-of-bounds",
    task: "实现 run(values)：对有限数值数组的元素求和，空数组返回0，不访问数组长度以外的元素。只修改 worker.cjs。",
    before: stub,
    after: `function run(values) {
  let total = 0;
  for (let i = 0; i <= values.length; i++) {
    total += values[i];
  }
  return total;
}
module.exports = { run };
`,
    test: `const {run}=require('./worker.cjs');test('empty and multi element sum',()=>{assert.equal(run([]),0);assert.equal(run([1,2,3]),6);});`,
  },
  {
    name: "requirement-omission",
    task: "实现 run(values)：输入非空有限数值数组，返回对象 {count,total,max}，分别为数量、总和、最大值，必须同时提供三个字段。只修改 worker.cjs，不修改测试。",
    before: stub,
    after: `function run(values) {
  let total = 0;
  for (const value of values) total += value;
  return { count: values.length, total };
}
module.exports = { run };
`,
    test: `const {run}=require('./worker.cjs');test('required three fields',()=>assert.deepEqual(run([2,5,3]),{count:3,total:10,max:5}));`,
  },
];
