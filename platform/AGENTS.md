# platform（跨平台适配层）模块指南

> **维护声明（MUST）**：使用 agent 修改代码时，如新增/删除文件、新增平台实现、约束变更，必须同步更新本文件。

## 1. 模块定位

platform 是跨平台适配层：以统一头文件声明平台能力接口（utils/util_helper/process_helper/qos_helper/jni_helper），由 `ohos`、`android`、`ios`、`default` 四个平台目录提供各自实现。平台选择经 `ets_utils_config.gni` 的 `platform_root` 变量分发，主逻辑一律调用根头文件声明的统一接口，不得直接调用平台 API。

## 2. 代码地图

| 路径 | 职责 | 风险 |
|------|------|------|
| `utils.h` | 平台通用工具声明 | 🟡 |
| `util_helper.h` | 编解码/工具类平台接口声明 | 🟡 |
| `process_helper.h` | 进程能力平台接口声明 | 🟡 |
| `qos_helper.h` | QoS 能力平台接口声明 | 🟡 |
| `jni_helper.h` | JNI 桥接接口声明（android） | 🟡 |
| `ohos/util_helper.cpp`、`ohos/qos_helper.cpp`、`ohos/process_helper.cpp` | OpenHarmony 平台实现（QoS 依赖 qos_manager） | 🟡 |
| `android/jni_helper.cpp` | Android 平台 JNI 实现 | 🟡 |
| `ios/process_helper.cpp` | iOS 平台进程实现 | 🟡 |
| `default/util_helper.cpp`、`default/qos_helper.cpp`、`default/process_helper.cpp`、`default/jni_helper.cpp` | 兜底默认实现（无平台特化时的回退） | 🔴 兜底行为缺失会破坏 host/未适配平台编译 |

## 3. 知识路由

| 任务/术语 | 先读 |
|-----------|------|
| 平台选择机制（platform_root） | `ets_utils_config.gni` + 根 `AGENTS.md` §4.2 Ask before |
| 消费方（process/timer/taskpool 等） | 对应模块 `AGENTS.md` |
| 新增平台适配 | 本文件 §4.2 |

**关键事实**：本层**无独立构建目标**，实现文件由各消费模块 BUILD.gn 按 `platform_root` 选择编入（如 taskpool 编入 `platform/ohos/qos_helper.cpp`）。

## 4. 约束与边界

### 4.1 Do not（禁止项）

1. 禁止在平台目录之外（业务模块内）直接调用平台 API——所有平台相关代码必须收敛到本目录
2. 禁止删除 `default/` 兜底实现——任何平台缺失特化时都依赖它编译
3. 禁止在头文件中引入平台特定 include（会传染所有平台编译）
4. 禁止修改统一接口签名而不更新全部平台实现
5. 禁止修改 `ets_utils_config.gni` 的平台选择逻辑而不验证全平台编译

### 4.2 Ask before（需确认后再修改）

- 新增平台能力接口时，须先确认四个平台目录均有实现（或明确走 default 兜底）
- 修改某平台实现时，须先确认该平台及 default 全部编译通过
- 新增目标平台时，须先确认 `ets_utils_config.gni` 分发逻辑同步更新

### 4.3 架构不变量

1. 统一接口在根头文件，实现在平台子目录，选择经 `platform_root`
2. `default/` 目录保证任意平台组合可编译
3. 平台实现不得反向依赖业务模块

## 5. 构建与验证

构建命令从 OpenHarmony 源码根目录执行，不在本子目录执行。

### 5.1 构建

本层**无独立构建目标**，经消费方目标验证。设备侧完整验证：

```sh
./build.sh --product-name rk3568 --build-target ets_utils
```

涉及平台宏的模块单独验证（示例）：taskpool（qos）、process（process_helper）、util（util_helper）。

### 5.2 测试

本层无专属单测；平台行为经消费方单测覆盖：`test_taskpool_unittest`、`test_process_unittest`、`test_util_unittest`（见对应模块 `AGENTS.md` §5.2）。

### 5.3 Done 定义

1. ✅ 全部件 `ets_utils` 构建成功，无平台宏相关告警
2. ✅ 修改平台的消费方单测通过（或按 §5.4 声明未验证项）
3. ✅ 统一接口与全部平台实现签名一致
4. ✅ 未违反本文件禁止事项

### 5.4 禁止伪造验证（强制）

如果构建环境不可用，**不要声称完成**。列出应执行的命令（含各平台预期结果）、未验证项与原因，并给出替代证据（CI 结果、人工 review checklist、`-fsyntax-only` 语法检查输出）。**不得将未执行的验证描述为已通过；不得在无证据时声称完成。**
