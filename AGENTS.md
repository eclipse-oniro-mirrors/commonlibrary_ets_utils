# AGENTS.md — commonlibrary_ets_utils

> **维护声明（MUST）**：使用 agent 修改本仓代码时，如新增代码结构（目录/文件）、新增构建目标、新增或废弃公共 API、修改架构边界或约束，**必须同步更新本文件**（含嵌套 AGENTS.md 与 `docs/knowledge/`）。过期文档会直接误导后续 agent。

> OpenHarmony 公共基础库子系统 · 语言工具集部件（SystemCapability.Utils.Lang）

---

## 1. 项目定位

本仓库对应 OpenHarmony 源码树 `commonlibrary/ets_utils`，为整个应用生态提供标准 JavaScript/TypeScript 语言级 API，涵盖二进制处理、URI/URL、XML、并发、系统工具、集合容器、流框架等能力。

- **子系统**：commonlibrary
- **部件名**：ets_utils
- **SysCap**：SystemCapability.Utils.Lang
- **ROM**：1400KB | **RAM**：~4096KB

---

## 2. 代码地图

### 2.1 目录职责与风险

| 目录 | 职责 | 风险 |
|------|------|------|
| `base_sdk/` | BusinessError 定义 + Kit 注册与静态/动态分发 | 🔴 全仓公共错误契约与 Kit 注册链路，变更影响所有模块 |
| `js_concurrent_module/common/helper/` | 并发模块共享辅助层（NAPI/错误/栈回溯等 12 个头文件） | 🔴 TaskPool、Worker、Utils、Timer、Console 共同编译引用，改坏即全局故障 |
| `tools/` | 全仓共享工具头（`ets_error.h`、`log.h`、`common_helper.h`、`security_fault_reporter.h`） | 🔴 全仓引用，HILOG 宏与错误码格式是硬契约 |
| `js_api_module/` | 标准 API：buffer、fastbuffer、uri、url、convertxml、xml | 🟡 公共 API 实现层，签名变更必须走兼容性评估 |
| `js_concurrent_module/` | 并发：taskpool、worker、utils（locks/ASON/条件变量） | 🟡 并发调度与线程安全核心，影响全局稳定性 |
| `js_sys_module/` | 系统：console、process、timer、dfx（含 Inner Kit 头文件） | 🟡 `sys_timer.h`、`console.h`/`log.h` 为 Inner Kit，下游消费 |
| `js_util_module/` | 工具：util、container、collections、json、stream | 🟡 公共 API；模块间独立、无共享层耦合 |
| `platform/` | 跨平台适配（ohos/android/ios/default + 平台宏分发） | 🟡 改坏会导致部分平台编译失败 |
| `docs/knowledge/` | 深度知识库（code_map/expert/routing/verify） | 🟢 文档，随代码同步维护 |

### 2.2 任务到路径与指南映射（Where to look）

| 任务类型 | 先看（代码路径） | 先读（嵌套指南） |
|----------|------------------|------------------|
| Buffer / Blob / 二进制 | `js_api_module/buffer/` | `js_api_module/buffer/AGENTS.md` |
| 高性能二进制（ArkPrivate 内置） | `js_api_module/fastbuffer/` | `js_api_module/fastbuffer/AGENTS.md` |
| URI（RFC 3986） | `js_api_module/uri/` | `js_api_module/uri/AGENTS.md` |
| URL（WHATWG）/ URLSearchParams | `js_api_module/url/` | `js_api_module/url/AGENTS.md` |
| XML 解析/序列化 | `js_api_module/xml/` | `js_api_module/xml/AGENTS.md` |
| XML 转 JS 对象 | `js_api_module/convertxml/` | `js_api_module/convertxml/AGENTS.md` |
| TaskPool 并行 | `js_concurrent_module/taskpool/` | `js_concurrent_module/taskpool/AGENTS.md` |
| Worker 多线程 | `js_concurrent_module/worker/` | `js_concurrent_module/worker/AGENTS.md` |
| 并发工具（锁/ASON/条件变量） | `js_concurrent_module/utils/` | `js_concurrent_module/utils/AGENTS.md` |
| 并发共享辅助层 | `js_concurrent_module/common/helper/` | `js_concurrent_module/common/helper/AGENTS.md` |
| Console 日志 | `js_sys_module/console/` | `js_sys_module/console/AGENTS.md` |
| Process 进程操作 | `js_sys_module/process/` | `js_sys_module/process/AGENTS.md` |
| Timer 定时器 | `js_sys_module/timer/` | `js_sys_module/timer/AGENTS.md` |
| 诊断维护（堆快照/栈回溯） | `js_sys_module/dfx/` | `js_sys_module/dfx/AGENTS.md` |
| TextEncoder/Base64/UUID 等 | `js_util_module/util/` | `js_util_module/util/AGENTS.md` |
| 集合容器（15 种） | `js_util_module/container/` | `js_util_module/container/AGENTS.md` |
| Sendable 集合桥接 | `js_util_module/collections/` | `js_util_module/collections/AGENTS.md` |
| JSON 增强（BigInt/循环引用/has/remove） | `js_util_module/json/` | `js_util_module/json/AGENTS.md` |
| 流框架 | `js_util_module/stream/` | `js_util_module/stream/AGENTS.md` |
| BusinessError / Kit 注册 | `base_sdk/` | `base_sdk/AGENTS.md` |
| HILOG 宏 / 错误码构造 / 安全上报 | `tools/` | `tools/AGENTS.md` |
| 平台适配（平台宏/平台实现） | `platform/` | `platform/AGENTS.md` |

