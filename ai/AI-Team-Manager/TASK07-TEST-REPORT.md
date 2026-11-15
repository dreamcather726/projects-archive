# Task 07 分阶段实施与验证记录

本报告按任务书顺序更新。截至2026-11-14，07.1–07.9已完成分阶段验证；07.10正式入口及UI端到端验收尚未完成。Task07尚未全部完成，正式多Worker入口未开放。下文早期门禁与阻塞保留为当时的历史记录，当前状态以对应阶段最新结论为准。原Task06交付与真实记录见TASK06-TEST-REPORT.md。

2026-11-14按用户要求清理已完成任务的本地测试内容：删除`.tools/`内Task02–Task06、Task07.1–07.9相关临时工程、运行记录、截图、宿主缓存、权限单测残留及临时探针，共121项、3650个普通文件，约18.64 MiB；4个测试链接只删除链接本身，不遍历目标。保留测试脚本、验收报告、浏览器测试依赖、VSIX和未完成源码。下文历史本地证据路径已随清理移除，仅保留既有验证摘要，未重新运行真实模型验收。清理后`npm run typecheck`通过，`npm test`共90项全部通过，证明测试入口可重建临时依赖；本轮验证生成的临时文件亦已清理，保留文件的SHA-256与清理前一致。

## 07.1 自适应规划（2026-11-13）

新增 `src/planning/planningTypes.ts`、`adaptiveTaskPlanner.ts`、`planningTools.ts`，复用现有GPT CLI、真实MCP桥、Git快照与本地原子存储。主GPT先调用get_project_planning_context，再通过submit_adaptive_plan提交结构化报告，不另建模型适配器。工具输入经本地校验后仅保存为VALIDATED_NOT_EXECUTED，不作为执行命令或用户授权。

报告含目标、真实项目状态、方案、任务分类、动态Worker槽位分配、精确文件范围、共享文件唯一所有权、接口/数据协议、依赖、测试、验收、范围外事项和停止策略。默认当前需求模式；没有明确授权不能设为连续开发。父/依赖标识、循环依赖、文件越权、大小写别名、设备名、敏感路径、非法命令与模型伪造完成状态均校验。顺序强依赖或文件重叠不具备并行收益时，不能仅为增加人数拆出多Worker。

修改ToolBridge支持调用方提供工具清单，默认原五工具保持兼容；DispatchCoordinator注册两个规划工具，原有调度面板增加“开发报告”按钮。旧Task06的独立单Worker流程仍保留，本阶段不改变其执行权限。新规划不会启动或创建真实Worker；独立会话与隔离是后续阶段。

### 验证结果

- `npm run typecheck`、`npm run compile`通过；`npm test`72/72通过。另重跑Windows.cmd与原调度相关9项测试通过，使用实际合并提示词和真实子进程。
- 本地Git测试实际建立仓库并保留USER DIRTY CONTENT，验证项目改变后旧证据失效、GPT owner/turn隔离、报告持久化与提交去重。没有提交或覆盖用户内容。
- 真实GPT MCP测试：`node test/planning-live.cjs`，成功证据 `.tools/task071-planning-1794554860649/result.json`，realGPT=true、projectUnchanged=true、claudeStarted=false。没有用预设计划冒充模型输出。
- 陌生工程为“光谱标定工具库”，正式逻辑中无该名称、领域、函数或固定案例匹配。

| 真实需求 | GPT结果 | plan_id | CLI total / cached Token |
| --- | --- | --- | --- |
| 一个比例纯函数及边界测试 | 1项任务、1个Worker | 9b510712-9849-476a-961d-0b4414472972 | 139701 / 120576 |
| 波长换算、基线估计、峰值检测三个独立模块 | 3项任务、3个Worker，无交叉写入，接口和独立测试明确 | 11cd27e6-7e15-4bfd-9df1-a4971a048d51 | 100468 / 72704 |

以上为实际主GPT会话累计usage（输入含缓存，不再次相加），不是固定成本承诺。两次输入/输出分别137270/2431和98029/2439。小任务最初一次提交被命令校验拒绝，GPT按明确格式修正后提交；历史失败运行保留在本地，不计为通过。之前调试运行未保存完整失败事件的缺口也不伪造补齐。

### 边界与后续门禁

