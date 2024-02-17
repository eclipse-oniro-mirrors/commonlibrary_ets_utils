/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef JS_CONCURRENT_MODULE_TASKPOOL_TASK_H
#define JS_CONCURRENT_MODULE_TASKPOOL_TASK_H

#include <list>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <uv.h>

#include "helper/concurrent_helper.h"
#include "napi/native_api.h"
#include "utils.h"
#include "utils/log.h"

namespace Commonlibrary::Concurrent::TaskPoolModule {
using namespace Commonlibrary::Platform;

enum ExecuteState { NOT_FOUND, WAITING, RUNNING, CANCELED };
enum TaskType { TASK, FUNCTION_TASK, SEQRUNNER_TASK, COMMON_TASK, GROUP_COMMON_TASK, GROUP_FUNCTION_TASK };

struct GroupInfo;
struct TaskInfo {
    napi_deferred deferred = nullptr;
    Priority priority {Priority::DEFAULT};
    napi_value serializationFunction = nullptr;
    napi_value serializationArguments = nullptr;
};

class Task {
public:
    Task(napi_env env, TaskType taskType, std::string name);
    Task() = default;
    ~Task() = default;

    static napi_value TaskConstructor(napi_env env, napi_callback_info cbinfo);
    static napi_value SetTransferList(napi_env env, napi_callback_info cbinfo);
    static napi_value SetCloneList(napi_env env, napi_callback_info cbinfo);
    static napi_value IsCanceled(napi_env env, napi_callback_info cbinfo);
    static napi_value OnReceiveData(napi_env env, napi_callback_info cbinfo);
    static napi_value SendData(napi_env env, napi_callback_info cbinfo);
    static napi_value AddDependency(napi_env env, napi_callback_info cbinfo);
    static napi_value RemoveDependency(napi_env env, napi_callback_info cbinfo);
    static napi_value GetTotalDuration(napi_env env, napi_callback_info info);
    static napi_value GetCPUDuration(napi_env env, napi_callback_info info);
    static napi_value GetIODuration(napi_env env, napi_callback_info info);
    static napi_value GetTaskDuration(napi_env env, napi_callback_info& info, std::string durationType);

    static Task* GenerateTask(napi_env env, napi_value task, napi_value func,
                              napi_value name, napi_value* args, size_t argc);
    static Task* GenerateFunctionTask(napi_env env, napi_value func, napi_value* args, size_t argc, TaskType type);
    static TaskInfo* GenerateTaskInfo(napi_env env, napi_value func, napi_value args,
                                      napi_value transferList, napi_value cloneList, Priority priority,
                                      bool defaultTransfer = true, bool defaultCloneSendable = false);
    static void TaskDestructor(napi_env env, void* data, void* hint);

    static void IncreaseTaskRef(const uv_async_t* req);

    static void ThrowNoDependencyError(napi_env env);

    void StoreTaskId(uint64_t taskId);
    napi_value GetTaskInfoPromise(napi_env env, napi_value task, TaskType taskType = TaskType::COMMON_TASK,
                                  Priority priority = Priority::DEFAULT);
    TaskInfo* GetTaskInfo(napi_env env, napi_value task, TaskType taskType, Priority priority);
    bool IsRepeatableTask();
    bool IsGroupTask();
    bool IsGroupCommonTask();
    bool IsGroupFunctionTask();
    bool IsCommonTask();
    bool IsSeqRunnerTask();
    bool IsFunctionTask();
    bool IsInitialized();
    void IncreaseRefCount();
    void DecreaseRefCount();
    bool IsReadyToHandle();
    void NotifyPendingTask();
    void CancelPendingTask(napi_env env, ExecuteState state);
    bool UpdateTask(uint64_t startTime, void* worker);
    napi_value DeserializeValue(napi_env env, bool isFunc, bool isArgs);
    void StoreTaskDuration();
    bool CanForSequenceRunner(napi_env env);
    bool CanForTaskGroup(napi_env env);
    bool CanExecute(napi_env env);
    bool CanExecuteDelayed(napi_env env);
    void SetHasDependency(bool hasDependency);
    bool HasDependency() const;
    void TryClearHasDependency();

private:
    Task(const Task &) = delete;
    Task& operator=(const Task &) = delete;
    Task(Task &&) = delete;
    Task& operator=(Task &&) = delete;

public:
    napi_env env_ = nullptr;
    TaskType taskType_ {TaskType::TASK};
    std::string name_ {};
    uint64_t taskId_ {};
    std::atomic<ExecuteState> taskState_ {ExecuteState::NOT_FOUND};
    uint64_t groupId_ {}; // 0 for task outside taskgroup
    uint64_t seqRunnerId_ {}; // 0 for task without seqRunner
    TaskInfo* currentTaskInfo_ {};
    std::list<TaskInfo*> pendingTaskInfos_ {}; // for a common task executes multiple times
    napi_value result_ = nullptr;
    uv_async_t* onResultSignal_ = nullptr;
    uv_async_t* increaseRefSignal_ = nullptr;
    std::atomic<bool> success_ {true};
    uint64_t startTime_ {};
    uint64_t cpuTime_ {};
    uint64_t ioTime_ {};
    void* worker_ {nullptr};
    napi_ref taskRef_ {};
    std::atomic<uint32_t> taskRefCount_ {};
    std::shared_mutex taskMutex_ {};
    bool hasDependency_ {false};
};

struct CallbackInfo {
    CallbackInfo(napi_env env, uint32_t count, napi_ref ref)
        : hostEnv(env), refCount(count), callbackRef(ref), onCallbackSignal(nullptr) {}
    ~CallbackInfo()
    {
        napi_delete_reference(hostEnv, callbackRef);
        if (onCallbackSignal != nullptr) {
            Common::Helper::ConcurrentHelper::UvHandleClose(onCallbackSignal);
        }
    }

    napi_env hostEnv;
    uint32_t refCount;
    napi_ref callbackRef;
    uv_async_t* onCallbackSignal;
};

struct TaskResultInfo {
    TaskResultInfo(napi_env env, uint64_t id, napi_value args) : hostEnv(env),
        taskId(id), serializationArgs(args) {}
    ~TaskResultInfo() = default;

    napi_env hostEnv;
    uint64_t taskId;
    napi_value serializationArgs;
};
} // namespace Commonlibrary::Concurrent::TaskPoolModule
#endif // JS_CONCURRENT_MODULE_TASKPOOL_TASK_H