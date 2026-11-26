# convertxml（@ohos.util.convertxml）模块指南

> **维护声明（MUST）**：使用 agent 修改代码时，如新增/删除文件、构建目标变更、公共 API 变更或约束变更，必须同步更新本文件。

## 1. 模块定位

convertxml 提供将 XML 转换为 JavaScript 对象的能力：标准/快速/大文档三种转换策略、完整 XML 支持（元素/属性/文本/CDATA/注释/声明/处理指令）、可配置解析选项、大文档内存优化（`largeConvertToJSObject()`）。解析引擎基于 libxml2，C++ 原生实现经 NAPI 暴露，TS 层位于 `src/js_convertxml.ts`。

## 2. 代码地图

| 路径 | 职责 | 风险 |
|------|------|------|
| `src/js_convertxml.ts` | TS 公共 API 实现 | 🟡 公共 API 定义层 |
| `js_convertxml.h` / `js_convertxml.cpp` | 原生 XML→JS 对象转换实现（libxml2） | 🔴 libxml2 集成与内存管理所在 |
| `native_module_convertxml.h` / `native_module_convertxml.cpp` | NAPI 模块入口 | 🟡 导出面须与 d.ts 一致 |
| `BUILD.gn` / `tsconfig.json` | 构建配置 | 🟢 |
| `test/` | 单元测试（`test_convertxml.cpp`、`test_ark.cpp`） | 🟢 |

## 3. 知识路由

| 任务/术语 | 先读 |
|-----------|------|
| XmlPullParser/XmlSerializer（另一 XML 模块） | `js_api_module/xml/AGENTS.md` |
| 大文档 OOM 失败模式 | 根 `AGENTS.md` §4.5（使用 `largeConvertToJSObject()`） |
| libxml2 约束 | 根 `AGENTS.md` §4.1 禁止事项 #7 |

**外部组件依赖**：`napi:ace_napi`、`libxml2:libxml2`、`hilog:libhilog`、`icu:shared_icuuc`、`c_utils:utils`（非标准系统）。与 `xml/` 模块共享 libxml2 能力。共享配置经 `ets_utils_config.gni`。

## 4. 约束与边界

### 4.1 Do not（禁止项）

1. 禁止绕过 libxml2 自行实现 XML 解析器
2. 禁止在本模块实现网络 I/O
3. 禁止破坏既有转换 API 契约（options 结构、返回对象形态）
4. 禁止修改 XML schema/DTD 校验行为
5. 禁止实现 JS 对象 → XML 序列化（本模块只做 XML → JS）
6. 禁止未经协调修改共享工具文件
7. 禁止不经构建守卫引入平台特定代码

### 4.2 Ask before（需确认后再修改）

- 修改解析 options 默认值时，须先确认既有应用转换结果不变
- 新增转换策略时，须先确认大文档内存路径（`largeConvertToJSObject()`）覆盖

### 4.3 架构不变量

1. 解析引擎只能是 libxml2
2. TS 层定义 Public API；C++ 层经 NAPI 提供实现
3. 对外错误统一为 BusinessError

## 5. 构建与验证

构建命令从 OpenHarmony 源码根目录执行，不在本子目录执行。

### 5.1 构建

```sh
./build.sh --product-name rk3568 --build-target convertxml
```

### 5.2 单元测试

```sh
./build.sh --product-name rk3568 --build-target test_convertxml_unittest
hdc file send ./out/rk3568/tests/unittest/ets_utils/ets_utils/jsapi/convertxml/napi/test_convertxml_unittest /data/local/tmp/
hdc shell "cd /data/local/tmp && chmod 777 ./test_convertxml_unittest && ./test_convertxml_unittest"
```

同时修改 xml/convertxml 任一模块时须双模块验证（共用 libxml2）。

### 5.3 Done 定义

1. ✅ 修改的文件无新增编译错误，`convertxml` 构建成功
2. ✅ `test_convertxml_unittest` 通过（或按 §5.4 声明未验证项）
3. ✅ 未引入公共 API 不兼容变更
4. ✅ libxml2 资源（parser context 等）正确释放
5. ✅ 未违反本文件禁止事项

### 5.4 禁止伪造验证（强制）

如果构建环境不可用，**不要声称完成**。列出应执行的命令、预期结果、未验证项与原因，并给出替代证据（CI 结果、人工 review checklist、`tsc --noEmit`/`-fsyntax-only` 语法检查输出）。**不得将未执行的验证描述为已通过；不得在无证据时声称完成。**
