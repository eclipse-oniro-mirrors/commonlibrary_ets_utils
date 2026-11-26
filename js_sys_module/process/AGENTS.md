# process（@ohos.process）模块指南

> **维护声明（MUST）**：使用 agent 修改代码时，如新增/删除文件、构建目标变更、公共 API 变更或约束变更，必须同步更新本文件。

## 1. 模块定位

process 模块提供进程管理 JS API：进程标识（PID/PPID/TID）、用户与组信息（UID/GID/有效 ID/组）、进程控制（kill/exit/abort/chdir）、系统信息（uptime/CPU 时间/system config）、环境变量读写、子进程拉起与管理（spawn）、子进程标准输出/错误处理、退出码与终止状态查询、线程优先级管理。主语言 C++，经 N-API 绑定暴露。

## 2. 代码地图

| 路径 | 职责 | 风险 |
|------|------|------|
| `native_module_process.cpp` | NAPI 模块注册入口 | 🟡 导出面须与 d.ts 一致 |
| `js_process.h` / `js_process.cpp` | 进程信息与控制实现 | 🔴 kill/exit 权限边界所在 |
| `js_childprocess.h` / `js_childprocess.cpp` | 子进程拉起、生命周期与 stdio | 🔴 子进程资源泄漏/僵尸进程风险 |
| `process_report_helper.h` / `process_report_helper.cpp` | 行为异常上报辅助 | 🟡 |
| `libprocess.map` | 导出符号裁剪 | 🟡 |
| `BUILD.gn` | 构建配置 | 🟢 |

## 3. 知识路由

| 任务/术语 | 先读 |
|-----------|------|
| 平台差异实现 | `platform/AGENTS.md`（`platform/*/process_helper.cpp`） |
| Timer（另一 js_sys 模块） | `js_sys_module/timer/AGENTS.md` |
| 安全上报工具 | `tools/AGENTS.md`（security_fault_reporter.h） |

**外部组件依赖**：`napi:ace_napi`、`hilog:libhilog`、`libuv:uv`（子进程与事件循环）。

## 4. 约束与边界

### 4.1 Do not（禁止项）

1. 禁止未经权限检查调用 kill/exit——目标进程与调用者身份必须校验
2. 禁止在子进程未被回收（waitpid）前释放其资源
3. 禁止对既有 process 公共 API 做不兼容变更；确需变更必须走废弃流程
4. 禁止在环境变量读写中引入注入风险（未校验键名合法性）
5. 禁止阻塞事件循环（长等待操作必须异步化）
6. 禁止持有 `napi_ref` 而不注册环境清理钩子
7. 禁止修改错误码语义
8. 禁止为通过测试删除或跳过既有用例

### 4.2 Ask before（需确认后再修改）

- 修改子进程 spawn 参数白名单前，须先确认安全策略（可执行文件路径限制）
- 修改 kill 语义或权限校验前，须先确认系统安全团队策略
- 修改线程优先级映射前，须先确认 QoS 策略一致

### 4.3 架构不变量

1. 子进程经 libuv 异步管理，退出后必须回收
2. 平台相关逻辑在 `platform/*/process_helper.cpp`
3. 对外错误统一为 BusinessError

## 5. 构建与验证

构建命令从 OpenHarmony 源码根目录执行，不在本子目录执行。

### 5.1 构建

```sh
./build.sh --product-name rk3568 --build-target process
```

### 5.2 单元测试

测试目标 `test_process_unittest` 定义于 `js_sys_module/test/BUILD.gn`：

```sh
./build.sh --product-name rk3568 --build-target test_process_unittest
hdc file send ./out/rk3568/tests/unittest/ets_utils/ets_utils/js_sys_module/process/test_process_unittest /data/local/tmp/
hdc shell "cd /data/local/tmp && chmod 777 ./test_process_unittest && ./test_process_unittest"
```

### 5.3 Done 定义

1. ✅ 修改的文件无新增编译错误，`process` 构建成功
2. ✅ `test_process_unittest` 通过（或按 §5.4 声明未验证项）
3. ✅ 未引入公共 API 不兼容变更
4. ✅ kill/exit 权限校验与子进程回收路径未被破坏
5. ✅ 未违反本文件禁止事项

### 5.4 禁止伪造验证（强制）

如果构建环境不可用，**不要声称完成**。列出应执行的命令、预期结果、未验证项与原因，并给出替代证据（CI 结果、人工 review checklist、`-fsyntax-only` 语法检查输出）。**不得将未执行的验证描述为已通过；不得在无证据时声称完成。**
