# xml（@ohos.xml）模块指南

> **维护声明（MUST）**：使用 agent 修改代码时，如新增/删除文件、构建目标变更、公共 API 变更或约束变更，必须同步更新本文件。

## 1. 模块定位

xml 模块提供 XML 解析与序列化：XmlSerializer（写元素/属性/声明/注释/CDATA/文本，仅支持 UTF-8 输出）与 XmlPullParser（事件驱动解析，支持 UTF-8、命名空间、注释、CDATA、文档类型声明与多种实体类型）。解析引擎基于 libxml2，C++ 实现，TS 包装层位于 `src/js_xml.ts`。

## 2. 代码地图

| 路径 | 职责 | 风险 |
|------|------|------|
| `src/js_xml.ts` | TS 公共 API 包装 | 🟡 公共 API 定义层，签名变更需兼容评估 |
| `js_xml.h` / `js_xml.cpp` | XmlSerializer/XmlPullParser 核心实现 | 🔴 libxml2 集成、UTF-8 约束与错误码契约所在 |
| `js_xml_sax.h` | SAX 解析辅助 | 🟡 |
| `js_xml_dynamic.h` / `js_xml_dynamic.cpp` | 动态解析支持 | 🟡 |
| `native_module_xml.h` / `native_module_xml.cpp` | NAPI 模块注册与导出 | 🟡 导出面须与 d.ts 一致 |
| `libxml.map` | 动态库导出符号裁剪 | 🔴 新增导出符号须同步维护 |
| `BUILD.gn` / `tsconfig.json` | 构建配置 | 🟢 |
| `test/` | 单元测试 | 🟢 |

## 3. 知识路由

| 任务/术语 | 先读 |
|-----------|------|
| XML → JS 对象（另一 XML 模块） | `js_api_module/convertxml/AGENTS.md`（两者共用 libxml2，需双模块验证） |
| libxml2 约束 | 根 `AGENTS.md` §4.1 禁止事项 #7 |
| 10200066 编码错误 | 根 `AGENTS.md` §3.3、§4.5 失败模式 |
| 专家经验 | `docs/knowledge/expert/ets-utils-expert.md` |

**外部组件依赖**：`napi:ace_napi`、`libxml2:libxml2`、`hilog:libhilog`、`icu:shared_icuuc`、`c_utils:utils`（非标准系统）。共享配置经 `ets_utils_config.gni`。

## 4. 约束与边界

### 4.1 Do not（禁止项）

1. 禁止绕过 libxml2 自行实现 XML 解析器
2. 禁止在 XmlSerializer 中使用非 UTF-8 编码（API 契约仅支持 UTF-8 输出）
3. 禁止修改既有错误码语义（10200066 编码错误、401 参数错等公共契约）
4. 禁止绕过 NAPI 类型检查；禁止未考虑编码的原始字符串操作
5. 禁止引入内存泄漏、缓冲区溢出、use-after-free；禁止向 JS 层暴露裸内存指针
6. 禁止吞掉或隐藏错误；禁止为通过测试删除或跳过既有用例
7. 禁止绕过 es2abc/TypeScript 编译流水线；禁止直接编辑 `.abc`/`.o` 构建产物
8. 禁止新增功能时不更新本文件

### 4.2 Ask before（需确认后再修改）

- 新增 XmlSerializer/XmlPullParser 方法时，须先确认 d.ts 与导出面同步、错误码与既有语义一致
- 修改实体类型或命名空间处理规则时，须先确认与 libxml2 既有行为及 W3C 规范兼容
- 修改 `libxml.map` 或导出列表时，须先确认下游符号消费不受影响

### 4.3 架构不变量

1. 解析引擎只能是 libxml2
2. TS 层定义 Public API；C++ 层经 NAPI 提供实现
3. 序列化输出编码只支持 UTF-8
4. 对外错误统一为 BusinessError

## 5. 构建与验证

构建命令从 OpenHarmony 源码根目录执行，不在本子目录执行。

### 5.1 构建

```sh
./build.sh --product-name rk3568 --build-target xml
# 聚合组
./build.sh --product-name rk3568 --build-target xml_packages
```

### 5.2 单元测试

```sh
./build.sh --product-name rk3568 --build-target test_xml_unittest
hdc file send ./out/rk3568/tests/unittest/ets_utils/ets_utils/jsapi/xml/napi/test_xml_unittest /data/local/tmp/
hdc shell "cd /data/local/tmp && chmod 777 ./test_xml_unittest && ./test_xml_unittest"
```

同时修改 xml/convertxml 任一模块时须双模块验证（共用 libxml2）。

### 5.3 Done 定义

1. ✅ 修改的文件无新增编译错误，`xml` 构建成功
2. ✅ `test_xml_unittest` 通过（或按 §5.4 声明未验证项）
3. ✅ 未引入公共 API 不兼容变更
4. ✅ libxml2 资源（parser context 等）正确释放
5. ✅ 未违反本文件禁止事项（尤其 UTF-8 约束）

### 5.4 禁止伪造验证（强制）

如果构建环境不可用，**不要声称完成**。列出应执行的命令、预期结果、未验证项与原因，并给出替代证据（CI 结果、人工 review checklist、`tsc --noEmit`/`-fsyntax-only` 语法检查输出）。**不得将未执行的验证描述为已通过；不得在无证据时声称完成。**
