# container（@ohos.util.container 系列）模块指南

> **维护声明（MUST）**：使用 agent 修改代码时，如新增/删除容器类型、构建目标变更、公共 API 变更或约束变更，必须同步更新本文件（含 `BUILD.gn` 的 `container_names` 列表与 §2 代码地图）。

## 1. 模块定位

container 提供线性与非线性集合容器：ArrayList、LinkedList、Vector、Stack、Queue、Deque、List、HashMap、HashSet、TreeMap、TreeSet、PlainArray、LightweightMap、LightweightSet 及 Struct（字典基类），共 15 种类型。容器优先经 `ArkPrivate.Load()` 取运行时内置实现，缺失时回退本仓 TS 实现。主语言 TypeScript，NAPI 仅做模块注册与 JS/ABC 代码内嵌。

## 2. 代码地图

| 路径 | 职责 | 风险 |
|------|------|------|
| `BUILD.gn` | `container_lib` 模板 + `container_packages` 聚合组 + `build_ts_js` action | 🟡 新增容器必须同步 `container_names` |
| `tsconfig.json` / `build_ts_js.py` | TS 编译配置与脚本 | 🟢 |
| `arraylist/` … `vector/`（15 个子目录） | 每容器：`js_*.ts` 实现 + `native_module_*.cpp` 注册 | 🟡 公共 API 实现层 |
| `struct/js_struct.ts`、`native_module_struct.cpp` | 字典结构基类（HashMap/HashSet 继承其 DictionaryClass） | 🔴 被多容器依赖 |
| `lightweightmap/`、`lightweightset/` | 轻量容器 | 🟡 |

每个容器子目录结构一致：`js_<name>.ts`（容器类逻辑）+ `native_module_<name>.cpp`（NAPI 注册与 JS/ABC 内嵌）。

## 3. 知识路由

| 任务/术语 | 先读 |
|-----------|------|
| 新增一种集合容器 | 本文件 §4.4 + 根 `AGENTS.md` §5.3（构建配置变更验证） |
| ArkPrivate 内置实现/回退 | 根 `AGENTS.md` §3.3（ArkPrivate）、§4.4 不变量 #3 |
| Sendable 集合（并发安全，另一模块） | `js_util_module/collections/AGENTS.md`（勿混淆） |
| util 模块（errorUtil 等依赖） | `js_util_module/util/AGENTS.md` |

**外部组件依赖**：`napi:ace_napi`（NAPI 注册）、`hilog:libhilog`（经 struct 模块 errorUtil 记日志）。运行时依赖：ArkTS 运行时（`ArkPrivate.Load()` 可选快路径）、globalThis 回退、`util.struct` 字典基类。

## 4. 约束与边界

### 4.1 Do not（禁止项）

1. 禁止在本模块实现线程管理/同步原语——并发由 taskpool/worker 模块负责
2. 禁止引入网络、文件系统、数据库等 I/O 操作
3. 禁止创建 UI/渲染逻辑或直接调用系统级 API
4. 禁止依赖 collections/json/stream/util（errorUtil 经 struct 既定路径除外）等其它 util 模块
5. 禁止不做废弃流程就变更既有方法签名或删除公共 API
6. 禁止在集合实例外维护全局可变状态
7. 禁止实现自定义内存分配器/内存池——内存由运行时 GC 管理

### 4.2 Ask before（需确认后再修改）

- 新增容器类型时，须先确认：类型已存在于 `container_names` 命名约定、`ArkPrivate` 是否提供内置版本、错误检查模式与既有容器一致
- 修改 `struct/` 字典基类时，须先确认 HashMap/HashSet 等derived容器全部回归
- 修改 Proxy 拦截行为（如 HandlerArrayList 的 get/set/deleteProperty 陷阱）时，须先确认原型修改防护语义不变

### 4.3 架构不变量

1. 每个容器模块独立可编译、可部署（`container_lib` 模板生成）
2. 优先 `ArkPrivate.Load()`，不可用时回退 TS 实现，两者行为必须一致
3. TypeScript 泛型类型安全贯穿实现
4. JS 与 ABC 代码内嵌进原生模块分发
5. 错误检查统一走 errorUtil 模式（401 参数错）

### 4.4 新增容器步骤

1. 新建 `js_<name>.ts` + `native_module_<name>.cpp`（遵循既有容器模板）
2. 在 `BUILD.gn` 的 `container_names` 列表登记新容器名
3. 按既有容器实现错误检查（errorUtil）与 Proxy 模式（如适用）
4. 补充对应测试（容器创建、CRUD、边界、迭代器、比较排序）
5. 更新本文件 §2 代码地图

## 5. 构建与验证

构建命令从 OpenHarmony 源码根目录执行，不在本子目录执行。

### 5.1 构建

```sh
# 15 种容器聚合组（注意：不存在名为 container 的构建目标）
./build.sh --product-name rk3568 --build-target container_packages
# 单个容器（如 ArrayList；目标名与 container_names 一致）
./build.sh --product-name rk3568 --build-target arraylist
```

### 5.2 测试

**本模块无独立单测目标**——不存在 `test_container_unittest`。改动后以编译验证（`container_packages`）+ 集成模块测试覆盖；新增容器时按 §4.4 第 4 步补测试。

### 5.3 Done 定义

1. ✅ 修改的文件无新增编译错误，`container_packages` 构建成功
2. ✅ 新增/修改容器均在 `container_names` 与本文件登记
3. ✅ 未引入公共 API 不兼容变更
4. ✅ ArkPrivate 快路径与 TS 回退行为一致（如适用）
5. ✅ 未违反本文件禁止事项

### 5.4 禁止伪造验证（强制）

如果构建环境不可用，**不要声称完成**。列出应执行的命令、预期结果、未验证项与原因，并给出替代证据（CI 结果、人工 review checklist、`tsc --noEmit` 语法检查输出）。**不得将未执行的验证描述为已通过；不得在无证据时声称完成。**
