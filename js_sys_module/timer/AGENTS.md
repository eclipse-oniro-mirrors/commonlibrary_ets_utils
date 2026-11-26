# timer（@ohos 事件循环定时器）模块指南

> **维护声明（MUST）**：使用 agent 修改代码时，如新增/删除文件、构建目标变更、公共 API 变更或约束变更，必须同步更新本文件。

## 1. 模块定位

timer 模块为运行时提供 setTimeout/setInterval/clearTimeout/clearInterval 能力：基于 libuv 的事件驱动定时、互斥锁保护的线程安全回调管理、按 env 隔离的定时器集合、内存安全的生命周期管理（env 销毁时清理）、可选 HiTrace 性能监控、Ace 容器 scope 支持。主语言 C++17，经 N-API 绑定暴露。**本模块不直接对应用暴露独立 d.ts**，其能力挂接运行时全局函数。

## 2. 代码地图

| 路径 | 职责 | 风险 |
|------|------|------|
| `sys_timer.h` / `sys_timer.cpp` | 定时器类实现 | 🔴 **Inner Kit 头文件**（下游系统消费），签名不得做不兼容变更 |
| `BUILD.gn` | 构建配置（被 taskpool/utils 依赖，Inner Kit 供下游链接） | 🟡 依赖方构建受此影响 |
| `test/` | 单元测试（`test.h` 测试基类 + 用例） | 🟢 |

## 3. 知识路由

| 任务/术语 | 先读 |
|-----------|------|
| console.time 计时（另一模块，勿混淆） | `js_sys_module/console/AGENTS.md` |
| 共享辅助层修改 | `js_concurrent_module/common/helper/AGENTS.md` |
| 依赖方（taskpool 超时 / utils 锁超时） | `js_concurrent_module/taskpool/AGENTS.md`、`js_concurrent_module/utils/AGENTS.md` |

**外部组件依赖**：`napi:ace_napi`、`libuv:uv`、`hilog:libhilog`、`hitrace:hitrace_meter`。源码含 `../common/helper/` 共享文件（相对并发公共层引用）。

## 4. 约束与边界

### 4.1 Do not（禁止项）

1. 禁止在定时回调中做阻塞操作——回调运行在事件循环线程
2. 禁止对 `sys_timer.h`（Inner Kit）公共符号做不兼容变更
3. 禁止绕过互斥锁直接访问定时器集合——必须走既有线程安全路径
4. 禁止在 env 销毁后触发回调（use-after-free）；清理钩子必须完整
5. 禁止修改 setTimeout/setInterval 既有语义（参数序、返回 id 语义）
6. 禁止为通过测试删除或跳过既有用例

### 4.2 Ask before（需确认后再修改）

- 修改回调入队策略前，须先确认 taskpool/utils 等依赖方超时行为不回归
- 修改 env 隔离机制前，须先确认多 env 场景（worker）用例覆盖
- 启用/修改 HiTrace 打点前，须先确认事件名与参数稳定性

### 4.3 架构不变量

1. 定时器经 libuv 异步调度，不阻塞事件循环
2. 每个 env 的定时器集合相互隔离
3. 回调管理必须线程安全

## 5. 构建与验证

构建命令从 OpenHarmony 源码根目录执行，不在本子目录执行。

### 5.1 构建

```sh
./build.sh --product-name rk3568 --build-target timer
```

### 5.2 单元测试

```sh
./build.sh --product-name rk3568 --build-target test_timer_unittest
hdc file send ./out/rk3568/tests/unittest/ets_utils/ets_utils/jssys/timer/napi/test_timer_unittest /data/local/tmp/
hdc shell "cd /data/local/tmp && chmod 777 ./test_timer_unittest && ./test_timer_unittest"
```

### 5.3 Done 定义

1. ✅ 修改的文件无新增编译错误，`timer` 构建成功
2. ✅ `test_timer_unittest` 通过（或按 §5.4 声明未验证项）
3. ✅ 未引入 Inner Kit 不兼容变更
4. ✅ 未违反本文件禁止事项（尤其线程安全与 env 清理）
5. ✅ 依赖方（taskpool/utils 超时路径）行为不回归

### 5.4 禁止伪造验证（强制）

如果构建环境不可用，**不要声称完成**。列出应执行的命令、预期结果、未验证项与原因，并给出替代证据（CI 结果、人工 review checklist、`-fsyntax-only` 语法检查输出）。**不得将未执行的验证描述为已通过；不得在无证据时声称完成。**