- 当前是真实规划与报告，尚未实现Task07多Worker执行、Worktree锁、用户编辑保护、隔离集成或新版独立会话UI。
- 规划阶段只接受已具备可验证argv规则的Node/npm和Python脚本测试命令；其他语言工具链目前需要后续增加明确执行规则，不把任意模型字符串交给Shell。
- 项目清单与上下文有大小限制，不重复发送整个代码仓库；大项目需缩小阶段范围。必要上下文仍可由GPT自身只读工具查看，不能保证语义规划没有遗漏，实际执行前还需独立授权与安全验证。
- 机器存在Ubuntu22.04 WSL2；用户/网络命名空间只读探测成功。尚未据此宣称文件系统沙箱已建立，07.3必须实际验证跨目录、脚本与共享Git元数据隔离。
- 原有构建产物dist/extension.js及node_modules/.package-lock.json的未提交改动保留，不纳入源码提交。未修改.gitignore，模型运行记录和VSIX不上传。

本阶段Git备份结果以交付消息中的实际提交和Push输出为准。

07.1已备份：`fddb5a4`，master → origin/master，Push成功。

## 07.2 多Worker与独立Session后端（2026-11-13）

新增 `src/workers/workerTypes.ts`、`claudeWorkerManager.ts`。复用原ClaudeController / ClaudeService / ClaudeStorage，每个Worker独立存储、独立Session与消息。Worker记录包含worker_id、session_id、project_id、task_id、workspace_path、branch_name、allowed_files、状态、执行结果与日志；所有事件绑定项目/任务/Worker/Session。支持默认3并发（可调1–8）、等待队列、历史恢复、分页日志、未读提醒及同工作区/同分支的空闲Worker重分配。

UI生命周期没有创建或销毁这些控制器的权限；选择会话仅切换后端查看目标，不发送模型消息。插件退出后不会自动重发排队或中断任务。此处Worker的COMPLETED只表示CLI执行结束，不等于开发任务通过GPT验收；后续调度器单独管理任务状态。

原Claude适配器仅增加可选Worker标识，保留默认Worker01兼容行为；新Worker也不会被统一标成01。修正了Windows终止子进程后缺少close事件时，停止/超时Promise可能不结束的问题，并用明确的平台故障注入验证。没有重写适配器或添加其他模型。

### 验证结果

- 类型检查通过；原完整74项测试通过；随后增加队列/定向暂停/取消测试，Worker专项3/3通过（最终完整测试数量将在后续阶段继续更新）。测试替身只用于故障与队列回归，不冒充真实模型。
- 真实CLI：`node test/workers-live.cjs`，证据 `.tools/task072-workers-1794558242143/result.json`，passed=true、realClaude=true、readOnly=true、projectUnchanged=true。
- 三个真实Session首轮工具流时间区间确实重叠，不仅比较UI的RUNNING标签；每个Worker只记住自己的随机标记，第二轮互不混淆。
- 停止并重建后端管理器，三个原Session均恢复；选择各Worker不新增模型请求。随后真实启动目标Worker，暂停操作实际终止该CLI请求，另两个Worker正常完成；恢复目标Worker继续同一Session，原记忆标记仍正确。
- 队列专项：最大并发设1，后续任务保持QUEUED；取消尚未启动任务不产生CLI请求；暂停当前Worker释放执行槽后，其他任务正常运行。

| Worker | 真实Session | 最终状态 | 实际请求数 / 已知Claude Token |
| --- | --- | --- | --- |
| 51eccbc8-a1b3-4565-8949-be8109d4de0e | 1e994f86-1b50-4259-bfc8-f589dbc7fc05 | COMPLETED | 3 / 11774（取消请求1次用量未知） |
| 8d9568df-570c-423b-99b1-75ab257e10ef | 7375978d-7af4-482d-a534-1ca2ae2a4c53 | COMPLETED | 2 / 11661 |
| dbf74b05-4353-4fbf-a218-9bece8229a48 | 51e13ba0-4b33-422d-9848-51e3723e447c | COMPLETED | 2 / 11671 |

这些是只读后台会话测试，不是多个Worker写入安全验收，也不是07.10的Webview标签切换验收。07.2模块尚未对用户开放多Worker执行入口；要先完成07.3、07.4门禁再接入正式调度。暂停是终止当前请求后显式续聊，不是操作系统冻结进程，不承诺恢复被中断工具的内部执行点。中断产生的候选变更不会自动回滚。