### 2.3 嵌套指南清单

| 子路径 | 适用范围 |
|--------|----------|
| `js_api_module/buffer/AGENTS.md` | Buffer/Blob 二进制处理 |
| `js_api_module/fastbuffer/AGENTS.md` | FastBuffer 高性能二进制 |
| `js_api_module/uri/AGENTS.md` | URI RFC 3986 解析 |
| `js_api_module/url/AGENTS.md` | URL/URLSearchParams |
| `js_api_module/xml/AGENTS.md` | XmlPullParser/XmlSerializer |
| `js_api_module/convertxml/AGENTS.md` | XML 转 JS 对象（libxml2） |
| `js_concurrent_module/taskpool/AGENTS.md` | TaskPool 任务调度 |
| `js_concurrent_module/worker/AGENTS.md` | Worker 线程与消息 |
| `js_concurrent_module/utils/AGENTS.md` | 异步锁/ASON/条件变量 |
| `js_concurrent_module/common/helper/AGENTS.md` | 并发共享辅助层 |
| `js_sys_module/console/AGENTS.md` | Console 日志 |
| `js_sys_module/timer/AGENTS.md` | setTimeout/setInterval |
| `js_sys_module/process/AGENTS.md` | 进程与子进程 |
| `js_sys_module/dfx/AGENTS.md` | 堆快照/运行时统计 |
| `js_util_module/util/AGENTS.md` | 编解码/Base64/UUID/LRUCache |
| `js_util_module/container/AGENTS.md` | 15 种集合容器 |
| `js_util_module/collections/AGENTS.md` | Sendable 集合 NAPI 桥接 |
| `js_util_module/json/AGENTS.md` | JSON 增强 |
| `js_util_module/stream/AGENTS.md` | Readable/Writable/Duplex/Transform |
| `base_sdk/AGENTS.md` | BusinessError 与 Kit 注册 |
| `tools/AGENTS.md` | 共享工具头 |
| `platform/AGENTS.md` | 平台适配 |

---

## 3. 知识路由

### 3.1 深度知识文档

| 场景 | 先读 |
|------|------|
| 完整目录/文件级 Code Map | `docs/knowledge/code_map/ets-utils-code-map.md` |
| 专家经验（坑点/失败模式/锁顺序） | `docs/knowledge/expert/ets-utils-expert.md` |
| 详细路由规则（场景/词汇/路径三维） | `docs/knowledge/routing/ets-utils-routing.md` |
| 编译与测试方法（按任务类型验证） | `docs/knowledge/verify/ets-utils-verify.md` |

### 3.2 路径路由表

