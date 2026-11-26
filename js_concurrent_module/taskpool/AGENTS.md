# taskpool（@ohos.taskpool）模块指南

> **维护声明（MUST）**：使用 agent 修改代码时，如新增/删除文件、构建目标变更、公共 API 变更或约束变更，必须同步更新本文件。

## 1. 模块定位

TaskPool 是 ArkTS 并发任务调度模块：基于线程池的并行执行环境，提供任务优先级（HIGH/MEDIUM/LOW/IDLE）、任务组（TaskGroup）、延时与周期任务、任务依赖管理、负载均衡与线程池动态伸缩、任务取消与超时、任务结果信息跟踪、顺序执行器（SequenceRunner）、异步执行器（AsyncRunner）、工作线程生命周期管理、任务时长统计与 HiSysEvent 事件上报。主语言 C++17，经 N-API 绑定暴露给 ArkTS。

## 2. 代码地图

| 路径 | 职责 | 风险 |
|------|------|------|
| `native_module_taskpool.cpp` | NAPI 模块注册入口 | 🟡 导出面须与 d.ts 一致 |
| `taskpool.h` / `taskpool.cpp` | 对外调度入口与 NAPI 方法绑定 | 🔴 调度语义核心 |
| `task.h` / `task.cpp` | 任务对象与结果信息（TaskInfo） | 🟡 |
| `task_group.h` / `task_group.cpp`、`task_group_manager.*` | 任务组提交与取消 | 🟡 |
| `task_manager.h` / `task_manager.cpp` | 任务生命周期与线程池管理、负载均衡 | 🔴 状态机绕过会破坏调度 |
| `task_queue.h` / `task_queue.cpp` | 全局任务队列（优先级、分组） | 🔴 饥饿/优先级反转风险所在 |
| `thread.h` / `thread.cpp`、`worker.h` / `worker.cpp`（本目录内部工作线程，非 @ohos.worker 模块） | 工作线程实现 | 🔴 线程生命周期与关闭时序 |
| `base_runner.*`、`task_runner.*` | Runner 基类与任务执行器 | 🟡 |
| `async_runner.*`、`async_runner_manager.*` | AsyncRunner 异步执行 | 🟡 |
| `sequence_runner.*`、`sequence_runner_manager.*` | SequenceRunner 顺序执行 | 🟡 |
| `log_manager.*` | 任务时长统计与日志 | 🟢 |
| `dfx_hisys_event.h` / `dfx_hisys_event.cpp` | HiSysEvent 上报（domain `ARKTS_TASKPOOL`） | 🟡 事件参数是下游硬性合约 |
| `libtaskpool.map` | 导出符号裁剪 | 🔴 新增导出符号须同步维护 |
| `test/` | 单元测试 | 🟢 |

## 3. 知识路由

| 任务/术语 | 先读 |
|-----------|------|
| 任务饥饿/优先级反转 | 本文件 §2（`task_queue`）+ `docs/knowledge/expert/ets-utils-expert.md` |
| Worker 线程通信（另一模块） | `js_concurrent_module/worker/AGENTS.md`（勿与本目录 `worker.cpp` 混淆） |
| 超时定时器（依赖 timer 模块） | `js_sys_module/timer/AGENTS.md` |
| 共享辅助层修改 | `js_concurrent_module/common/helper/AGENTS.md` |
| HiSysEvent 契约 | 根 `AGENTS.md` §4.2（`hisysevent.yaml`） |

**外部组件依赖**：`js_sys_module/timer:timer`（超时）、`ffrt:libffrt`（经 `ets_utils_config.gni` 的 `ENABLE_TASKPOOL_FFRT` 条件启用）、`qos_manager:qos`（ohos 平台 QoS）、`hilog:libhilog`、`hitrace:hitrace_meter`、HiSysEvent（`ENABLE_TASKPOOL_HISYSEVENT`）。源码含 `../common/helper/` 共享文件与 `platform/ohos/qos_helper.cpp`。

## 4. 约束与边界

### 4.1 Do not（禁止项）

1. 禁止对既有 taskpool 公共 API 做不兼容变更；确需变更必须走废弃流程
2. 禁止绕过任务状态机——任务提交/取消/超时必须经 TaskManager/TaskGroupManager 既有路径
3. 禁止直接操作线程池底层结构——伸缩与关闭必须走既有生命周期管理
4. 禁止修改 HiSysEvent 事件名与参数定义（`hisysevent.yaml` 中 domain `ARKTS_TASKPOOL`，如 `ARK_FFRT_UV` FAULT 事件）
5. 禁止持有 `napi_ref`/`napi_deferred` 而不注册环境清理钩子
6. 禁止修改错误码语义与错误消息格式
7. 禁止为通过测试删除或跳过既有用例
8. 禁止未经协调修改 `../common/helper/` 共享文件
9. 禁止绕过 es2abc/TypeScript 编译流水线；禁止直接编辑 `.abc`/`.o` 构建产物

### 4.2 Ask before（需确认后再修改）

- 修改线程池伸缩策略（worker 扩容/缩容阈值）前，须先确认长任务阻塞短任务、空闲收缩等场景用例覆盖
- 修改优先级调度语义前，须先确认优先级反转/饥饿防护用例通过
- 修改 SequenceRunner/AsyncRunner 行为前，须先确认对应序列语义用例通过
- 更换 ffrt 依赖启用方式前，须先确认 `ets_utils_config.gni` 与全平台编译

### 4.3 架构不变量

1. 任务提交后进入全局 TaskQueue，由 TaskManager 统一调度到工作线程
2. TaskGroup 取消语义为组内任务统一处理
3. 依赖 timer 模块实现任务超时
4. 对外错误统一为 BusinessError

## 5. 构建与验证

构建命令从 OpenHarmony 源码根目录执行，不在本子目录执行。

### 5.1 构建

```sh
./build.sh --product-name rk3568 --build-target taskpool
# 源码集（供其它目标静态链接）
./build.sh --product-name rk3568 --build-target taskpool_static
```

### 5.2 单元测试

```sh
./build.sh --product-name rk3568 --build-target test_taskpool_unittest
hdc file send ./out/rk3568/tests/unittest/ets_utils/ets_utils/js_concurrent_module/taskpool/test_taskpool_unittest /data/local/tmp/
hdc shell "cd /data/local/tmp && chmod 777 ./test_taskpool_unittest && ./test_taskpool_unittest"
```

同时修改 `common/helper/` 时须补跑 `test_concurrent_unittest`（见 `js_concurrent_module/common/helper/AGENTS.md`）。

### 5.3 Done 定义

1. ✅ 修改的文件无新增编译错误，`taskpool`（及 `taskpool_static`，如适用）构建成功
2. ✅ `test_taskpool_unittest` 通过（或按 §5.4 声明未验证项）
3. ✅ 未引入公共 API 不兼容变更
4. ✅ 未违反本文件禁止事项（尤其 HiSysEvent 契约与线程生命周期）
5. ✅ HiSysEvent 事件参数与 `hisysevent.yaml` 定义一致（如涉及）

### 5.4 禁止伪造验证（强制）

如果构建环境不可用，**不要声称完成**。列出应执行的命令、预期结果、未验证项与原因，并给出替代证据（CI 结果、人工 review checklist、`tsc --noEmit`/`-fsyntax-only` 语法检查输出）。**不得将未执行的验证描述为已通过；不得在无证据时声称完成。**
