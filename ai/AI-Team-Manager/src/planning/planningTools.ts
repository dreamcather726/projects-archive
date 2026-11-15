const text = { type: 'string', minLength: 1, maxLength: 12000 };
const list = { type: 'array', items: text, maxItems: 100 };
const taskProperties = {
  task_id: text, task_name: text, priority: {type: 'integer', minimum: 1, maximum: 5}, parent_task_id: { type: ['string', 'null'] }, task_description: text, assigned_worker: text,
  allowed_files: list, read_only_files: list, forbidden_files: list, dependencies: list,
  interface_contracts: list, acceptance_criteria: list,
  test_commands: { ...list, description: 'Current verified argv catalog: node --test; npm test; npm run compile; python3 -B test_example.py or python -B test_example.py using the exact task script path. Python test scripts must call unittest.main. No extra flags, -m, shell expressions or unknown runtimes.' }, base_commit: text,
  status: { type: 'string', enum: ['PENDING'] },
};
export const planningTools = [
  { name: 'get_project_planning_context', description: 'Collect real current project manifest, baseline commit, dirty files and bounded architecture context. Planning is read-only and does not start a Worker.', inputSchema: { type: 'object', properties: {}, additionalProperties: false } },
  { name: 'submit_adaptive_plan', description: 'Validate and save a structured adaptive development report bound to fresh project evidence. This tool does not authorize or execute any development.', inputSchema: {
    type: 'object', properties: {
      orchestration_version: { type: 'integer', enum: [2] },
      analysis: { type: 'object', properties: {
        scale: {type:'string',enum:['SIMPLE','MEDIUM','LARGE','EXTRA_LARGE'],description:'SIMPLE: one function, small fix or UI change. MEDIUM: one complete module, protocol, API or component. LARGE: complete website, backend plus database, robot subsystem or device communication system. EXTRA_LARGE: complete multi-module software platform. Scale describes delivery scope; independently choose the minimum useful worker count from dependencies and configured limits.'},
        technical_domain:text, estimated_work:text, technical_complexity:{type:'string',enum:['LOW','MEDIUM','HIGH']},
        modules:list,expected_files:list,dependency_reasoning:text,parallelizable:{type:'boolean'},risks:list,human_confirmation:list,worker_rationale:text,
      }, required:['scale','technical_domain','estimated_work','technical_complexity','modules','expected_files','dependency_reasoning','parallelizable','risks','human_confirmation','worker_rationale'],additionalProperties:false },
      evidence_id: text, task_type: { type: 'string', enum: ['SMALL_CHANGE', 'MODULE_TASK', 'MULTI_MODULE', 'PROJECT_PHASE', 'FULL_PROJECT'] },
      goal: text, current_project_state: text, technical_approach: text, worker_count: { type: 'integer', minimum: 1, maximum: 20 }, parallel_rationale: { type: 'string', maxLength: 6000 },
      tasks: { type: 'array', minItems: 1, maxItems: 40, items: { type: 'object', properties: taskProperties, required: Object.keys(taskProperties), additionalProperties: false } },
      shared_file_owners: { type: 'array', maxItems: 100, items: { type: 'object', properties: { file: text, task_id: text }, required: ['file', 'task_id'], additionalProperties: false } },
      out_of_scope: list, completion_policy: { type: 'string', enum: ['STOP_AFTER_REQUEST', 'CONTINUE_APPROVED_PLAN'] },
    }, required: ['orchestration_version', 'analysis', 'evidence_id', 'task_type', 'goal', 'current_project_state', 'technical_approach', 'worker_count', 'parallel_rationale', 'tasks', 'shared_file_owners', 'out_of_scope', 'completion_policy'], additionalProperties: false,
  } },
];
export const PLANNING_INSTRUCTIONS = "开发需求先调用 get_project_planning_context，读取必要代码，再 submit_adaptive_plan。必须提交 orchestration_version=2 和真实结构化 analysis：规模 SIMPLE/MEDIUM/LARGE/EXTRA_LARGE、领域、工作量、复杂度、模块、预计文件、文件与接口依赖、并行性、风险、人工确认点和 Worker 数量理由。简单任务用一个 Worker；中型一般1–2，大型一般2–4，超大型按实际依赖动态分配；所有数量不得超过上下文 orchestration_limits.max_workers。不适合并行时用一个 Worker，禁止强行拆分。任务包含名称、1最高至5最低优先级、父任务、精确读写文件、依赖、接口、验收与实际测试命令。共享文件唯一所有者，依赖代码显式列 read_only_files；先通过审查的上游候选才能用于下游。经验证计划可通过 execute_adaptive_plan 在现有隔离网关真实执行；规划保存本身不执行、不授权。用户仅规划或讨论时禁止执行；不覆盖已有修改、不扩大需求。当前测试 argv 目录仅 node --test、npm test、npm run compile、python3 -B test_example.py 或 python -B test_example.py（实际脚本路径，包含 unittest.main）。不支持 -m、额外参数或未知工具链，需如实报告限制。普通聊天无需规划。开发执行后通过 get_team_task_output 使用上次 revision 和 until_decision=true 和 wait_ms=60000 等待关键事件，禁止无变化重复询问或重发任务。READY 仅表示已审查的集成候选；最终交付仍需原授权，完成当前需求后停止。";
