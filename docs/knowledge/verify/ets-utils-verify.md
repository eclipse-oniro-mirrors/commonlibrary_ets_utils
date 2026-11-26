# ets_utils 编译和测试方法

## 构建前提

所有构建命令从 OpenHarmony 源码根目录执行，不在本仓子目录执行。本仓库在源码树中的路径为 `commonlibrary/ets_utils`。

---

## 构建命令

### 全量构建

```sh
./build.sh --product-name rk3568 --build-target ets_utils --ccache
```

### 单模块构建

| 模块 | 构建目标 |
|------|---------|
| Buffer | `./build.sh --product-name rk3568 --build-target buffer` |
| FastBuffer | `./build.sh --product-name rk3568 --build-target fastbuffer` |
| URI | `./build.sh --product-name rk3568 --build-target uri` |
| URL | `./build.sh --product-name rk3568 --build-target url` |
| ConvertXML | `./build.sh --product-name rk3568 --build-target convertxml` |
| XML | `./build.sh --product-name rk3568 --build-target xml` |
| TaskPool | `./build.sh --product-name rk3568 --build-target taskpool` |
| Worker | `./build.sh --product-name rk3568 --build-target worker` |
| 并发工具（locks/ASON/condition） | `./build.sh --product-name rk3568 --build-target utils` |
| Console | `./build.sh --product-name rk3568 --build-target console` |
| Process | `./build.sh --product-name rk3568 --build-target process` |
| Timer | `./build.sh --product-name rk3568 --build-target timer` |
| DFX | `./build.sh --product-name rk3568 --build-target dfx` |
| Util | `./build.sh --product-name rk3568 --build-target util` |
| Container（15 种容器聚合） | `./build.sh --product-name rk3568 --build-target container_packages` |
| Container（单个容器，如 ArrayList） | `./build.sh --product-name rk3568 --build-target arraylist` |
| Collections | `./build.sh --product-name rk3568 --build-target collections` |
| JSON | `./build.sh --product-name rk3568 --build-target json` |
| Stream | `./build.sh --product-name rk3568 --build-target stream` |
| BaseSDK | `./build.sh --product-name rk3568 --build-target base_sdk_ets` |
| BaseTransfer | `./build.sh --product-name rk3568 --build-target base_transfer_ets` |

> 注意：不存在名为 `container` 的构建目标，聚合目标为 `container_packages`，单个容器目标与 `js_util_module/container/BUILD.gn` 的 `container_names` 列表一致。

### 静态库目标（部分模块支持）

```sh
./build.sh --product-name rk3568 --build-target uri_static
./build.sh --product-name rk3568 --build-target taskpool_static
./build.sh --product-name rk3568 --build-target worker_static
```

---

## 测试命令

### 单元测试构建与运行

```sh
# 构建测试（示例：URI；目标与产物路径按下方表格替换）
./build.sh --product-name rk3568 --build-target test_uri_unittest

# 部署到设备
hdc file send ./out/rk3568/tests/unittest/ets_utils/ets_utils/jsapi/uri/napi/test_uri_unittest /data/local/tmp/

# 执行测试
hdc shell "cd /data/local/tmp && chmod 777 ./test_uri_unittest && ./test_uri_unittest"
```

### 各模块测试目标与路径

| 模块 | 构建目标 | 产物路径（相对 `out/rk3568/tests/unittest/ets_utils/`） |
|------|---------|---------|
| Buffer | `test_buffer_unittest` | `ets_utils/jsapi/buffer/napi` |
| ConvertXML | `test_convertxml_unittest` | `ets_utils/jsapi/convertxml/napi` |
| URI | `test_uri_unittest` | `ets_utils/jsapi/uri/napi` |
| URL | `test_url_unittest` | `ets_utils/jsapi/url/napi` |
| XML | `test_xml_unittest` | `ets_utils/jsapi/xml/napi` |
| TaskPool | `test_taskpool_unittest` | `ets_utils/js_concurrent_module/taskpool` |
| Worker | `test_worker_unittest` | `ets_utils/js_concurrent_module/worker` |
| 并发公共（common/helper） | `test_concurrent_unittest` | `ets_utils/js_concurrent_module/helper` |
| 并发工具（locks/condition） | `test_utils_unittest` | `ets_utils/js_concurrent_module/utils` |
| Console | `test_console_unittest` | `ets_utils/jssys/console/napi` |
| Timer | `test_timer_unittest` | `ets_utils/jssys/timer/napi` |
| Process | `test_process_unittest`（定义于 `js_sys_module/test/`） | `ets_utils/js_sys_module/process` |
| Util | `test_util_unittest` | `ets_utils/jsutil/util/napi` |
| dfx / fastbuffer / json / stream / collections / container | 无独立单测目标 | 改动后经编译验证与相关集成模块测试覆盖 |