| 修改路径 | 先读 |
|----------|------|
| `js_api_module/*/src/js_*.ts` | 对应模块 `AGENTS.md`（公共 API 定义层） |
| `js_api_module/*/native_module_*.cpp` | 对应模块 `AGENTS.md`（NAPI 绑定层） |
| `js_concurrent_module/common/helper/` | `js_concurrent_module/common/helper/AGENTS.md`（共享层，TaskPool/Worker/Utils/Timer/Console 五方消费需全部验证） |
| `js_concurrent_module/utils/locks/` | `js_concurrent_module/utils/AGENTS.md`（死锁检测与异步锁） |
| `js_concurrent_module/worker/worker.h` | `js_concurrent_module/worker/AGENTS.md`（Inner Kit） |
| `js_sys_module/timer/sys_timer.h` | `js_sys_module/timer/AGENTS.md`（Inner Kit） |
| `js_sys_module/console/console.h`、`console/log.h` | `js_sys_module/console/AGENTS.md`（Inner Kit + HILOG domain 0x3D00） |
| `js_sys_module/process/js_childprocess.cpp` | `js_sys_module/process/AGENTS.md`（子进程边界） |
| `platform/*/process_helper.cpp`、`platform/*/util_helper.cpp` | `platform/AGENTS.md` + `js_sys_module/process/AGENTS.md`、`js_util_module/util/AGENTS.md` |
| `base_sdk/ets/@ohos.base.ets` | `base_sdk/AGENTS.md`（BusinessError 契约） |
| `base_sdk/transfer/kitRegister/` | `base_sdk/AGENTS.md`（Kit 注册映射） |
| `js_util_module/json/src/json_js.ts` | `js_util_module/json/AGENTS.md` |
| `tools/ets_error.h` | `tools/AGENTS.md`（全仓错误码格式） |
| `tools/log.h` | `tools/AGENTS.md`（HILOG 宏约束） |
| `hisysevent.yaml` | 根 §4.2 Ask before（HiSysEvent 硬性合约） |
| `ets_utils_config.gni` | 根 §4.2 Ask before（全局构建变量与平台依赖） |

### 3.3 术语路由表

| 术语 | 含义与风险 | 先读 |
|------|-----------|------|
| TaskPool | 不是通用"线程池"，而是 ArkTS 并发任务调度 API（@ohos.taskpool），任务须为 @Concurrent 函数；改生命周期/调度影响所有并行业务 | `js_concurrent_module/taskpool/AGENTS.md` |
| Worker | 不是通用 Web Worker，而是 ArkTS Worker 线程通信实现（@ohos.worker）；跨线程数据必须结构化克隆或 Sendable | `js_concurrent_module/worker/AGENTS.md` |
| Sendable | 不是"可序列化对象"，而是可跨线程共享引用的 ArkTS 对象；误用于普通场景会崩溃 | `js_concurrent_module/utils/AGENTS.md` |
| ArkPrivate | 不是公开 API，而是运行时内部加载器（`ArkPrivate.Load()`）；仅 container/fastbuffer 等允许使用 | `js_util_module/container/AGENTS.md` |
| BusinessError | 不是 JS Error，而是 OpenHarmony 统一业务错误（code+message）；错误码是公共契约，禁止私自增改语义 | `base_sdk/AGENTS.md` |
| Buffer/Blob | 不是通用 ArrayBuffer，而是 Node.js 兼容二进制对象（@ohos.buffer）；边界检查错误码 10200001/10200013 | `js_api_module/buffer/AGENTS.md` |
| URI | 不是 URL，而是 RFC 3986 统一资源标识符（@ohos.uri）；解析规则是标准契约 | `js_api_module/uri/AGENTS.md` |
| URL | 不是 URI，而是 WHATWG URL（@ohos.url），含 URLSearchParams | `js_api_module/url/AGENTS.md` |
| Inner Kit | 不是普通头文件，而是系统内部消费的共享头（`sys_timer.h`、`console.h`/`log.h`、`worker.h`）；签名不兼容变更影响下游系统 | 对应模块 `AGENTS.md` |
| ASON | 不是 JSON，而是 Sendable 对象序列化（ArkTS Sendable Object Notation）；实现位于运行时，本仓仅桥接 | `js_concurrent_module/utils/AGENTS.md` |
| BigIntMode | 不是通用枚举，而是 JSON BigInt 解析模式（DEFAULT/PARSE_AS_BIGINT/ALWAYS_PARSE_AS_BIGINT） | `js_util_module/json/AGENTS.md` |
| libxml2 | 不是可选依赖，而是 XML 的事实解析引擎；禁止自行实现解析器 | `js_api_module/xml/AGENTS.md` |
| HiSysEvent domain | `hisysevent.yaml` 中 domain（ARKTS_TASKPOOL）是硬性合约 | 根 §4.2、`js_concurrent_module/taskpool/AGENTS.md` |
| 10200001/10200013/10200066 | Buffer 越界/长度非法、XML 编码错误的公共错误码，禁止变更语义 | `js_api_module/buffer/AGENTS.md`、`js_api_module/xml/AGENTS.md` |
| es2abc | 不是本仓文件，而是构建系统 `build/config/components/ets_frontend/` 提供的字节码编译链 | 根 §5.1 |
| ets_utils_host_unittest | 不是设备单测，而是 Linux host 上的单测聚合目标（免设备验证） | 根 §5.2 |

