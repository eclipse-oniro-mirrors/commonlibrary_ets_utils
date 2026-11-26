# tools（全仓共享工具头）模块指南

> **维护声明（MUST）**：使用 agent 修改代码时，如新增/删除头文件、约束变更，必须同步更新本文件。

## 1. 模块定位

tools 是全仓共享的 header-only 工具集：BusinessError 构造（ErrorHelper）、HILOG 日志宏、API 版本判断辅助（ApiHelper）、安全故障上报。**四个头文件被仓库内多个模块直接 include，无独立构建目标。**

## 2. 代码地图

| 路径 | 职责 | 风险 |
|------|------|------|
| `ets_error.h` | `OHOS::Tools::ErrorHelper`：BusinessError 构造（errName 固定 `"BusinessError"`） | 🔴 全仓错误码格式契约 |
| `log.h` | HILOG 宏封装（`LOG_DOMAIN 0xD003F05`、`LOG_TAG "NAPI"`；非 HILOG 平台（如 Linux host）降级为 no-op） | 🔴 宏行为变更影响全仓日志输出 |
| `common_helper.h` | `OHOS::Tools::ApiHelper`：`apiVersion` 版本判断与平台适配 | 🟡 版本判断逻辑跨模块须一致 |
| `security_fault_reporter.h` | `ReportEtsUtilsSecurityFault` / `ReportEtsUtilsIncompatibleEvent` 安全故障与不兼容事件上报 | 🟡 DFX 上报链路契约 |

## 3. 知识路由

| 任务/术语 | 先读 |
|-----------|------|
| BusinessError 类型定义（base_sdk） | `base_sdk/AGENTS.md`（`ets/@ohos.base.ets`） |
| 并发侧 ErrorHelper（另一套） | `js_concurrent_module/common/helper/AGENTS.md`（`error_helper.h`，职责相近但服务并发模块） |
| Console 模块 JSAPP 日志（另一套 domain/tag） | `js_sys_module/console/AGENTS.md`（domain `0x3D00`，勿与 `tools/log.h` 的 `0xD003F05` 混淆） |
| DFX/HiSysEvent 链路 | 根 `AGENTS.md` §4.2 Ask before |

## 4. 约束与边界

### 4.1 Do not（禁止项）

1. 禁止改变 `ErrorHelper` 构造的错误格式（errName/错误码/错误消息拼接规则）
2. 禁止修改 `log.h` 的 `LOG_DOMAIN`/`LOG_TAG` 默认值
3. 禁止移除非 HILOG 平台的 no-op 降级路径（host 编译依赖它）
4. 禁止引入新的外部依赖（会传导给全部使用方）
5. 禁止在头文件中定义非 inline 的全局状态（多模块 include 会重定义）
6. 禁止修改安全故障上报的事件名与参数结构

### 4.2 Ask before（需确认后再修改）

- 修改 `ets_error.h` 前，须先确认全部使用模块的错误码格式兼容
- 修改 `common_helper.h` 版本判断逻辑前，须先确认跨模块版本行为一致
- 修改 `security_fault_reporter.h` 前，须先确认 DFX 下游消费（hiview 等）不受影响

### 4.3 架构不变量

1. 本目录只放 header-only 共享工具
2. 所有宏与 inline 函数不得引入链接期状态
3. 与 `js_concurrent_module/common/helper/` 中同名工具（ErrorHelper）保持各自职责边界：tools 服务通用模块，并发辅助层服务并发模块

## 5. 构建与验证

构建命令从 OpenHarmony 源码根目录执行，不在本子目录执行。

### 5.1 构建

本目录**无独立构建目标**，经使用方目标编译验证。全仓覆盖验证：

```sh
./build.sh --product-name rk3568 --build-target ets_utils
```

### 5.2 测试

本目录无专属单测；行为经使用方模块单测覆盖（如 `test_uri_unittest`、`test_buffer_unittest` 等，见对应模块 `AGENTS.md` §5.2）。

### 5.3 Done 定义

1. ✅ 全部件 `ets_utils` 构建成功（含 Linux host 单测组 `ets_utils_host_unittest`，如环境支持）
2. ✅ 未改变错误格式、日志常量与上报事件契约
3. ✅ 未引入外部依赖或链接期状态
4. ✅ 未违反本文件禁止事项

### 5.4 禁止伪造验证（强制）

如果构建环境不可用，**不要声称完成**。列出应执行的命令、预期结果、未验证项与原因，并给出替代证据（CI 结果、人工 review checklist、`-fsyntax-only` 语法检查输出）。**不得将未执行的验证描述为已通过；不得在无证据时声称完成。**