> 注意：并发工具模块只有一个单测目标 `test_utils_unittest`（源码含 `test_locks.cpp`、`test_condition.cpp`），不存在 `test_locks_unittest`/`test_condition_unittest` 目标。

### Host 单元测试（Linux，无需设备）

```sh
./build.sh --product-name rk3568 --build-target ets_utils_host_unittest
```

聚合上表全部单测目标（host toolchain），产物位于 `out/rk3568/clang_x64/tests/unittest/ets_utils/`。注意：该目标仅在 Linux host 且非独立编译（indep）模式下生效。

---

## Done 定义

任务完成必须满足：

1. 所有修改的文件无新增编译错误
2. 相关模块构建成功（`./build.sh --product-name rk3568 --build-target <module>`）
3. 相关测试用例通过（`test_<module>_unittest`，以目标表为准）
4. 未引入公共 API 不兼容变更
5. 未违反专家经验中列出的任何禁止事项
6. NAPI 原生代码的内存管理正确（无泄漏、无悬垂引用）
7. 如修改了跨平台代码，确认所有平台宏分支编译正确

---

## 验证循环

### 最小验证步骤

1. **编译验证**：仅构建修改的模块
2. **单测验证**：仅运行修改模块的单元测试
3. **依赖影响**：如果修改了公共头文件（inner_kits）或共享辅助代码（common/helper、tools），需额外构建依赖模块

### 按任务类型的额外验证

| 任务类型 | 额外验证 |
|---------|---------|
| Buffer/FastBuffer NAPI 绑定 | 运行 buffer + fastbuffer 相关测试，验证 ArkPrivate.Load |
| XML/ConvertXML | 同时验证两个模块（共用 libxml2） |
| 并发辅助（common/helper） | 运行 TaskPool + Worker + Concurrent Utils 测试 |
| Inner Kit 头文件 | 验证所有依赖模块构建通过 |
| platform/ 适配 | 验证所有平台宏分支编译 |
| JSON 模块 | 验证 parse/stringify/has/remove 全功能 + BigInt 三模式 |
| Collections 桥接层 | 验证 NAPI 绑定 + ArkTS 运行时集合行为一致 |
| Container 容器 | 编译 `container_packages` + 新增容器单测（如适用） |

### 回退方案

- 如果构建失败，先检查 GN 依赖声明是否完整
- 如果测试失败，区分是 NAPI 层还是 TS 层的问题
- 如果跨平台编译失败，检查 `platform/` 目录下对应平台的条件编译宏

---

## API 兼容性检查

- 检查修改的 .ts 文件中 export 的函数/类/接口签名是否变更
- 如有签名变更，确认已走 API 废弃流程
- 检查方法参数数量、类型、返回值是否与已有 API 一致

### API 兼容性验证工具

- SDK API 检查：通过 OpenHarmony 兼容性测试套件验证（`./build.sh --product-name rk3568 --build-target sdk_test`）
- NAPI 接口一致性：对比 `@ohos.*.d.ts` 声明与 NAPI 注册函数列表，确认导出函数无遗漏或签名变更

## 最终回复格式

任务完成时，回复必须包含：

1. **变更摘要**（修改文件列表含完整路径 + 风险等级）
2. **验证结果**（执行的构建/测试命令及结果、Done 定义逐项确认 ✅/❌）
3. **兼容性影响**（公共 API/Inner Kit/HiSysEvent 等评估，如相关）
4. **未验证项**（因环境限制未运行的验证 + 残留风险）
5. **遗留项**（后续待办或风险）

## 禁止伪造验证（强制）

如果构建环境不可用，**不要声称完成**。列出应执行的命令、预期结果、未验证项与原因，并给出替代证据（如 CI 结果链接、人工 review checklist、静态分析输出）。**不得将未执行的验证描述为已通过；不得在无证据时声称完成。** 可行的替代手段：

1. 语法检查修改文件（TS 使用 `tsc --noEmit`；C++ 使用编译器的 `-fsyntax-only` 仅语法检查模式）
2. 明确列出需要人工验证的项

## 静态分析

- C++ 代码已通过构建系统启用 CFI 和分支保护（pac_ret）
- TypeScript 代码通过 tsconfig.json 检查
- 如需额外检查：`./build.sh --product-name rk3568 --build-target ets_utils --gn-args="is_clang_check=true"`
