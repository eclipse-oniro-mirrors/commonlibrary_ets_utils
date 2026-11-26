# console（@ohos.util.console 语义）模块指南

> **维护声明（MUST）**：使用 agent 修改代码时，如新增/删除文件、构建目标变更、公共 API 变更或约束变更，必须同步更新本文件。

## 1. 模块定位

console 模块实现标准 Web Console API：多级日志（debug/info/log/warn/error/fatal）、格式化占位符（%s/%d/%f/%j 等）、Unicode 边框表格渲染（console.table）、计时器（time/timeEnd/timeLog）与计数器（count/countReset）、分组缩进（group/groupEnd）、堆栈捕获（trace）、断言（assert）、JS + 原生混合栈支持。主语言 C++17，经 N-API 绑定暴露，输出走 HILOG（domain `0x3D00`，默认 tag `JSAPP`）。

## 2. 代码地图

| 路径 | 职责 | 风险 |
|------|------|------|
| `console.h` / `console.cpp` | Console API 类实现 | 🔴 **Inner Kit 头文件**（下游系统消费），签名不得做不兼容变更 |
| `log.h` | HILOG 封装（`LOG_DOMAIN 0x3D00`、`LOG_TAG "JSAPP"`） | 🔴 domain/tag 是硬性契约，修改即破坏日志归因 |
| `BUILD.gn` | 构建配置（编入 common/helper 源，host 走 `libhilog_linux` 条件依赖） | 🟡 条件依赖分支影响 host 构建 |
| `test/` | 单元测试 | 🟢 |

## 3. 知识路由

| 任务/术语 | 先读 |
|-----------|------|
| Timer 模块（计时器 API，另一模块） | `js_sys_module/timer/AGENTS.md`（勿与 console.time 计时混淆） |
| 共享工具头（另一套 HILOG 宏） | `tools/AGENTS.md`（`tools/log.h` 的 domain/tag 供 NAPI 层，非 JSAPP 日志） |
| 共享辅助层修改 | `js_concurrent_module/common/helper/AGENTS.md` |
| 专家经验 | `docs/knowledge/expert/ets-utils-expert.md` |

**外部组件依赖**：`napi:ace_napi`、`hilog:libhilog`（Linux host 用 `innerkits:libhilog_linux`）。源码含 `../common/helper/` 共享文件（相对 js_sys_module 的并发公共层）。

## 4. 约束与边界

### 4.1 Do not（禁止项）

1. 禁止修改 HILOG domain（`0x3D00`）与默认 tag（`"JSAPP"`）
2. 禁止在日志中输出敏感信息（密钥、个人数据）
3. 禁止对 `console.h`（Inner Kit）公共符号做不兼容变更
4. 禁止偏离 Web Console API 语义（占位符格式、表格渲染、分组行为）
5. 禁止引入网络 I/O 或文件系统 I/O
6. 禁止在无锁保护下并发操作共享格式化状态
7. 禁止为通过测试删除或跳过既有用例

### 4.2 Ask before（需确认后再修改）

- 修改混合栈（JS + 原生）构建逻辑前，须先确认栈帧格式与既有消费工具兼容
- 新增日志方法前，须先确认与标准 Console API 命名不冲突
- 修改 `log.h` 输出级别映射前，须先确认日志过滤行为不变

### 4.3 架构不变量

1. 全部输出经 HILOG，domain 固定 `0x3D00`、默认 tag `JSAPP`
2. 格式化语义与 Web Console API 对齐
3. 对外错误统一为 BusinessError（如适用）

## 5. 构建与验证

构建命令从 OpenHarmony 源码根目录执行，不在本子目录执行。

### 5.1 构建

```sh
./build.sh --product-name rk3568 --build-target console
```

### 5.2 单元测试

```sh
./build.sh --product-name rk3568 --build-target test_console_unittest
hdc file send ./out/rk3568/tests/unittest/ets_utils/ets_utils/jssys/console/napi/test_console_unittest /data/local/tmp/
hdc shell "cd /data/local/tmp && chmod 777 ./test_console_unittest && ./test_console_unittest"
```

### 5.3 Done 定义

1. ✅ 修改的文件无新增编译错误，`console` 构建成功
2. ✅ `test_console_unittest` 通过（或按 §5.4 声明未验证项）
3. ✅ 未引入公共 API / Inner Kit 不兼容变更
4. ✅ domain/tag 与日志格式未被破坏
5. ✅ 未违反本文件禁止事项

### 5.4 禁止伪造验证（强制）

如果构建环境不可用，**不要声称完成**。列出应执行的命令、预期结果、未验证项与原因，并给出替代证据（CI 结果、人工 review checklist、`tsc --noEmit`/`-fsyntax-only` 语法检查输出）。**不得将未执行的验证描述为已通过；不得在无证据时声称完成。**
