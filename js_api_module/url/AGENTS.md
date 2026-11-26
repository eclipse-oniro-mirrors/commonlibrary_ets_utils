# url（@ohos.url）模块指南

> **维护声明（MUST）**：使用 agent 修改代码时，如新增/删除文件、构建目标变更、公共 API 变更或约束变更，必须同步更新本文件。

## 1. 模块定位

url 实现标准 Web URL API：URL 解析与构造（protocol/hostname/port/path/query/fragment）、URLSearchParams 查询参数管理、percent 编解码、IPv4/IPv6 地址支持、多协议支持（HTTP/HTTPS/FTP/WS/WSS/FILE 等）、组件提取与修改、标准合规校验（WHATWG URL Standard）。TS 层定义 API，C++ 层经 NAPI 提供高性能路径。

## 2. 代码地图

| 路径 | 职责 | 风险 |
|------|------|------|
| `src/js_url.ts` | TS 公共 API 实现 | 🟡 公共 API 定义层 |
| `js_url.h` / `js_url.cpp` | 原生 URL 实现（WHATWG 解析） | 🔴 WHATWG 语义与错误码契约所在 |
| `url_helper.h` | URL 辅助函数 | 🟡 |
| `native_module_url.h` / `native_module_url.cpp` | NAPI 模块入口 | 🟡 导出面须与 d.ts 一致 |
| `BUILD.gn` / `tsconfig.json` | 构建配置 | 🟢 |
| `test/` | 单元测试（`test_napi.cpp`、`test_ark.cpp`） | 🟢 |

## 3. 知识路由

| 任务/术语 | 先读 |
|-----------|------|
| URI（RFC 3986，勿与 URL 混淆） | `js_api_module/uri/AGENTS.md` |
| WHATWG 标准合规 | 本文件 §4.1 |
| 编解码（Base64/TextEncoder） | `js_util_module/util/AGENTS.md` |

**外部组件依赖**：`napi:ace_napi`、`icu:shared_icuuc`、`hilog:libhilog`、`bounds_checking_function:libsec_shared`、`c_utils:utils`（非标准系统）。共享配置经 `ets_utils_config.gni`。

## 4. 约束与边界

### 4.1 Do not（禁止项）

1. 禁止在本模块实现网络 I/O 或 HTTP 客户端/服务器功能
2. 禁止实现脱离 WHATWG URL 标准的浏览器特有行为
3. 禁止在标准之外添加协议特定校验
4. 禁止破坏与 WHATWG URL 标准兼容的既有 URL API 契约
5. 禁止绕过 SecureC（libsec）做字符串操作
6. 禁止未经协调修改共享工具文件
7. 禁止不经构建守卫引入平台特定代码

### 4.2 Ask before（需确认后再修改）

- 新增协议 scheme 支持时，须先确认与 WHATWG 标准及既有应用兼容
- 修改 IPv4/IPv6 解析或 percent 编码规则时，须先确认标准合规测试覆盖

### 4.3 架构不变量

1. TS 层定义 Public API；C++ 层经 NAPI 提供高性能实现
2. URL 解析语义以 WHATWG URL Standard 为准
3. 对外错误统一为 BusinessError

## 5. 构建与验证

构建命令从 OpenHarmony 源码根目录执行，不在本子目录执行。

### 5.1 构建

```sh
./build.sh --product-name rk3568 --build-target url
```

### 5.2 单元测试

```sh
./build.sh --product-name rk3568 --build-target test_url_unittest
hdc file send ./out/rk3568/tests/unittest/ets_utils/ets_utils/jsapi/url/napi/test_url_unittest /data/local/tmp/
hdc shell "cd /data/local/tmp && chmod 777 ./test_url_unittest && ./test_url_unittest"
```

### 5.3 Done 定义

1. ✅ 修改的文件无新增编译错误，`url` 构建成功
2. ✅ `test_url_unittest` 通过（或按 §5.4 声明未验证项）
3. ✅ 未引入公共 API 不兼容变更
4. ✅ 未违反本文件禁止事项
5. ✅ 新增/修改错误路径有明确 BusinessError 返回与日志

### 5.4 禁止伪造验证（强制）

如果构建环境不可用，**不要声称完成**。列出应执行的命令、预期结果、未验证项与原因，并给出替代证据（CI 结果、人工 review checklist、`tsc --noEmit`/`-fsyntax-only` 语法检查输出）。**不得将未执行的验证描述为已通过；不得在无证据时声称完成。**
