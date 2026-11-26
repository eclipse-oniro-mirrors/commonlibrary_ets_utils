# util（@ohos.util）模块指南

> **维护声明（MUST）**：使用 agent 修改代码时，如新增/删除文件、构建目标变更、公共 API 变更或约束变更，必须同步更新本文件。

## 1. 模块定位

util 是基础工具库：文本编解码（TextEncoder/TextDecoder，多编码）、Base64 编解码（Base64Helper）、字节流解码（StringDecoder）、UUID 生成（v4，加密安全随机）、类型检查（Types）、LRU 缓存（LRUCache）、有理数（RationalNumber）、范围判断（Scope）、AOP 切面（Aspect）、异步风格转换（promisify/callbackWrapper）。C++ 层提供性能路径，TS 层（`src/util_js.ts`）承载容器与泛型类实现。

## 2. 代码地图

| 路径 | 职责 | 风险 |
|------|------|------|
| `src/util_js.ts` | TS 公共 API：LRUCache、RationalNumber、Scope、Aspect、promisify 等 | 🟡 公共 API 定义层 |
| `js_base64.h` / `js_base64.cpp` | Base64 编解码 | 🔴 编码表与错误码契约 |
| `js_textencoder.h` / `js_textencoder.cpp` | 文本编码（UTF-8/UTF-16LE 等） | 🔴 编码边界与 ICU 行为 |
| `js_textdecoder.h` / `js_textdecoder.cpp` | 文本解码（含 fatal/BOM 语义） | 🔴 非法序列处理是公共契约 |
| `js_stringdecoder.h` / `js_stringdecoder.cpp` | 字节流按字符切分 | 🟡 |
| `js_uuid.h` / `js_uuid.cpp` | UUID v4 生成 | 🟡 随机源必须加密安全 |
| `js_types.h` / `js_types.cpp` | 类型检查（isXxx） | 🟢 |
| `native_module_util.h` / `native_module_util.cpp` | NAPI 模块注册入口 | 🟡 导出面须与 d.ts 一致 |
| `libutil.map` | 导出符号裁剪 | 🟡 |
| `BUILD.gn` / `tsconfig.json` | 构建配置 | 🟢 |
| `test/` | 单元测试 | 🟢 |

## 3. 知识路由

| 任务/术语 | 先读 |
|-----------|------|
| Buffer/Base64（二进制模块） | `js_api_module/buffer/AGENTS.md` |
| Stream（依赖 TextEncoder/StringDecoder） | `js_util_module/stream/AGENTS.md` |
| 容器（另一模块） | `js_util_module/container/AGENTS.md` |
| 专家经验（编码坑点） | `docs/knowledge/expert/ets-utils-expert.md` |

**外部组件依赖**：`napi:ace_napi`、`hilog:libhilog`、`icu:shared_icuuc`、`bounds_checking_function:libsec_shared`。共享配置经 `ets_utils_config.gni`。

## 4. 约束与边界

### 4.1 Do not（禁止项）

1. 禁止对既有 util 公共 API 做不兼容变更；确需变更必须走废弃流程
2. 禁止改变 TextEncoder/TextDecoder 编码语义（默认编码、fatal 行为、BOM 处理）
3. 禁止在 Base64 中偏离标准字母表/填充语义
4. 禁止使用非加密安全随机源生成 UUID
5. 禁止绕过 es2abc/TypeScript 编译流水线；禁止直接编辑 `.abc`/`.o` 构建产物
6. 禁止修改错误码语义与错误消息格式（401 参数错等公共契约）
7. 禁止绕过边界检查的内存操作；禁止向 JS 层暴露裸内存指针
8. 禁止为通过测试删除或跳过既有用例
9. 禁止新增功能时不更新本文件

### 4.2 Ask before（需确认后再修改）

- 新增 TextDecoder 编码支持时，须先确认 ICU 覆盖与既有应用解码结果不变
- 修改 LRUCache 淘汰策略前，须先确认容量边界与更新语义用例通过
- 修改 RationalNumber 精度或比较语义前，须先确认约分/溢出用例通过
- 修改 promisify/callbackWrapper 行为前，须先确认错误优先回调约定不变

### 4.3 架构不变量

1. C++ 层提供编解码性能路径；TS 层承载纯 JS 可实现的功能
2. 所有编解码经 ICU 与既有安全函数（libsec）
3. 对外错误统一为 BusinessError

## 5. 构建与验证

构建命令从 OpenHarmony 源码根目录执行，不在本子目录执行。

### 5.1 构建

```sh
./build.sh --product-name rk3568 --build-target util
# 聚合组（含 TS 编译流水线）
./build.sh --product-name rk3568 --build-target util_packages
```

### 5.2 单元测试

```sh
./build.sh --product-name rk3568 --build-target test_util_unittest
hdc file send ./out/rk3568/tests/unittest/ets_utils/ets_utils/jsutil/util/napi/test_util_unittest /data/local/tmp/
hdc shell "cd /data/local/tmp && chmod 777 ./test_util_unittest && ./test_util_unittest"
```

### 5.3 Done 定义

1. ✅ 修改的文件无新增编译错误，`util` 构建成功
2. ✅ `test_util_unittest` 通过（或按 §5.4 声明未验证项）
3. ✅ 未引入公共 API 不兼容变更（.ts 导出面与 d.ts 一致）
4. ✅ 未违反本文件禁止事项（尤其编码语义与随机源安全）
5. ✅ 新增/修改错误路径有明确 BusinessError 返回与日志

### 5.4 禁止伪造验证（强制）

如果构建环境不可用，**不要声称完成**。列出应执行的命令、预期结果、未验证项与原因，并给出替代证据（CI 结果、人工 review checklist、`tsc --noEmit`/`-fsyntax-only` 语法检查输出）。**不得将未执行的验证描述为已通过；不得在无证据时声称完成。**