### 3.4 编辑前声明（强制）

改动代码前，必须在回复中声明：

1. **任务类别**（引用 §2.2 场景行或 §3.3 术语）
2. **已读取的知识文档**（嵌套 AGENTS.md / `docs/knowledge/` 文档路径）
3. **已识别的约束条件**（引用禁止事项编号或架构不变量，并简要说明约束内容，例如："禁止事项 #6——不可绕过 Buffer 边界检查"）
4. **验证计划**（将执行 §5.3 中对应变更类型的验证命令）

未完成以上声明，不得开始编辑代码。

---

## 4. 约束与边界

### 4.1 Do not（禁止项）

1. **禁止** 删除失败的测试用例来使用例"通过"
2. **禁止** 未明确用户请求时提交代码
3. **禁止** 对已有公共 API 做不兼容变更
4. **禁止** 绕过 BusinessError 错误码规范
5. **禁止** Buffer 模块中引入网络 I/O 或文件系统 I/O
6. **禁止** 绕过 Buffer 边界检查（bounds checking）
7. **禁止** 绕过 libxml2 使用自定义 XML 解析器
8. **禁止** 在 XmlSerializer 中使用非 UTF-8 编码
9. **禁止** 在 `js_util_module/collections/` 中实现集合逻辑——它是纯 NAPI 桥接层
10. **禁止** 在 JSON 模块中实现异步解析/序列化——JSON 操作必须保持同步
11. **禁止** 在 JSON 模块中自行实现 JSON 解析器——必须基于运行时内置的 `JSON.parseBigInt`/`JSON.stringifyBigInt`
12. **禁止** 跨 Worker 直接共享可变状态
13. **禁止** 修改 Console HILOG domain（0x3D00）和默认 tag（"JSAPP"）
14. **禁止** 在 Process 模块中未经权限检查调用 kill/exit
15. **禁止** 在 NAPI 模块中使用原始裸指针管理内存——必须使用智能指针或 NAPI 引用

### 4.2 Ask before（需确认后再修改）

以下高风险路径的修改需经用户确认后方可进行：