07.2已备份：`3498572`，master → origin/master，Push成功。

## 07.3 隔离与文件锁模块（未完成真实Claude验收）

### 文件与调用关系

- `src/isolation/workerWorkspaceManager.ts`：从指定真实Git提交创建独立分支/Worktree；不checkout项目内容，而用原始blob物化，避免项目smudge过滤器和checkout Hook执行。禁止链接、子模块、Windows设备名、大小写冲突和超限目录。省略敏感/生成文件会明确返回omitted_files，后续执行器必须拒绝不完整的构建证据。
- `fileLockManager.ts`：跨进程mkdir事务锁、原子JSON记录、逻辑文件/目录重叠检测、Windows大小写归一。登记真实写入PID，进程存活时拒绝释放；进程异常退出后保留租约，确认退出才能显式回收。未知/损坏记录不覆盖，不按超时抢占活进程。所有申请/授予/拒绝/释放持久审计。
- `fileOwnershipManager.ts`、`scopedWorkerPolicy.ts`：在现有PermissionHost的执行前Hook中检查任务归属、读写范围、真实路径及文件锁。普通源码授权操作本地判断，不逐文件调用GPT；配置/清空仍需现有人工审批。禁止Bash、PowerShell、Notebook执行、网络和其他Agent工具，不能通过审批扩大文件范围。
- `workerExecutionSession.ts`：每次请求创建独立、带随机凭据的MCP入口，绑定当前Worker/任务/进程；只暴露精确授权测试和候选Diff。Diff只读授权路径并脱敏，不读取被省略的敏感文件。复用ClaudeService和PermissionHost，没有新增CLI适配器。
- `isolatedTestRunner.ts`：源码先形成带hash验证的独立快照，再通过WSL用户/挂载/网络/PID/IPC命名空间和chroot执行；系统运行库递归只读、源码只读、仅临时目录可写，不挂载主项目、其他Worker、共享Git或用户主目录。清空环境、去除全部能力和提权能力，限制时间/CPU/内存/进程/文件大小；不回退到Windows宿主执行。
- 现有ClaudeService只增加可选restricted、MCP配置和写入进程登记参数；默认旧Worker01行为不变。PermissionHost增加可选MCP工具的ask规则，以便Hook不可用时不自动放行。ToolBridge复用既有真实HTTP MCP服务。

调用链：独立Worktree → 文件所有权/原子写锁 → WorkerExecutionSession → 既有ClaudeService（官方restricted模式）→ 既有PreToolUse/PermissionRequest → 精确文件策略；测试经Worker专属MCP → IsolatedTestRunner → 实际OS隔离。该链路仍为后端模块，尚未对用户开放或接入正式多Worker调度。

### 已执行验证

- `npm run typecheck`、`npm run compile`通过；完整回归80/80通过，包括已有GPT/Claude/UI/审查/修复/调度测试。
- 新增`test/isolation.test.cjs`验证真实Worktree、独立分支、不执行smudge过滤器、原子文件冲突、目录重叠、Windows大小写、存活PID禁止释放、真实子进程异常退出后的租约回收、损坏记录fail-closed、执行前策略及不支持restricted时禁止降级。替身只用于故障测试，不冒充模型验收。
- `node test/isolation-os-live.cjs`通过，最终真实证据`.tools/task073-os-1794563485968/result.json`（早期通过记录亦保留）。两个真实候选Worktree均保留；主项目、其他Worker内容不变；脚本直接和间接写源码、通过`/proc/1/root`访问宿主、访问共享Git/宿主路径、建立网络连接、重新chroot均被阻止；`/tmp`合法临时写入正常。测试可读取任务授权的源码，未授权模块不进入执行快照；改变命令参数、未知发行版/执行器直接拒绝。
- 实际能力探针：`CapEff=0`、`CapBnd=0`、`NoNewPrivs=1`。没有仅凭Worktree就声称具有安全沙箱。
- 初次OS测试暴露WSL嵌套系统挂载必须递归bind及递归只读的问题，已修正并用真实OS探针重新验证；早期失败记录保留，不计为通过。

### 真实Claude阻塞与未验收项

