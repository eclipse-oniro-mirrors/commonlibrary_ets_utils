# collections（@ohos.util.collections）模块指南

> **维护声明（MUST）**：使用 agent 修改代码时，如新增/删除文件、构建目标变更、公共 API 变更或约束变更，必须同步更新本文件。

## 1. 模块定位

collections 是 Sendable 集合的 NAPI 桥接层：注册并导出可在并发场景跨线程共享的集合构造器——SendableArray、SendableSet、SendableMap、SendableInt8Array、SendableUint8Array、SendableInt16Array、SendableUint16Array、SendableInt32Array、SendableUint32Array、SendableArrayBuffer、BitVector。**集合逻辑在 ArkTS 运行时内置实现，本模块只做构造器注册与导出，不含任何集合实现代码。**

## 2. 代码地图

| 路径 | 职责 | 风险 |
|------|------|------|
| `native_module_collections.cpp` | NAPI 模块注册：从运行时全局取 Sendable 构造器并导出 | 🔴 单文件承载模块；导出名与运行时内置名不一致即运行时崩溃 |
| `libcollections.map` | 导出符号裁剪 | 🟡 |
| `BUILD.gn` | 构建配置 | 🟢 |

## 3. 知识路由

| 任务/术语 | 先读 |
|-----------|------|
| Sendable 语义与使用约束 | 根 `AGENTS.md` §3.3（Sendable）、`js_concurrent_module/utils/AGENTS.md` |
| 普通集合容器（非并发，另一模块） | `js_util_module/container/AGENTS.md`（勿混淆） |
| Worker/TaskPool 中的共享数据 | `js_concurrent_module/worker/AGENTS.md`、`js_concurrent_module/taskpool/AGENTS.md` |

**外部组件依赖**：`napi:ace_napi`、`hilog:libhilog`、ArkTS 运行时（Sendable 构造器来源）。

## 4. 约束与边界

### 4.1 Do not（禁止项）

1. 禁止在本模块实现集合逻辑——它是纯 NAPI 桥接层，任何集合行为变更属于运行时
2. 禁止对既有导出名做不兼容变更（SendableArray 等 11 个构造器名是公共契约）
3. 禁止从非运行时渠道构造 Sendable 对象
4. 禁止在桥接层缓存运行时全局对象而不随 env 生命周期管理
5. 禁止为通过测试删除或跳过既有用例
6. 禁止新增功能时不更新本文件

### 4.2 Ask before（需确认后再修改）

- 新增导出（如新的 Sendable 类型）前，须先确认运行时内置构造器已存在且命名与 d.ts 一致
- 修改模块初始化时机前，须先确认 worker/taskpool 场景下跨 env 取值行为

### 4.3 架构不变量

1. 本模块只注册导出，不实现集合行为
2. Sendable 构造器一律经运行时全局获取
3. 对外错误统一为 BusinessError

## 5. 构建与验证

构建命令从 OpenHarmony 源码根目录执行，不在本子目录执行。

### 5.1 构建

```sh
./build.sh --product-name rk3568 --build-target collections
```

### 5.2 测试

**本模块无独立单测目标**——不存在 `test_collections_unittest`。改动后以编译验证 + 运行时 Sendable 行为集成测试覆盖（跨 worker 传递、并发读写场景）。

### 5.3 Done 定义

1. ✅ 修改的文件无新增编译错误，`collections` 构建成功
2. ✅ 导出名与运行时内置构造器、d.ts 声明一一对应
3. ✅ 未引入公共 API 不兼容变更
4. ✅ 未在本模块引入集合实现逻辑
5. ✅ 未违反本文件禁止事项

### 5.4 禁止伪造验证（强制）

如果构建环境不可用，**不要声称完成**。列出应执行的命令、预期结果、未验证项与原因，并给出替代证据（CI 结果、人工 review checklist、`-fsyntax-only` 语法检查输出）。**不得将未执行的验证描述为已通过；不得在无证据时声称完成。**
