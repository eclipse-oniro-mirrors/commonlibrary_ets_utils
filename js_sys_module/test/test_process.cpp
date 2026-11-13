/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <dirent.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <unistd.h>
#include <vector>
#include "test.h"

#include "napi/native_api.h"
#include "napi/native_node_api.h"

#include "securec.h"
#include "js_childprocess.h"
#include "js_process.h"
#include "tools/log.h"

#define ASSERT_CHECK_CALL(call)   \
    {                             \
        ASSERT_EQ(call, napi_ok); \
    }

#define ASSERT_CHECK_VALUE_TYPE(env, value, type)               \
    {                                                           \
        napi_valuetype valueType = napi_undefined;              \
        ASSERT_TRUE(value != nullptr);                          \
        ASSERT_CHECK_CALL(napi_typeof(env, value, &valueType)); \
        ASSERT_EQ(valueType, type);                             \
    }
OHOS::JsSysModule::Process::ChildProcess* RunCommand(napi_env env, napi_value command, napi_value options)
{
    OHOS::JsSysModule::Process::ChildProcess* objectInfo = nullptr;
    objectInfo = new OHOS::JsSysModule::Process::ChildProcess();
    objectInfo->InitOptionsInfo(env, options);

    objectInfo->Spawn(env, command);
    return objectInfo;
}
static std::string testStr = "";

static int g_injectCreateAsyncWorkOrdinal = 0;
static int g_injectQueueWorkQosOrdinal = 0;
static int g_injectCreateArraybufferFail = 0;
static int g_injectMemcallFail = 0;
static int g_trackWorkHandles = 0;
static std::vector<void*> g_pendingWorkHandles;
static pid_t g_injectKillFailPid = 0;
static std::vector<int> g_fillFds;
static rlim_t g_originalNoFileLimit = 0;

template<typename T>
T RealSymbol(const char* name)
{
    void* sym = dlsym(RTLD_NEXT, name);
    if (sym == nullptr) {
        return nullptr;
    }
    return reinterpret_cast<T>(sym);
}

extern "C" napi_status napi_create_async_work(napi_env env, napi_value asyncResource,
    napi_value asyncResourceName, napi_async_execute_callback execute,
    napi_async_complete_callback complete, void* data, napi_async_work* result)
{
    if (g_injectCreateAsyncWorkOrdinal > 0) {
        g_injectCreateAsyncWorkOrdinal--;
        if (g_injectCreateAsyncWorkOrdinal == 0) {
            return napi_generic_failure;
        }
    }
    using Fn = napi_status (*)(napi_env, napi_value, napi_value, napi_async_execute_callback,
        napi_async_complete_callback, void*, napi_async_work*);
    Fn real = RealSymbol<Fn>("napi_create_async_work");
    if (real == nullptr) {
        return napi_generic_failure;
    }
    napi_status status = real(env, asyncResource, asyncResourceName, execute, complete, data, result);
    if (g_trackWorkHandles != 0 && status == napi_ok && result != nullptr && *result != nullptr) {
        g_pendingWorkHandles.push_back(*result);
    }
    return status;
}

extern "C" napi_status napi_queue_async_work_with_qos(napi_env env, napi_async_work work, napi_qos_t qos)
{
    if (g_injectQueueWorkQosOrdinal > 0) {
        g_injectQueueWorkQosOrdinal--;
        if (g_injectQueueWorkQosOrdinal == 0) {
            return napi_generic_failure;
        }
    }
    using Fn = napi_status (*)(napi_env, napi_async_work, napi_qos_t);
    Fn real = RealSymbol<Fn>("napi_queue_async_work_with_qos");
    if (real == nullptr) {
        return napi_generic_failure;
    }
    return real(env, work, qos);
}

extern "C" napi_status napi_delete_async_work(napi_env env, napi_async_work work)
{
    for (auto it = g_pendingWorkHandles.begin(); it != g_pendingWorkHandles.end(); ++it) {
        if (*it == reinterpret_cast<void*>(work)) {
            g_pendingWorkHandles.erase(it);
            return napi_ok;
        }
    }
    using Fn = napi_status (*)(napi_env, napi_async_work);
    Fn real = RealSymbol<Fn>("napi_delete_async_work");
    if (real == nullptr) {
        return napi_generic_failure;
    }
    return real(env, work);
}

extern "C" napi_status napi_create_arraybuffer(napi_env env, size_t byteLength, void** data,
                                               napi_value* result)
{
    if (g_injectCreateArraybufferFail != 0) {
        g_injectCreateArraybufferFail = 0;
        return napi_generic_failure;
    }
    using Fn = napi_status (*)(napi_env, size_t, void**, napi_value*);
    Fn real = RealSymbol<Fn>("napi_create_arraybuffer");
    if (real == nullptr) {
        return napi_generic_failure;
    }
    return real(env, byteLength, data, result);
}

extern "C" errno_t memcpy_s(void* dest, size_t destMax, const void* src, size_t count)
{
    if (g_injectMemcallFail != 0) {
        g_injectMemcallFail = 0;
        return EINVAL;
    }
    using Fn = errno_t (*)(void*, size_t, const void*, size_t);
    Fn real = RealSymbol<Fn>("memcpy_s");
    if (real == nullptr) {
        return EINVAL;
    }
    return real(dest, destMax, src, count);
}

extern "C" int Kill(pid_t pid, int sig) __asm__ ("kill");
extern "C" int Kill(pid_t pid, int sig)
{
    if (g_injectKillFailPid != 0 && pid == g_injectKillFailPid && sig == SIGKILL) {
        g_injectKillFailPid = 0;
        errno = EINVAL;
        return -1;
    }
    using Fn = int (*)(pid_t, int);
    Fn real = RealSymbol<Fn>("kill");
    if (real == nullptr) {
        errno = EINVAL;
        return -1;
    }
    return real(pid, sig);
}

int HighestOpenFd()
{
    DIR* dir = opendir("/proc/self/fd");
    if (dir == nullptr) {
        return -1;
    }
    int highest = -1;
    struct dirent* entry = nullptr;
    while ((entry = readdir(dir)) != nullptr) {
        char* end = nullptr;
        long fd = strtol(entry->d_name, &end, 10); // 10: decimal
        if (end == nullptr || *end != '\0' || fd < 0) {
            continue;
        }
        if (fd > static_cast<long>(highest)) {
            highest = static_cast<int>(fd);
        }
    }
    (void)closedir(dir);
    return highest;
}