- `node test/isolation-live.cjs`真实运行失败，证据`.tools/task073-isolation-1794562016479/records.json`；CLI为`2.1.322 (Claude Code)`，官方`--restricted`下明确返回`Not logged in · Please run /login`，分类`AUTHENTICATION_FAILED`。
- 随后同一原生Claude、同一机器、不使用工具的普通模式实际回复`OK`，证明不是CLI未安装或普通连接不可用。受限模式不加载用户/项目设置（[官方CLI说明](https://code.claude.com/docs/en/cli-reference#cli-flags)）；可能与现有连接依赖这些设置有关，但未读取凭据或擅自修改路由以推断原因。
- OS隔离探针已通过，但受限Claude真实Read/Edit、MCP测试、拒绝未授权文件、同Session续聊及离线Hook拦截尚未通过本阶段联合验收，不能用80项单元测试替代。原有普通Claude/GPT功能的回归通过也不等于新受限执行路径通过。
- 等待用户在本机配置受限模式可用的既有授权/启动环境后重跑；不要求把密钥发给插件，不修改CC Switch、全局权限或.gitignore，不用全局绕过参数。
- 当前只支持x86_64 WSL2/Linux命名空间与Python精确脚本。隔离源码只读；需要原地构建输出、Node/npm、依赖安装、删除/移动文件等能力尚未开放，不能声称通用构建全部可用。测试取消将等待隔离任务退出（Linux侧45秒硬上限）；不是即时冻结运行中的脚本。
- 07.3未标记完成；07.4–07.10尚未进入实施。多Worker正式写入入口继续关闭，现有Task06单Worker入口保持原行为，不应把它当作Task07隔离执行。

本阶段源码提交为安全门禁下的开发进度备份，不是Task07完成发布；实际Commit/Push结果见本轮交付信息。未打包或安装新的Task07 VSIX。

### 07.3后续验收：兼容路径通过

前述阻塞记录保留为真实历史；本节为当前结果。复测restricted仍提示未登录。官方safe-mode可连接但禁用显式MCP，因此未采用。随后验证官方bare模式：既有CLI连接可用、显式MCP真实调用成功。采用`--bare --tools "" --strict-mcp-config`并只允许插件提供的精确MCP工具；不加载用户/项目Hook、技能、其他Agent或其他MCP，也不开放Bash等任何内置工具。[官方bare说明](https://code.claude.com/docs/en/headless#start-faster-with-bare-mode)。没有读取、复制或保存API Key，也未修改CC Switch。

新增`boundedWorkerExecution.ts`作为WorkerExecutionSession正式入口，复用现有ScopedWorkerPolicy/FileLockManager和ToolBridge。允许的MCP调用仍必须经过服务端实际参数、范围、路径、写锁、活进程检查，不能用CLI允许有限MCP名称来绕过参数审批。普通操作本地判定，配置/清空走人工审批，未知/越权直接拒绝。写入采用当前内容校验和原子替换，停止时先关闭入口并等待本请求操作退出再释放锁。原native restricted实验入口保留，但正式路径不依赖它。

真实测试`node test/isolation-live.cjs`通过，证据`.tools/task073-isolation-1794570757825/result.json`及同目录records/permissions。真实Session为`1808881d-ba22-49bf-82ea-810cab113ae0`：Claude读取并实际修改motor_control.c，再通过MCP运行隔离Python测试，真实退出0并输出MOTOR_TEST_PASSED。下一轮实际请求写protocol.c，在服务端写入前被拒绝；protocol.c及主项目所有文件不变。随后确认CLI进程退出并释放文件锁。

这是MCP执行前网关验收，不冒充PreToolUse Hook验收（bare明确不运行Hooks）。原Hook路径的既有回归仍通过。完整80项回归通过，随后新增真实HTTP MCP读写/拒绝/关闭失效专项，隔离专项6/6通过。类型检查与编译通过。07.3完成；07.4用户编辑保护尚未验收，正式多Worker入口继续关闭。

07.3完成提交`cc36beb`，已推送origin/master；此前门禁下进度备份为`0829130`。

## 07.4 用户编辑保护

新增`src/protection/userEditProtectionManager.ts`、`vscodeEditorState.ts`：真实Git HEAD/status、目标磁盘hash、VS Code打开文档/未保存缓冲区版本与hash、持久手动锁共同构成交付指纹。不采集或保存用户未保存正文。目标正在打开/编辑、未提交、未跟踪、链接别名、手动锁或基准变化时拒绝交付；候选始终留在独立Worktree。一次性交付授权60秒有效，实际操作前重新采集，版本/内容/编辑器发生变化则拒绝调用集成函数。交付串行化，不复用旧授权，不擅自提交/清除用户内容。

新增`test/protection.test.cjs`验证真实Git脏状态、打开/未保存缓冲区、手动锁恢复、过期/复用/失效授权及不调用集成回调。`test/task074-host.cjs`、`run-protection-host.cjs`及宿主API构建入口验证真实VS Code文档：WorkspaceEdit产生真实未保存内容，管理器同时识别OPEN_IN_EDITOR和UNSAVED_BUFFER，拒绝授权；磁盘原文及未保存正文均保留。隔离宿主重启后重复验证通过，证据`.tools/task074-host-1794571219479/stage1.json`、`stage2.json`。初次宿主暴露Windows盘符大小写差异，已修正为同一真实路径的大小写兼容比较后重跑。

类型检查与保护专项通过。其他编辑器未保存内容无法可靠检测，授权必须由用户确认没有其他编辑器待保存目标；检测服务异常则保持停止，不把保存文件视为允许覆盖。此阶段提供安全交付门禁，不宣称已完成07.9集成操作或主项目自动覆盖。07.4完成，下一步接入依赖调度器。

07.4提交`d07cb35`，已推送origin/master。

## 07.5 依赖调度与真实隔离执行

新增`scheduling/taskDependencyScheduler.ts`统一管理任务状态、DAG就绪/等待、并发上限、同Worker槽位顺序执行、失败下游暂停、定向暂停/取消/恢复、持久重启等待及集成完成门禁。任务执行只产生REVIEWING候选；必须收到当前任务独立审查PASS且无严重问题/缺证据，才允许WAITING_INTEGRATION，并解除下游依赖。CLI说完成不改变验收状态。文件锁等待不会空转抢锁。

新增`teamTaskExecutor.ts`接合既有ClaudeWorkerManager、独立Worktree、原子锁和有限MCP网关，收集真实前后源码、按当前快照实际运行的测试、文件Diff及权限日志。依赖只携带自身有证据的代码变更；已准备候选不因恢复而覆盖，依赖指纹变化必须重新协调。顺序同槽位支持闲置Worker/Session复用；UI和后台执行生命周期独立。原ClaudeController仅增加可选权限执行对象的runOptions传递，不更换适配器。

`test/scheduler.test.cjs`验证DAG、并发上限、失败传播、独立审查门禁、缺证据不能完成、循环依赖及重启不自动执行；该测试的审查对象明确为合成门禁输入，不冒充GPT。Worker既有专项通过。类型检查通过。

真实`node test/scheduler-live.cjs`通过，证据`.tools/task075-scheduler-1794571580314/result.json`：3个Claude、3个独立Worktree/分支/Session，真实CLI区间1794571581947–1794571616640、1794571581949–1794571605722、1794571581988–1794571604947重叠。各自仅新增一个授权Python模块并实际自测通过，主项目Git状态干净。最终3项任务均为REVIEWING，明确gptReviewPerformed=false；这不是07.8的审查验收。安全门禁已完成，但正式GPT多Worker工具与新版UI还将在后续阶段接入，不提前声称端到端完成。

07.5提交`6561341`，已推送origin/master。

## 07.6 本地规则与GPT用量预算

扩展既有ScopedWorkerPolicy作为本地PolicyEngine：MCP普通读写、授权测试不接受GPT审批回调，只依赖精确文件、真实路径、锁与有限命令；每次实际本地允许后计数，配置/清空转人工，越权本地拒绝。07.5的真实3Worker开发过程没有调用GPT审批，每条工具日志保存在候选audit中；不能把它称为已完成独立审查。

新增`usage/tokenLedger.ts`：按项目/计划/任务和请求用途登记CLI请求，保留输入/输出/缓存/缓存写入/推理原始usage及未知字段。中断未知用量不按0补齐，后续请求暂停，需用户显式调整预算/确认未知用量。重启中的请求记为中断，不自动续发。缓存输入已经包含在input中，不再次相加；Provider内部实际HTTP请求次数无法由CLI确认，显示null，不把CLI一次请求冒充一次Provider API请求。预算为新请求前门禁，不能精确预知或追溯撤销已发生的Token；默认项目GPT预算500000可调，Claude不加项目Token上限。

ChatBridge仅增加可选finalize统计入口，原GPT对话行为不变；正式Team协调器将在后续把主规划及独立审查/修复调用统一记账。账本/原GPT专项21项在修复未知中断确认失效逻辑后通过专项；原GPT20项此前已通过。真实`node test/usage-live.cjs`通过，证据`.tools/task076-usage-1794572002063/result.json`：一次用量验收CLI请求，真实input17507、cached13696、output5、reasoning0、total17512；不是将cached再加到total。此调用明确标记ACCEPTANCE_TEST，不冒充实际代码审查。预算耗尽、未知中断、重复ID与恢复逻辑有独立测试。

## 07.7 自主测试修复与结果收集

TeamTaskExecutor 对当前快照核对实际测试数量，零测试或缺少测试数量不按通过处理。普通失败向同一 Worker/Session 反馈实际测试输出，最多三轮；代码无进展、取消、启动失败或越权时停止。各轮快照、测试、自主修复次数和审查修复次数分别保存，结果留在独立 Worktree。

87 项现有回归及新增重试边界测试通过。真实 `node test/autonomy-live.cjs` 通过，证据 `.tools/task077-autonomy-1794582539181/result.json`：预先运行既有测试确实失败，Claude 读取并修复 conversion.py，既有只读 unittest 实际通过；只修改授权实现文件，主夹具工作区未变化，GPT 调用为零。此阶段不冒充已完成多 Worker GPT 审查、安全集成和新版 UI。

## 07.8 多 Worker 独立审查与定向复审

新增 TeamWorkflow，将依赖调度器、真实 Worker 候选、ReviewEngine 和 TokenLedger 串联。每个实际 GPT 审查批次在请求前检查预算，结束后记录真实 usage；失败或缺失用量不会按零补齐。审查只接收当前候选的增量、必要上下文和实际测试记录。经验证的问题发回同一 Worker/Session，最多两次定向修复，其他已通过模块不重做；候选只进入 WAITING_INTEGRATION。

真实 `node test/team-review-live.cjs` 通过，证据 `.tools/task078-review-1794583409337/result.json`：两个真实 Claude 独立开发；夹具明确注入 volume.py 的乘数错误及真实失败测试，GPT 首次 REQUEST_CHANGES，原 Worker 修复后复审 PASS，另一个模块只审查一次。主工作区未变化，全部实际 GPT 请求用途和 usage 入账。首次运行的复审因整个快照 digest 与单文件 hash 的说明歧义被 BLOCKED，已补充明确的快照绑定元数据并完整重跑；失败记录保留，不计为通过。

## 07.9 独立集成与项目继承

IntegrationManager 在独立 Worktree 按依赖顺序合并真实增量，重复写入的基准不匹配时生成冲突记录，不覆盖任意一方。重新运行计划测试及可发现的已有 Python 测试；缺少已验证运行时、测试数量或独立集成审查时保持停止。Git 原始对象与精确补丁避免执行 checkout/clean 过滤器，实际生效的过滤器与 Git 属性变更需要人工处理。

交付前重新核对候选、审查、集成快照、补丁、主项目 HEAD、Git 状态和编辑器指纹。一次性授权后使用 Git 的 index/worktree 一致性检查应用补丁；不强推、不重置、不丢弃用户修改。可选 Git 同步只使用既有明确 GitHub 上游，用户暂存区非空时拒绝代为提交，失败保留本地成果。项目上下文保存已完成任务、接口、测试、审查和范围；下次规划必须结合当前真实代码重新验证，不自动运行历史任务。

真实 `node test/integration-live.cjs .tools/task078-review-1794583409337` 通过，证据同目录 `integration-result.json`：复用真实已审查候选，完成新的 GPT 集成审查；未确认外部编辑器状态及真实模拟未保存缓冲区时均拒绝交付，主夹具未变化；确认安全后只应用并暂存候选补丁，不提交或推送测试夹具。首次检查因无关的全局 Git LFS 配置而过度拒绝，已改为检查目标路径实际生效的 filter 后重验通过。尚待 07.10 正式入口及 UI 端到端验收。
