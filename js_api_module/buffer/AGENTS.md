# buffer（@ohos.buffer）模块指南

> **维护声明（MUST）**：使用 agent 修改代码时，如新增/删除文件、构建目标变更、公共 API 变更或约束变更，必须同步更新本文件。

## 1. 模块定位

buffer 提供与 Node.js 兼容的 Buffer/Blob API：多类型读写（Int8/Uint16/Float32 等）、不可变二进制对象 Blob、多编码支持（UTF-8、UTF-16LE、ASCII、Base64、Hex、Latin1、Binary）、二进制与字符串互转、内存池优化、带边界检查的直接内存操作、大小端支持与统一 BusinessError 错误处理。TS 层定义 API（`src/js_buffer.ts`），C++ 层经 NAPI 提供高性能路径。

## 2. 代码地图

| 路径 | 职责 | 风险 |
|------|------|------|
| `src/js_buffer.ts` | TS 公共 API 实现 | 🟡 公共 API 定义层，签名变更需兼容评估 |
| `js_buffer.h` / `js_buffer.cpp` | 原生 Buffer 实现 | 🔴 边界检查与 Node.js 语义兼容性所在 |
| `js_blob.h` / `js_blob.cpp` | 原生 Blob 实现 | 🔴 不可变语义 |
| `converter.h` / `converter.cpp` | 编码转换工具 | 🟡 |
| `native_module_buffer.cpp` | NAPI 模块入口 | 🟡 导出面须与 d.ts 一致 |
| `BUILD.gn` / `tsconfig.json` | 构建配置 | 🟢 |
| `test/` | 单元测试（`test_napi.cpp`、`test_ark.cpp`） | 🟢 |

## 3. 知识路由

| 任务/术语 | 先读 |
|-----------|------|
| FastBuffer（运行时内置高性能缓冲） | `js_api_module/fastbuffer/AGENTS.md` |
| 10200001/10200013 越界错误 | 根 `AGENTS.md` §3.3、§4.5 失败模式 |
| Base64/TextEncoder 编解码 | `js_util_module/util/AGENTS.md` |
| 专家经验（内存池/坑点） | `docs/knowledge/expert/ets-utils-expert.md` |

**外部组件依赖**：`napi:ace_napi`、`hilog:libhilog`、`icu:shared_icuuc`、`bounds_checking_function:libsec_shared`。共享配置经 `ets_utils_config.gni`；本模块自包含、跨模块依赖最少。

## 4. 约束与边界

### 4.1 Do not（禁止项）

1. 禁止在本模块实现网络 I/O 或文件系统 I/O
2. 禁止绕过边界检查（bounds checking）做内存操作
3. 禁止破坏与 Node.js Buffer API 的兼容契约
4. 禁止向 JS 暴露未经安全检查的裸内存指针
5. 禁止未理解性能影响就修改内存池行为
6. 禁止未经协调修改共享工具文件
7. 禁止不经构建守卫（平台宏/条件编译）引入平台特定代码

### 4.2 Ask before（需确认后再修改）

- 修改内存池/分配策略时，须先确认对高频分配场景的性能影响
- 新增编码支持时，须先确认与 Node.js Buffer 语义及 ICU 依赖一致性
- 修改错误码语义（401/10200001/10200013）时，须先确认公共 API 契约不变

### 4.3 架构不变量

1. TS 层定义 Public API；C++ 层经 NAPI 提供高性能路径
2. 所有内存访问必须经边界检查路径
3. 对外错误统一为 BusinessError，错误码语义固定

## 5. 构建与验证

构建命令从 OpenHarmony 源码根目录执行，不在本子目录执行。

### 5.1 构建

```sh
./build.sh --product-name rk3568 --build-target buffer
```

### 5.2 单元测试

```sh
./build.sh --product-name rk3568 --build-target test_buffer_unittest
hdc file send ./out/rk3568/tests/unittest/ets_utils/ets_utils/jsapi/buffer/napi/test_buffer_unittest /data/local/tmp/
hdc shell "cd /data/local/tmp && chmod 777 ./test_buffer_unittest && ./test_buffer_unittest"
```

### 5.3 Done 定义

1. ✅ 修改的文件无新增编译错误，`buffer` 构建成功
2. ✅ `test_buffer_unittest` 通过（或按 §5.4 声明未验证项）
3. ✅ 未引入公共 API 不兼容变更
4. ✅ 未绕过边界检查，未违反本文件禁止事项
5. ✅ 新增/修改错误路径有明确 BusinessError 返回与日志

### 5.4 禁止伪造验证（强制）

如果构建环境不可用，**不要声称完成**。列出应执行的命令、预期结果、未验证项与原因，并给出替代证据（CI 结果、人工 review checklist、`tsc --noEmit`/`-fsyntax-only` 语法检查输出）。**不得将未执行的验证描述为已通过；不得在无证据时声称完成。**
