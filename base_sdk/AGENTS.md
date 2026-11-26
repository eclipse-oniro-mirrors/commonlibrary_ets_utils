# base_sdk（BusinessError 与 Kit 注册）模块指南

> **维护声明（MUST）**：使用 agent 修改代码时，如新增/删除文件、Kit 注册变更、约束变更，必须同步更新本文件。

## 1. 模块定位

base_sdk 承载两个基础 SDK 资产：`ets/@ohos.base.ets` 定义 **BusinessError**（OpenHarmony 统一业务错误类型，code + message）——全生态公共错误契约；`transfer/` 提供 Kit 注册与静态/动态分发（`@ohos.transfer.ets` 入口、`register.ets` 聚合、`kitRegister/` 下各 Kit 的注册回调映射，如 arkui、ipc、window、mediaLibraryKit 等）。产物为预置 ETS 资源（`ohos_prebuilt_etc`），非可执行代码。

## 2. 代码地图

| 路径 | 职责 | 风险 |
|------|------|------|
| `ets/@ohos.base.ets` | BusinessError 类型定义 | 🔴 全仓及全生态错误契约，字段/语义变更影响面极大 |
| `transfer/@ohos.transfer.ets` | transfer Kit 入口 | 🔴 Kit 注册链路起点 |
| `transfer/register.ets` | 注册聚合 | 🔴 漏注册即 Kit 不可用 |
| `transfer/kitRegister/*.ets`（arkui、ipc、window、mediaLibraryKit 等） | 各 Kit 注册回调 | 🟡 新增/删除须与 Kit 团队同步 |
| `BUILD.gn` | 定义 `base_sdk_ets`、`base_transfer_ets` 预置目标 | 🟢 |

## 3. 知识路由

| 任务/术语 | 先读 |
|-----------|------|
| BusinessError 使用约束 | 根 `AGENTS.md` §3.3（BusinessError）、§4.1 禁止事项 #4 |
| 错误码构造工具（运行时侧） | `tools/AGENTS.md`（`ets_error.h`）、`js_concurrent_module/common/helper/AGENTS.md`（`error_helper.h`） |
| 各模块错误码语义 | 对应模块 `AGENTS.md` |

## 4. 约束与边界

### 4.1 Do not（禁止项）

1. 禁止对 `@ohos.base.ets` 中 BusinessError 的字段与语义做不兼容变更
2. 禁止绕过 BusinessError 抛出裸 JS Error（面向公共 API 时）
3. 禁止未经协调修改 `kitRegister/` 下任何注册映射——每个文件对应一个 Kit 的注册链路
4. 禁止在 transfer 注册中引入循环依赖
5. 禁止新增功能时不更新本文件

### 4.2 Ask before（需确认后再修改）

- 新增 Kit 注册文件前，须先确认对应 Kit 团队提供注册回调与映射关系
- 修改 `register.ets` 聚合顺序前，须先确认注册链路无断裂（静态/动态分发两路）
- 修改 BusinessError 类型前，须先确认全生态兼容性评估（走 API 评审）

### 4.3 架构不变量

1. BusinessError 是全仓唯一统一业务错误类型
2. Kit 注册经 `@ohos.transfer.ets` → `register.ets` → `kitRegister/*.ets` 链路完成
3. 产物为预置 ETS 资源，随系统镜像分发

## 5. 构建与验证

构建命令从 OpenHarmony 源码根目录执行，不在本子目录执行。

### 5.1 构建

```sh
./build.sh --product-name rk3568 --build-target base_sdk_ets
./build.sh --product-name rk3568 --build-target base_transfer_ets
```

### 5.2 测试

本目录无独立单测目标。BusinessError 行为经各业务模块单测间接覆盖；Kit 注册经系统级 Kit 加载验证。

### 5.3 Done 定义

1. ✅ `base_sdk_ets`、`base_transfer_ets` 构建成功
2. ✅ 未引入 BusinessError 不兼容变更
3. ✅ Kit 注册链路完整（无漏注册、无断链）
4. ✅ 未违反本文件禁止事项

### 5.4 禁止伪造验证（强制）

如果构建环境不可用，**不要声称完成**。列出应执行的命令、预期结果、未验证项与原因，并给出替代证据（CI 结果、人工 review checklist）。**不得将未执行的验证描述为已通过；不得在无证据时声称完成。**
