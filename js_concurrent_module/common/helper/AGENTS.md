# common/helper（并发共享辅助层）模块指南

> **维护声明（MUST）**：使用 agent 修改代码时，如新增/删除文件、构建目标变更、公共 API 变更或约束变更，必须同步更新本文件。

## 1. 模块定位

common/helper 是并发模块的共享辅助层：NAPI 辅助（参数获取/异常构造）、BusinessError 构造（ErrorHelper）、异步栈回溯（async_stack_helper）、HiTrace 封装（hitrace_helper）、路径处理（path_helper）、并发辅助（concurrent_helper/hybrid_concurrent_helper）、对象辅助（object_helper）。**本层被 taskpool、worker、utils、timer、console 五个模块共同编译引用，是全仓并发能力的公共底座。**

## 2. 代码地图

| 路径 | 职责 | 风险 |
|------|------|------|
| `napi_helper.h` / `napi_helper.cpp` | NAPI 参数/值辅助函数 | 🔴 全部并发模块共用，改错即全局故障 |
| `error_helper.h` | `ErrorHelper`：`NewError`/`ThrowError` 构造 BusinessError（errName 固定 `"BusinessError"`） | 🔴 并发侧错误码与错误格式契约 |
| `async_stack_helper.h` / `async_stack_helper.cpp` | 异步栈回溯辅助 | 🟡 |
| `hitrace_helper.h` | HiTrace 打点封装 | 🟡 |
| `concurrent_helper.h` / `concurrent_helper.cpp` | 并发通用辅助 | 🔴 |
| `hybrid_concurrent_helper.h` / `hybrid_concurrent_helper.cpp` | 混合并发辅助 | 🟡 |
| `object_helper.h` | 对象辅助 | 🟡 |
| `path_helper.h` | 路径处理 | 🟢 |

## 3. 知识路由

| 任务/术语 | 先读 |
|-----------|------|
| 修改对 taskpool/worker/utils 的影响 | 对应模块 `AGENTS.md`（§5.2 各自单测） |
| BusinessError 契约（另一套，base_sdk） | `base_sdk/AGENTS.md` |
| HILOG 宏（另一套，tools） | `tools/AGENTS.md` |
| 平台相关辅助（qos 等） | `platform/AGENTS.md` |

**消费方**（源码经各消费方 BUILD.gn 直接编入，无独立构建目标）：`js_concurrent_module/taskpool`、`js_concurrent_module/worker`、`js_concurrent_module/utils`、`js_sys_module/timer`、`js_sys_module/console`。

## 4. 约束与边界

### 4.1 Do not（禁止项）

1. 禁止未经协调修改本层任何文件——每次变更必须同步验证全部五个消费方
2. 禁止引入新的外部依赖（会传导给全部消费方）
3. 禁止改变 `ErrorHelper` 构造的错误格式（errName/errTitle/errMessage 拼接规则是既有契约）
4. 禁止在头文件中定义非 inline 的全局状态（多模块编入会重定义）
5. 禁止持有 `napi_ref`/env 资源而不随 env 生命周期管理
6. 禁止为通过测试删除或跳过既有用例

### 4.2 Ask before（需确认后再修改）

- 新增头文件前，须先确认命名不与 `tools/` 下同名工具冲突（ErrorHelper 在 `tools/ets_error.h` 与本层 `error_helper.h` 各有一套，职责不同）
- 修改 `napi_helper` 行为前，须先确认 taskpool/worker/utils 三方单测通过
- 修改 `async_stack_helper` 前，须先确认混合栈输出格式与既有消费工具兼容

### 4.3 架构不变量

1. 本层只放共享辅助，不放模块特定逻辑
2. 所有文件以头文件为主，编入消费方目标
3. 对外错误统一为 BusinessError

## 5. 构建与验证

构建命令从 OpenHarmony 源码根目录执行，不在本子目录执行。

### 5.1 构建

本层**无独立构建目标**，经消费方目标编译验证：

```sh
./build.sh --product-name rk3568 --build-target taskpool
./build.sh --product-name rk3568 --build-target worker
./build.sh --product-name rk3568 --build-target utils
./build.sh --product-name rk3568 --build-target timer
./build.sh --product-name rk3568 --build-target console
```

### 5.2 单元测试

测试目标 `test_concurrent_unittest` 定义于 `js_concurrent_module/test/BUILD.gn`：

```sh
./build.sh --product-name rk3568 --build-target test_concurrent_unittest
hdc file send ./out/rk3568/tests/unittest/ets_utils/ets_utils/js_concurrent_module/helper/test_concurrent_unittest /data/local/tmp/
hdc shell "cd /data/local/tmp && chmod 777 ./test_concurrent_unittest && ./test_concurrent_unittest"
```

**最小验证要求**：修改本层后，至少运行 `test_concurrent_unittest` + TaskPool/Worker/Utils 三方单测（timer/console 的单测按涉及面补跑）。

### 5.3 Done 定义

1. ✅ 全部消费方（taskpool/worker/utils/timer/console）编译通过
2. ✅ `test_concurrent_unittest` 及涉及模块的单测通过（或按 §5.4 声明未验证项）
3. ✅ 未引入新外部依赖、未改变错误格式契约
4. ✅ 未违反本文件禁止事项

### 5.4 禁止伪造验证（强制）

如果构建环境不可用，**不要声称完成**。列出应执行的命令、预期结果、未验证项与原因，并给出替代证据（CI 结果、人工 review checklist、`-fsyntax-only` 语法检查输出）。**不得将未执行的验证描述为已通过；不得在无证据时声称完成。**