bool ExhaustFdTable(size_t freeSlots)
{
    int highest = HighestOpenFd();
    if (highest < 0) {
        return false;
    }
    struct rlimit limit;
    if (getrlimit(RLIMIT_NOFILE, &limit) != 0) {
        return false;
    }
    if (limit.rlim_cur <= static_cast<rlim_t>(highest) + freeSlots + 1) {
        return false;
    }
    g_originalNoFileLimit = limit.rlim_cur;
    struct rlimit lowered = limit;
    lowered.rlim_cur = static_cast<rlim_t>(highest) + freeSlots + 1;
    if (setrlimit(RLIMIT_NOFILE, &lowered) != 0) {
        return false;
    }
    int devNull = open("/dev/null", O_RDONLY);
    if (devNull >= 0) {
        g_fillFds.push_back(devNull);
        size_t maxFds = static_cast<size_t>(lowered.rlim_cur);
        for (size_t i = 0; i < maxFds; i++) {
            int fd = dup(devNull);
            if (fd < 0) {
                break;
            }
            g_fillFds.push_back(fd);
        }
    }

    for (size_t i = 0; i < freeSlots && !g_fillFds.empty(); i++) {
        close(g_fillFds.back());
        g_fillFds.pop_back();
    }
    return true;
}

void RestoreFdTable()
{
    for (int fd : g_fillFds) {
        close(fd);
    }
    g_fillFds.clear();
    if (g_originalNoFileLimit == 0) {
        return;
    }
    struct rlimit limit;
    if (getrlimit(RLIMIT_NOFILE, &limit) == 0) {
        limit.rlim_cur = g_originalNoFileLimit;
        (void)setrlimit(RLIMIT_NOFILE, &limit);
    }
    g_originalNoFileLimit = 0;
}

