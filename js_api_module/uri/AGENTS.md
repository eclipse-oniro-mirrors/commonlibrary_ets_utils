# uri（@ohos.uri）模块指南

> **维护声明（MUST）**：使用 agent 修改代码时，如新增/删除文件、构建目标变更、公共 API 变更或约束变更，必须同步更新本文件。

## 1. 模块定位

URI（Uniform Resource Identifier）模块实现 RFC 3986 标准的 URI 解析与操作：字符串解析与校验、组件提取（scheme/authority/path/query/fragment）、归一化与相等比较、查询参数管理（add/get/hasKey/hasValue/setAll/remove 等）、路径段操作、IPv4/IPv6 支持、层级与不透明 URI 处理。主语言为 C++，经 N-API 绑定暴露给 ArkTS，TS 包装层位于 `src/js_uri.ts`。

## 2. 代码地图

| 路径 | 职责 | 风险 |
|------|------|------|
| `src/js_uri.ts` | TS 公共 API 包装（Uri 类与公开接口面） | 🟡 公共 API 定义层，签名变更需兼容评估 |
| `js_uri.h` / `js_uri.cpp` | 核心 C++ 实现：RFC 3986 解析、字符校验规则、组件提取、归一化算法 | 🔴 RFC 3986 语义与错误码契约所在 |
| `native_module_uri.h` / `native_module_uri.cpp` | NAPI 模块注册与导出（UriConstructor、Normalize、Equals、IsAbsolute 等） | 🟡 导出面须与 d.ts 一致 |
| `liburi.map` | 动态库导出符号裁剪 | 🔴 新增导出符号须同步维护 |
| `BUILD.gn` / `tsconfig.json` | 构建配置 | 🟢 |
| `test/` | 单元测试（NAPI 绑定 + ArkTS 运行时集成） | 🟢 |

## 3. 知识路由

| 任务/术语 | 先读 |
|-----------|------|
| RFC 3986 解析行为变更 | 本文件 §4 + 根 `AGENTS.md` §4.5 失败模式 |
| URL（WHATWG）相关任务 | `js_api_module/url/AGENTS.md`（勿混淆两套标准） |
| NAPI 绑定层修改 | 根 `AGENTS.md` §3.2 路径路由 + `tools/AGENTS.md` |
| 专家经验（错误码/坑点） | `docs/knowledge/expert/ets-utils-expert.md` |

**外部组件依赖**：`napi:ace_napi`（NAPI 绑定）、`c_utils:utils`（非标准系统）、`hilog:libhilog`（日志）、`icu:shared_icuuc`（编码/国际化）、`bounds_checking_function:libsec_shared`（内存安全，测试）、`ets_runtime:libark_jsruntime`（测试）、`libuv:uv`（测试）。

**构建系统协作**（本仓外，禁止当作本仓文件）：`ets_utils_config.gni`（经 `//commonlibrary/ets_utils/ets_utils_config.gni` 引用）、TypeScript 编译脚本 `js_api_module/build_ts_js.py`、es2abc 字节码编译链（`build/config/components/ets_frontend/es2abc_config.gni`，构建系统提供）。

## 4. 约束与边界

### 4.1 Do not（禁止项）

1. 禁止对已有 Uri 公共 API 做不兼容变更（删方法、改参数类型、改返回值）；确需变更必须走废弃流程
2. 禁止实现违反 RFC 3986 的解析逻辑；禁止绕过或修改字符校验规则（`g_ruleAlpha`、`g_ruleDigit`、`g_ruleScheme` 等）
3. 禁止修改错误消息格式与错误码（越界 10200001 等，错误码语义是公共契约）
4. 禁止绕过 NAPI 类型检查与安全机制；禁止不经编码考虑的原始字符串操作
5. 禁止引入内存泄漏、缓冲区溢出、use-after-free；禁止向 JS 层暴露裸内存指针
6. 禁止吞掉或隐藏错误；禁止为通过测试删除或跳过既有用例
7. 禁止污染全局命名空间或修改既有全局对象原型
8. 禁止对非依赖清单内的模块（collections/json/stream）建立依赖
9. 禁止绕过 es2abc/TypeScript 编译流水线，禁止直接编辑 `.abc`/`.o` 构建产物
10. 禁止新增功能时不更新本文件（而非 CLAUDE.md——本仓嵌套指南一律为 AGENTS.md）

### 4.2 Ask before（需确认后再修改）

- 修改 URI 组件长度上限或校验规则时，须先确认与 RFC 3986 及既有应用的兼容性
- 修改归一化算法（scheme/路径/查询/host）时，须先确认不改变既有相等比较结果
- 修改 `native_module_uri.cpp` 导出列表或 `liburi.map` 时，须先确认 d.ts 与导出面一致

### 4.3 架构不变量

1. TS 层只定义公共 API；C++ 层通过 NAPI 提供高性能实现
2. 解析与校验逻辑只能在 `js_uri.cpp` 实现，TS 层不得复造解析规则
3. 所有对外错误必须为 BusinessError（401 参数错 / 10200001 越界）

## 5. 构建与验证

构建命令从 OpenHarmony 源码根目录执行，不在本子目录执行。

### 5.1 构建

```sh
./build.sh --product-name rk3568 --build-target uri
# 静态库
./build.sh --product-name rk3568 --build-target uri_static
# 聚合组（含 build_ts_js / uri）
./build.sh --product-name rk3568 --build-target uri_packages
```

### 5.2 单元测试

```sh
./build.sh --product-name rk3568 --build-target test_uri_unittest
hdc file send ./out/rk3568/tests/unittest/ets_utils/ets_utils/jsapi/uri/napi/test_uri_unittest /data/local/tmp/
hdc shell "cd /data/local/tmp && chmod 777 ./test_uri_unittest && ./test_uri_unittest"
```

测试内容：`test_napi.cpp`（构造器、方法绑定、参数校验、BusinessError 错误码）与 `test_ark.cpp`（解析、组件提取、查询管理、路径操作、边界条件、RFC 合规）。扩展组件时按解析覆盖、组件提取、查询操作、路径操作、归一化、编码解码、错误处理七大优先级补齐用例。

### 5.3 Done 定义

1. ✅ 修改的文件无新增编译错误，`uri` 构建成功
2. ✅ `test_uri_unittest` 通过（或按 §5.4 声明未验证项）
3. ✅ 未引入公共 API 不兼容变更（.ts 导出面与 d.ts 一致）
4. ✅ 未违反本文件禁止事项
5. ✅ 新增/修改的错误路径有明确 BusinessError 返回与日志

### 5.4 禁止伪造验证（强制）

如果构建环境不可用，**不要声称完成**。列出应执行的命令、预期结果、未验证项与原因，并给出替代证据（CI 结果、人工 review checklist、`tsc --noEmit`/`-fsyntax-only` 语法检查输出）。**不得将未执行的验证描述为已通过；不得在无证据时声称完成。**
