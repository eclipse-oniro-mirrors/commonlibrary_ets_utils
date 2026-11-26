# worker（@ohos.worker）模块指南

> **维护声明（MUST）**：使用 agent 修改代码时，如新增/删除文件、构建目标变更、公共 API 变更或约束变更，必须同步更新本文件。

## 1. 模块定位

Worker 模块实现 Web Worker 风格的 ArkTS 多线程能力：宿主与 worker 线程双向消息传递（onmessage/onerror/onmessageerror/onallerrors）、线程优先级（HIGH/MEDIUM/LOW/IDLE/DEADLINE/VIP）、global call 同步调用、Sendable 对象高效传输、ParentPort 嵌套 worker 通信、Classic/Module 脚本模式、线程生命周期（STARTING/RUNNING/TERMINATING/TERMINATED）。主语言 C++17，经 N-API 绑定暴露给 ArkTS。

## 2. 代码地图

| 路径 | 职责 | 风险 |
|------|------|------|
| `native_module_worker.cpp` | NAPI 模块注册入口 | 🟡 导出面须与 d.ts 一致 |
| `worker.h` / `worker.cpp` | Worker 核心：线程生命周期、消息处理、优先级、global call | 🔴 **Inner Kit 头文件**（下游系统消费），签名不得做不兼容变更 |
| `worker_runner.h` / `worker_runner.cpp` | worker 线程入口与脚本执行 | 🔴 线程启动/退出时序 |
| `message_queue.h` / `message_queue.cpp` | 消息队列 | 🔴 跨线程数据边界 |
| `thread.h` / `thread.cpp` | 线程封装 | 🟡 |
| `libworker.map` | 导出符号裁剪 | 🔴 新增导出符号须同步维护 |
| `test/` | 单元测试 | 🟢 |

## 3. 知识路由

| 任务/术语 | 先读 |
|-----------|------|
| Sendable/结构化克隆数据边界 | 根 `AGENTS.md` §3.3（Sendable）、§4.5 失败模式 |
| TaskPool（任务调度，另一并发模块） | `js_concurrent_module/taskpool/AGENTS.md` |
| 共享辅助层修改 | `js_concurrent_module/common/helper/AGENTS.md` |
| Inner Kit 兼容性 | 根 `AGENTS.md` §4.2 Ask before |

**外部组件依赖**：`napi:ace_napi`、`hilog:libhilog`、`hitrace:hitrace_meter`、`libuv:uv`。源码含 `../common/helper/` 共享文件。

## 4. 约束与边界

### 4.1 Do not（禁止项）

1. 禁止跨 worker 线程直接共享可变状态——数据必须结构化克隆或使用 Sendable 对象
2. 禁止对 `worker.h`（Inner Kit）公共符号做不兼容变更
3. 禁止对既有 worker 公共 API 做不兼容变更；确需变更必须走废弃流程
4. 禁止绕过消息队列直接跨线程投递 NAPI 值
5. 禁止持有 `napi_ref`/`napi_deferred` 而不注册环境清理钩子；禁止跨 env 使用 NAPI handle
6. 禁止修改错误码语义与错误消息格式
7. 禁止在 terminate 后访问线程资源（use-after-free）
8. 禁止为通过测试删除或跳过既有用例
9. 禁止未经协调修改 `../common/helper/` 共享文件

### 4.2 Ask before（需确认后再修改）

- 修改消息序列化路径前，须先确认结构化克隆与 Sendable 传输用例覆盖
- 修改线程生命周期状态机前，须先确认 terminate/异常退出/嵌套 worker 用例通过
- 修改 `libworker.map` 或导出列表前，须先确认下游 Inner Kit 消费方不受影响

### 4.3 架构不变量

1. 宿主与 worker 线程只经消息端口通信
2. 错误事件（onerror/onmessageerror/onallerrors）语义与 Web Worker 对齐
3. 对外错误统一为 BusinessError

## 5. 构建与验证

构建命令从 OpenHarmony 源码根目录执行，不在本子目录执行。

### 5.1 构建

```sh
./build.sh --product-name rk3568 --build-target worker
# 源码集与聚合组
./build.sh --product-name rk3568 --build-target worker_static
./build.sh --product-name rk3568 --build-target worker_packages
```

### 5.2 单元测试

```sh
./build.sh --product-name rk3568 --build-target test_worker_unittest
hdc file send ./out/rk3568/tests/unittest/ets_utils/ets_utils/js_concurrent_module/worker/test_worker_unittest /data/local/tmp/
hdc shell "cd /data/local/tmp && chmod 777 ./test_worker_unittest && ./test_worker_unittest"
```

同时修改 `common/helper/` 时须补跑 `test_concurrent_unittest`（见 `js_concurrent_module/common/helper/AGENTS.md`）。

### 5.3 Done 定义

1. ✅ 修改的文件无新增编译错误，`worker` 构建成功
2. ✅ `test_worker_unittest` 通过（或按 §5.4 声明未验证项）
3. ✅ 未引入公共 API / Inner Kit 不兼容变更
4. ✅ 未违反本文件禁止事项（尤其跨线程数据边界与 env 清理）
5. ✅ 线程生命周期各状态均有测试覆盖（如涉及）

### 5.4 禁止伪造验证（强制）

如果构建环境不可用，**不要声称完成**。列出应执行的命令、预期结果、未验证项与原因，并给出替代证据（CI 结果、人工 review checklist、`tsc --noEmit`/`-fsyntax-only` 语法检查输出）。**不得将未执行的验证描述为已通过；不得在无证据时声称完成。**