napi_value Method(napi_env env, napi_callback_info info)
{
    napi_value thisVar = nullptr;
    size_t argc = 0;
    napi_value args[6] = { 0 }; // 6:six args
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, nullptr, &thisVar, nullptr));
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr));

    napi_value name = args[0];
    napi_value value = args[1];

    std::string buffer1 = "";
    size_t bufferSize1 = 0;
    napi_get_value_string_utf8(env, name, nullptr, 0, &bufferSize1);
    buffer1.reserve(bufferSize1 + 1);
    buffer1.resize(bufferSize1);
    napi_get_value_string_utf8(env, name, buffer1.data(), bufferSize1 + 1, &bufferSize1);

    std::string buffer2 = "";
    size_t bufferSize2 = 0;
    napi_get_value_string_utf8(env, value, nullptr, 0, &bufferSize2);
    buffer2.reserve(bufferSize2 + 1);
    buffer2.resize(bufferSize2);
    napi_get_value_string_utf8(env, value, buffer2.data(), bufferSize2 + 1, &bufferSize2);
    testStr += buffer1 + buffer2;
    napi_value result = nullptr;
    napi_get_boolean(env, true, &result);
    return result;
}
/**
 * @tc.name: ProcessUptimeTest001
 * @tc.desc: Test process Uptime.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessUptimeTest001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::Process process;
    napi_value timeStart = process.Uptime(env);
    sleep(1);
    napi_value timeEnd = process.Uptime(env);
    double start = 0;
    double end = 0;
    napi_get_value_double(env, timeStart, &start);
    napi_get_value_double(env, timeEnd, &end);
    ASSERT_GE(end - start, 1);
}

/**
 * @tc.name: ProcessUptimeTest002
 * @tc.desc: Test process Uptime.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessUptimeTest002, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::Process process;
    napi_value timeStart = process.Uptime(env);
    sleep(2);
    napi_value timeEnd = process.Uptime(env);
    double start = 0;
    double end = 0;
    napi_get_value_double(env, timeStart, &start);
    napi_get_value_double(env, timeEnd, &end);
    ASSERT_GE(end - start, 2);
}

/**
 * @tc.name: ProcessUptimeTest003
 * @tc.desc: Test process Uptime.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessUptimeTest003, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::Process process;
    napi_value timeStart = process.Uptime(env);
    sleep(3);
    napi_value timeEnd = process.Uptime(env);
    double start = 0;
    double end = 0;
    napi_get_value_double(env, timeStart, &start);
    napi_get_value_double(env, timeEnd, &end);
    ASSERT_GE(end - start, 3);
}

/**
 * @tc.name: ProcessUptimeTest004
 * @tc.desc: Test process Uptime.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessUptimeTest004, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::Process process;
    napi_value timeStart = process.Uptime(env);
    sleep(4);
    napi_value timeEnd = process.Uptime(env);
    double start = 0;
    double end = 0;
    napi_get_value_double(env, timeStart, &start);
    napi_get_value_double(env, timeEnd, &end);
    ASSERT_GE(end - start, 4);
}

/**
 * @tc.name: ProcessKillTest001
 * @tc.desc: Test process kill signal.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessKillTest001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::Process process;

    std::string command("ls; sleep 1");
    napi_value temp = nullptr;
    napi_create_string_utf8(env, command.c_str(), command.length(), &temp);

    OHOS::JsSysModule::Process::ChildProcess* childprocess = RunCommand(env, temp, nullptr);

    napi_value pid = childprocess->Getpid(env);
    childprocess->Close();
    napi_value signal = nullptr;
    napi_create_int32(env, 9, &signal);
    napi_value result = process.Kill(env, pid, signal);
    bool res = false;
    napi_get_value_bool(env, result, &res);
    ASSERT_FALSE(res);
}

/**
 * @tc.name: ProcessKillTest002
 * @tc.desc: Test process kill signal.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessKillTest002, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::Process process;

    std::string command("ls; sleep 1");
    napi_value temp = nullptr;
    napi_create_string_utf8(env, command.c_str(), command.length(), &temp);

    OHOS::JsSysModule::Process::ChildProcess* childprocess = RunCommand(env, temp, nullptr);

    napi_value pid = childprocess->Getpid(env);
    childprocess->Close();
    napi_value signal = nullptr;
    napi_create_int32(env, 999, &signal);
    napi_value result = process.Kill(env, pid, signal);
    bool res = false;
    napi_get_value_bool(env, result, &res);
    ASSERT_FALSE(res);
}

/**
 * @tc.name: ProcessRunCmdTest001
 * @tc.desc: Test process RunCmd fork process.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessRunCmdTest001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::Process process;

    std::string command("each abc");
    napi_value temp = nullptr;
    napi_create_string_utf8(env, command.c_str(), command.length(), &temp);

    OHOS::JsSysModule::Process::ChildProcess* childprocess = RunCommand(env, temp, nullptr);

    napi_value output = childprocess->GetOutput(env);
    childprocess->Close();
    bool res = false;
    napi_is_promise(env, output, &res);
    ASSERT_TRUE(res);
}

/**
 * @tc.name: ProcessRunCmdTest002
 * @tc.desc: Test process RunCmd fork process.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessRunCmdTest002, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::Process process;

    std::string command("mkdir test.txt");
    napi_value temp = nullptr;
    napi_create_string_utf8(env, command.c_str(), command.length(), &temp);

    OHOS::JsSysModule::Process::ChildProcess* childprocess = RunCommand(env, temp, nullptr);

    napi_value errorOutput = childprocess->GetErrorOutput(env);
    childprocess->Close();
    bool res = false;
    napi_is_promise(env, errorOutput, &res);
    ASSERT_TRUE(res);
}

/**
 * @tc.name: ProcessRunCmdTest003
 * @tc.desc: Test process RunCmd fork process.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessRunCmdTest003, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::Process process;

    std::string command("mkdir 3.txt");
    napi_value temp = nullptr;
    napi_create_string_utf8(env, command.c_str(), command.length(), &temp);

    OHOS::JsSysModule::Process::ChildProcess* childprocess = RunCommand(env, temp, nullptr);

    napi_value errorOutput = childprocess->GetErrorOutput(env);
    childprocess->Close();
    bool res = false;
    napi_is_promise(env, errorOutput, &res);
    ASSERT_TRUE(res);
}

/**
 * @tc.name: ProcessRunCmdTest004
 * @tc.desc: Test process RunCmd fork process.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessRunCmdTest004, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::Process process;

    std::string command("mkdir 4.txt");
    napi_value temp = nullptr;
    napi_create_string_utf8(env, command.c_str(), command.length(), &temp);

    OHOS::JsSysModule::Process::ChildProcess* childprocess = RunCommand(env, temp, nullptr);

    napi_value errorOutput = childprocess->GetErrorOutput(env);
    childprocess->Close();
    bool res = false;
    napi_is_promise(env, errorOutput, &res);
    ASSERT_TRUE(res);
}

/**
 * @tc.name: ProcessGetUidTest001
 * @tc.desc: Test process uid.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessGetUidTest001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::Process process;
    napi_value napiUid = process.GetUid(env);
    int32_t uid = 0;
    napi_get_value_int32(env, napiUid, &uid);
    bool res = false;
    if (uid >= 0) {
        res = true;
    }
    ASSERT_TRUE(res);
}

/**
 * @tc.name: ProcessGetGidTest001
 * @tc.desc: Test process gid.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessGetGidTest001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::Process process;
    napi_value napiGid = process.GetGid(env);
    int32_t gid = 0;
    napi_get_value_int32(env, napiGid, &gid);
    bool res = false;
    if (gid >= 0) {
        res = true;
    }
    ASSERT_TRUE(res);
}

/**
 * @tc.name: ProcessGetEUidTest001
 * @tc.desc: Test process euid.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessGetEUidTest001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::Process process;
    napi_value napiEuid = process.GetEUid(env);
    int32_t euid = 0;
    napi_get_value_int32(env, napiEuid, &euid);
    bool res = false;
    if (euid >= 0) {
        res = true;
    }
    ASSERT_TRUE(res);
}

/**
 * @tc.name: ProcessGetEGidTest001
 * @tc.desc: Test process egid.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessGetEGidTest001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::Process process;
    napi_value napiEgid = process.GetEGid(env);
    int32_t egid = 0;
    napi_get_value_int32(env, napiEgid, &egid);
    bool res = false;
    if (egid >= 0) {
        res = true;
    }
    ASSERT_TRUE(res);
}

/**
 * @tc.name: ProcessGetPidTest001
 * @tc.desc: Test process pid.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessGetPidTest001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::Process process;
    napi_value napiPid = process.GetPid(env);
    int32_t pid = 0;
    napi_get_value_int32(env, napiPid, &pid);
    bool res = false;
    if (pid > 0) {
        res = true;
    }
    ASSERT_TRUE(res);
}

/**
 * @tc.name: ProcessGetPidTest001
 * @tc.desc: Test process ppid.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessGetPpidTest001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::Process process;
    napi_value napiPpid = process.GetPpid(env);
    int32_t ppid = 0;
    napi_get_value_int32(env, napiPpid, &ppid);
    bool res = false;
    if (ppid > 0) {
        res = true;
    }
    ASSERT_TRUE(res);
}

/**
 * @tc.name:childProcessPpidTest001
 * @tc.desc: test get the parent process ID.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, childProcessPpidTest001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::Process process;

    std::string command("ls; sleep 1s;");
    napi_value temp = nullptr;
    napi_create_string_utf8(env, command.c_str(), command.length(), &temp);

    OHOS::JsSysModule::Process::ChildProcess* childprocess = RunCommand(env, temp, nullptr);
    napi_value result = childprocess->Getppid(env);
    childprocess->Close();
    int32_t ppid = 0;
    napi_get_value_int32(env, result, &ppid);
    bool res = false;
    if (ppid >= 0) {
        res = true;
    }
    ASSERT_TRUE(res);
}

/**
 * @tc.name:childProcesspidTest001
 * @tc.desc: test get the specific pid value.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, childProcesspidTest001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::Process process;

    std::string command("ls; sleep 1s;");
    napi_value temp = nullptr;
    napi_create_string_utf8(env, command.c_str(), command.length(), &temp);

    OHOS::JsSysModule::Process::ChildProcess* childprocess = RunCommand(env, temp, nullptr);
    napi_value result = childprocess->Getpid(env);
    childprocess->Close();
    int32_t pid = 0;
    napi_get_value_int32(env, result, &pid);
    bool res = false;
    if (pid >= 0) {
        res = true;
    }
    ASSERT_TRUE(res);
}

/**
 * @tc.name: ProcessGetGroupsTest001
 * @tc.desc: Test process groups.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessGetGroupsTest001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::Process process;
    napi_value element = nullptr;
    napi_value groups = process.GetGroups(env);
    napi_get_element(env, groups, 1, &element);
    int32_t indexOne = 0;
    napi_get_value_int32(env, element, &indexOne);
    bool res = false;
    if (indexOne >= 0) {
        res = true;
    }
    ASSERT_TRUE(res);
}

/**
 * @tc.name: ProcessChdirTest001
 * @tc.desc: Test process gid.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessChdirTest001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::Process process;
    // "/" exists on both OHOS devices and the linux host, unlike the device-only
    // "/system/lib"; restore the original cwd afterwards so the change does not
    // leak into subsequent tests.
    std::string catalogue = "/";
    napi_value temp = nullptr;
    napi_create_string_utf8(env, catalogue.c_str(), catalogue.length(), &temp);
    napi_value cwdBefore = process.Cwd(env);
    size_t originSize = 0;
    std::string origin = "";
    if (napi_get_value_string_utf8(env, cwdBefore, nullptr, 0, &originSize) == napi_ok) {
        origin.resize(originSize);
        napi_get_value_string_utf8(env, cwdBefore, origin.data(), originSize + 1, &originSize);
    }
    process.Chdir(env, temp);
    napi_value cwd = process.Cwd(env);
    size_t bufferSize = 0;
    if (napi_get_value_string_utf8(env, cwd, nullptr, 0, &bufferSize) != napi_ok) {
        HILOG_ERROR("can not get str size");
    }
    std::string result = "";
    result.reserve(bufferSize + 1);
    result.resize(bufferSize);
    if (napi_get_value_string_utf8(env, cwd, result.data(), bufferSize + 1, &bufferSize) != napi_ok) {
        HILOG_ERROR("can not get str value");
    }
    bool changedOk = (result == catalogue);
    // Restore the original working directory and verify the round trip.
    napi_value originValue = nullptr;
    napi_create_string_utf8(env, origin.c_str(), origin.length(), &originValue);
    process.Chdir(env, originValue);
    napi_value cwdAfter = process.Cwd(env);
    size_t afterSize = 0;
    if (napi_get_value_string_utf8(env, cwdAfter, nullptr, 0, &afterSize) != napi_ok) {
        HILOG_ERROR("can not get str size");
    }
    std::string restored = "";
    restored.reserve(afterSize + 1);
    restored.resize(afterSize);
    if (napi_get_value_string_utf8(env, cwdAfter, restored.data(), afterSize + 1, &afterSize) != napi_ok) {
        HILOG_ERROR("can not get str value");
    }
    bool restoredOk = (restored == origin);
    ASSERT_TRUE(changedOk && restoredOk);
}

/**
 * @tc.name: ProcessOn001
 * @tc.desc: Test process gid.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessOn001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::Process process;
    napi_value temp = nullptr;
    std::string cbNameEvent = "add";
    napi_create_string_utf8(env, cbNameEvent.c_str(), cbNameEvent.length(), &temp);
    std::string cbName = "cbMethod";
    napi_value cb = nullptr;
    napi_create_function(env, cbName.c_str(), cbName.size(), Method, nullptr, &cb);
    process.On(env, temp, cb);
    napi_value convertResult = nullptr;
    convertResult = process.Off(env, temp);
    bool res = false;
    napi_get_value_bool(env, convertResult, &res);
    ASSERT_FALSE(res);
}

/**
 * @tc.name: ProcessOn002
 * @tc.desc: Test process gid.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessOn002, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::Process process;
    napi_value temp = nullptr;
    std::string cbNameEvent = "UnHandleRejection";
    napi_create_string_utf8(env, cbNameEvent.c_str(), cbNameEvent.length(), &temp);
    std::string cbName = "cbMethod";
    napi_value cb = nullptr;
    napi_create_function(env, cbName.c_str(), cbName.size(), Method, nullptr, &cb);
    process.On(env, temp, cb);
    napi_value convertResult = nullptr;
    convertResult = process.Off(env, temp);
    bool res = false;
    napi_get_value_bool(env, convertResult, &res);
    ASSERT_TRUE(res);
}

/**
 * @tc.name: ProcessGetTid001
 * @tc.desc: Test process gid.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessGetTid001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::Process process;
    napi_value napiTid = process.GetTid(env);
    int32_t tid = 0;
    napi_get_value_int32(env, napiTid, &tid);
    bool res = false;
    if (tid != 0) {
        res = true;
    }
    ASSERT_TRUE(res);
}

/**
 * @tc.name: ProcessIsIsolatedProcess001
 * @tc.desc: test whether the process is isolated.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessIsolatedProcess001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::Process process;
    napi_value result = process.IsIsolatedProcess(env);
    bool res = false;
    napi_get_value_bool(env, result, &res);
    if (res) {
        ASSERT_TRUE(res);
    } else {
        ASSERT_FALSE(res);
    }
}

/**
 * @tc.name: ProcessIsAppUid001
 * @tc.desc: test whether the process is AppUid.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessIsAppUid001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::Process process;
    napi_value uid = nullptr;
    napi_create_int32(env, 9, &uid);
    napi_value result = process.IsAppUid(env, uid);
    bool res = false;
    napi_get_value_bool(env, result, &res);
    ASSERT_FALSE(res);
}

/**
 * @tc.name: ProcessIs64Bit001
 * @tc.desc: test the operating environment is 64-bit.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessIs64Bit001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::Process process;
    napi_value result = process.Is64Bit(env);
    bool res = false;
    napi_get_value_bool(env, result, &res);
    if (res) {
        ASSERT_TRUE(res);
    } else {
        ASSERT_FALSE(res);
    }
}

/**
 * @tc.name: ProcessGetEnvironmentVar001
 * @tc.desc: test get the value corresponding to the environment variable.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessGetEnvironmentVar001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::Process process;
    napi_value temp = nullptr;
    std::string envVar = "PATH";
    napi_create_string_utf8(env, envVar.c_str(), envVar.length(), &temp);
    napi_value result = process.GetEnvironmentVar(env, temp);
    napi_valuetype valuetype;
    napi_typeof(env, result, &valuetype);
    bool res = false;
    if (valuetype == napi_string) {
        res = true;
    }
    napi_get_value_bool(env, result, &res);
    ASSERT_TRUE(res);
}

/**
 * @tc.name: ProcesGetUidForName001
 * @tc.desc: test Get process uid by process name.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcesGetUidForName001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::Process process;
    napi_value temp = nullptr;
    std::string user = "root";
    napi_create_string_utf8(env, user.c_str(), user.length(), &temp);
    napi_value result = process.GetUidForName(env, temp);
    int32_t num = 0;
    napi_get_value_int32(env, result, &num);
    bool res = false;
    if (num >= 0) {
        res = true;
    }
    napi_get_value_bool(env, result, &res);
    ASSERT_TRUE(res);
}

/**
 * @tc.name: ProcesGetUidForName002
 * @tc.desc: test Get process uid by process name.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcesGetUidForName002, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::Process process;
    napi_value temp = nullptr;
    std::string user = "1234";
    napi_create_string_utf8(env, user.c_str(), user.length(), &temp);
    napi_value result = process.GetUidForName(env, temp);
    int32_t num = 0;
    napi_get_value_int32(env, result, &num);
    bool res = false;
    if (num == -1) {
        res = true;
    }
    napi_get_value_bool(env, result, &res);
    ASSERT_TRUE(res);
}

/**
 * @tc.name: ProcesGetThreadPriority001
 * @tc.desc: test Get thread priority based on specified tid.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcesGetThreadPriority001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::Process process;
    napi_value napiTid = process.GetTid(env);
    napi_value result = process.GetThreadPriority(env, napiTid);
    napi_valuetype valuetype;
    napi_typeof(env, result, &valuetype);
    bool res = false;
    if (valuetype == napi_number) {
        res = true;
    }
    ASSERT_TRUE(res);
}

/**
 * @tc.name: ProcesGetGetStartRealtime001
 * @tc.desc: test Get the real-time elapsed time from system startup to process startup.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcesGetGetStartRealtime001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::Process process;
    napi_value result = process.GetStartRealtime(env);
    double num = 0;
    napi_get_value_double(env, result, &num);
    bool res = false;
    if (num == 0) {
        res = true;
    }
    ASSERT_TRUE(res);
}

/**
 * @tc.name: ProcesGetPastCputime001
 * @tc.desc: test Get the CPU time from the process startup to the current time.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcesGetPastCputime001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::Process process;
    napi_value result = process.GetPastCputime(env);
    int32_t num = 0;
    napi_get_value_int32(env, result, &num);
    bool res = false;
    if (num != 0) {
        res = true;
    }
    napi_get_value_bool(env, result, &res);
    ASSERT_TRUE(res);
}

/**
 * @tc.name: ProcesGetSystemConfig001
 * @tc.desc: test Get system configuration information.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcesGetSystemConfig001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::Process process;
    napi_value config = nullptr;
    napi_create_int32(env, _SC_NPROCESSORS_CONF, &config);
    napi_value result = process.GetSystemConfig(env, config);
    int32_t num = 0;
    napi_get_value_int32(env, result, &num);
    bool res = false;
    if (num != 0) {
        res = true;
    }
    napi_get_value_bool(env, result, &res);
    ASSERT_TRUE(res);
}

/**
 * @tc.name: ProcessCloseTest001
 * @tc.desc: Close the target process.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessCloseTest001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::Process process;

    std::string command("ls; sleep 1");
    napi_value temp = nullptr;
    napi_create_string_utf8(env, command.c_str(), command.length(), &temp);

    OHOS::JsSysModule::Process::ChildProcess* childprocess = RunCommand(env, temp, nullptr);
    childprocess->Wait(env);
    childprocess->Close();
    napi_value exitCode = childprocess->GetExitCode(env);
    int32_t num = 0;
    napi_value result = nullptr;
    napi_get_value_int32(env, exitCode, &num);
    bool res = false;
    if (num == 0) {
        res = true;
    }
    napi_get_value_bool(env, result, &res);
    ASSERT_TRUE(res);
}

/**
 * @tc.name: ProcessCloseTest002
 * @tc.desc: Close the target process.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessCloseTest002, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::Process process;

    std::string command("ls; sleep 2");
    napi_value temp = nullptr;
    napi_create_string_utf8(env, command.c_str(), command.length(), &temp);

    OHOS::JsSysModule::Process::ChildProcess* childprocess = RunCommand(env, temp, nullptr);
    childprocess->Wait(env);
    childprocess->Close();
    napi_value exitCode = childprocess->GetExitCode(env);
    int32_t num = 0;
    napi_value result = nullptr;
    napi_get_value_int32(env, exitCode, &num);
    bool res = false;
    if (num == 0) {
        res = true;
    }
    napi_get_value_bool(env, result, &res);
    ASSERT_TRUE(res);
}

/**
 * @tc.name:childProcessKillTest001
 * @tc.desc: Send a signal to process.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, childProcessKillTest001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::Process process;

    std::string command("ls; sleep 1s;");
    napi_value temp = nullptr;
    napi_create_string_utf8(env, command.c_str(), command.length(), &temp);

    OHOS::JsSysModule::Process::ChildProcess* childprocess = RunCommand(env, temp, nullptr);
    childprocess->Wait(env);
    napi_value signo = nullptr;
    napi_create_int32(env, 9, &signo);
    childprocess->Kill(env, signo);
    napi_value result = childprocess->GetKilled(env);
    childprocess->Close();
    bool res = false;
    napi_get_value_bool(env, result, &res);
    ASSERT_FALSE(res);
}

/**
 * @tc.name:childProcessKillTest002
 * @tc.desc: Send a signal to process.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, childProcessKillTest002, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::Process process;

    std::string command("ls; sleep 2s;");
    napi_value temp = nullptr;
    napi_create_string_utf8(env, command.c_str(), command.length(), &temp);

    OHOS::JsSysModule::Process::ChildProcess* childprocess = RunCommand(env, temp, nullptr);
    childprocess->Wait(env);
    napi_value signo = nullptr;
    napi_create_int32(env, 9, &signo);
    childprocess->Kill(env, signo);
    napi_value result = childprocess->GetKilled(env);
    childprocess->Close();
    bool res = false;
    napi_get_value_bool(env, result, &res);
    ASSERT_FALSE(res);
}

/**
 * @tc.name: ProcessManagerKillTest001
 * @tc.desc: Test processManager kill a running process.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessManagerKillTest001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::ProcessManager processManager;

    std::string command("ls; sleep 1");
    napi_value temp = nullptr;
    napi_create_string_utf8(env, command.c_str(), command.length(), &temp);

    OHOS::JsSysModule::Process::ChildProcess* childprocess = RunCommand(env, temp, nullptr);
    napi_value pid = childprocess->Getpid(env);
    napi_value signal = nullptr;
    napi_create_int32(env, 9, &signal);
    napi_value result = processManager.Kill(env, pid, signal);
    childprocess->Close();
    bool res = false;
    napi_get_value_bool(env, result, &res);
    ASSERT_FALSE(res);
}

/**
 * @tc.name: ProcessManagerKillTest002
 * @tc.desc: Test processManager kill with an invalid signal.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessManagerKillTest002, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::ProcessManager processManager;

    std::string command("sleep 60");
    napi_value temp = nullptr;
    napi_create_string_utf8(env, command.c_str(), command.length(), &temp);

    OHOS::JsSysModule::Process::ChildProcess* childprocess = RunCommand(env, temp, nullptr);
    napi_value pid = childprocess->Getpid(env);
    napi_value signal = nullptr;
    napi_create_int32(env, 999, &signal);
    // The signal 999 is invalid, uv_kill fails and returns false.
    napi_value result = processManager.Kill(env, signal, pid);
    childprocess->Close();
    bool res = false;
    napi_get_value_bool(env, result, &res);
    ASSERT_FALSE(res);
}

static void MakeChildOptions(napi_env env, int32_t timeout, int32_t killSignal, int64_t maxBuffer,
    napi_value &options)
{
    options = nullptr;
    napi_create_object(env, &options);
    if (timeout >= 0) {
        napi_value value = nullptr;
        napi_create_int32(env, timeout, &value);
        napi_set_named_property(env, options, "timeout", value);
    }
    if (killSignal != 0) {
        napi_value value = nullptr;
        napi_create_int32(env, killSignal, &value);
        napi_set_named_property(env, options, "killSignal", value);
    }
    if (maxBuffer >= 0) {
        napi_value value = nullptr;
        napi_create_int64(env, maxBuffer, &value);
        napi_set_named_property(env, options, "maxBuffer", value);
    }
}

static napi_value MakeCmd(napi_env env, const std::string &command)
{
    napi_value value = nullptr;
    napi_create_string_utf8(env, command.c_str(), command.length(), &value);
    return value;
}

static int32_t ChildPidToInt(napi_env env, napi_value pidValue)
{
    int32_t pid = -1;
    napi_get_value_int32(env, pidValue, &pid);
    return pid;
}

static bool ChildBoolValue(napi_env env, napi_value value)
{
    bool result = false;
    napi_get_value_bool(env, value, &result);
    return result;
}

static int32_t ChildInt32Value(napi_env env, napi_value value)
{
    int32_t result = -1;
    napi_get_value_int32(env, value, &result);
    return result;
}

/**
 * @tc.name: ProcessChildPidGuardTest001
 * @tc.desc: Without Spawn every pid_ guard branch is taken: Getpid/Wait return undefined,
 *           Kill does not set killed_ and Close does nothing.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessChildPidGuardTest001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::ChildProcess* childprocess = new OHOS::JsSysModule::Process::ChildProcess();
    napi_value signal = nullptr;
    napi_create_int32(env, SIGTERM, &signal);

    napi_value pid = childprocess->Getpid(env);
    ASSERT_CHECK_VALUE_TYPE(env, pid, napi_undefined);
    napi_value waitResult = childprocess->Wait(env);
    ASSERT_CHECK_VALUE_TYPE(env, waitResult, napi_undefined);
    childprocess->Kill(env, signal);
    ASSERT_FALSE(ChildBoolValue(env, childprocess->GetKilled(env)));
    childprocess->Close();
}

/**
 * @tc.name: ProcessChildPidTest001
 * @tc.desc: Spawn copies the child pid into pid_ and Getpid reports it.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessChildPidTest001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::ChildProcess* childprocess = RunCommand(env, MakeCmd(env, "echo branchpid"),
        nullptr);
    napi_value pid = childprocess->Getpid(env);
    ASSERT_CHECK_VALUE_TYPE(env, pid, napi_number);
    ASSERT_GT(ChildPidToInt(env, pid), 0);
    childprocess->Wait(env);
    delete childprocess;
}

/**
 * @tc.name: ProcessChildWaitTest001
 * @tc.desc: Wait resolves the promise and records exit code 0 for a finished command.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessChildWaitTest001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::ChildProcess* childprocess = RunCommand(env, MakeCmd(env, "echo hello"),
        nullptr);
    napi_value result = childprocess->Wait(env);
    bool isPromise = false;
    napi_is_promise(env, result, &isPromise);
    ASSERT_TRUE(isPromise);
    ASSERT_EQ(ChildInt32Value(env, childprocess->GetExitCode(env)), 0);
    delete childprocess;
}

/**
 * @tc.name: ProcessChildWaitTest002
 * @tc.desc: A second Wait skips waitpid (isWait_ is already false) and keeps the exit code.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessChildWaitTest002, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::ChildProcess* childprocess = RunCommand(env, MakeCmd(env, "echo hello"),
        nullptr);
    childprocess->Wait(env);
    napi_value again = childprocess->Wait(env);
    bool isPromise = false;
    napi_is_promise(env, again, &isPromise);
    ASSERT_TRUE(isPromise);
    ASSERT_EQ(ChildInt32Value(env, childprocess->GetExitCode(env)), 0);
    delete childprocess;
}

/**
 * @tc.name: ProcessChildKillTest001
 * @tc.desc: Kill with a terminating signal stops the child and marks killed_.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessChildKillTest001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::ChildProcess* childprocess = RunCommand(env, MakeCmd(env, "sleep 5"), nullptr);
    napi_value signal = nullptr;
    napi_create_int32(env, SIGTERM, &signal);
    childprocess->Kill(env, signal);
    ASSERT_TRUE(ChildBoolValue(env, childprocess->GetKilled(env)));
    childprocess->Wait(env);
    ASSERT_EQ(ChildInt32Value(env, childprocess->GetExitCode(env)), SIGTERM);
    delete childprocess;
}

/**
 * @tc.name: ProcessChildKillTest002
 * @tc.desc: Kill with a non-terminating signal reports killed_ but leaves a trapping child
 *           alive and keeps the reader flag set.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessChildKillTest002, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::ChildProcess* childprocess = RunCommand(env, MakeCmd(env,
        "trap '' USR1; sleep 5"), nullptr);
    usleep(200000); // 200000: 200 ms: give the trap time to be installed
    napi_value signal = nullptr;
    napi_create_int32(env, SIGUSR1, &signal);
    childprocess->Kill(env, signal);
    ASSERT_TRUE(ChildBoolValue(env, childprocess->GetKilled(env)));
    int32_t pid = ChildPidToInt(env, childprocess->Getpid(env));
    ASSERT_GT(pid, 0);
    ASSERT_EQ(kill(pid, 0), 0);
    childprocess->Close();
    childprocess->Wait(env);
    ASSERT_EQ(ChildInt32Value(env, childprocess->GetExitCode(env)), SIGKILL);
    delete childprocess;
}

/**
 * @tc.name: ProcessChildKillTest003
 * @tc.desc: Kill on an already reaped child fails and does not set killed_.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessChildKillTest003, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::ChildProcess* childprocess = RunCommand(env, MakeCmd(env, "echo hi"), nullptr);
    childprocess->Wait(env);
    usleep(100000); // 100000: 100 ms
    napi_value signal = nullptr;
    napi_create_int32(env, SIGTERM, &signal);
    childprocess->Kill(env, signal);
    ASSERT_FALSE(ChildBoolValue(env, childprocess->GetKilled(env)));
    delete childprocess;
}

/**
 * @tc.name: ProcessChildCloseTest001
 * @tc.desc: Close kills a running child with SIGKILL, records the exit code and a follow-up
 *           Wait skips waitpid because isWait_ became false.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessChildCloseTest001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::ChildProcess* childprocess = RunCommand(env, MakeCmd(env, "sleep 5"), nullptr);
    childprocess->Close();
    ASSERT_EQ(ChildInt32Value(env, childprocess->GetExitCode(env)), SIGKILL);
    ASSERT_FALSE(ChildBoolValue(env, childprocess->GetKilled(env)));
    napi_value result = childprocess->Wait(env);
    bool isPromise = false;
    napi_is_promise(env, result, &isPromise);
    ASSERT_TRUE(isPromise);
    ASSERT_EQ(ChildInt32Value(env, childprocess->GetExitCode(env)), SIGKILL);
    delete childprocess;
}

/**
 * @tc.name: ProcessChildCloseTest002
 * @tc.desc: Close on an already exited child skips the kill (WNOHANG reaps it first) and the
 *           destructor does not block on the reaped pid.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessChildCloseTest002, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::ChildProcess* childprocess = RunCommand(env, MakeCmd(env, "echo hi"), nullptr);
    usleep(300000); // 300000: 300 ms: let the child exit
    childprocess->Close();
    napi_value result = childprocess->Wait(env);
    bool isPromise = false;
    napi_is_promise(env, result, &isPromise);
    ASSERT_TRUE(isPromise);
    delete childprocess;
}

/**
 * @tc.name: ProcessChildTimeoutTest001
 * @tc.desc: timeout <= 0 makes the timeout listener return immediately: the child keeps
 *           running.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessChildTimeoutTest001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    napi_value options = nullptr;
    MakeChildOptions(env, 0, 0, -1, options);
    OHOS::JsSysModule::Process::ChildProcess* childprocess = RunCommand(env, MakeCmd(env, "sleep 5"), options);
    usleep(500000); // 500000: 500 ms
    int32_t pid = ChildPidToInt(env, childprocess->Getpid(env));
    ASSERT_EQ(kill(pid, 0), 0);
    childprocess->Close();
    childprocess->Wait(env);
    ASSERT_EQ(ChildInt32Value(env, childprocess->GetExitCode(env)), SIGKILL);
    delete childprocess;
}

/**
 * @tc.name: ProcessChildTimeoutTest003
 * @tc.desc: A non-terminating timeout signal leaves a trapping child alive (the reader flag
 *           is not cleared).
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessChildTimeoutTest003, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    napi_value options = nullptr;
    MakeChildOptions(env, 300, SIGUSR1, -1, options); // 300: 300 ms
    OHOS::JsSysModule::Process::ChildProcess* childprocess = RunCommand(env, MakeCmd(env,
        "trap '' USR1; sleep 5"), options);
    usleep(800000); // 800000: 800 ms > 300 ms timeout
    int32_t pid = ChildPidToInt(env, childprocess->Getpid(env));
    ASSERT_EQ(kill(pid, 0), 0);
    childprocess->Close();
    childprocess->Wait(env);
    ASSERT_EQ(ChildInt32Value(env, childprocess->GetExitCode(env)), SIGKILL);
    delete childprocess;
}

/**
 * @tc.name: ProcessChildTimeoutTest004
 * @tc.desc: The timeout kill fails on a reaped child and only logs (Close reaped the child
 *           while the reader flag is still set).
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessChildTimeoutTest004, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    napi_value options = nullptr;
    MakeChildOptions(env, 500, SIGTERM, -1, options); // 500: 500 ms
    OHOS::JsSysModule::Process::ChildProcess* childprocess = RunCommand(env, MakeCmd(env, "echo hi"), options);
    usleep(300000); // 300000: 300 ms
    childprocess->Close();
    usleep(400000); // 400000: 400 ms
    napi_value result = childprocess->Wait(env);
    bool isPromise = false;
    napi_is_promise(env, result, &isPromise);
    ASSERT_TRUE(isPromise);
    delete childprocess;
}

/**
 * @tc.name: ProcessChildStdioTest001
 * @tc.desc: Stderr output goes through the dataMutex snapshot path of GetErrorOutput.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessChildStdioTest001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::ChildProcess* childprocess = RunCommand(env, MakeCmd(env, "echo brancherr >&2"),
        nullptr);
    napi_value errorOutput = childprocess->GetErrorOutput(env);
    bool isPromise = false;
    napi_is_promise(env, errorOutput, &isPromise);
    ASSERT_TRUE(isPromise);
    childprocess->Wait(env);
    ASSERT_EQ(ChildInt32Value(env, childprocess->GetExitCode(env)), 0);
    delete childprocess;
}

/**
 * @tc.name: ProcessChildMaxBufferTest001
 * @tc.desc: Stdout beyond maxBuffer overflows the reader thread: the child is killed with
 *           SIGKILL and the snapshot returns the truncated data.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessChildMaxBufferTest001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    napi_value options = nullptr;
    MakeChildOptions(env, -1, 0, 1024, options); // timeout absent, maxBuffer 1024
    OHOS::JsSysModule::Process::ChildProcess* childprocess = RunCommand(env, MakeCmd(env,
        "dd if=/dev/zero bs=1024 count=4096"), options);
    childprocess->Wait(env);
    ASSERT_EQ(ChildInt32Value(env, childprocess->GetExitCode(env)), SIGKILL);
    napi_value output = childprocess->GetOutput(env);
    bool isPromise = false;
    napi_is_promise(env, output, &isPromise);
    ASSERT_TRUE(isPromise);
    delete childprocess;
}

/**
 * @tc.name: ProcessChildOutputNulTest001
 * @tc.desc: NUL bytes in the child output are appended by the exact read size (binary-safe
 *           append branch).
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessChildOutputNulTest001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::ChildProcess* childprocess = RunCommand(env, MakeCmd(env,
        "dd if=/dev/zero bs=1 count=8 2>/dev/null"), nullptr);
    napi_value output = childprocess->GetOutput(env);
    bool isPromise = false;
    napi_is_promise(env, output, &isPromise);
    ASSERT_TRUE(isPromise);
    childprocess->Wait(env);
    ASSERT_EQ(ChildInt32Value(env, childprocess->GetExitCode(env)), 0);
    delete childprocess;
}

/**
 * @tc.name: ProcessChildTimeoutWorkCreateFailTest001
 * @tc.desc: Spawn creates the two reader works first, then fails the
 *           timeout work creation, so the else branch runs and optionsInfo_ keeps ownership.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessChildTimeoutWorkCreateFailTest001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    napi_value options = nullptr;
    MakeChildOptions(env, 500, SIGTERM, -1, options); // 500: 500ms
    OHOS::JsSysModule::Process::ChildProcess* childprocess =
        new OHOS::JsSysModule::Process::ChildProcess();
    childprocess->InitOptionsInfo(env, options);
    g_trackWorkHandles = 1;
    g_injectCreateAsyncWorkOrdinal = 3; // 3: two reader works succeed, the timeout work fails
    childprocess->Spawn(env, MakeCmd(env, "sleep 2"));
    int32_t pid = ChildPidToInt(env, childprocess->Getpid(env));
    childprocess->Wait(env);
    ASSERT_GT(pid, 0);
    delete childprocess;
}

/**
 * @tc.name: ProcessChildGetOutputMemFailTest001
 * @tc.desc: Memcpy_s fails inside GetOutput, the error branch returns undefined after deleting the async work.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessChildGetOutputMemFailTest001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::ChildProcess* childprocess = RunCommand(env, MakeCmd(env,
        "echo branchfail"), nullptr);
    usleep(300000); // 300000: 300 ms
    g_trackWorkHandles = 1;
    g_injectMemcallFail = 1;
    napi_value failedOutput = childprocess->GetOutput(env);
    ASSERT_CHECK_VALUE_TYPE(env, failedOutput, napi_undefined);
    childprocess->Wait(env);
    delete childprocess;
}

/**
 * @tc.name: ProcessChildGetErrorOutputMemFailTest001
 * @tc.desc: Memcpy_s fails inside GetErrorOutput, the error branch returns undefined after deleting the async work.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessChildGetErrorOutputMemFailTest001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::ChildProcess* childprocess = RunCommand(env, MakeCmd(env,
        "echo branchfail >&2"), nullptr);
    usleep(300000); // 300000: 300 ms
    g_trackWorkHandles = 1;
    g_injectMemcallFail = 1;
    napi_value failedError = childprocess->GetErrorOutput(env);
    ASSERT_CHECK_VALUE_TYPE(env, failedError, napi_undefined);
    childprocess->Wait(env);
    delete childprocess;
}

/**
 * @tc.name: ProcessChildGetOutputArraybufferFailTest001
 * @tc.desc: The NAPI_CALL macro returns nullptr.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessChildGetOutputArraybufferFailTest001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::ChildProcess* childprocess = RunCommand(env, MakeCmd(env,
        "echo branchab"), nullptr);
    usleep(300000); // 300000: 300 ms
    g_injectCreateArraybufferFail = 1;
    napi_value output = childprocess->GetOutput(env);
    ASSERT_TRUE(output == nullptr);
    childprocess->Wait(env);
    delete childprocess;
}

/**
 * @tc.name: ProcessChildGetErrorOutputArraybufferFailTest001
 * @tc.desc: The NAPI_CALL macro returns nullptr.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessChildGetErrorOutputArraybufferFailTest001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::ChildProcess* childprocess = RunCommand(env, MakeCmd(env,
        "echo branchaberr >&2"), nullptr);
    usleep(300000); // 300000: 300 ms
    g_injectCreateArraybufferFail = 1;
    napi_value errorOutput = childprocess->GetErrorOutput(env);
    ASSERT_TRUE(errorOutput == nullptr);
    childprocess->Wait(env);
    delete childprocess;
}

/**
 * @tc.name: ProcessChildTimeoutWorkQueueFailTest001
 * @tc.desc: The timeout work was created (worker != nullptr), the queue call fails, so
 *           the else branch deletes the work and optionsInfo_ keeps ownership.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessChildTimeoutWorkQueueFailTest001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    napi_value options = nullptr;
    MakeChildOptions(env, 500, SIGTERM, -1, options); // 500: 500 ms
    OHOS::JsSysModule::Process::ChildProcess* childprocess =
        new OHOS::JsSysModule::Process::ChildProcess();
    childprocess->InitOptionsInfo(env, options);
    g_injectQueueWorkQosOrdinal = 3; // 3: two reader queues succeed, the timeout queue fails
    childprocess->Spawn(env, MakeCmd(env, "sleep 2"));
    usleep(700000); // 700000: 700 ms
    int32_t pid = ChildPidToInt(env, childprocess->Getpid(env));
    childprocess->Wait(env);
    ASSERT_GT(pid, 0);
    delete childprocess;
}

/**
 * @tc.name: ProcessChildFirstPipeFailTest001
 * @tc.desc: Together with the destructor path.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessChildFirstPipeFailTest001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ASSERT_TRUE(ExhaustFdTable(0));
    OHOS::JsSysModule::Process::ChildProcess* childprocess =
        new OHOS::JsSysModule::Process::ChildProcess();
    childprocess->InitOptionsInfo(env, nullptr);
    childprocess->Spawn(env, MakeCmd(env, "echo nofd"));
    napi_value pid = childprocess->Getpid(env);
    ASSERT_CHECK_VALUE_TYPE(env, pid, napi_undefined);
    delete childprocess;
    RestoreFdTable();
}

/**
 * @tc.name: ProcessChildSecondPipeFailTest001
 * @tc.desc: The first pipe succeeded, the second one fails, no fork happens and the destructor
 *           closes the leaked read end.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessChildSecondPipeFailTest001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ASSERT_TRUE(ExhaustFdTable(2)); // 2: leave two slots so only the first pipe succeeds
    OHOS::JsSysModule::Process::ChildProcess* childprocess =
        new OHOS::JsSysModule::Process::ChildProcess();
    childprocess->InitOptionsInfo(env, nullptr);
    childprocess->Spawn(env, MakeCmd(env, "echo halffd"));
    napi_value pid = childprocess->Getpid(env);
    ASSERT_CHECK_VALUE_TYPE(env, pid, napi_undefined);
    delete childprocess;
    RestoreFdTable();
}

/**
 * @tc.name: ProcessChildDestructorWaitBranchTest001
 * @tc.desc: A ChildProcess is destroyed while the child is still running and isWait_ is true.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessChildDestructorWaitBranchTest001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::ChildProcess* childprocess = RunCommand(env, MakeCmd(env, "sleep 2"),
        nullptr);
    int32_t pid = ChildPidToInt(env, childprocess->Getpid(env));
    ASSERT_GT(pid, 0);
    usleep(300000); // 300000: 300 ms
    ASSERT_EQ(kill(pid, 0), 0);
    delete childprocess;
}

/**
 * @tc.name: ProcessChildCloseKillFailTest001
 * @tc.desc: Kill fails inside Close after the WNOHANG check passed.
 * @tc.type: FUNC
 */
HWTEST_F(NativeEngineTest, ProcessChildCloseKillFailTest001, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    OHOS::JsSysModule::Process::ChildProcess* childprocess = RunCommand(env, MakeCmd(env, "sleep 5"),
        nullptr);
    usleep(300000); // 300000: 300 ms
    int32_t pid = ChildPidToInt(env, childprocess->Getpid(env));
    ASSERT_GT(pid, 0);
    g_injectKillFailPid = pid;
    childprocess->Close();
    ASSERT_TRUE(ChildBoolValue(env, childprocess->GetKilled(env)) == false);
    ASSERT_EQ(kill(pid, 0), 0);
    childprocess->Close();
    childprocess->Wait(env);
    ASSERT_EQ(ChildInt32Value(env, childprocess->GetExitCode(env)), SIGKILL);
    delete childprocess;
}