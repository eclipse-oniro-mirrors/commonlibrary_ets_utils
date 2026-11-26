# utils（@ohos.arakts.utils，锁/ASON/条件变量）模块指南

> **维护声明（MUST）**：使用 agent 修改代码时，如新增/删除文件、构建目标变更、公共 API 变更或约束变更，必须同步更新本文件。

## 1. 模块定位

utils 并发工具模块提供异步同步原语：具名/匿名异步锁（共享/独占模式）、带超时/取消/环境清理钩子的锁请求生命周期、基于等待图与三色 DFS 环检测的死锁探测、跨线程条件变量（wait/notify/waitFor）、ASON（ArkTS Sendable Object Notation）Sendable 对象序列化桥接、`isSendable()` 类型守卫、锁状态查询（`query`/`queryAll`）。主语言 C++17，经 N-API 绑定暴露。

## 2. 代码地图

| 路径 | 职责 | 风险 |
|------|------|------|
| `src/utils_js.ts` | TS 公共 API：Lock、ASON、ConditionVariable 包装 | 🟡 公共 API 定义层 |
| `native_utils_module.cpp` | NAPI 模块注册入口 | 🟡 |
| `utils.h` / `utils.cpp` | 模块初始化：注册 locks + ASON + ConditionVariable + isSendable | 🟡 |
| `locks/common.h` | tid_t 类型别名、INVALID_TID 常量 | 🟢 |
| `locks/async_lock.h/.cpp` | AsyncLock：锁状态机、pending/held 列表 | 🔴 状态机绕过会破坏锁语义 |
| `locks/async_lock_manager.h/.cpp` | 锁注册表、死锁检查、query API | 🔴 死锁检测核心 |
| `locks/lock_request.h/.cpp` | LockRequest：基于 Promise、超时定时器、env 清理 | 🔴 生命周期钩子遗漏即泄漏 |
| `locks/graph.h/.cpp` | 通用等待图 + DFS 环检测 | 🔴 |
| `locks/deadlock_helpers.h/.cpp` | 死锁告警消息生成 | 🟡 |
| `locks/weak_wrap.h` | 弱指针包装工具 | 🟢 |
| `condition/condition_manager.*` | NAPI 构造与方法绑定 | 🟡 |
| `condition/condition_variable.*` | 具名条件注册表、wait/notify、引用计数 | 🟡 |
| `condition/condition_task.*` | 带 timeout 与延迟 Promise 的等待任务 | 🟡 |
| `json/json_manager.*` | ASON：从运行时全局取 serialize/deserialize | 🟢 纯桥接 |
| `test/` | 单元测试（`test_ark.cpp`、`test_locks.cpp`、`test_condition.cpp`） | 🟢 |

## 3. 知识路由

| 任务/术语 | 先读 |
|-----------|------|
| 死锁检测/锁获取顺序 | 本文件 §4 + `docs/knowledge/expert/ets-utils-expert.md` |
| Sendable/ASON 相关 | 根 `AGENTS.md` §3.3 术语路由 |
| 共享辅助层修改 | `js_concurrent_module/common/helper/AGENTS.md` |
| 超时定时器 | `js_sys_module/timer/AGENTS.md`（依赖 timer 模块） |

**外部组件依赖**：`napi:ace_napi`、`hilog:libhilog`（非 Linux host）/`innerkits:libhilog_linux`（Linux host）、`hitrace:hitrace_meter`、`libuv:uv`（锁请求超时定时器）、`js_sys_module/timer:timer`。

## 4. 约束与边界

### 4.1 Do not（禁止项）

1. 禁止绕过锁状态机——加锁必须经 `AsyncLock::LockAsync()` / `ProcessPendingLockRequest()`
2. 禁止直接用裸 `std::mutex` 管理锁状态——必须使用 `asyncLockMutex_` 成员并遵循加锁顺序：`AsyncLockManager::lockMutex` → `AsyncLock::asyncLockMutex_`
3. 禁止在 `Graph::FindCycle()` 之外自造死锁检测——所有环检测必须用三色 DFS
4. 禁止持有 `napi_ref`/`napi_deferred` 而不注册环境清理钩子（`AddEnvCleanupHook`）
5. 禁止在本模块实现 ASON 序列化逻辑——本模块是纯桥接层，逻辑在运行时
6. 禁止破坏 `arkts.utils` 既有 API 契约
7. 禁止未经协调修改 `../common/helper/` 共享文件
8. 禁止移除 `EmptyExecuteCallback` 占位回调——env 销毁且 `napi_send_event` 不可用场景必需

### 4.2 Ask before（需确认后再修改）

- 新增 `LockMode` 枚举值前，须先确认 `CanAcquireLockUnsafe()` 更新方案与三类测试用例（acquire / pending-then-acquire / release-then-pending-wakes）
- 更换死锁检测算法前，须先确认既有死锁用例通过并补三线程循环等待用例
- 修改 `LockRequest` 超时行为前，须先确认 `test_locks.cpp` 超时用例同步更新
- 修改 `ConditionVariable::wait()` Promise 解析路径前，须先确认 `test_condition.cpp` wait-notify/wait-timeout 用例覆盖

### 4.3 架构不变量

1. 具名锁存于 `lockMap`（string → AsyncLock*），匿名锁存于 `anonymousLockMap`（自增 uint32_t id）；引用计数归零且无 pending 时删除
2. 死锁检测在每次锁请求无法立即满足时运行
3. 条件变量经静态 `condMap_` 跨线程共享，引用计数归零移除
4. 修改 `BUILD.gn` 的 `locks_sources` 时必须保证 `utils` 与 `utils_static` 双目标编译通过

### 4.4 修改规则

- 修改 `src/utils_js.ts` 后运行 `./build.sh --product-name rk3568 --build-target utils` 验证 `.ts → .js → .abc` 流水线
- 修改 `LockRequest` env 清理逻辑时，确认所有 early-return 路径均调用 `RemoveEnvCleanupHook`，避免残留钩子

## 5. 构建与验证

构建命令从 OpenHarmony 源码根目录执行，不在本子目录执行。

### 5.1 构建

```sh
./build.sh --product-name rk3568 --build-target utils
./build.sh --product-name rk3568 --build-target utils_static
```

### 5.2 单元测试

本模块只有一个单测目标 `test_utils_unittest`（源文件 `test_locks.cpp`、`test_condition.cpp`、`test_ark.cpp` 编入同一目标；**不存在** `test_locks_unittest`/`test_condition_unittest` 目标）：

```sh
./build.sh --product-name rk3568 --build-target test_utils_unittest
hdc file send ./out/rk3568/tests/unittest/ets_utils/ets_utils/js_concurrent_module/utils/test_utils_unittest /data/local/tmp/
hdc shell "cd /data/local/tmp && chmod 777 ./test_utils_unittest && ./test_utils_unittest"
```

### 5.3 Done 定义

1. ✅ 修改的文件无新增编译错误，`utils`/`utils_static` 构建成功
2. ✅ `test_utils_unittest` 通过（或按 §5.4 声明未验证项）
3. ✅ 未引入 `arkts.utils` API 不兼容变更
4. ✅ 未违反本文件禁止事项（尤其锁顺序与 env 清理钩子）
5. ✅ 新增锁模式/条件行为均有对应测试用例

### 5.4 禁止伪造验证（强制）

如果构建环境不可用，**不要声称完成**。列出应执行的命令、预期结果、未验证项与原因，并给出替代证据（CI 结果、人工 review checklist、`tsc --noEmit`/`-fsyntax-only` 语法检查输出）。**不得将未执行的验证描述为已通过；不得在无证据时声称完成。**
