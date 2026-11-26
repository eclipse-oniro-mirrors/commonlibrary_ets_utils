# json（@ohos.util JSON 增强）模块指南

> **维护声明（MUST）**：使用 agent 修改代码时，如新增/删除文件、构建目标变更、公共 API 变更或约束变更，必须同步更新本文件。

## 1. 模块定位

json 模块提供增强的 JSON 能力：BigInt 支持（BigIntMode：DEFAULT / PARSE_AS_BIGINT / ALWAYS_PARSE_AS_BIGINT，`JSON.parseBigInt`/`JSON.stringifyBigInt`）、循环引用检测与报错（`circular Reference`）、参数校验，以及对象工具方法 `JSON.has`（键存在检查）与 `JSON.remove`（键删除，返回被删值）。**解析与序列化基于运行时内置实现，本仓 TS 层（`src/json_js.ts`）负责 API 包装与参数校验，禁止自造解析器。**

## 2. 代码地图

| 路径 | 职责 | 风险 |
|------|------|------|
| `src/json_js.ts` | TS 公共 API：parse/stringify/has/remove 包装与校验 | 🔴 BigIntMode 三模式与 `circular Reference` 错误消息是公共契约 |
| `native_module_json.cpp` | NAPI 模块注册入口 | 🟡 导出面须与 d.ts 一致 |
| `BUILD.gn` / `tsconfig.json` | 构建配置 | 🟢 |

## 3. 知识路由

| 任务/术语 | 先读 |
|-----------|------|
| BigIntMode 三模式语义 | 本文件 §1 + 根 `AGENTS.md` §3.3（BigIntMode） |
| 循环引用失败模式 | 根 `AGENTS.md` §4.5 |
| Sendable 序列化（ASON，另一模块） | `js_concurrent_module/utils/AGENTS.md`（勿混淆） |
| util 模块 | `js_util_module/util/AGENTS.md` |

**外部组件依赖**：`napi:ace_napi`、ArkTS 运行时（内置 JSON.parseBigInt/stringifyBigInt）。共享配置经 `ets_utils_config.gni`。

## 4. 约束与边界

### 4.1 Do not（禁止项）

1. 禁止实现异步的 JSON 解析/序列化——JSON 操作必须保持同步
2. 禁止自行实现 JSON 解析器——必须基于运行时内置的 `JSON.parseBigInt`/`JSON.stringifyBigInt`
3. 禁止对既有 JSON API 语义做不兼容变更（错误消息 `circular Reference` 等是既有契约）
4. 禁止让 has/remove 接受数组/非纯对象输入（必须报 401 参数错误）
5. 禁止绕过 es2abc/TypeScript 编译流水线；禁止直接编辑 `.abc`/`.o` 构建产物
6. 禁止修改错误码语义
7. 禁止为通过测试删除或跳过既有用例

### 4.2 Ask before（需确认后再修改）

- 调整 BigIntMode 枚举或其行为前，须先确认三模式用例与 d.ts 同步更新
- 修改 has/remove 返回语义前，须先确认既有应用依赖行为不变

### 4.3 架构不变量

1. parse/stringify 委托运行时内置实现，本仓只做包装与校验
2. 循环引用在 stringify 时必须检测并抛 BusinessError
3. 对外错误统一为 BusinessError

## 5. 构建与验证

构建命令从 OpenHarmony 源码根目录执行，不在本子目录执行。

### 5.1 构建

```sh
./build.sh --product-name rk3568 --build-target json
# 聚合组（含 TS 编译流水线）
./build.sh --product-name rk3568 --build-target json_packages
```

### 5.2 测试

**本模块无独立单测目标**——不存在 `test_json_unittest`。改动后以编译验证 + 运行时 JSON 行为集成测试覆盖（BigInt 三模式、循环引用、has/remove 全功能）。

### 5.3 Done 定义

1. ✅ 修改的文件无新增编译错误，`json` 构建成功
2. ✅ 未引入公共 API 不兼容变更
3. ✅ 未违反本文件禁止事项（尤其同步语义与运行时委托）
4. ✅ BigInt 三模式与循环引用行为不回归

### 5.4 禁止伪造验证（强制）

如果构建环境不可用，**不要声称完成**。列出应执行的命令、预期结果、未验证项与原因，并给出替代证据（CI 结果、人工 review checklist、`tsc --noEmit` 语法检查输出）。**不得将未执行的验证描述为已通过；不得在无证据时声称完成。**
