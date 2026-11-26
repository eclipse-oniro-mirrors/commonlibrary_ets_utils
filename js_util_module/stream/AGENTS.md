# stream（@ohos.stream）模块指南

> **维护声明（MUST）**：使用 agent 修改代码时，如新增/删除文件、构建目标变更、公共 API 变更或约束变更，必须同步更新本文件。

## 1. 模块定位

stream 模块提供流式处理框架：四种核心流类型（Readable/Writable/Duplex/Transform）、背压管理（highWaterMark、doRead 返回值、drain 事件）、pipe 流串联、pause/resume 流控、事件驱动数据处理、经 TextEncoder/StringDecoder 的文本编解码集成，适用于大数据集与实时流场景。TS 层（`src/stream_js.ts`）承载实现，C++ 层只做 NAPI 注册与 JS/ABC 内嵌。

## 2. 代码地图

| 路径 | 职责 | 风险 |
|------|------|------|
| `src/stream_js.ts` | TS 公共 API：四类流、背压与事件实现 | 🔴 背压/事件语义错误会导致内存溢出 |
| `native_module_stream.cpp` | NAPI 模块注册与 JS/ABC 内嵌 | 🟡 |
| `stream_gen_obj.gni` | GN 对象生成模板 | 🟡 |
| `BUILD.gn` / `tsconfig.json` | 构建配置 | 🟢 |

## 3. 知识路由

| 任务/术语 | 先读 |
|-----------|------|
| 背压失败模式 | 根 `AGENTS.md` §4.5（doRead 返回值/drain 事件） |
| TextEncoder/StringDecoder 依赖 | `js_util_module/util/AGENTS.md` |
| Buffer（二进制基础） | `js_api_module/buffer/AGENTS.md` |

**外部组件依赖**：`napi:ace_napi`、`hilog:libhilog`、util 模块（TextEncoder/StringDecoder）。共享配置经 `ets_utils_config.gni`。

## 4. 约束与边界

### 4.1 Do not（禁止项）

1. 禁止在流实现中引入网络 I/O 或文件系统 I/O——流是数据结构抽象，不绑定具体源
2. 禁止绕过背压机制直接写入（必须尊重 highWaterMark 与 doRead 返回值）
3. 禁止对既有 stream 公共 API 做不兼容变更；确需变更必须走废弃流程
4. 禁止在事件回调中做阻塞操作
5. 禁止绕过 es2abc/TypeScript 编译流水线；禁止直接编辑 `.abc`/`.o` 构建产物
6. 禁止修改错误码语义
7. 禁止为通过测试删除或跳过既有用例
8. 禁止新增功能时不更新本文件

### 4.2 Ask before（需确认后再修改）

- 修改 highWaterMark 或背压判定逻辑前，须先确认大数据集场景内存行为
- 修改 pipe 错误传播语义前，须先确认流级联错误用例覆盖
- 修改 pause/resume 时序前，须先确认与 Node.js Stream 语义对齐

### 4.3 架构不变量

1. 流为纯数据结构抽象，I/O 源由使用方提供
2. 事件语义与 Node.js Stream 模型对齐
3. 对外错误统一为 BusinessError

## 5. 构建与验证

构建命令从 OpenHarmony 源码根目录执行，不在本子目录执行。

### 5.1 构建

```sh
./build.sh --product-name rk3568 --build-target stream
# 聚合组（含 TS 编译流水线）
./build.sh --product-name rk3568 --build-target stream_packages
```

### 5.2 测试

**本模块无独立单测目标**——不存在 `test_stream_unittest`。改动后以编译验证 + 应用侧流处理集成测试覆盖（四类流、背压、pipe、编解码）。

### 5.3 Done 定义

1. ✅ 修改的文件无新增编译错误，`stream` 构建成功
2. ✅ 未引入公共 API 不兼容变更
3. ✅ 未违反本文件禁止事项（尤其背压与 I/O 约束）
4. ✅ 四类流与 pipe/pause/resume 行为不回归

### 5.4 禁止伪造验证（强制）

如果构建环境不可用，**不要声称完成**。列出应执行的命令、预期结果、未验证项与原因，并给出替代证据（CI 结果、人工 review checklist、`tsc --noEmit` 语法检查输出）。**不得将未执行的验证描述为已通过；不得在无证据时声称完成。**
