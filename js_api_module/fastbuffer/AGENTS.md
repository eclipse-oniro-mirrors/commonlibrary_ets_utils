# fastbuffer（@ohos.buffer 内置快路径）模块指南

> **维护声明（MUST）**：使用 agent 修改代码时，如新增/删除文件、构建目标变更、公共 API 变更或约束变更，必须同步更新本文件。

## 1. 模块定位

fastbuffer 提供高性能二进制处理：各数据类型读写、多编码支持（UTF-8、UTF-16、ASCII、Base64、Hex、Latin1、Binary）、大小端转换、16/32/64 位字节序交换、带/不带初始化的内存分配、Buffer 拼接与比较、subarray/切片、统一 BusinessError 错误处理与内存池优化。TS 层（`src/js_fastbuffer.ts`）经 `ArkPrivate.Load()` 桥接运行时内置实现，C++ 仅 NAPI 胶水代码。

## 2. 代码地图

| 路径 | 职责 | 风险 |
|------|------|------|
| `src/js_fastbuffer.ts` | TS 公共 API 实现（经 ArkPrivate.Load 走运行时内置实现） | 🔴 内置实现的桥接层，错改破坏快路径 |
| `native_module_fastbuffer.cpp` | NAPI 模块入口与 JS/ABC 内嵌 | 🟡 |
| `BUILD.gn` / `tsconfig.json` | 构建配置 | 🟢 |

## 3. 知识路由

| 任务/术语 | 先读 |
|-----------|------|
| Buffer/Blob（完整模块） | `js_api_module/buffer/AGENTS.md` |
| ArkPrivate.Load 机制 | 根 `AGENTS.md` §3.3（ArkPrivate）、§4.4 不变量 #3 |
| 边界检查错误码 | 根 `AGENTS.md` §3.3（10200001/10200013） |

**外部组件依赖**：`napi:ace_napi`、`hilog:libhilog`、`icu:shared_icuuc`、`bounds_checking_function:libsec_shared`。共享配置经 `ets_utils_config.gni`。

## 4. 约束与边界

### 4.1 Do not（禁止项）

1. 禁止在本模块实现网络 I/O 或文件系统操作
2. 禁止绕过边界检查做内存操作
3. 禁止破坏与既有 FastBuffer API 的兼容契约
4. 禁止向 JS 暴露未经安全检查的裸内存指针
5. 禁止未理解性能影响就修改内存池行为
6. 禁止不经构建守卫引入平台特定代码
7. 禁止在桥接层复造运行时内置实现逻辑（本模块是桥接，不是实现）

### 4.2 Ask before（需确认后再修改）

- 变更 `ArkPrivate.Load()` 取值键或回退策略时，须先确认运行时内置实现同步存在
- 修改错误码语义时，须先确认与 buffer 模块公共契约一致

### 4.3 架构不变量

1. fastbuffer 经 `ArkPrivate.Load()` 优先取运行时内置实现，本仓 TS 为桥接/回退
2. 对外错误统一为 BusinessError
3. 与 buffer 模块的语义/错误码保持一致

## 5. 构建与验证

构建命令从 OpenHarmony 源码根目录执行，不在本子目录执行。

### 5.1 构建

```sh
./build.sh --product-name rk3568 --build-target fastbuffer
```

### 5.2 测试

**本模块无独立单测目标**——不存在 `test_fastbuffer_unittest`。改动后以编译验证 + buffer 模块集成测试覆盖（两者共享 Buffer 语义与错误码）。

### 5.3 Done 定义

1. ✅ 修改的文件无新增编译错误，`fastbuffer` 构建成功
2. ✅ 未引入公共 API 不兼容变更，与 buffer 模块语义一致
3. ✅ 未绕过边界检查，未违反本文件禁止事项
4. ✅ ArkPrivate 快路径与回退路径行为一致（如适用）

### 5.4 禁止伪造验证（强制）

如果构建环境不可用，**不要声称完成**。列出应执行的命令、预期结果、未验证项与原因，并给出替代证据（CI 结果、人工 review checklist、`tsc --noEmit` 语法检查输出）。**不得将未执行的验证描述为已通过；不得在无证据时声称完成。**