- **Inner Kit 头文件**（`sys_timer.h`、`console.h`/`log.h`、`worker.h`）签名变更 → 确认下游消费方已评估
- **base_sdk/transfer/** Kit 注册映射变更 → 确认 Kit 注册链路无断裂
- **js_concurrent_module/common/helper/** → 确认 TaskPool/Worker/Utils/Timer/Console 五方均已验证
- **platform/** 平台适配宏变更 → 确认所有目标平台编译通过
- **构建产物**（`.abc`、gen_obj `.o`/`.c`）→ 修改源码后重新生成，禁止直接编辑产物
- **第三方依赖版本升级**（libxml2、openssl、ICU）→ 确认许可证兼容性及全量回归测试
- **`hisysevent.yaml`** HiSysEvent 事件定义变更 → 确认下游消费方不受影响（典型消费方：故障管理 hiview、事件统计 hisyseventservice、DFX 上报链路）
- **`ets_utils_config.gni`** 全局构建变量与平台依赖（`platform_root`、`hilog_windows/linux/mac` 等）→ 确认所有目标平台编译通过

### 4.3 Agent 常见错误模式

| 模式 | 风险 | 正确做法 |
|------|------|---------|
| ❌ 添加 BUILD.gn 未声明的依赖 | 编译通过但模块耦合度增加 | 新增依赖须同步更新 BUILD.gn deps |
| ❌ 在 TS 层调用 NAPI 内部函数 | 分层违规 | TS 层只通过 Public API 调用，NAPI 绑定在 C++ 层 |
| ❌ 直接编辑 .abc/.o 构建产物 | 源码与产物不一致 | 始终修改 .ts/.cpp 源码并重新构建 |
| ❌ 幻觉不存在的 NAPI 辅助函数 | 编译失败或运行时崩溃 | 调用 NAPI 函数前确认其存在于头文件中 |
| ❌ 为不存在的测试目标编写命令 | 验证无法执行 | 单测目标以 §5.2 表为准，勿构造目标 |

### 4.4 架构不变量

1. 每个子模块是独立可编译的构建单元
2. TS 层定义 Public API，C++ 层通过 NAPI 实现高性能路径
3. container/fastbuffer 优先使用 `ArkPrivate.Load()` 获取运行时内置实现
4. Inner Kit 头文件（`sys_timer.h`、`console.h`/`log.h`、`worker.h`）签名不得做不兼容变更
5. 平台相关代码统一在 `platform/` 目录，主逻辑不得直接调用平台 API

### 4.5 常见失败模式

| 模式 | 典型表现 | 正确做法 |
|------|---------|---------|
| 跨线程共享非 Sendable 对象 | 运行时崩溃 | 使用 Sendable 对象或结构化克隆 |
| Buffer 越界读写 | 10200001/10200013 错误 | 读写前检查 offset + length |
| XML 非UTF-8 编码 | 10200066 错误 | 统一使用 UTF-8 |
| ConvertXML 大文档 OOM | 内存溢出 | 使用 `largeConvertToJSObject()` |
| Worker 线程泄漏 | 线程不释放 | 确保 terminate() 被调用 |
| 异步锁死锁 | 永久等待 | 遵循 DAG 锁获取顺序 |
| Stream 背压未处理 | 内存溢出 | 检查 doRead 返回值，尊重 drain 事件 |
| JSON 循环引用序列化 | BusinessError "circular Reference" | 序列化前检测循环引用，或使用 reviver 过滤 |
| JSON BigInt 模式误用 | 大数精度丢失 | 需要大整数时使用 `PARSE_AS_BIGINT` 或 `ALWAYS_PARSE_AS_BIGINT` |
| JSON has/remove 传入数组 | BusinessError 401 | has/remove 只接受纯对象，不接受数组 |

详细专家经验：`docs/knowledge/expert/ets-utils-expert.md`

---

## 5. 构建和验证

**所有构建命令从 OpenHarmony 源码根目录执行，不在本子目录执行。** 产物目标名以本节表格为准，勿构造表中不存在的目标。

### 5.1 构建

```sh
# 全部件构建
./build.sh --product-name rk3568 --build-target ets_utils --ccache
# 单模块构建（示例：buffer；其余模块目标见 verify 文档目标表）
./build.sh --product-name rk3568 --build-target buffer
```

### 5.2 单元测试

仅以下模块有独立单测目标（勿为其它模块构造不存在的目标）：

| 模块 | 测试目标 | 产物路径（相对 `out/rk3568/tests/unittest/ets_utils/`） |
|------|---------|------|
| Buffer | `test_buffer_unittest` | `ets_utils/jsapi/buffer/napi` |
| ConvertXML | `test_convertxml_unittest` | `ets_utils/jsapi/convertxml/napi` |
| URI | `test_uri_unittest` | `ets_utils/jsapi/uri/napi` |
| URL | `test_url_unittest` | `ets_utils/jsapi/url/napi` |
| XML | `test_xml_unittest` | `ets_utils/jsapi/xml/napi` |
| TaskPool | `test_taskpool_unittest` | `ets_utils/js_concurrent_module/taskpool` |
| 并发公共（common/helper） | `test_concurrent_unittest` | `ets_utils/js_concurrent_module/helper` |
| 并发工具（locks/condition） | `test_utils_unittest` | `ets_utils/js_concurrent_module/utils` |
| Worker | `test_worker_unittest` | `ets_utils/js_concurrent_module/worker` |
| Console | `test_console_unittest` | `ets_utils/jssys/console/napi` |
| Process | `test_process_unittest`（定义于 `js_sys_module/test/`） | `ets_utils/js_sys_module/process` |
| Timer | `test_timer_unittest` | `ets_utils/jssys/timer/napi` |
| Util | `test_util_unittest` | `ets_utils/jsutil/util/napi` |

```sh
# 设备单测（示例：URI；目标与产物路径按上表替换）
./build.sh --product-name rk3568 --build-target test_uri_unittest
hdc file send ./out/rk3568/tests/unittest/ets_utils/ets_utils/jsapi/uri/napi/test_uri_unittest /data/local/tmp/
hdc shell "cd /data/local/tmp && chmod 777 test_uri_unittest && ./test_uri_unittest"
```

**无独立单测目标**：dfx、fastbuffer、json、stream、collections、container——改动后以编译验证 + 相关集成模块测试覆盖。

**Host 单测（Linux 无需设备）**：`./build.sh --product-name rk3568 --build-target ets_utils_host_unittest`（聚合上表全部单测，产物在 `out/rk3568/clang_x64/tests/unittest/ets_utils/`）。

### 5.3 按变更类型验证表

| 变更类型 | 最小验证 |
|---------|---------|
| 修改模块内部实现 | 编译该模块 + 对应单测（如有） |
| 新增/修改公共 API（.ts 导出） | 编译 + 单测 + §5.6 API 兼容性检查 |
| 修改 Inner Kit 头文件 | 编译全部依赖模块（timer/console/worker） |
| 修改 `common/helper/`、`tools/` 共享层 | 编译 + TaskPool/Worker/Utils/Timer/Console 单测 |
| 修改 `platform/` | 编译全部件 `ets_utils`，确认平台宏分支无告警 |
| 修改构建配置（BUILD.gn/gni） | 编译该模块 + `--build-target ets_utils` |
| 修改 DFX/HiSysEvent | 编译 + `hisysevent.yaml` 与代码参数一致性检查 |
| 仅测试变更 | 运行变更测试 + 相邻相关测试 |

不同任务类型的额外验证步骤：`docs/knowledge/verify/ets-utils-verify.md` 的"按任务类型的额外验证"表。

### 5.4 Done 定义

任务完成必须满足：

1. ✅ 所有修改的文件无新增编译错误
2. ✅ 相关模块构建成功
3. ✅ 相关测试用例通过
4. ✅ 未引入公共 API 不兼容变更
5. ✅ 未违反任何禁止事项
6. ✅ NAPI 原生代码的内存管理正确

### 5.5 禁止伪造验证（强制）

如果构建环境不可用，**不要声称完成**。列出应执行的命令、预期结果、未验证项与原因，并给出替代证据（如 CI 结果链接、人工 review checklist）。**不得将未执行的验证描述为已通过；不得在无证据时声称完成。**

### 5.6 API 兼容性检查

- 检查修改的 .ts 文件中 export 的函数/类/接口签名是否变更
- 如有签名变更，确认已走 API 废弃流程
- 检查方法参数数量、类型、返回值是否与已有 API 一致
- SDK API 检查：`./build.sh --product-name rk3568 --build-target sdk_test`
- NAPI 接口一致性：对比 `@ohos.*.d.ts` 声明与 NAPI 注册函数列表

### 5.7 最终回复格式

任务完成时，回复必须包含：

1. **变更摘要**（修改文件列表含完整路径 + 风险等级）
2. **验证结果**（执行的构建/测试命令及结果、Done 定义逐项确认 ✅/❌）
3. **兼容性影响**（公共 API/Inner Kit/HiSysEvent 等评估，如相关）
4. **未验证项**（因环境限制未运行的验证 + 残留风险）
5. **遗留项**（后续待办或风险）

### 5.8 静态分析

- C++ 代码默认通过构建系统启用 CFI 和分支保护（pac_ret），修改后须确认无新增编译告警
- TypeScript 代码默认通过各模块 `tsconfig.json` 检查，修改后须运行 `tsc --noEmit`（在对应模块目录下）确认无类型错误
- 如需额外检查：`./build.sh --product-name rk3568 --build-target ets_utils --gn-args="is_clang_check=true"`

详细验证方法：`docs/knowledge/verify/ets-utils-verify.md`
