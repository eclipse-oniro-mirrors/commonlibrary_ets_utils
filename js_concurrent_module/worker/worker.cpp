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

#include "worker.h"

#include <unordered_map>

#include "sys_timer.h"
#include "helper/concurrent_helper.h"
#include "helper/error_helper.h"
#include "helper/hitrace_helper.h"
#include "helper/path_helper.h"
#if defined(OHOS_PLATFORM)
#include "parameters.h"
#endif
#ifdef ENABLE_QOS
#include "qos.h"
#endif
#include "native_engine.h"

namespace Commonlibrary::Concurrent::WorkerModule {
using namespace OHOS::JsSysModule;
static constexpr int8_t NUM_WORKER_ARGS = 2;
static constexpr uint8_t NUM_GLOBAL_CALL_ARGS = 3;
static std::list<Worker *> g_workers;
static std::mutex g_workersMutex;
static std::list<Worker *> g_limitedworkers;
static std::mutex g_limitedworkersMutex;
static constexpr uint32_t WORKER_TYPE = 1;
static constexpr uint8_t BEGIN_INDEX_OF_ARGUMENTS = 2;
static constexpr uint32_t DEFAULT_TIMEOUT = 5000;
static constexpr uint32_t GLOBAL_CALL_ID_MAX = 4294967295;
static constexpr size_t GLOBAL_CALL_MAX_COUNT = 65535;
static constexpr uint32_t THREAD_NAME_MAX_LENGTH = 15;

#ifdef ENABLE_QOS
static const std::unordered_map<WorkerPriority, OHOS::QOS::QosLevel> WORKERPRIORITY_QOSLEVEL_MAP = {
    {WorkerPriority::HIGH, OHOS::QOS::QosLevel::QOS_USER_INITIATED},
    {WorkerPriority::MEDIUM, OHOS::QOS::QosLevel::QOS_DEFAULT},
    {WorkerPriority::LOW, OHOS::QOS::QosLevel::QOS_UTILITY},
    {WorkerPriority::IDLE, OHOS::QOS::QosLevel::QOS_BACKGROUND},
    {WorkerPriority::DEADLINE, OHOS::QOS::QosLevel::QOS_DEADLINE_REQUEST},
    {WorkerPriority::VIP, OHOS::QOS::QosLevel::QOS_USER_INTERACTIVE},
};
#endif

#if defined(ENABLE_WORKER_EVENTHANDLER)
std::shared_ptr<OHOS::AppExecFwk::EventHandler> Worker::GetMainThreadHandler()
{
    static std::shared_ptr<OHOS::AppExecFwk::EventHandler> mainThreadHandler;
    static std::mutex mainThreadHandlerMutex;
    if (mainThreadHandler == nullptr) {
        std::lock_guard<std::mutex> lock(mainThreadHandlerMutex);
        if (mainThreadHandler == nullptr) {
            mainThreadHandler = std::make_shared<OHOS::AppExecFwk::EventHandler>(
                OHOS::AppExecFwk::EventRunner::GetMainEventRunner());
        }
    }
    return mainThreadHandler;
}
#endif

Worker::Worker(napi_env env, napi_ref thisVar)
    : hostEnv_(env), workerRef_(thisVar)
{
    workerWrapper_ = std::make_shared<WorkerWrapper>(this);
}

napi_value Worker::InitWorker(napi_env env, napi_value exports)
{
    HITRACE_HELPER_METER_NAME(__PRETTY_FUNCTION__);
    napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("postMessage", PostMessage),
        DECLARE_NAPI_FUNCTION("postMessageWithSharedSendable", PostMessageWithSharedSendable),
        DECLARE_NAPI_FUNCTION("terminate", Terminate),
        DECLARE_NAPI_FUNCTION("on", On),
        DECLARE_NAPI_FUNCTION("registerGlobalCallObject", RegisterGlobalCallObject),
        DECLARE_NAPI_FUNCTION("unregisterGlobalCallObject", UnregisterGlobalCallObject),
        DECLARE_NAPI_FUNCTION("once", Once),
        DECLARE_NAPI_FUNCTION("off", Off),
        DECLARE_NAPI_FUNCTION("addEventListener", AddEventListener),
        DECLARE_NAPI_FUNCTION("dispatchEvent", DispatchEvent),
        DECLARE_NAPI_FUNCTION("removeEventListener", RemoveEventListener),
        DECLARE_NAPI_FUNCTION("removeAllListener", RemoveAllListener),
        DECLARE_NAPI_FUNCTION("cancelTasks", CancelTask),
    };
    // for worker.ThreadWorker
    const char threadWorkerName[] = "ThreadWorker";
    napi_value threadWorkerClazz = nullptr;
    napi_define_class(env, threadWorkerName, sizeof(threadWorkerName), Worker::ThreadWorkerConstructor, nullptr,
        sizeof(properties) / sizeof(properties[0]), properties, &threadWorkerClazz);
    napi_set_named_property(env, exports, "ThreadWorker", threadWorkerClazz);

    // for worker.Worker
    const char workerName[] = "Worker";
    napi_value workerClazz = nullptr;
    napi_define_class(env, workerName, sizeof(workerName), Worker::WorkerConstructor, nullptr,
        sizeof(properties) / sizeof(properties[0]), properties, &workerClazz);
    napi_set_named_property(env, exports, "Worker", workerClazz);

    // for worker.LimitedWorker
    const char limitedWorkerName[] = "RestrictedWorker";
    napi_value limitedWorkerClazz = nullptr;
    napi_define_class(env, limitedWorkerName, sizeof(limitedWorkerName), Worker::LimitedWorkerConstructor, nullptr,
        sizeof(properties) / sizeof(properties[0]), properties, &limitedWorkerClazz);
    napi_set_named_property(env, exports, "RestrictedWorker", limitedWorkerClazz);

    InitPriorityObject(env, exports);

#if defined(ENABLE_CONCURRENCY_INTEROP)
    if (reinterpret_cast<NativeEngine*>(env)->IsMainThread() && ANIHelper::GetAniVm() == nullptr) {
        HILOG_INFO("worker:: get aniVm is null in main thread.");
    }
#endif

    return InitPort(env, exports);
}

void Worker::InitPriorityObject(napi_env env, napi_value exports)
{
    napi_value priorityObj = NapiHelper::CreateObject(env);

    napi_value highPriority = NapiHelper::CreateUint32(env, static_cast<uint32_t>(WorkerPriority::HIGH));
    napi_value mediumPriority = NapiHelper::CreateUint32(env, static_cast<uint32_t>(WorkerPriority::MEDIUM));
    napi_value lowPriority = NapiHelper::CreateUint32(env, static_cast<uint32_t>(WorkerPriority::LOW));
    napi_value idlePriority = NapiHelper::CreateUint32(env, static_cast<uint32_t>(WorkerPriority::IDLE));
    napi_property_descriptor exportPriority[] = {
        DECLARE_NAPI_PROPERTY("HIGH", highPriority),
        DECLARE_NAPI_PROPERTY("MEDIUM", mediumPriority),
        DECLARE_NAPI_PROPERTY("LOW", lowPriority),
        DECLARE_NAPI_PROPERTY("IDLE", idlePriority),
    };
    napi_define_properties(env, priorityObj, sizeof(exportPriority) / sizeof(exportPriority[0]), exportPriority);

    napi_property_descriptor exportObjs[] = {
        DECLARE_NAPI_PROPERTY("ThreadWorkerPriority", priorityObj),
    };
    napi_define_properties(env, exports, sizeof(exportObjs) / sizeof(exportObjs[0]), exportObjs);
}

napi_value Worker::InitPort(napi_env env, napi_value exports)
{
    NativeEngine* engine = reinterpret_cast<NativeEngine*>(env);
    Worker* worker = nullptr;
    if (engine->IsRestrictedWorkerThread()) {
        std::lock_guard<std::mutex> lock(g_limitedworkersMutex);
        for (auto item = g_limitedworkers.begin(); item != g_limitedworkers.end(); item++) {
            if ((*item)->IsSameWorkerEnv(env)) {
                worker = *item;
            }
        }
    } else if (engine->IsWorkerThread()) {
        std::lock_guard<std::mutex> lock(g_workersMutex);
        for (auto item = g_workers.begin(); item != g_workers.end(); item++) {
            if ((*item)->IsSameWorkerEnv(env)) {
                worker = *item;
            }
        }
    } else {
        return exports;
    }

    if (worker == nullptr) {
        ErrorHelper::ThrowError(env, ErrorHelper::ERR_WORKER_NOT_RUNNING, "worker is null when InitWorker");
        return exports;
    }

    napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION_WITH_DATA("postMessage", PostMessageToHost, worker),
        DECLARE_NAPI_FUNCTION_WITH_DATA("postMessageWithSharedSendable", PostMessageWithSharedSendableToHost, worker),
        DECLARE_NAPI_FUNCTION_WITH_DATA("callGlobalCallObjectMethod", GlobalCall, worker),
        DECLARE_NAPI_FUNCTION_WITH_DATA("close", CloseWorker, worker),
        DECLARE_NAPI_FUNCTION_WITH_DATA("cancelTasks", ParentPortCancelTask, worker),
        DECLARE_NAPI_FUNCTION_WITH_DATA("addEventListener", ParentPortAddEventListener, worker),
        DECLARE_NAPI_FUNCTION_WITH_DATA("dispatchEvent", ParentPortDispatchEvent, worker),
        DECLARE_NAPI_FUNCTION_WITH_DATA("removeEventListener", ParentPortRemoveEventListener, worker),
        DECLARE_NAPI_FUNCTION_WITH_DATA("removeAllListener", ParentPortRemoveAllListener, worker),
    };
    napi_value workerPortObj = nullptr;
    napi_create_object(env, &workerPortObj);
    napi_define_properties(env, workerPortObj, sizeof(properties) / sizeof(properties[0]), properties);

    // 5. register worker name in DedicatedWorkerGlobalScope
    std::string name = worker->GetName();
    if (!name.empty()) {
        napi_value nameValue = nullptr;
        napi_create_string_utf8(env, name.c_str(), name.length(), &nameValue);
        napi_set_named_property(env, workerPortObj, "name", nameValue);
    }

    napi_set_named_property(env, workerPortObj, "self", workerPortObj);

    if (worker->isNewVersion_) {
        napi_set_named_property(env, exports, "workerPort", workerPortObj);
    } else {
        napi_set_named_property(env, exports, "parentPort", workerPortObj);
    }
    // register worker Port.
    napi_create_reference(env, workerPortObj, 1, &worker->workerPort_);
#if defined(ENABLE_WORKER_EVENTHANDLER)
    GetMainThreadHandler();
#endif
    return exports;
}

napi_value Worker::LimitedWorkerConstructor(napi_env env, napi_callback_info cbinfo)
{
    if (CanCreateWorker(env, WorkerVersion::NEW)) {
        return Constructor(env, cbinfo, true);
    }
    HILOG_ERROR("worker:: using both Worker and LimitedWorker is not supported");
    ErrorHelper::ThrowError(env, ErrorHelper::ERR_WORKER_INITIALIZATION,
        "Using both Worker and LimitedWorker is not supported.");
    return nullptr;
}

napi_value Worker::ThreadWorkerConstructor(napi_env env, napi_callback_info cbinfo)
{
    HITRACE_HELPER_METER_NAME("ThreadWorkerConstructor: [Add Thread]");
    if (CanCreateWorker(env, WorkerVersion::NEW)) {
        return Constructor(env, cbinfo, false, WorkerVersion::NEW);
    }
    HILOG_ERROR("worker:: ThreadWorker construct failed");
    ErrorHelper::ThrowError(env, ErrorHelper::ERR_WORKER_INITIALIZATION,
        "Using both Worker and ThreadWorker is not supported.");
    return nullptr;
}

napi_value Worker::WorkerConstructor(napi_env env, napi_callback_info cbinfo)
{
    HITRACE_HELPER_METER_NAME("WorkerConstructor: [Add Thread]");
    if (CanCreateWorker(env, WorkerVersion::OLD)) {
        return Constructor(env, cbinfo, false, WorkerVersion::OLD);
    }
    HILOG_ERROR("worker:: using both Worker and other Workers is not supported");
    ErrorHelper::ThrowError(env, ErrorHelper::ERR_WORKER_INITIALIZATION,
        "Using both Worker and other Workers is not supported.");
    return nullptr;
}

napi_value Worker::Constructor(napi_env env, napi_callback_info cbinfo, bool limitSign, WorkerVersion version)
{
    napi_value thisVar = nullptr;
    void* data = nullptr;
    size_t argc = 2;  // 2: max args number is 2
    napi_value args[argc];
    napi_get_cb_info(env, cbinfo, &argc, args, &thisVar, &data);
    // check argv count
    if (argc < 1) {
        HILOG_ERROR("worker:: the number of create worker param must be more than 1 with new");
        ErrorHelper::ThrowError(env, ErrorHelper::TYPE_ERROR, "the number of parameters must be more than 1.");
        return nullptr;
    }
    // check 1st param is string
    if (!NapiHelper::IsString(env, args[0])) {
        HILOG_ERROR("worker:: the type of Worker 1st param must be string");
        ErrorHelper::ThrowError(env, ErrorHelper::TYPE_ERROR, "the type of the first param must be string.");
        return nullptr;
    }
    WorkerParams* workerParams = nullptr;
    if (argc == 2) {  // 2: max args number is 2
        workerParams = CheckWorkerArgs(env, args[1], version);
        if (workerParams == nullptr) {
            HILOG_ERROR("Worker:: arguments check failed.");
            return nullptr;
        }
    }

    Worker* worker = nullptr;
    if (limitSign) {
        bool success = WorkerManager::IncrementWorkerCount(WorkerType::LIMITED_WORKER);
        if (!success) {
            HILOG_ERROR("worker:: IncrementWorkerCount failed");
            ErrorHelper::ThrowError(env, ErrorHelper::ERR_WORKER_INITIALIZATION,
                "the number of limiteworkers exceeds the maximum.");
            CloseHelp::DeletePointer(workerParams, false);
            return nullptr;
        }

        std::lock_guard<std::mutex> lock(g_limitedworkersMutex);
        // 2. new worker instance
        worker = new Worker(env, nullptr);
        if (worker == nullptr) {
            HILOG_ERROR("worker:: create worker error");
            WorkerManager::DecrementWorkerCount(WorkerType::LIMITED_WORKER);
            ErrorHelper::ThrowError(env, ErrorHelper::ERR_WORKER_INITIALIZATION, "create worker error");
            CloseHelp::DeletePointer(workerParams, false);
            return nullptr;
        }
        worker->workerType_ = WorkerType::LIMITED_WORKER;
        g_limitedworkers.push_back(worker);
        HILOG_INFO("worker:: limited workers num %{public}zu", g_limitedworkers.size());
    } else {
        WorkerType workerType = (version == WorkerVersion::NEW) ? THREAD_WORKER : OLD_WORKER;
        bool success = WorkerManager::IncrementWorkerCount(workerType);
        if (!success) {
            HILOG_ERROR("worker:: IncrementWorkerCount failed");
            ErrorHelper::ThrowError(env, ErrorHelper::ERR_WORKER_INITIALIZATION,
                "the number of workers exceeds the maximum.");
            CloseHelp::DeletePointer(workerParams, false);
            return nullptr;
        }

        std::lock_guard<std::mutex> lock(g_workersMutex);
        // 2. new worker instance
        worker = new Worker(env, nullptr);
        if (worker == nullptr) {
            HILOG_ERROR("worker:: create worker error");
            WorkerManager::DecrementWorkerCount(workerType);
            ErrorHelper::ThrowError(env, ErrorHelper::ERR_WORKER_INITIALIZATION, "create worker error");
            CloseHelp::DeletePointer(workerParams, false);
            return nullptr;
        }
        worker->workerType_ = workerType;
        g_workers.push_back(worker);
        HILOG_INFO("worker:: workers num %{public}zu", g_workers.size());
    }

    if (workerParams != nullptr) {
        if (!workerParams->name_.empty()) {
            worker->name_ = workerParams->name_;
        }
        // default classic
        worker->SetScriptMode(workerParams->type_);
        worker->workerPriority_ = workerParams->workerPriority_;

        CloseHelp::DeletePointer(workerParams, false);
        workerParams = nullptr;
    }
    worker->isLimitedWorker_ = limitSign;
    worker->isNewVersion_ = (version != WorkerVersion::OLD) ? true : false;

    // 3. execute in thread
    char* script = NapiHelper::GetChars(env, args[0]);
    if (script == nullptr) {
        WorkerManager::DecrementWorkerCount(worker->workerType_);
        HILOG_ERROR("worker:: the file path is invaild, maybe path is null");
        ErrorHelper::ThrowError(env, ErrorHelper::ERR_WORKER_INVALID_FILEPATH,
            "the file path is invaild, maybe path is null.");
        return nullptr;
    }
    if (limitSign) {
        napi_add_env_cleanup_hook(env, LimitedWorkerHostEnvCleanCallback, worker);
    } else {
        napi_add_env_cleanup_hook(env, WorkerHostEnvCleanCallback, worker);
    }
    napi_status status = napi_wrap(env, thisVar, worker, WorkerDestructor, nullptr, &worker->workerRef_);
    if (status != napi_ok) {
        WorkerManager::DecrementWorkerCount(worker->workerType_);
        HILOG_ERROR("worker::Constructor napi_wrap return value is %{public}d", status);
        WorkerDestructor(env, worker, nullptr);
        CloseHelp::DeletePointer(script, true);
        return nullptr;
    }

#if defined(ENABLE_CONCURRENCY_INTEROP)
    if (reinterpret_cast<NativeEngine*>(env)->IsMainThread() && ANIHelper::GetAniVm() == nullptr) {
        HILOG_ERROR("worker:: get aniVm is null in main thread.");
    }
#endif

    worker->StartExecuteInThread(env, script);
    return thisVar;
}

void Worker::WorkerDestructor(napi_env env, void *data, void *hint)
{
    Worker* worker = static_cast<Worker*>(data);
    if (worker == nullptr) {
        HILOG_WARN("worker:: worker is null.");
        return;
    }
    if (worker->isLimitedWorker_) {
        napi_remove_env_cleanup_hook(env, LimitedWorkerHostEnvCleanCallback, worker);
    } else {
        napi_remove_env_cleanup_hook(env, WorkerHostEnvCleanCallback, worker);
    }
    std::lock_guard<std::recursive_mutex> lock(worker->liveStatusLock_);
    if (worker->isHostEnvExited_) {
        HILOG_INFO("worker:: host env exit.");
        return;
    }
    if (worker->UpdateHostState(INACTIVE)) {
#if defined(ENABLE_WORKER_EVENTHANDLER)
        if (!worker->isMainThreadWorker_ || worker->isLimitedWorker_) {
            worker->CloseHostHandle();
        }
#else
        worker->CloseHostHandle();
#endif
        worker->ReleaseHostThreadContent();
    }
    if (!worker->IsRunning()) {
        HILOG_DEBUG("worker:: worker is not running");
        return;
    }
    worker->TerminateInner();
}

void Worker::WorkerHostEnvCleanCallback(void* data)
{
    Worker* worker = static_cast<Worker*>(data);
    if (worker == nullptr) {
        HILOG_INFO("worker:: worker is nullptr when host env exit.");
        return;
    }
    if (!IsValidWorker(worker)) {
        HILOG_INFO("worker:: worker is terminated when host env exit.");
        return;
    }
    HostEnvCleanCallbackInner(worker);
}

void Worker::LimitedWorkerHostEnvCleanCallback(void* data)
{
    Worker* limitedWorker = static_cast<Worker*>(data);
    if (limitedWorker == nullptr) {
        HILOG_INFO("worker:: limitedWorker is nullptr when host env exit.");
        return;
    }
    if (!IsValidLimitedWorker(limitedWorker)) {
        HILOG_INFO("worker:: limitedWorker is terminated when host env exit.");
        return;
    }
    HostEnvCleanCallbackInner(limitedWorker);
}

void Worker::HostEnvCleanCallbackInner(Worker* worker)
{
    std::lock_guard<std::recursive_mutex> lock(worker->liveStatusLock_);
    worker->isHostEnvExited_ = true;
#if defined(ENABLE_WORKER_EVENTHANDLER)
    if (!worker->isMainThreadWorker_ || worker->isLimitedWorker_) {
        worker->CloseHostHandle();
    }
#else
    worker->CloseHostHandle();
#endif
    worker->ReleaseHostThreadContent();
    worker->RemoveAllListenerInner();
    worker->ClearGlobalCallObject();
}

Worker::WorkerParams* Worker::CheckWorkerArgs(napi_env env, napi_value argsValue, WorkerVersion version)
{
    WorkerParams* workerParams = nullptr;
    if (NapiHelper::IsObject(env, argsValue)) {
        workerParams = new WorkerParams();
        bool hasPriorityValue = NapiHelper::HasNameProperty(env, argsValue, "priority");
        if (version != WorkerVersion::OLD && hasPriorityValue) {
            workerParams->workerPriority_ = Worker::GetPriorityArg(env, argsValue);
            if (workerParams->workerPriority_ == WorkerPriority::INVALID) {
                CloseHelp::DeletePointer(workerParams, false);
                WorkerThrowError(env, ErrorHelper::TYPE_ERROR, "the priority value is invalid");
                return nullptr;
            }
        }
        napi_value nameValue = NapiHelper::GetNameProperty(env, argsValue, "name");
        if (NapiHelper::IsNotUndefined(env, nameValue)) {
            if (!NapiHelper::IsString(env, nameValue)) {
                CloseHelp::DeletePointer(workerParams, false);
                WorkerThrowError(env, ErrorHelper::TYPE_ERROR, "the type of name must be string.");
                return nullptr;
            }
            char* nameStr = NapiHelper::GetChars(env, nameValue);
            if (nameStr == nullptr) {
                CloseHelp::DeletePointer(workerParams, false);
                ErrorHelper::ThrowError(env, ErrorHelper::ERR_WORKER_INITIALIZATION, "the name of worker is null.");
                return nullptr;
            }
            workerParams->name_ = std::string(nameStr);
            CloseHelp::DeletePointer(nameStr, true);
        }
        napi_value typeValue = NapiHelper::GetNameProperty(env, argsValue, "type");
        if (NapiHelper::IsNotUndefined(env, typeValue)) {
            if (!NapiHelper::IsString(env, typeValue)) {
                CloseHelp::DeletePointer(workerParams, false);
                WorkerThrowError(env, ErrorHelper::TYPE_ERROR, "the type of type's value must be string.");
                return nullptr;
            }
            char* typeStr = NapiHelper::GetChars(env, typeValue);
            if (typeStr == nullptr) {
                CloseHelp::DeletePointer(workerParams, false);
                ErrorHelper::ThrowError(env, ErrorHelper::ERR_WORKER_INITIALIZATION, "the type of worker is null.");
                return nullptr;
            }
            if (strcmp("classic", typeStr) == 0) {
                workerParams->type_ = CLASSIC;
                CloseHelp::DeletePointer(typeStr, true);
            } else {
                CloseHelp::DeletePointer(workerParams, false);
                CloseHelp::DeletePointer(typeStr, true);
                ErrorHelper::ThrowError(env, ErrorHelper::TYPE_ERROR,
                    "the type must be classic, unsupport others now.");
                return nullptr;
            }
        }
    }
    return workerParams;
}

napi_value Worker::ParseTransferListArg(napi_env env, napi_value secondArg, bool& isValid, const std::string& errMsg)
{
    isValid = false;
    napi_value transferList = NapiHelper::GetUndefinedValue(env);
    bool isArray = NapiHelper::IsArray(env, secondArg);
    bool isObject = !isArray && NapiHelper::IsObject(env, secondArg);
    if (isArray) {
        transferList = secondArg;
        isValid = true;
        return transferList;
    }

    if (isObject) {
        napi_value transferProp;
        napi_status status = napi_get_named_property(env, secondArg, "transfer", &transferProp);
        if (status != napi_ok) {
            ErrorHelper::ThrowError(env, ErrorHelper::TYPE_ERROR, "Failed to access transfer property");
            return nullptr;
        }

        if (NapiHelper::IsNotUndefined(env, transferProp)) {
            if (!NapiHelper::IsArray(env, transferProp)) {
                ErrorHelper::ThrowError(env, ErrorHelper::TYPE_ERROR, errMsg.c_str());
                return nullptr;
            }
            transferList = transferProp;
        }
        isValid = true;
        return transferList;
    }

    ErrorHelper::ThrowError(env, ErrorHelper::TYPE_ERROR, errMsg.c_str());
    return nullptr;
}

WorkerPriority Worker::GetPriorityArg(napi_env env, napi_value argsValue)
{
    napi_value priorityValue = NapiHelper::GetNameProperty(env, argsValue, "priority");
    if (!NapiHelper::IsNumber(env, priorityValue)) {
        HILOG_ERROR("worker:: GetPriorityArg error, not number");
        return WorkerPriority::INVALID;
    }

    int32_t priority = static_cast<int32_t>(WorkerPriority::INVALID);
    napi_get_value_int32(env, priorityValue, static_cast<int32_t*>(&priority));
    if (priority <= static_cast<int32_t>(WorkerPriority::INVALID) ||
        priority >= static_cast<int32_t>(WorkerPriority::MAX)) {
        HILOG_ERROR("worker:: GetPriorityArg error, value not in scope");
        return WorkerPriority::INVALID;
    }
    return static_cast<WorkerPriority>(priority);
}

napi_value Worker::PostMessage(napi_env env, napi_callback_info cbinfo)
{
    HITRACE_HELPER_METER_NAME(__PRETTY_FUNCTION__);
    return CommonPostMessage(env, cbinfo, true);
}

napi_value Worker::PostMessageWithSharedSendable(napi_env env, napi_callback_info cbinfo)
{
    HITRACE_HELPER_METER_NAME(__PRETTY_FUNCTION__);
    return CommonPostMessage(env, cbinfo, false);
}

napi_value Worker::CommonPostMessage(napi_env env, napi_callback_info cbinfo, bool cloneSendable)
{
    HITRACE_HELPER_METER_NAME(__PRETTY_FUNCTION__);
    size_t argc = NapiHelper::GetCallbackInfoArgc(env, cbinfo);
    if (argc < 1) {
        ErrorHelper::ThrowError(env, ErrorHelper::TYPE_ERROR, "Worker messageObject must be not null with postMessage");
        return nullptr;
    }
    napi_value* argv = new napi_value[argc];
    ObjectScope<napi_value> scope(argv, true);
    napi_value thisVar = nullptr;
    napi_get_cb_info(env, cbinfo, &argc, argv, &thisVar, nullptr);
    Worker* worker = nullptr;
    napi_unwrap(env, thisVar, reinterpret_cast<void**>(&worker));

    if (worker == nullptr || worker->IsTerminated() || worker->IsTerminating()) {
        HILOG_ERROR("worker:: worker is nullptr when PostMessage, maybe worker is terminated");
        WorkerThrowError(env, ErrorHelper::ERR_WORKER_NOT_RUNNING, "maybe worker is terminated when PostMessage");
        return nullptr;
    }

    MessageDataType data = nullptr;
    napi_status serializeStatus = napi_ok;
    bool defaultClone = cloneSendable ? true : false;
    napi_value undefined = NapiHelper::GetUndefinedValue(env);
    napi_value transferList = undefined;
    std::string errMessage = "the type of the transfer list must be an array.";
    std::string serializeErr = "";
    if (argc >= NUM_WORKER_ARGS) {
        bool isValidTransfer = false;
        napi_value secondArg = argv[1];
        transferList = ParseTransferListArg(env, secondArg, isValidTransfer, errMessage);
        if (transferList == nullptr || !isValidTransfer) {
            return nullptr;
        }
        serializeStatus = napi_serialize_inner_with_error(env, argv[0], transferList, undefined, false, defaultClone,
                                                          &data, serializeErr);
    } else {
        serializeStatus = napi_serialize_inner_with_error(env, argv[0], undefined, undefined, false, defaultClone,
                                                          &data, serializeErr);
    }
    if (serializeStatus != napi_ok || data == nullptr) {
        worker->HostOnMessageErrorInner();
        serializeErr = "failed to serialize message.\nSerialize error: " + serializeErr;
        WorkerThrowError(env, ErrorHelper::ERR_WORKER_SERIALIZATION, serializeErr.c_str());
        return nullptr;
    }
    worker->PostMessageInner(data);
    return NapiHelper::GetUndefinedValue(env);
}

napi_value Worker::Terminate(napi_env env, napi_callback_info cbinfo)
{
    HITRACE_HELPER_METER_NAME(__PRETTY_FUNCTION__);
    napi_value thisVar = nullptr;
    napi_get_cb_info(env, cbinfo, nullptr, nullptr, &thisVar, nullptr);
    Worker* worker = nullptr;
    napi_unwrap(env, thisVar, reinterpret_cast<void**>(&worker));
    if (worker == nullptr) {
        HILOG_ERROR("worker:: worker is nullptr when Terminate, maybe worker is terminated");
        WorkerThrowError(env, ErrorHelper::ERR_WORKER_NOT_RUNNING, "worker is nullptr when Terminate");
        return nullptr;
    }
    bool expected = false;
    if (worker->isTerminated_.compare_exchange_weak(expected, true)) {
        HILOG_DEBUG("worker:: Terminate worker");
    } else {
        HILOG_DEBUG("worker:: worker is terminated when Terminate");
        return nullptr;
    }
    if (worker->IsTerminated() || worker->IsTerminating()) {
        HILOG_DEBUG("worker:: worker is not in running when Terminate");
        return nullptr;
    }
    worker->TerminateInner();
    return NapiHelper::GetUndefinedValue(env);
}

napi_value Worker::On(napi_env env, napi_callback_info cbinfo)
{
    return AddListener(env, cbinfo, PERMANENT);
}

napi_value Worker::Once(napi_env env, napi_callback_info cbinfo)
{
    return AddListener(env, cbinfo, ONCE);
}

napi_value Worker::RegisterGlobalCallObject(napi_env env, napi_callback_info cbinfo)
{
    size_t argc = NapiHelper::GetCallbackInfoArgc(env, cbinfo);
    if (argc != NUM_WORKER_ARGS) {
        ErrorHelper::ThrowError(env, ErrorHelper::TYPE_ERROR, "the number of parameters must be 2.");
        return nullptr;
    }
    // check 1st param is string
    napi_value thisVar = nullptr;
    void* data = nullptr;
    napi_value* args = new napi_value[argc];
    ObjectScope<napi_value> scope(args, true);
    napi_get_cb_info(env, cbinfo, &argc, args, &thisVar, &data);
    if (!NapiHelper::IsString(env, args[0])) {
        ErrorHelper::ThrowError(env, ErrorHelper::TYPE_ERROR, "the type of instanceName must be string.");
        return nullptr;
    }
    std::string instanceName = NapiHelper::GetString(env, args[0]);

    Worker* worker = nullptr;
    napi_unwrap(env, thisVar, (void**)&worker);
    if (worker == nullptr) {
        ErrorHelper::ThrowError(env, ErrorHelper::ERR_WORKER_NOT_RUNNING, "maybe worker is terminated");
        return nullptr;
    }
    napi_ref obj = NapiHelper::CreateReference(env, args[1], 1);
    worker->AddGlobalCallObject(instanceName, obj);
    return nullptr;
}

napi_value Worker::UnregisterGlobalCallObject(napi_env env, napi_callback_info cbinfo)
{
    size_t argc = NapiHelper::GetCallbackInfoArgc(env, cbinfo);
    if (argc > 1) {
        ErrorHelper::ThrowError(env, ErrorHelper::TYPE_ERROR, "the number of the parameters must be 1 or 0.");
        return nullptr;
    }
    napi_value thisVar = nullptr;
    void* data = nullptr;
    napi_value* args = new napi_value[argc];
    ObjectScope<napi_value> scope(args, true);
    napi_get_cb_info(env, cbinfo, &argc, args, &thisVar, &data);
    Worker* worker = nullptr;
    napi_unwrap(env, thisVar, (void**)&worker);
    if (worker == nullptr) {
        ErrorHelper::ThrowError(env, ErrorHelper::ERR_WORKER_NOT_RUNNING, "maybe worker is terminated");
        return nullptr;
    }
    if (argc == 0) {
        worker->ClearGlobalCallObject();
        HILOG_DEBUG("worker:: clear all registered globalCallObject");
        return nullptr;
    }
    // check 1st param is string
    if (!NapiHelper::IsString(env, args[0])) {
        ErrorHelper::ThrowError(env, ErrorHelper::TYPE_ERROR, "the type of instanceName must be string.");
        return nullptr;
    }
    std::string instanceName = NapiHelper::GetString(env, args[0]);
    if (!worker->RemoveGlobalCallObject(instanceName)) {
        HILOG_ERROR("worker:: unregister unexist globalCallObject");
    }
    return nullptr;
}

napi_value Worker::Off(napi_env env, napi_callback_info cbinfo)
{
    return RemoveListener(env, cbinfo);
}

napi_value Worker::RemoveEventListener(napi_env env, napi_callback_info cbinfo)
{
    return RemoveListener(env, cbinfo);
}

napi_value Worker::AddEventListener(napi_env env, napi_callback_info cbinfo)
{
    return AddListener(env, cbinfo, PERMANENT);
}

napi_value Worker::AddListener(napi_env env, napi_callback_info cbinfo, ListenerMode mode)
{
    size_t argc = NapiHelper::GetCallbackInfoArgc(env, cbinfo);
    if (argc < NUM_WORKER_ARGS) {
        ErrorHelper::ThrowError(env, ErrorHelper::TYPE_ERROR, "the number of listener parameters is not less than 2.");
        return nullptr;
    }
    // check 1st param is string
    napi_value thisVar = nullptr;
    void* data = nullptr;
    napi_value* args = new napi_value[argc];
    ObjectScope<napi_value> scope(args, true);
    napi_get_cb_info(env, cbinfo, &argc, args, &thisVar, &data);
    if (!NapiHelper::IsString(env, args[0])) {
        ErrorHelper::ThrowError(env, ErrorHelper::TYPE_ERROR, "the type of listener first param must be string.");
        return nullptr;
    }
    if (!NapiHelper::IsCallable(env, args[1])) {
        ErrorHelper::ThrowError(env, ErrorHelper::TYPE_ERROR,
            "the type of listener the second param must be callable.");
        return nullptr;
    }
    Worker* worker = nullptr;
    napi_unwrap(env, thisVar, (void**)&worker);
    if (worker == nullptr) {
        HILOG_ERROR("worker:: worker is nullptr when addListener, maybe worker is terminated");
        WorkerThrowError(env, ErrorHelper::ERR_WORKER_NOT_RUNNING, "maybe worker is terminated");
        return nullptr;
    }

    napi_ref callback = NapiHelper::CreateReference(env, args[1], 1);
    auto listener = new WorkerListener(env, callback, mode);
    if (mode == ONCE && argc > NUM_WORKER_ARGS) {
        if (NapiHelper::IsObject(env, args[NUM_WORKER_ARGS])) {
            napi_value onceValue = NapiHelper::GetNameProperty(env, args[NUM_WORKER_ARGS], "once");
            bool isOnce = NapiHelper::GetBooleanValue(env, onceValue);
            if (!isOnce) {
                listener->SetMode(PERMANENT);
            }
        }
    }
    char* typeStr = NapiHelper::GetChars(env, args[0]);
    worker->AddListenerInner(env, typeStr, listener);
    CloseHelp::DeletePointer(typeStr, true);
    return NapiHelper::GetUndefinedValue(env);
}

napi_value Worker::RemoveListener(napi_env env, napi_callback_info cbinfo)
{
    size_t argc = NapiHelper::GetCallbackInfoArgc(env, cbinfo);
    if (argc < 1) {
        ErrorHelper::ThrowError(env, ErrorHelper::TYPE_ERROR, "the number of parameters is not less than 1.");
        return nullptr;
    }
    // check 1st param is string
    napi_value thisVar = nullptr;
    void* data = nullptr;
    napi_value* args = new napi_value[argc];
    ObjectScope<napi_value> scope(args, true);
    napi_get_cb_info(env, cbinfo, &argc, args, &thisVar, &data);
    if (!NapiHelper::IsString(env, args[0])) {
        ErrorHelper::ThrowError(env, ErrorHelper::TYPE_ERROR,
            "the type of removelistener the first param must be string.");
        return nullptr;
    }

    Worker* worker = nullptr;
    napi_unwrap(env, thisVar, reinterpret_cast<void**>(&worker));
    if (worker == nullptr) {
        HILOG_ERROR("worker:: worker is nullptr when RemoveListener, maybe worker is terminated");
        WorkerThrowError(env, ErrorHelper::ERR_WORKER_NOT_RUNNING, "maybe worker is terminated");
        return nullptr;
    }

    if (argc > 1 && !NapiHelper::IsCallable(env, args[1])) {
        ErrorHelper::ThrowError(env, ErrorHelper::TYPE_ERROR,
            "the type of removelistener the second param must be callable.");
        return nullptr;
    }

    char* typeStr = NapiHelper::GetChars(env, args[0]);
    if (typeStr == nullptr) {
        ErrorHelper::ThrowError(env, ErrorHelper::TYPE_ERROR, "the type of remove listener type must be not null");
        return nullptr;
    }

    napi_ref callback = nullptr;
    if (argc > 1 && NapiHelper::IsCallable(env, args[1])) {
        napi_create_reference(env, args[1], 1, &callback);
    }
    worker->RemoveListenerInner(env, typeStr, callback);
    CloseHelp::DeletePointer(typeStr, true);
    NapiHelper::DeleteReference(env, callback);
    return NapiHelper::GetUndefinedValue(env);
}

void CallWorkCallback(napi_env env, napi_value recv, size_t argc, const napi_value* argv, const char* type)
{
    napi_value callback = nullptr;
    napi_get_named_property(env, recv, type, &callback);
    if (NapiHelper::IsCallable(env, callback)) {
        napi_value callbackResult = nullptr;
        napi_call_function(env, recv, callback, argc, argv, &callbackResult);
    }
}

napi_value Worker::DispatchEvent(napi_env env, napi_callback_info cbinfo)
{
    size_t argc = 1;
    napi_value args[1];
    napi_value thisVar = nullptr;
    void* data = nullptr;
    napi_get_cb_info(env, cbinfo, &argc, args, &thisVar, &data);
    if (argc < 1) {
        ErrorHelper::ThrowError(env, ErrorHelper::TYPE_ERROR, "the number of the parameters must be more than 1.");
        return NapiHelper::CreateBooleanValue(env, false);
    }

    // check 1st param is event
    if (!NapiHelper::IsObject(env, args[0])) {
        ErrorHelper::ThrowError(env, ErrorHelper::TYPE_ERROR,
            "the type of DispatchEvent first param must be event object.");
        return NapiHelper::CreateBooleanValue(env, false);
    }

    Worker* worker = nullptr;
    napi_unwrap(env, thisVar, reinterpret_cast<void**>(&worker));
    if (worker == nullptr) {
        HILOG_ERROR("worker:: worker is nullptr when DispatchEvent, maybe worker is terminated");
        WorkerThrowError(env, ErrorHelper::ERR_WORKER_NOT_RUNNING, "maybe worker has been terminated");
        return NapiHelper::CreateBooleanValue(env, false);
    }

    napi_value typeValue = NapiHelper::GetNameProperty(env, args[0], "type");
    if (!NapiHelper::IsString(env, typeValue)) {
        ErrorHelper::ThrowError(env, ErrorHelper::TYPE_ERROR, "the type of event type must be string.");
        return NapiHelper::CreateBooleanValue(env, false);
    }

    napi_value obj = NapiHelper::GetReferenceValue(env, worker->workerRef_);

    char* typeStr = NapiHelper::GetChars(env, typeValue);
    if (typeStr == nullptr) {
        ErrorHelper::ThrowError(env, ErrorHelper::TYPE_ERROR, "dispatchEvent event type must be not null");
        return NapiHelper::CreateBooleanValue(env, false);
    }
    if (strcmp(typeStr, "error") == 0) {
        CallWorkCallback(env, obj, 1, args, "onerror");
    } else if (strcmp(typeStr, "messageerror") == 0) {
        CallWorkCallback(env, obj, 1, args, "onmessageerror");
    } else if (strcmp(typeStr, "message") == 0) {
        CallWorkCallback(env, obj, 1, args, "onmessage");
    }

    worker->HandleEventListeners(env, obj, 1, args, typeStr);

    CloseHelp::DeletePointer(typeStr, true);
    return NapiHelper::CreateBooleanValue(env, true);
}

napi_value Worker::RemoveAllListener(napi_env env, napi_callback_info cbinfo)
{
    napi_value thisVar = nullptr;
    napi_get_cb_info(env, cbinfo, nullptr, nullptr, &thisVar, nullptr);
    Worker* worker = nullptr;
    napi_unwrap(env, thisVar, reinterpret_cast<void**>(&worker));
    if (worker == nullptr) {
        HILOG_ERROR("worker:: worker is nullptr when RemoveAllListener, maybe worker is terminated");
        WorkerThrowError(env, ErrorHelper::ERR_WORKER_NOT_RUNNING, "maybe worker is terminated");
        return nullptr;
    }

    worker->RemoveAllListenerInner();
    return NapiHelper::GetUndefinedValue(env);
}

napi_value Worker::CancelTask(napi_env env, napi_callback_info cbinfo)
{
    napi_value thisVar = nullptr;
    napi_get_cb_info(env, cbinfo, nullptr, nullptr, &thisVar, nullptr);
    Worker* worker = nullptr;
    napi_unwrap(env, thisVar, reinterpret_cast<void**>(&worker));
    if (worker == nullptr) {
        HILOG_ERROR("worker:: worker is nullptr when CancelTask, maybe worker is terminated");
        return nullptr;
    }

    if (worker->IsTerminated() || worker->IsTerminating()) {
        HILOG_INFO("worker:: worker is not in running");
        return nullptr;
    }

    if (!worker->ClearWorkerTasks()) {
        HILOG_ERROR("worker:: clear worker task error");
    }
    return NapiHelper::GetUndefinedValue(env);
}

napi_value Worker::PostMessageToHost(napi_env env, napi_callback_info cbinfo)
{
    HITRACE_HELPER_METER_NAME(__PRETTY_FUNCTION__);
    return CommonPostMessageToHost(env, cbinfo, true);
}

napi_value Worker::PostMessageWithSharedSendableToHost(napi_env env, napi_callback_info cbinfo)
{
    HITRACE_HELPER_METER_NAME(__PRETTY_FUNCTION__);
    return CommonPostMessageToHost(env, cbinfo, false);
}

napi_value Worker::CommonPostMessageToHost(napi_env env, napi_callback_info cbinfo, bool cloneSendable)
{
    HITRACE_HELPER_METER_NAME(__PRETTY_FUNCTION__);
    size_t argc = NapiHelper::GetCallbackInfoArgc(env, cbinfo);
    if (argc < 1) {
        ErrorHelper::ThrowError(env, ErrorHelper::TYPE_ERROR, "the number of parameters must be more than 1.");
        return nullptr;
    }
    napi_value* argv = new napi_value[argc];
    ObjectScope<napi_value> scope(argv, true);
    Worker* worker = nullptr;
    napi_get_cb_info(env, cbinfo, &argc, argv, nullptr, reinterpret_cast<void**>(&worker));

    if (worker == nullptr) {
        HILOG_ERROR("worker:: when post message to host occur worker is nullptr");
        WorkerThrowError(env, ErrorHelper::ERR_WORKER_NOT_RUNNING, "worker is nullptr when post message to host");
        return nullptr;
    }

    if (!worker->IsRunning()) {
        // if worker is not running, don't send any message to host thread
        HILOG_DEBUG("worker:: when post message to host occur worker is not in running.");
        return nullptr;
    }

    MessageDataType data = nullptr;
    napi_status serializeStatus = napi_ok;
    bool defaultClone = cloneSendable ? true : false;
    napi_value undefined = NapiHelper::GetUndefinedValue(env);
    napi_value transferList = undefined;
    std::string errMessage = "Transfer list must be an Array";
    std::string serializeErr = "";
    if (argc >= NUM_WORKER_ARGS) {
        bool isValidTransfer = false;
        napi_value secondArg = argv[1];
        transferList = ParseTransferListArg(env, secondArg, isValidTransfer, errMessage);
        if (transferList == nullptr || !isValidTransfer) {
            return nullptr;
        }
        serializeStatus = napi_serialize_inner_with_error(env, argv[0], transferList, undefined, false, defaultClone,
                                                          &data, serializeErr);
    } else {
        serializeStatus = napi_serialize_inner_with_error(env, argv[0], undefined, undefined, false, defaultClone,
                                                          &data, serializeErr);
    }

    if (serializeStatus != napi_ok || data == nullptr) {
        worker->WorkerOnMessageErrorInner();
        serializeErr = "failed to serialize message.\nSerialize error: " + serializeErr;
        WorkerThrowError(env, ErrorHelper::ERR_WORKER_SERIALIZATION, serializeErr.c_str());
        return nullptr;
    }
    worker->PostMessageToHostInner(data);
    return NapiHelper::GetUndefinedValue(env);
}

napi_value Worker::GlobalCall(napi_env env, napi_callback_info cbinfo)
{
    HITRACE_HELPER_METER_NAME(__PRETTY_FUNCTION__);
    size_t argc = NapiHelper::GetCallbackInfoArgc(env, cbinfo);
    if (argc < NUM_GLOBAL_CALL_ARGS) {
        ErrorHelper::ThrowError(env, ErrorHelper::TYPE_ERROR, "the number of parameters must be equal or more than 3.");
        return nullptr;
    }
    napi_value* args = new napi_value[argc];
    ObjectScope<napi_value> scope(args, true);
    Worker* worker = nullptr;
    napi_get_cb_info(env, cbinfo, &argc, args, nullptr, reinterpret_cast<void**>(&worker));
    if (worker == nullptr) {
        HILOG_ERROR("worker:: worker is null when callGlobalCallObjectMethod to host");
        ErrorHelper::ThrowError(env, ErrorHelper::ERR_WORKER_NOT_RUNNING,
            "worker is null when callGlobalCallObjectMethod to host");
        return nullptr;
    }

    if (!worker->IsRunning()) {
        // if worker is not running, don't send any message to host thread
        HILOG_DEBUG("worker:: when post message to host occur worker is not in running.");
        return nullptr;
    }

    if (!NapiHelper::IsString(env, args[0])) {
        ErrorHelper::ThrowError(env, ErrorHelper::TYPE_ERROR, "the type of instanceName must be string.");
        return nullptr;
    }
    if (!NapiHelper::IsString(env, args[1])) {
        ErrorHelper::ThrowError(env, ErrorHelper::TYPE_ERROR, "the type of methodname must be string.");
        return nullptr;
    }
    if (!NapiHelper::IsNumber(env, args[2])) { // 2: the index of argument "timeout"
        ErrorHelper::ThrowError(env, ErrorHelper::TYPE_ERROR, "the type of timeout must be number.");
        return nullptr;
    }

    napi_status serializeStatus = napi_ok;
    MessageDataType data = nullptr;
    napi_value argsArray;
    napi_create_array_with_length(env, argc - 1, &argsArray);
    size_t index = 0;
    uint32_t timeout = 0;
    for (size_t i = 0; i < argc; i++) {
        if (i == 2) { // 2: index of time limitation arg
            timeout = NapiHelper::GetUint32Value(env, args[i]);
            continue;
        }
        napi_set_element(env, argsArray, index, args[i]);
        index++;
    }
    if (timeout <= 0 || timeout > DEFAULT_TIMEOUT) {
        timeout = DEFAULT_TIMEOUT;
    }

    // defautly not transfer
    napi_value undefined = NapiHelper::GetUndefinedValue(env);
    // meaningless to copy sendable object when call globalObject
    bool defaultClone = true;
    bool defaultTransfer = false;
    std::string serializeErr = "";
    serializeStatus = napi_serialize_inner_with_error(env, argsArray, undefined, undefined, defaultTransfer,
                                                      defaultClone, &data, serializeErr);
    if (serializeStatus != napi_ok || data == nullptr) {
        serializeErr = "failed to serialize message.\nSerialize error: " + serializeErr;
        ErrorHelper::ThrowError(env, ErrorHelper::ERR_WORKER_SERIALIZATION, serializeErr.c_str());
        return nullptr;
    }
    worker->hostGlobalCallQueue_.Push(worker->globalCallId_, data);

    std::lock_guard<std::recursive_mutex> lock(worker->liveStatusLock_);
    if (env != nullptr && !worker->HostIsStop() && !worker->isHostEnvExited_) {
        worker->InitGlobalCallStatus(env);
#if defined(ENABLE_WORKER_EVENTHANDLER)
        if (worker->isMainThreadWorker_ && !worker->isLimitedWorker_) {
            worker->PostWorkerGlobalCallTask();
        } else {
            uv_async_send(worker->hostOnGlobalCallSignal_);
        }
#else
        uv_async_send(worker->hostOnGlobalCallSignal_);
#endif
    } else {
        HILOG_ERROR("worker:: worker host engine is nullptr when callGloballCallObjectMethod.");
        ErrorHelper::ThrowError(env, ErrorHelper::ERR_WORKER_NOT_RUNNING, "worker is null");
        return nullptr;
    }

    {
        std::unique_lock lock(worker->globalCallMutex_);
        if (!worker->cv_.wait_for(lock, std::chrono::milliseconds(timeout), [worker]() {
            return !worker->workerGlobalCallQueue_.IsEmpty() || !worker->globalCallSuccess_;
        })) {
            worker->IncreaseGlobalCallId();
            HILOG_ERROR("worker:: callGlobalCallObjectMethod has exceeded the waiting time limitation, skip this turn");
            ErrorHelper::ThrowError(env, ErrorHelper::ERR_GLOBAL_CALL_TIMEOUT);
            return nullptr;
        }
    }
    worker->IncreaseGlobalCallId();
    if (!worker->globalCallSuccess_) {
        worker->HandleGlobalCallError(env);
        return nullptr;
    }
    if (!worker->workerGlobalCallQueue_.DeQueue(&data)) {
        HILOG_ERROR("worker:: message returned from host is empty when callGloballCallObjectMethod");
        return nullptr;
    }
    napi_value res = nullptr;
    serializeStatus = napi_deserialize(env, data, &res);
    napi_delete_serialization_data(env, data);
    if (serializeStatus != napi_ok || res == nullptr) {
        ErrorHelper::ThrowError(env, ErrorHelper::ERR_WORKER_SERIALIZATION, "failed to serialize message.");
        return nullptr;
    }
    return res;
}

void Worker::InitGlobalCallStatus(napi_env env)
{
    // worker side event data queue shall be empty before uv_async_send
    workerGlobalCallQueue_.Clear(env);
    ClearGlobalCallError(env);
    globalCallSuccess_ = true;
}

void Worker::IncreaseGlobalCallId()
{
    if (UNLIKELY(globalCallId_ == GLOBAL_CALL_ID_MAX)) {
        globalCallId_ = 1;
    } else {
        globalCallId_++;
    }
}

napi_value Worker::CloseWorker(napi_env env, napi_callback_info cbinfo)
{
    HITRACE_HELPER_METER_NAME(__PRETTY_FUNCTION__);
    Worker* worker = nullptr;
    napi_get_cb_info(env, cbinfo, nullptr, nullptr, nullptr, (void**)&worker);
    if (worker != nullptr) {
        worker->CloseInner();
    } else {
        WorkerThrowError(env, ErrorHelper::ERR_WORKER_NOT_RUNNING, "worker is null");
        return nullptr;
    }
    return NapiHelper::GetUndefinedValue(env);
}

napi_value Worker::ParentPortCancelTask(napi_env env, napi_callback_info cbinfo)
{
    Worker* worker = nullptr;
    napi_get_cb_info(env, cbinfo, nullptr, nullptr, nullptr, reinterpret_cast<void**>(&worker));
    if (worker == nullptr) {
        HILOG_ERROR("worker:: worker is nullptr when CancelTask, maybe worker is terminated");
        return nullptr;
    }

    if (worker->IsTerminated() || worker->IsTerminating()) {
        HILOG_INFO("worker:: worker is not in running");
        return nullptr;
    }

    if (!worker->ClearWorkerTasks()) {
        HILOG_ERROR("worker:: clear worker task error");
    }
    return NapiHelper::GetUndefinedValue(env);
}

napi_value Worker::ParentPortAddEventListener(napi_env env, napi_callback_info cbinfo)
{
    size_t argc = NapiHelper::GetCallbackInfoArgc(env, cbinfo);
    if (argc < NUM_WORKER_ARGS) {
        ErrorHelper::ThrowError(env, ErrorHelper::TYPE_ERROR,
            "worker listener param count must be more than WORKPARAMNUM.");
        return nullptr;
    }

    napi_value* args = new napi_value[argc];
    ObjectScope<napi_value> scope(args, true);
    Worker* worker = nullptr;
    napi_get_cb_info(env, cbinfo, &argc, args, nullptr, reinterpret_cast<void**>(&worker));

    if (!NapiHelper::IsString(env, args[0])) {
        ErrorHelper::ThrowError(env, ErrorHelper::TYPE_ERROR,
            "the type of worker listener first param must be string.");
        return nullptr;
    }

    if (!NapiHelper::IsCallable(env, args[1])) {
        ErrorHelper::ThrowError(env, ErrorHelper::TYPE_ERROR,
            "the type of worker listener second param must be callable.");
        return nullptr;
    }

    if (worker == nullptr || !worker->IsNotTerminate()) {
        HILOG_ERROR("worker:: when post message to host occur worker is nullptr");
        WorkerThrowError(env, ErrorHelper::ERR_WORKER_NOT_RUNNING, "worker is not running.");
        return nullptr;
    }

    napi_ref callback = NapiHelper::CreateReference(env, args[1], 1);
    auto listener = new WorkerListener(env, callback, PERMANENT);
    if (argc > NUM_WORKER_ARGS && NapiHelper::IsObject(env, args[NUM_WORKER_ARGS])) {
        napi_value onceValue = NapiHelper::GetNameProperty(env, args[NUM_WORKER_ARGS], "once");
        bool isOnce = NapiHelper::GetBooleanValue(env, onceValue);
        if (isOnce) {
            listener->SetMode(ONCE);
        }
    }
    char* typeStr = NapiHelper::GetChars(env, args[0]);
    worker->ParentPortAddListenerInner(env, typeStr, listener);
    CloseHelp::DeletePointer(typeStr, true);
    return NapiHelper::GetUndefinedValue(env);
}

napi_value Worker::ParentPortDispatchEvent(napi_env env, napi_callback_info cbinfo)
{
    size_t argc = 1;
    napi_value args[1];
    Worker* worker = nullptr;
    napi_get_cb_info(env, cbinfo, &argc, args, nullptr, reinterpret_cast<void**>(&worker));
    if (argc < 1) {
        ErrorHelper::ThrowError(env, ErrorHelper::TYPE_ERROR, "DispatchEvent param count must be more than 1.");
        return NapiHelper::CreateBooleanValue(env, false);
    }

    if (!NapiHelper::IsObject(env, args[0])) {
        ErrorHelper::ThrowError(env, ErrorHelper::TYPE_ERROR,
            "the type of worker DispatchEvent first param must be Event.");
        return NapiHelper::CreateBooleanValue(env, false);
    }

    napi_value typeValue = NapiHelper::GetNameProperty(env, args[0], "type");
    if (!NapiHelper::IsString(env, typeValue)) {
        ErrorHelper::ThrowError(env, ErrorHelper::TYPE_ERROR, "the type of worker event must be string.");
        return NapiHelper::CreateBooleanValue(env, false);
    }

    if (worker == nullptr || !worker->IsNotTerminate()) {
        HILOG_ERROR("worker:: when post message to host occur worker is nullptr");
        WorkerThrowError(env, ErrorHelper::ERR_WORKER_NOT_RUNNING, "worker is nullptr.");
        return NapiHelper::CreateBooleanValue(env, false);
    }

    char* typeStr = NapiHelper::GetChars(env, typeValue);
    if (typeStr == nullptr) {
        ErrorHelper::ThrowError(env, ErrorHelper::TYPE_ERROR, "worker listener type must be not null.");
        return NapiHelper::CreateBooleanValue(env, false);
    }

    napi_value obj = NapiHelper::GetReferenceValue(env, worker->workerPort_);

    if (strcmp(typeStr, "error") == 0) {
        CallWorkCallback(env, obj, 1, args, "onerror");
    } else if (strcmp(typeStr, "messageerror") == 0) {
        CallWorkCallback(env, obj, 1, args, "onmessageerror");
    } else if (strcmp(typeStr, "message") == 0) {
        CallWorkCallback(env, obj, 1, args, "onmessage");
    }

    worker->ParentPortHandleEventListeners(env, obj, 1, args, typeStr, true);

    CloseHelp::DeletePointer(typeStr, true);
    return NapiHelper::CreateBooleanValue(env, true);
}

napi_value Worker::ParentPortRemoveEventListener(napi_env env, napi_callback_info cbinfo)
{
    size_t argc = NapiHelper::GetCallbackInfoArgc(env, cbinfo);
    if (argc < 1) {
        ErrorHelper::ThrowError(env, ErrorHelper::TYPE_ERROR, "the number of parameters must be more than 2.");
        return nullptr;
    }

    napi_value* args = new napi_value[argc];
    ObjectScope<napi_value> scope(args, true);
    Worker* worker = nullptr;
    napi_get_cb_info(env, cbinfo, &argc, args, nullptr, reinterpret_cast<void**>(&worker));

    if (!NapiHelper::IsString(env, args[0])) {
        ErrorHelper::ThrowError(env, ErrorHelper::TYPE_ERROR, "the type of worker listener 1st param must be string.");
        return nullptr;
    }

    if (argc > 1 && !NapiHelper::IsCallable(env, args[1])) {
        ErrorHelper::ThrowError(env, ErrorHelper::TYPE_ERROR,
            "the type of worker listener second param must be callable.");
        return nullptr;
    }

    if (worker == nullptr || !worker->IsNotTerminate()) {
        HILOG_ERROR("worker:: when post message to host occur worker is nullptr");
        WorkerThrowError(env, ErrorHelper::ERR_WORKER_NOT_RUNNING, "worker is not running.");
        return nullptr;
    }

    napi_ref callback = nullptr;
    if (argc > 1 && NapiHelper::IsCallable(env, args[1])) {
        napi_create_reference(env, args[1], 1, &callback);
    }

    char* typeStr = NapiHelper::GetChars(env, args[0]);
    if (typeStr == nullptr) {
        ErrorHelper::ThrowError(env, ErrorHelper::TYPE_ERROR, "worker listener type must be not null.");
        return nullptr;
    }
    worker->ParentPortRemoveListenerInner(env, typeStr, callback);
    CloseHelp::DeletePointer(typeStr, true);
    NapiHelper::DeleteReference(env, callback);
    return NapiHelper::GetUndefinedValue(env);
}

napi_value Worker::ParentPortRemoveAllListener(napi_env env, napi_callback_info cbinfo)
{
    Worker* worker = nullptr;
    napi_get_cb_info(env, cbinfo, nullptr, nullptr, nullptr, reinterpret_cast<void**>(&worker));

    if (worker == nullptr || !worker->IsNotTerminate()) {
        HILOG_ERROR("worker:: when post message to host occur worker is nullptr");
        WorkerThrowError(env, ErrorHelper::ERR_WORKER_NOT_RUNNING,
            "worker is nullptr when ParentPortRemoveAllListener");
        return nullptr;
    }

    worker->ParentPortRemoveAllListenerInner();
    return NapiHelper::GetUndefinedValue(env);
}

void Worker::GetContainerScopeId(napi_env env)
{
    NativeEngine* hostEngine = reinterpret_cast<NativeEngine*>(env);
    scopeId_ = hostEngine->GetContainerScopeIdFunc();
}

void Worker::AddGlobalCallObject(const std::string &instanceName, napi_ref obj)
{
    globalCallObjects_.insert_or_assign(instanceName, obj);
}

bool Worker::RemoveGlobalCallObject(const std::string &instanceName)
{
    for (auto iter = globalCallObjects_.begin(); iter != globalCallObjects_.end(); iter++) {
        if (iter->first == instanceName) {
            NapiHelper::DeleteReference(hostEnv_, iter->second);
            globalCallObjects_.erase(iter);
            return true;
        }
    }
    return false;
}

void Worker::ClearGlobalCallObject()
{
    for (auto iter = globalCallObjects_.begin(); iter != globalCallObjects_.end(); iter++) {
        napi_ref objRef = iter->second;
        NapiHelper::DeleteReference(hostEnv_, objRef);
    }
    globalCallObjects_.clear();
}

void Worker::StartExecuteInThread(napi_env env, const char* script)
{
    HILOG_INFO("worker:: Start execute in the thread!");
    // 1. init hostHandle in host loop
    uv_loop_t* loop = NapiHelper::GetLibUV(env);
    if (loop == nullptr) {
        ErrorHelper::ThrowError(env, ErrorHelper::ERR_WORKER_NOT_RUNNING, "engine loop is null");
        CloseHelp::DeletePointer(script, true);
        return;
    }
    GetContainerScopeId(env);
#if defined(ENABLE_WORKER_EVENTHANDLER)
    if (!OHOS::AppExecFwk::EventRunner::IsAppMainThread()) {
        isMainThreadWorker_ = false;
        InitHostHandle(loop);
    } else if (isLimitedWorker_) {
        InitHostHandle(loop);
    }
#else
    InitHostHandle(loop);
#endif

    // 2. copy the script
    script_ = std::string(script);
    // isBundle : FA mode and BundlePack.
    bool isBundle = reinterpret_cast<NativeEngine*>(env)->GetIsBundle();
    // if worker file is packed in har, need find moduleName in hostVM, and concat new recordName.
    bool isHar = script_.find_first_of(PathHelper::NAME_SPACE_TAG) == 0;
    if ((isHar && script_.find(PathHelper::PREFIX_BUNDLE) == std::string::npos) ||
        (!isBundle && script_.find_first_of(PathHelper::POINT_TAG) == 0)) {
        PathHelper::ConcatFileNameForWorker(env, script_, fileName_, isRelativePath_);
        HILOG_INFO("worker:: Concated worker recordName: %{public}s, fileName: %{public}s",
                   script_.c_str(), fileName_.c_str());
    }
    // check the path is vaild.
    if (!isBundle) {
        if (!PathHelper::CheckWorkerPath(env, script_, fileName_, isRelativePath_)) {
            EraseWorker();
            HILOG_ERROR("worker:: the file path is invaild, can't find the file : %{public}s.", script);
            CloseHelp::DeletePointer(script, true);
            ErrorHelper::ThrowError(env, ErrorHelper::ERR_WORKER_INVALID_FILEPATH,
                "the file path is invaild, can't find the file.");
            return;
        }
    }

    // 3. create WorkerRunner to Execute
    if (!runner_) {
        runner_ = std::make_unique<WorkerRunner>(WorkerStartCallback(ExecuteInThread, this));
    }
    if (runner_) {
        runner_->Execute(); // start a new thread
    } else {
        HILOG_ERROR("runner_ is nullptr");
    }
    CloseHelp::DeletePointer(script, true);
}

void Worker::ExecuteInThread(const void* data)
{
    HITRACE_HELPER_START_TRACE(__PRETTY_FUNCTION__);
    auto worker = reinterpret_cast<Worker*>(const_cast<void*>(data));
#ifdef ENABLE_QOS
    worker->SetQOSLevel();
#endif
    // 1. create a runtime, nativeengine
    napi_env workerEnv = nullptr;
    {
        std::lock_guard<std::recursive_mutex> lock(worker->liveStatusLock_);
        if (worker->HostIsStop() || worker->isHostEnvExited_) {
            HILOG_ERROR("worker:: host thread is stop");
            worker->EraseWorker();
            CloseHelp::DeletePointer(worker, false);
            return;
        }
        napi_env env = worker->GetHostEnv();
        if (worker->isLimitedWorker_) {
            napi_create_limit_runtime(env, &workerEnv);
        } else {
            napi_create_runtime(env, &workerEnv);
        }
        if (workerEnv == nullptr) {
            HILOG_ERROR("worker:: Worker create runtime error");
            worker->EraseWorker();
            ErrorHelper::ThrowError(env, ErrorHelper::ERR_WORKER_NOT_RUNNING, "Worker create runtime error");
            return;
        }
        if (worker->isLimitedWorker_) {
            reinterpret_cast<NativeEngine*>(workerEnv)->MarkRestrictedWorkerThread();
        } else {
            // mark worker env is workerThread
            reinterpret_cast<NativeEngine*>(workerEnv)->MarkWorkerThread();
        }
        // for load balance in taskpool
        reinterpret_cast<NativeEngine*>(env)->IncreaseSubEnvCounter();

        worker->SetWorkerEnv(workerEnv);
    }

    uv_loop_t* loop = worker->GetWorkerLoop();
    if (loop == nullptr) {
        HILOG_ERROR("worker:: Worker loop is nullptr");
        worker->EraseWorker();
        return;
    }

#if defined(ENABLE_CONCURRENCY_INTEROP)
    worker->AttachWorkerEnvToAniVm();
#endif

    reinterpret_cast<NativeEngine*>(workerEnv)->RegisterNapiUncaughtExceptionHandler(
        [workerEnv, worker] (napi_value exception) -> void {
        if (!NativeEngine::IsAlive(reinterpret_cast<NativeEngine*>(workerEnv))) {
            HILOG_WARN("napi_env has been destoryed!");
            return;
        }
        if (!IsValidWorker(worker)) {
            HILOG_WARN("worker:: the worker is not Valid.");
            return;
        }
        NapiErrorManager::GetInstance()->NotifyUncaughtException(workerEnv, exception, worker->GetName(), WORKER_TYPE);
        worker->HandleWorkerUncaughtException(workerEnv, exception);
    });
    reinterpret_cast<NativeEngine*>(workerEnv)->RegisterAllPromiseCallback(
        [workerEnv, worker] (napi_value* args) -> void {
        if (!NativeEngine::IsAlive(reinterpret_cast<NativeEngine*>(workerEnv))) {
            HILOG_WARN("napi_env has been destoryed!");
            return;
        }
        if (!IsValidWorker(worker)) {
            HILOG_WARN("worker:: the worker is not Valid.");
            return;
        }
        NapiErrorManager::GetInstance()->NotifyUnhandledRejection(workerEnv, args, worker->GetName(), WORKER_TYPE);
    });

    // 2. add some preparation for the worker
    if (worker->PrepareForWorkerInstance()) {
        if (ConcurrentHelper::UvHandleInit(loop, worker->workerOnMessageSignal_,
                                           Worker::WorkerOnMessage, worker) == 0) {
            worker->workerOnMessageInitState_ = true;
        }
        if (ConcurrentHelper::UvHandleInit(loop, worker->workerOnTerminateSignal_,
                                           Worker::WorkerOnMessage, worker) == 0) {
            worker->workerOnTerminateInitState_ = true;
        }
#if !defined(WINDOWS_PLATFORM) && !defined(MAC_PLATFORM)
        ConcurrentHelper::UvHandleInit(loop, worker->debuggerOnPostTaskSignal_, Worker::HandleDebuggerTask, worker);
#endif
        worker->UpdateWorkerState(RUNNING);
        // in order to invoke worker send before subThread start
        uv_async_send(worker->workerOnMessageSignal_);
        HITRACE_HELPER_FINISH_TRACE;
        // 3. start worker loop
        worker->Loop();
    } else {
        HILOG_ERROR("worker:: worker PrepareForWorkerInstance fail");
        worker->UpdateWorkerState(TERMINATED);
        HITRACE_HELPER_FINISH_TRACE;
    }
    worker->ReleaseWorkerThreadContent();
    std::lock_guard<std::recursive_mutex> lock(worker->liveStatusLock_);
    worker->EraseWorker();
    if (worker->HostIsStop() || worker->isHostEnvExited_) {
        HILOG_INFO("worker:: host is stopped");
        CloseHelp::DeletePointer(worker, false);
    } else {
        worker->PublishWorkerOverSignal();
    }
}

bool Worker::PrepareForWorkerInstance()
{
    std::string rawFileName = script_;
    {
        std::lock_guard<std::recursive_mutex> lock(liveStatusLock_);
        if (HostIsStop() || isHostEnvExited_) {
            HILOG_INFO("worker:: host is stopped");
            return false;
        }
        auto workerEngine = reinterpret_cast<NativeEngine*>(workerEnv_);
        auto hostEngine = reinterpret_cast<NativeEngine*>(hostEnv_);
        // 1. init worker environment
#if !defined(WINDOWS_PLATFORM) && !defined(MAC_PLATFORM)
        workerEngine->SetDebuggerPostTaskFunc([this](std::function<void()>&& task) {
            this->DebuggerOnPostTask(std::move(task));
        });
#endif
        if (!hostEngine->CallInitWorkerFunc(workerEngine)) {
            HILOG_ERROR("worker:: CallInitWorkerFunc error");
            return false;
        }
        // 2. get uril content
        if (isRelativePath_) {
            rawFileName = fileName_;
        }
    }
    // add timer interface
    Timer::RegisterTime(workerEnv_);
    napi_value execScriptResult = nullptr;
    napi_status status = napi_run_actor(workerEnv_, const_cast<char*>(rawFileName.c_str()),
                                        const_cast<char*>(script_.c_str()),  &execScriptResult);
    if (status != napi_ok || execScriptResult == nullptr) {
        // An exception occurred when running the script.
        HILOG_ERROR("worker:: run script exception occurs, will handle exception");
        HandleException();
        return false;
    }

    ApplyNameSetting();
    return true;
}

void Worker::ApplyNameSetting()
{
    std::string threadName = "WorkerThread";
    if (!name_.empty()) {
        napi_value nameValue = nullptr;
        napi_create_string_utf8(workerEnv_, name_.c_str(), name_.length(), &nameValue);
        NapiHelper::SetNamePropertyInGlobal(workerEnv_, "name", nameValue);

        threadName += "_" + name_;
        if (threadName.length() > THREAD_NAME_MAX_LENGTH) {
            threadName = threadName.substr(0, THREAD_NAME_MAX_LENGTH);
        }
    }
#if defined IOS_PLATFORM || defined MAC_PLATFORM
    pthread_setname_np(threadName.c_str());
#elif !defined(WINDOWS_PLATFORM)
    pthread_setname_np(pthread_self(), threadName.c_str());
#endif
}

void Worker::HostOnMessage(const uv_async_t* req)
{
    HITRACE_HELPER_METER_NAME(__PRETTY_FUNCTION__);
    Worker* worker = static_cast<Worker*>(req->data);
    if (worker == nullptr) {
        HILOG_ERROR("worker:: worker is null when host onmessage.");
        return;
    }
    worker->HostOnMessageInner();
}

void Worker::HostOnMessageInner()
{
    if (hostEnv_ == nullptr || HostIsStop()) {
        HILOG_ERROR("worker:: host thread maybe is over when host onmessage.");
        return;
    }

    napi_status status = napi_ok;
    HandleScope scope(hostEnv_, status);
    NAPI_CALL_RETURN_VOID(hostEnv_, status);

    NativeEngine* engine = reinterpret_cast<NativeEngine*>(hostEnv_);
    ContainerScope containerScope(engine, scopeId_);
    if (!containerScope.IsInitialized()) {
        HILOG_DEBUG("worker:: InitContainerScopeFunc error when HostOnMessageInner begin(only stage model)");
    }

    napi_value obj = NapiHelper::GetReferenceValue(hostEnv_, workerRef_);
    napi_value callback = NapiHelper::GetNameProperty(hostEnv_, obj, "onmessage");
    bool isCallable = NapiHelper::IsCallable(hostEnv_, callback);

    MessageDataType data = nullptr;
    while (hostMessageQueue_.DeQueue(&data)) {
        // receive close signal.
        if (data == nullptr) {
            HILOG_DEBUG("worker:: worker received close signal");
#if defined(ENABLE_WORKER_EVENTHANDLER)
            if ((!isMainThreadWorker_ || isLimitedWorker_) && !isHostEnvExited_) {
                CloseHostHandle();
            }
#else
            if (!isHostEnvExited_) {
                CloseHostHandle();
            }
#endif
            CloseHostCallback();
            return;
        }
        // handle data, call worker onMessage function to handle.
        napi_status status = napi_ok;
        HandleScope scope(hostEnv_, status);
        NAPI_CALL_RETURN_VOID(hostEnv_, status);
        napi_value result = nullptr;
        status = napi_deserialize(hostEnv_, data, &result);
        napi_delete_serialization_data(hostEnv_, data);
        if (status != napi_ok || result == nullptr) {
            HostOnMessageErrorInner();
            continue;
        }
        napi_value event = nullptr;
        napi_create_object(hostEnv_, &event);
        napi_set_named_property(hostEnv_, event, "data", result);
        napi_value argv[1] = { event };
        if (isCallable) {
            napi_value callbackResult = nullptr;
            napi_call_function(hostEnv_, obj, callback, 1, argv, &callbackResult);
        }
        // handle listeners.
        HandleEventListeners(hostEnv_, obj, 1, argv, "message");
        HandleHostException();
#if defined(ENABLE_WORKER_EVENTHANDLER)
        if (isMainThreadWorker_ && !isLimitedWorker_) {
            auto handler = OHOS::AppExecFwk::EventHandler::Current();
            if (handler && (handler->HasPendingHigherEvent() && !hostMessageQueue_.IsEmpty())) {
                PostWorkerMessageTask();
                break;
            }
        }
#endif
    }
}

void Worker::HostOnGlobalCall(const uv_async_t* req)
{
    HITRACE_HELPER_METER_NAME(__PRETTY_FUNCTION__);
    Worker* worker = static_cast<Worker*>(req->data);
    if (worker == nullptr) {
        HILOG_ERROR("worker:: worker is null");
        return;
    }
    worker->HostOnGlobalCallInner();
}

void Worker::HostOnGlobalCallInner()
{
    if (hostEnv_ == nullptr || HostIsStop()) {
        HILOG_ERROR("worker:: host thread maybe is over when host onmessage.");
        globalCallSuccess_ = false;
        cv_.notify_one();
        return;
    }

    napi_status scopeStatus = napi_ok;
    HandleScope handleScope(hostEnv_, scopeStatus);
    NAPI_CALL_RETURN_VOID(hostEnv_, scopeStatus);

    NativeEngine* engine = reinterpret_cast<NativeEngine*>(hostEnv_);
    ContainerScope containerScope(engine, scopeId_);
    if (!containerScope.IsInitialized()) {
        HILOG_WARN("worker:: InitContainerScopeFunc error when HostOnGlobalCallInner begin(only stage model)");
    }

    if (hostGlobalCallQueue_.IsEmpty()) {
        HILOG_ERROR("worker:: message queue is empty when HostOnGlobalCallInner");
        globalCallSuccess_ = false;
        cv_.notify_one();
        return;
    }
    MessageDataType data = nullptr;
    uint32_t currentCallId = 0;
    size_t size = hostGlobalCallQueue_.GetSize();
    if (size < 0 || size > GLOBAL_CALL_MAX_COUNT) {
        HILOG_ERROR("worker:: hostGlobalCallQueue_ size error");
        globalCallSuccess_ = false;
        cv_.notify_one();
        return;
    }
    for (size_t i = 0; i < size; i++) {
        std::pair<uint32_t, MessageDataType> pair = hostGlobalCallQueue_.Front();
        hostGlobalCallQueue_.Pop();
        if (pair.first == globalCallId_) {
            currentCallId = pair.first;
            data = pair.second;
            break;
        }
        napi_delete_serialization_data(hostEnv_, pair.second);
    }
    napi_value argsArray = nullptr;
    napi_status status = napi_ok;
    status = napi_deserialize(hostEnv_, data, &argsArray);
    napi_delete_serialization_data(hostEnv_, data);
    if (status != napi_ok || argsArray == nullptr) {
        AddGlobalCallError(ErrorHelper::ERR_WORKER_SERIALIZATION);
        globalCallSuccess_ = false;
        cv_.notify_one();
        return;
    }
    napi_value instanceName = nullptr;
    napi_get_element(hostEnv_, argsArray, 0, &instanceName);
    napi_value methodName = nullptr;
    napi_get_element(hostEnv_, argsArray, 1, &methodName);

    std::string instanceNameStr = NapiHelper::GetString(hostEnv_, instanceName);
    auto iter = globalCallObjects_.find(instanceNameStr);
    if (iter == globalCallObjects_.end()) {
        HILOG_ERROR("worker:: there is no instance: %{public}s registered for global call", instanceNameStr.c_str());
        AddGlobalCallError(ErrorHelper::ERR_TRIGGER_NONEXIST_EVENT);
        globalCallSuccess_ = false;
        cv_.notify_one();
        return;
    }
    napi_ref objRef = iter->second;
    napi_value obj = NapiHelper::GetReferenceValue(hostEnv_, objRef);
    bool hasProperty = false;
    napi_has_property(hostEnv_, obj, methodName, &hasProperty);
    if (!hasProperty) {
        std::string methodNameStr = NapiHelper::GetString(hostEnv_, methodName);
        HILOG_ERROR("worker:: registered obj for global call has no method: %{public}s", methodNameStr.c_str());
        AddGlobalCallError(ErrorHelper::ERR_CALL_METHOD_ON_BINDING_OBJ);
        globalCallSuccess_ = false;
        cv_.notify_one();
        return;
    }
    napi_value method = nullptr;
    napi_get_property(hostEnv_, obj, methodName, &method);
    // call method must not be generator function or async function
    bool validMethod = NapiHelper::IsCallable(hostEnv_, method) && !NapiHelper::IsAsyncFunction(hostEnv_, method) &&
        !NapiHelper::IsGeneratorFunction(hostEnv_, method);
    if (!validMethod) {
        std::string methodNameStr = NapiHelper::GetString(hostEnv_, methodName);
        HILOG_ERROR("worker:: method %{public}s shall be callable and not async or generator method",
            methodNameStr.c_str());
        AddGlobalCallError(ErrorHelper::ERR_CALL_METHOD_ON_BINDING_OBJ);
        globalCallSuccess_ = false;
        cv_.notify_one();
        return;
    }
    uint32_t argc = 0;
    napi_get_array_length(hostEnv_, argsArray, &argc);
    napi_value* args = nullptr;
    ObjectScope<napi_value> scope(args, true);
    if (argc > BEGIN_INDEX_OF_ARGUMENTS) {
        args = new napi_value[argc - BEGIN_INDEX_OF_ARGUMENTS];
        for (uint32_t index = 0; index < argc - BEGIN_INDEX_OF_ARGUMENTS; index++) {
            napi_get_element(hostEnv_, argsArray, index + BEGIN_INDEX_OF_ARGUMENTS, &args[index]);
        }
    }

    napi_value res = nullptr;
    napi_call_function(hostEnv_, obj, method, argc - BEGIN_INDEX_OF_ARGUMENTS, args, &res);
    bool hasPendingException = NapiHelper::IsExceptionPending(hostEnv_);
    if (hasPendingException) {
        napi_value exception = nullptr;
        napi_get_and_clear_last_exception(hostEnv_, &exception);
        napi_throw(hostEnv_, exception);
        globalCallSuccess_ = false;
        cv_.notify_one();
        return;
    }
    // defautly not transfer
    napi_value undefined = NapiHelper::GetUndefinedValue(hostEnv_);
    // meaningless to copy sendable object when call globalObject
    bool defaultClone = true;
    bool defaultTransfer = false;
    status = napi_serialize_inner(hostEnv_, res, undefined, undefined, defaultTransfer, defaultClone, &data);
    if (status != napi_ok || data == nullptr) {
        AddGlobalCallError(ErrorHelper::ERR_WORKER_SERIALIZATION);
        globalCallSuccess_ = false;
        cv_.notify_one();
        return;
    }
    // drop and destruct result if timeout
    if (currentCallId != globalCallId_ || currentCallId == 0) {
        napi_delete_serialization_data(hostEnv_, data);
        cv_.notify_one();
        return;
    }
    workerGlobalCallQueue_.EnQueue(data);
    globalCallSuccess_ = true;
    cv_.notify_one();
}

void Worker::AddGlobalCallError(int32_t errCode, napi_value errData)
{
    globalCallErrors_.push({errCode, errData});
}

void Worker::HandleGlobalCallError(napi_env env)
{
    while (!globalCallErrors_.empty()) {
        std::pair<int32_t, napi_value> pair = globalCallErrors_.front();
        globalCallErrors_.pop();
        int32_t errCode = pair.first;
        ErrorHelper::ThrowError(env, errCode);
    }
}

void Worker::ClearGlobalCallError(napi_env env)
{
    while (!globalCallErrors_.empty()) {
        std::pair<int32_t, napi_value> pair = globalCallErrors_.front();
        globalCallErrors_.pop();
        if (pair.second != nullptr) {
            napi_delete_serialization_data(env, pair.second);
        }
    }
}

void Worker::CallHostFunction(size_t argc, const napi_value* argv, const char* methodName) const
{
    if (hostEnv_ == nullptr) {
        HILOG_ERROR("worker:: host thread maybe is over");
        return;
    }
    if (HostIsStop()) {
        HILOG_ERROR("worker:: host thread maybe is over");
        WorkerThrowError(hostEnv_, ErrorHelper::ERR_WORKER_NOT_RUNNING,
            "host thread maybe is over when CallHostFunction");
        return;
    }
    napi_value obj = NapiHelper::GetReferenceValue(hostEnv_, workerRef_);
    napi_value callback = NapiHelper::GetNameProperty(hostEnv_, obj, methodName);
    bool isCallable = NapiHelper::IsCallable(hostEnv_, callback);
    if (!isCallable) {
        HILOG_DEBUG("worker:: host thread %{public}s is not Callable", methodName);
        return;
    }
    napi_value callbackResult = nullptr;
    napi_call_function(hostEnv_, obj, callback, argc, argv, &callbackResult);
    HandleHostException();
}

void Worker::CloseHostCallback()
{
    {
        napi_status status = napi_ok;
        HandleScope scope(hostEnv_, status);
        NAPI_CALL_RETURN_VOID(hostEnv_, status);
        napi_value exitValue = nullptr;
        if (isErrorExit_) {
            napi_create_int32(hostEnv_, 1, &exitValue); // 1 : exit because of error
        } else {
            napi_create_int32(hostEnv_, 0, &exitValue); // 0 : exit normally
        }
        napi_value argv[1] = { exitValue };
        CallHostFunction(1, argv, "onexit");
        napi_value obj = NapiHelper::GetReferenceValue(hostEnv_, workerRef_);
        // handle listeners
        HandleEventListeners(hostEnv_, obj, 1, argv, "exit");
    }
    CloseHelp::DeletePointer(this, false);
}

void Worker::HostOnError(const uv_async_t* req)
{
    Worker* worker = static_cast<Worker*>(req->data);
    if (worker == nullptr) {
        HILOG_ERROR("worker:: worker is null");
        return;
    }
    worker->HostOnErrorInner();
}

void Worker::HostOnErrorInner()
{
    if (hostEnv_ == nullptr || HostIsStop()) {
        HILOG_ERROR("worker:: host thread maybe is over when host onerror.");
        return;
    }
    napi_status status = napi_ok;
    HandleScope scope(hostEnv_, status);
    NAPI_CALL_RETURN_VOID(hostEnv_, status);
    NativeEngine* hostEngine = reinterpret_cast<NativeEngine*>(hostEnv_);
    ContainerScope containerScope(hostEngine, scopeId_);
    if (!containerScope.IsInitialized()) {
        HILOG_WARN("worker:: InitContainerScopeFunc error when HostOnErrorInner begin(only stage model)");
    }

    napi_value obj = NapiHelper::GetReferenceValue(hostEnv_, workerRef_);
    bool hasOnAllError = NapiHelper::HasNameProperty(hostEnv_, obj, "onAllErrors");
    napi_value callback = nullptr;
    if (hasOnAllError) {
        HILOG_INFO("worker:: host thread register onAllErrors.");
        callback = NapiHelper::GetNameProperty(hostEnv_, obj, "onAllErrors");
    } else {
        callback = NapiHelper::GetNameProperty(hostEnv_, obj, "onerror");
    }
    bool isCallable = NapiHelper::IsCallable(hostEnv_, callback);

    MessageDataType data;
    while (errorQueue_.DeQueue(&data)) {
        napi_value result = nullptr;
        napi_deserialize(hostEnv_, data, &result);
        napi_delete_serialization_data(hostEnv_, data);

        napi_value argv[1] = { result };
        if (isCallable) {
            napi_value callbackResult = nullptr;
            napi_call_function(hostEnv_, obj, callback, 1, argv, &callbackResult);
        }
        // handle listeners
        bool isHandle = HandleEventListeners(hostEnv_, obj, 1, argv, "error");
        if (!isCallable && !isHandle && !hasOnAllError) {
            napi_value businessError = ErrorHelper::ObjectToError(hostEnv_, result);
            napi_throw(hostEnv_, businessError);
            HandleHostException();
            TerminateInner();
            return;
        }
        HandleHostException();
    }
    if (!hasOnAllError) {
        // if host thread not register onAllErrors, worker still terminate.
        TerminateInner();
    }
}

void Worker::PostMessageInner(MessageDataType data)
{
    if (IsTerminated()) {
        HILOG_DEBUG("worker:: worker has been terminated when PostMessageInner.");
        return;
    }
    workerMessageQueue_.EnQueue(data);
    std::lock_guard<std::mutex> lock(workerOnmessageMutex_);
    if (data == nullptr) {
        HILOG_INFO("worker:: host post nullptr to worker.");
        if (workerOnTerminateInitState_) {
            ConcurrentHelper::UvCheckAndAsyncSend(workerOnTerminateSignal_);
        }
    } else {
        if (workerOnMessageInitState_) {
            ConcurrentHelper::UvCheckAndAsyncSend(workerOnMessageSignal_);
        }
    }
}

void Worker::HostOnMessageErrorInner()
{
    if (hostEnv_ == nullptr || HostIsStop()) {
        HILOG_ERROR("worker:: host thread maybe is over");
        return;
    }
    napi_value obj = NapiHelper::GetReferenceValue(hostEnv_, workerRef_);
    CallHostFunction(0, nullptr, "onmessageerror");
    // handle listeners
    HandleEventListeners(hostEnv_, obj, 0, nullptr, "messageerror");
}

void Worker::TerminateInner()
{
    if (IsTerminated() || IsTerminating()) {
        HILOG_INFO("worker:: worker is not in running when TerminateInner");
        return;
    }
    // 1. Update State
    UpdateWorkerState(TERMINATEING);
    // 2. send null signal
    PostMessageInner(nullptr);
}

void Worker::CloseInner()
{
    bool expected = false;
    if (isTerminated_.compare_exchange_weak(expected, true)) {
        HILOG_INFO("worker:: Close worker");
    } else {
        HILOG_DEBUG("worker:: worker is terminated when Close");
        return;
    }
    UpdateWorkerState(TERMINATEING);
    TerminateWorker();
}

bool Worker::UpdateWorkerState(RunnerState state)
{
    bool done = false;
    do {
        RunnerState oldState = runnerState_.load(std::memory_order_acquire);
        if (oldState >= state) {
            // make sure state sequence is start, running, terminating, terminated
            return false;
        }
        done = runnerState_.compare_exchange_strong(oldState, state);
    } while (!done);
    return true;
}

bool Worker::UpdateHostState(HostState state)
{
    bool done = false;
    do {
        HostState oldState = hostState_.load(std::memory_order_acquire);
        if (oldState >= state) {
            // make sure state sequence is ACTIVE, INACTIVE
            return false;
        }
        done = hostState_.compare_exchange_strong(oldState, state);
    } while (!done);
    return true;
}

void Worker::TerminateWorker()
{
    HITRACE_HELPER_METER_NAME(__PRETTY_FUNCTION__);
    // when there is no active handle, worker loop will stop automatic.
    {
        std::lock_guard<std::mutex> lock(workerOnmessageMutex_);
        ConcurrentHelper::UvHandleClose(workerOnMessageSignal_);
        ConcurrentHelper::UvHandleClose(workerOnTerminateSignal_);
    }
#if !defined(WINDOWS_PLATFORM) && !defined(MAC_PLATFORM)
    ConcurrentHelper::UvHandleClose(debuggerOnPostTaskSignal_);
#endif
    CloseWorkerCallback();
    uv_loop_t* loop = GetWorkerLoop();
    if (loop != nullptr) {
        Timer::ClearEnvironmentTimer(workerEnv_);
        uv_stop(loop);
    }
    UpdateWorkerState(TERMINATED);
}

void Worker::PublishWorkerOverSignal()
{
    if (HostIsStop()) {
        return;
    }
    // post nullptr tell host worker is not running
    hostMessageQueue_.EnQueue(nullptr);
#if defined(ENABLE_WORKER_EVENTHANDLER)
    if (isMainThreadWorker_ && !isLimitedWorker_) {
        PostWorkerOverTask();
    } else {
        uv_async_send(hostOnMessageSignal_);
    }
#else
    uv_async_send(hostOnMessageSignal_);
#endif
}

#if defined(ENABLE_WORKER_EVENTHANDLER)
void Worker::PostWorkerOverTask()
{
    std::weak_ptr<WorkerWrapper> weak = workerWrapper_;
    auto hostOnOverSignalTask = [weak]() {
        auto strong = weak.lock();
        if (strong) {
            HILOG_INFO("worker:: host receive terminate.");
            HITRACE_HELPER_METER_NAME("Worker:: HostOnTerminateSignal");
            strong->GetWorker()->HostOnMessageInner();
        } else {
            HILOG_INFO("worker:: worker is null.");
        }
    };
    GetMainThreadHandler()->PostTask(hostOnOverSignalTask, "WorkerHostOnOverSignalTask",
        0, OHOS::AppExecFwk::EventQueue::Priority::HIGH);
}

void Worker::PostWorkerErrorTask()
{
    auto hostOnErrorTask = [this]() {
        if (IsValidWorker(this)) {
            HILOG_INFO("worker:: host receive error.");
            HITRACE_HELPER_METER_NAME("Worker:: HostOnErrorMessage");
            this->HostOnErrorInner();
        }
    };
    GetMainThreadHandler()->PostTask(hostOnErrorTask, "WorkerHostOnErrorTask",
        0, OHOS::AppExecFwk::EventQueue::Priority::HIGH);
}

void Worker::PostWorkerMessageTask()
{
    auto hostOnMessageTask = [this]() {
        if (IsValidWorker(this)) {
            HILOG_DEBUG("worker:: host thread receive message.");
            HITRACE_HELPER_METER_NAME("Worker:: HostOnMessage");
            this->HostOnMessageInner();
        }
    };
    GetMainThreadHandler()->PostTask(hostOnMessageTask, "WorkerHostOnMessageTask",
        0, OHOS::AppExecFwk::EventQueue::Priority::HIGH);
}

void Worker::PostWorkerGlobalCallTask()
{
    auto hostOnGlobalCallTask = [this]() {
        if (IsValidWorker(this)) {
            HILOG_DEBUG("worker:: host thread receive globalCall signal.");
            HITRACE_HELPER_METER_NAME("Worker:: HostOnGlobalCallSignal");
            this->HostOnGlobalCallInner();
        }
    };
    GetMainThreadHandler()->PostTask(hostOnGlobalCallTask, "WorkerHostOnGlobalCallTask",
        0, OHOS::AppExecFwk::EventQueue::Priority::HIGH);
}

void Worker::PostWorkerExceptionTask()
{
    auto hostOnAllErrorsTask = [this]() {
        if (IsValidWorker(this)) {
            HILOG_INFO("worker:: host receive exception.");
            HITRACE_HELPER_METER_NAME("Worker:: HostOnAllErrorsMessage");
            this->HostOnAllErrorsInner();
        }
    };
    GetMainThreadHandler()->PostTask(hostOnAllErrorsTask, "WorkerHostOnAllErrorsTask",
        0, OHOS::AppExecFwk::EventQueue::Priority::HIGH);
}
#endif

bool Worker::IsValidWorker(Worker* worker)
{
    std::lock_guard<std::mutex> lock(g_workersMutex);
    std::list<Worker*>::iterator it = std::find(g_workers.begin(), g_workers.end(), worker);
    if (it == g_workers.end()) {
        return false;
    }
    return true;
}

bool Worker::IsValidLimitedWorker(Worker* limitedWorker)
{
    std::lock_guard<std::mutex> lock(g_limitedworkersMutex);
    std::list<Worker*>::iterator it = std::find(g_limitedworkers.begin(), g_limitedworkers.end(), limitedWorker);
    if (it == g_limitedworkers.end()) {
        return false;
    }
    return true;
}

void Worker::WorkerOnMessage(const uv_async_t* req)
{
    HITRACE_HELPER_METER_NAME(__PRETTY_FUNCTION__);
    Worker* worker = static_cast<Worker*>(req->data);
    if (worker == nullptr) {
        HILOG_ERROR("worker::worker is null");
        return;
    }
    worker->WorkerOnMessageInner();
}

void Worker::WorkerOnMessageInner()
{
    if (IsTerminated()) {
        return;
    }
    WorkerRunningScope workerRunningScope(workerEnv_);
    napi_status status;
    napi_handle_scope scope = nullptr;
    status = napi_open_handle_scope(workerEnv_, &scope);
    if (status != napi_ok || scope == nullptr) {
        HILOG_ERROR("worker:: WorkerOnMessage open handle scope failed.");
        return;
    }
    MessageDataType data = nullptr;
    while (!IsTerminated() && workerMessageQueue_.DeQueue(&data)) {
        if (data == nullptr) {
            HILOG_DEBUG("worker:: worker reveive terminate signal");
            // Close handlescope need before TerminateWorker
            napi_close_handle_scope(workerEnv_, scope);
            TerminateWorker();
            return;
        }
        napi_value result = nullptr;
        status = napi_deserialize(workerEnv_, data, &result);
        napi_delete_serialization_data(workerEnv_, data);
        if (status != napi_ok || result == nullptr) {
            WorkerOnMessageErrorInner();
            continue;
        }

        napi_value event = nullptr;
        napi_create_object(workerEnv_, &event);
        napi_set_named_property(workerEnv_, event, "data", result);
        napi_value argv[1] = { event };
        CallWorkerFunction(1, argv, "onmessage", true);

        napi_value obj = NapiHelper::GetReferenceValue(workerEnv_, this->workerPort_);
        ParentPortHandleEventListeners(workerEnv_, obj, 1, argv, "message", true);
    }
    napi_close_handle_scope(workerEnv_, scope);
}

bool Worker::HandleEventListeners(napi_env env, napi_value recv, size_t argc, const napi_value* argv, const char* type)
{
    std::string listener(type);
    auto iter = eventListeners_.find(listener);
    if (iter == eventListeners_.end()) {
        HILOG_DEBUG("worker:: there is no listener for type %{public}s in host thread", type);
        return false;
    }

    std::list<WorkerListener*>& listeners = iter->second;
    std::list<WorkerListener*>::iterator it = listeners.begin();
    while (it != listeners.end()) {
        WorkerListener* data = *it++;
        napi_value callbackObj = NapiHelper::GetReferenceValue(env, data->callback_);
        if (!NapiHelper::IsCallable(env, callbackObj)) {
            HILOG_WARN("worker:: host thread listener %{public}s is not callable", type);
            return false;
        }
        napi_value callbackResult = nullptr;
        napi_call_function(env, recv, callbackObj, argc, argv, &callbackResult);
        if (!data->NextIsAvailable()) {
            listeners.remove(data);
            CloseHelp::DeletePointer(data, false);
        }
    }
    return true;
}

void Worker::HandleHostException() const
{
    if (!NapiHelper::IsExceptionPending(hostEnv_)) {
        return;
    }
    auto hostEngine = reinterpret_cast<NativeEngine*>(hostEnv_);
    hostEngine->HandleUncaughtException();
}

void Worker::HandleException()
{
    if (!NapiHelper::IsExceptionPending(workerEnv_)) {
        return;
    }

    napi_status status = napi_ok;
    HandleScope scope(workerEnv_, status);
    NAPI_CALL_RETURN_VOID(workerEnv_, status);
    napi_value exception;
    napi_get_and_clear_last_exception(workerEnv_, &exception);
    if (exception == nullptr) {
        return;
    }
    NapiErrorManager::GetInstance()->NotifyUncaughtException(workerEnv_, exception, this->GetName(), WORKER_TYPE);
    HandleUncaughtException(exception);
}

void Worker::HandleUncaughtException(napi_value exception)
{
    napi_value obj = ErrorHelper::TranslateErrorEvent(workerEnv_, exception);

    // WorkerGlobalScope onerror
    WorkerOnErrorInner(obj);

    if (hostEnv_ == nullptr) {
        HILOG_ERROR("worker:: host engine is nullptr.");
        return;
    }
    MessageDataType data = nullptr;
    napi_value undefined = NapiHelper::GetUndefinedValue(workerEnv_);
    napi_serialize_inner(workerEnv_, obj, undefined, undefined, false, true, &data);
    {
        std::lock_guard<std::recursive_mutex> lock(liveStatusLock_);
        if (HostIsStop() || isHostEnvExited_) {
            return;
        }
        errorQueue_.EnQueue(data);
#if defined(ENABLE_WORKER_EVENTHANDLER)
        if (isMainThreadWorker_ && !isLimitedWorker_) {
            PostWorkerErrorTask();
        } else {
            uv_async_send(hostOnErrorSignal_);
        }
#else
        uv_async_send(hostOnErrorSignal_);
#endif
    }
}

void Worker::WorkerOnMessageErrorInner()
{
    isErrorExit_ = true;
    CallWorkerFunction(0, nullptr, "onmessageerror", true);
    napi_value obj = NapiHelper::GetReferenceValue(workerEnv_, this->workerPort_);
    ParentPortHandleEventListeners(workerEnv_, obj, 0, nullptr, "messageerror", true);
}

void Worker::PostMessageToHostInner(MessageDataType data)
{
    std::lock_guard<std::recursive_mutex> lock(liveStatusLock_);
    if (hostEnv_ != nullptr && !HostIsStop() && !isHostEnvExited_) {
        hostMessageQueue_.EnQueue(data);
#if defined(ENABLE_WORKER_EVENTHANDLER)
        if (isMainThreadWorker_ && !isLimitedWorker_) {
            PostWorkerMessageTask();
        } else {
            uv_async_send(hostOnMessageSignal_);
        }
#else
        uv_async_send(hostOnMessageSignal_);
#endif
    } else {
        HILOG_ERROR("worker:: worker host engine is nullptr when PostMessageToHostInner.");
    }
}

bool Worker::WorkerListener::operator==(const WorkerListener& listener) const
{
    napi_value obj = NapiHelper::GetReferenceValue(listener.env_, listener.callback_);
    napi_value compareObj = NapiHelper::GetReferenceValue(env_, callback_);
    // the env of listener and cmp listener must be same env because of Synchronization method
    return NapiHelper::StrictEqual(env_, compareObj, obj);
}

void Worker::AddListenerInner(napi_env env, const char* type, const WorkerListener* listener)
{
    std::string typestr(type);
    auto iter = eventListeners_.find(typestr);
    if (iter == eventListeners_.end()) {
        std::list<WorkerListener*> listeners;
        listeners.emplace_back(const_cast<WorkerListener*>(listener));
        eventListeners_[typestr] = listeners;
    } else {
        std::list<WorkerListener*>& listenerList = iter->second;
        std::list<WorkerListener*>::iterator it = std::find_if(
            listenerList.begin(), listenerList.end(), Worker::FindWorkerListener(env, listener->callback_));
        if (it != listenerList.end()) {
            return;
        }
        listenerList.emplace_back(const_cast<WorkerListener*>(listener));
    }
}

void Worker::RemoveListenerInner(napi_env env, const char* type, napi_ref callback)
{
    std::string typestr(type);
    auto iter = eventListeners_.find(typestr);
    if (iter == eventListeners_.end()) {
        return;
    }
    std::list<WorkerListener*>& listenerList = iter->second;
    if (callback != nullptr) {
        std::list<WorkerListener*>::iterator it =
            std::find_if(listenerList.begin(), listenerList.end(), Worker::FindWorkerListener(env, callback));
        if (it != listenerList.end()) {
            CloseHelp::DeletePointer(*it, false);
            listenerList.erase(it);
        }
    } else {
        for (auto it = listenerList.begin(); it != listenerList.end(); it++) {
            CloseHelp::DeletePointer(*it, false);
        }
        eventListeners_.erase(typestr);
    }
}

Worker::~Worker()
{
    std::lock_guard<std::recursive_mutex> lock(liveStatusLock_);
    if (!HostIsStop() && !isHostEnvExited_) {
        ReleaseHostThreadContent();
        RemoveAllListenerInner();
        ClearGlobalCallObject();
    }
}

void Worker::RemoveAllListenerInner()
{
    for (auto iter = eventListeners_.begin(); iter != eventListeners_.end(); iter++) {
        std::list<WorkerListener*>& listeners = iter->second;
        for (auto item = listeners.begin(); item != listeners.end(); item++) {
            WorkerListener* listener = *item;
            CloseHelp::DeletePointer(listener, false);
        }
    }
    eventListeners_.clear();
}

void Worker::ReleaseHostThreadContent()
{
    ClearHostMessage(hostEnv_);
    if (!HostIsStop()) {
        napi_status status = napi_ok;
        HandleScope scope(hostEnv_, status);
        NAPI_CALL_RETURN_VOID(hostEnv_, status);
        // 3. set thisVar's nativepointer be null
        napi_value thisVar = NapiHelper::GetReferenceValue(hostEnv_, workerRef_);
        Worker* worker = nullptr;
        napi_remove_wrap(hostEnv_, thisVar, reinterpret_cast<void**>(&worker));
        hostEnv_ = nullptr;
        // 4. set workerRef_ be null
        workerRef_ = nullptr;
    }
}

void Worker::WorkerOnErrorInner(napi_value error)
{
    isErrorExit_ = true;
    napi_value argv[1] = { error };
    CallWorkerFunction(1, argv, "onerror", false);
    napi_value obj = NapiHelper::GetReferenceValue(workerEnv_, this->workerPort_);
    ParentPortHandleEventListeners(workerEnv_, obj, 1, argv, "error", false);
}

bool Worker::CallWorkerFunction(size_t argc, const napi_value* argv, const char* methodName, bool tryCatch)
{
    if (workerEnv_ == nullptr) {
        HILOG_ERROR("Worker:: worker is not running when call workerPort.%{public}s.", methodName);
        return false;
    }
    napi_value callback = NapiHelper::GetNamePropertyInParentPort(workerEnv_, workerPort_, methodName);
    bool isCallable = NapiHelper::IsCallable(workerEnv_, callback);
    if (!isCallable) {
        HILOG_WARN("worker:: workerPort.%{public}s is not Callable", methodName);
        return false;
    }
    napi_value workerPortObj = NapiHelper::GetReferenceValue(workerEnv_, workerPort_);
    napi_value callbackResult = nullptr;
    napi_call_function(workerEnv_, workerPortObj, callback, argc, argv, &callbackResult);
    if (tryCatch && callbackResult == nullptr) {
        HILOG_ERROR("worker:: workerPort.%{public}s handle exception", methodName);
        HandleException();
        return false;
    }
    return true;
}

void Worker::CloseWorkerCallback()
{
    CallWorkerFunction(0, nullptr, "onclose", true);
    // off worker inited environment
    {
        std::lock_guard<std::recursive_mutex> lock(liveStatusLock_);
        if (HostIsStop() || isHostEnvExited_) {
            return;
        }
        auto hostEngine = reinterpret_cast<NativeEngine*>(hostEnv_);
        if (!hostEngine->CallOffWorkerFunc(reinterpret_cast<NativeEngine*>(workerEnv_))) {
            HILOG_ERROR("worker:: CallOffWorkerFunc error");
        }
    }
}

void Worker::ReleaseWorkerThreadContent()
{
    HITRACE_HELPER_METER_NAME(__PRETTY_FUNCTION__);
    {
        std::lock_guard<std::recursive_mutex> lock(liveStatusLock_);
        if (!HostIsStop() && !isHostEnvExited_) {
            auto hostEngine = reinterpret_cast<NativeEngine*>(hostEnv_);
            auto workerEngine = reinterpret_cast<NativeEngine*>(workerEnv_);
            if (hostEngine != nullptr && workerEngine != nullptr) {
                if (!hostEngine->DeleteWorker(workerEngine)) {
                    HILOG_ERROR("worker:: DeleteWorker error");
                }
                hostEngine->DecreaseSubEnvCounter();
            }
        }
    }
    // 1. delete worker listener
    ParentPortRemoveAllListenerInner();

    // 2. delete worker's parentPort
    NapiHelper::DeleteReference(workerEnv_, workerPort_);
    workerPort_ = nullptr;

    // 3. clear message send to worker thread
    workerMessageQueue_.Clear(workerEnv_);
    workerGlobalCallQueue_.Clear(workerEnv_);

#if defined(ENABLE_CONCURRENCY_INTEROP)
    DetachWorkerFromAniVm();
#endif

    CloseHelp::DeletePointer(reinterpret_cast<NativeEngine*>(workerEnv_), false);
    workerEnv_ = nullptr;
}

void Worker::ParentPortAddListenerInner(napi_env env, const char* type, const WorkerListener* listener)
{
    std::string typestr(type);
    auto iter = parentPortEventListeners_.find(typestr);
    if (iter == parentPortEventListeners_.end()) {
        std::list<WorkerListener*> listeners;
        listeners.emplace_back(const_cast<WorkerListener*>(listener));
        parentPortEventListeners_[typestr] = listeners;
    } else {
        std::list<WorkerListener*>& listenerList = iter->second;
        std::list<WorkerListener*>::iterator it = std::find_if(
            listenerList.begin(), listenerList.end(), Worker::FindWorkerListener(env, listener->callback_));
        if (it != listenerList.end()) {
            return;
        }
        listenerList.emplace_back(const_cast<WorkerListener*>(listener));
    }
}

void Worker::ParentPortRemoveAllListenerInner()
{
    for (auto iter = parentPortEventListeners_.begin(); iter != parentPortEventListeners_.end(); iter++) {
        std::list<WorkerListener*>& listeners = iter->second;
        for (auto item = listeners.begin(); item != listeners.end(); item++) {
            WorkerListener* listener = *item;
            CloseHelp::DeletePointer(listener, false);
        }
    }
    parentPortEventListeners_.clear();
}

void Worker::ParentPortRemoveListenerInner(napi_env env, const char* type, napi_ref callback)
{
    std::string typestr(type);
    auto iter = parentPortEventListeners_.find(typestr);
    if (iter == parentPortEventListeners_.end()) {
        return;
    }
    std::list<WorkerListener*>& listenerList = iter->second;
    if (callback != nullptr) {
        std::list<WorkerListener*>::iterator it =
            std::find_if(listenerList.begin(), listenerList.end(), Worker::FindWorkerListener(env, callback));
        if (it != listenerList.end()) {
            CloseHelp::DeletePointer(*it, false);
            listenerList.erase(it);
        }
    } else {
        for (auto it = listenerList.begin(); it != listenerList.end(); it++) {
            CloseHelp::DeletePointer(*it, false);
        }
        parentPortEventListeners_.erase(typestr);
    }
}

void Worker::ParentPortHandleEventListeners(napi_env env, napi_value recv, size_t argc,
                                            const napi_value* argv, const char* type, bool tryCatch)
{
    std::string listener(type);
    auto iter = parentPortEventListeners_.find(listener);
    if (iter == parentPortEventListeners_.end()) {
        HILOG_DEBUG("worker:: there is no listener for type %{public}s in worker thread", type);
        return;
    }

    std::list<WorkerListener*>& listeners = iter->second;
    std::list<WorkerListener*>::iterator it = listeners.begin();
    while (it != listeners.end()) {
        WorkerListener* data = *it++;
        napi_value callbackObj = NapiHelper::GetReferenceValue(env, data->callback_);
        if (!NapiHelper::IsCallable(env, callbackObj)) {
            HILOG_WARN("worker:: workerPort.addEventListener %{public}s is not callable", type);
            return;
        }
        napi_value callbackResult = nullptr;
        napi_call_function(env, recv, callbackObj, argc, argv, &callbackResult);
        if (!data->NextIsAvailable()) {
            listeners.remove(data);
            CloseHelp::DeletePointer(data, false);
        }
        if (tryCatch && callbackResult == nullptr) {
            HandleException();
            return;
        }
    }
}

void Worker::WorkerThrowError(napi_env env, int32_t errCode, const char* errMessage)
{
    auto mainThreadEngine = NativeEngine::GetMainThreadEngine();
    if (mainThreadEngine == nullptr) {
        HILOG_ERROR("worker:: mainThreadEngine is nullptr");
        return;
    }
    if (mainThreadEngine->IsTargetWorkerVersion(WorkerVersion::NEW)) {
        ErrorHelper::ThrowError(env, errCode, errMessage);
    }
}

bool Worker::CanCreateWorker(napi_env env, WorkerVersion target)
{
    auto mainThreadEngine = NativeEngine::GetMainThreadEngine();
    if (mainThreadEngine == nullptr) {
        HILOG_ERROR("worker:: mainThreadEngine is nullptr");
        return false;
    }
    if (mainThreadEngine->CheckAndSetWorkerVersion(WorkerVersion::NONE, target) ||
        mainThreadEngine->IsTargetWorkerVersion(target)) {
        return true;
    }
    return false;
}

#if !defined(WINDOWS_PLATFORM) && !defined(MAC_PLATFORM)
void Worker::HandleDebuggerTask(const uv_async_t* req)
{
    Worker* worker = static_cast<Worker*>(req->data);
    if (worker == nullptr) {
        HILOG_ERROR("worker::worker is null");
        return;
    }

    worker->debuggerMutex_.lock();
    auto task = std::move(worker->debuggerQueue_.front());
    worker->debuggerQueue_.pop();
    worker->debuggerMutex_.unlock();
    task();
}

void Worker::DebuggerOnPostTask(std::function<void()>&& task)
{
    if (IsTerminated()) {
        HILOG_ERROR("worker:: worker has been terminated.");
        return;
    }
    if (ConcurrentHelper::IsUvActive(debuggerOnPostTaskSignal_)) {
        std::lock_guard<std::mutex> lock(debuggerMutex_);
        debuggerQueue_.push(std::move(task));
        uv_async_send(debuggerOnPostTaskSignal_);
    }
}
#endif

void Worker::InitHostHandle(uv_loop_t* loop)
{
    ConcurrentHelper::UvHandleInit(loop, hostOnMessageSignal_, Worker::HostOnMessage, this);
    ConcurrentHelper::UvHandleInit(loop, hostOnErrorSignal_, Worker::HostOnError, this);
    ConcurrentHelper::UvHandleInit(loop, hostOnAllErrorsSignal_, Worker::HostOnAllErrors, this);
    ConcurrentHelper::UvHandleInit(loop, hostOnGlobalCallSignal_, Worker::HostOnGlobalCall, this);
}

void Worker::CloseHostHandle()
{
    if (ConcurrentHelper::IsUvActive(hostOnMessageSignal_)) {
        ConcurrentHelper::UvHandleClose(hostOnMessageSignal_);
    }
    if (ConcurrentHelper::IsUvActive(hostOnErrorSignal_)) {
        ConcurrentHelper::UvHandleClose(hostOnErrorSignal_);
    }
    if (ConcurrentHelper::IsUvActive(hostOnAllErrorsSignal_)) {
        ConcurrentHelper::UvHandleClose(hostOnAllErrorsSignal_);
    }
    if (ConcurrentHelper::IsUvActive(hostOnGlobalCallSignal_)) {
        ConcurrentHelper::UvHandleClose(hostOnGlobalCallSignal_);
    }
}

void Worker::EraseWorker()
{
    if (!isLimitedWorker_) {
        std::lock_guard<std::mutex> lock(g_workersMutex);
        std::list<Worker*>::iterator it = std::find(g_workers.begin(), g_workers.end(), this);
        if (it != g_workers.end()) {
            Worker* worker = *it;
            if (worker != nullptr) {
                WorkerManager::DecrementWorkerCount(worker->workerType_);
            }
            g_workers.erase(it);
        }
    } else {
        std::lock_guard<std::mutex> lock(g_limitedworkersMutex);
        std::list<Worker*>::iterator it = std::find(g_limitedworkers.begin(), g_limitedworkers.end(), this);
        if (it != g_limitedworkers.end()) {
            Worker* worker = *it;
            if (worker != nullptr) {
                WorkerManager::DecrementWorkerCount(worker->workerType_);
            }
            g_limitedworkers.erase(it);
        }
    }
}

void Worker::ClearHostMessage(napi_env env)
{
    hostMessageQueue_.Clear(env);
    hostGlobalCallQueue_.Clear(env);
    errorQueue_.Clear(env);
    exceptionQueue_.Clear(env);
}

#ifdef ENABLE_QOS
void Worker::SetQOSLevel()
{
    if (workerPriority_ == WorkerPriority::INVALID) {
        return;
    }
    auto iter = WORKERPRIORITY_QOSLEVEL_MAP.find(workerPriority_);
    if (iter == WORKERPRIORITY_QOSLEVEL_MAP.end()) {
        HILOG_ERROR("worker:: not in WORKERPRIORITY_QOSLEVEL_MAP");
        return;
    }
    OHOS::QOS::QosLevel qosLevel = iter->second;
    if (qosLevel != OHOS::QOS::QosLevel::QOS_MAX) {
        HILOG_INFO("SetThreadQos %{public}d", qosLevel);
        int ret = SetThreadQos(qosLevel);
        if (ret != 0) {
            HILOG_ERROR("worker:: SetThreadQos failed, return %{public}d", ret);
        }
        if (qosUpdatedCallback_ != nullptr) {
            qosUpdatedCallback_();
        }
    }
}
#endif

void Worker::HostOnAllErrors(const uv_async_t* req)
{
    Worker* worker = static_cast<Worker*>(req->data);
    if (worker == nullptr) {
        HILOG_ERROR("worker:: worker is null");
        return;
    }
    worker->HostOnAllErrorsInner();
}

void Worker::HandleWorkerUncaughtException(napi_env env, napi_value exception)
{
    if (NapiHelper::IsExceptionPending(env)) {
        napi_get_and_clear_last_exception(env, &exception);
    }

    if (hostEnv_ == nullptr) {
        HILOG_ERROR("worker:: host engine is nullptr.");
        return;
    }

    MessageDataType data = nullptr;
    napi_value undefined = NapiHelper::GetUndefinedValue(env);
    napi_serialize_inner(env, exception, undefined, undefined, false, true, &data);
    {
        std::lock_guard<std::recursive_mutex> lock(liveStatusLock_);
        if (HostIsStop() || isHostEnvExited_) {
            return;
        }
        exceptionQueue_.EnQueue(data);
#if defined(ENABLE_WORKER_EVENTHANDLER)
        if (isMainThreadWorker_ && !isLimitedWorker_) {
            PostWorkerExceptionTask();
        } else {
            uv_async_send(hostOnAllErrorsSignal_);
        }
#else
        uv_async_send(hostOnAllErrorsSignal_);
#endif
    }
}

void Worker::HostOnAllErrorsInner()
{
    if (hostEnv_ == nullptr || HostIsStop()) {
        HILOG_ERROR("worker:: host thread maybe is over when host onerror.");
        return;
    }
    napi_status status = napi_ok;
    HandleScope scope(hostEnv_, status);
    NAPI_CALL_RETURN_VOID(hostEnv_, status);
    NativeEngine* hostEngine = reinterpret_cast<NativeEngine*>(hostEnv_);
    ContainerScope containerScope(hostEngine, scopeId_);
    if (!containerScope.IsInitialized()) {
        HILOG_WARN("worker:: InitContainerScopeFunc error when HostOnAllErrorsInner begin(only stage model)");
    }

    napi_value obj = NapiHelper::GetReferenceValue(hostEnv_, workerRef_);
    napi_value callback = NapiHelper::GetNameProperty(hostEnv_, obj, "onAllErrors");
    bool isCallable = NapiHelper::IsCallable(hostEnv_, callback);
    if (!isCallable) {
        HILOG_INFO("worker:: worker may not register onAllErrors.");
        exceptionQueue_.Clear(hostEnv_);
        return;
    }

    MessageDataType data = nullptr;
    while (exceptionQueue_.DeQueue(&data)) {
        if (data == nullptr) {
            return;
        }
        napi_value result = nullptr;
        napi_deserialize(hostEnv_, data, &result);
        napi_delete_serialization_data(hostEnv_, data);

        napi_value argv[1] = { result };
        napi_value callbackResult = nullptr;
        napi_call_function(hostEnv_, obj, callback, 1, argv, &callbackResult);
        HandleHostException();
    }
}

#if defined(ENABLE_CONCURRENCY_INTEROP)
void Worker::AttachWorkerEnvToAniVm()
{
    if (!ANIHelper::IsConcurrencySupportInterop()) {
        return;
    }
    std::string interop = "--interop=enable";
    ani_option interopEnabled {interop.data(), (void *)workerEnv_};
    ani_options aniArgs {1, &interopEnabled};
    auto* aniVm = ANIHelper::GetAniVm();
    if (aniVm == nullptr) {
        HILOG_ERROR("worker:: AttachWorkerEnvToAniVm aviVm is null");
        return;
    }
    ani_status status = aniVm->AttachCurrentThread(&aniArgs, ANI_VERSION_1, &aniEnv_);
    if (status != ANI_OK || aniEnv_ == nullptr) {
        HILOG_ERROR("worker:: AttachCurrentThread failed.");
    }
}

void Worker::DetachWorkerFromAniVm()
{
    if (!ANIHelper::IsConcurrencySupportInterop()) {
        return;
    }
    auto* aniVm = ANIHelper::GetAniVm();
    if (aniVm == nullptr) {
        HILOG_ERROR("worker:: aviVm is null when DetachWorkerFromAniVm.");
        return;
    }
    ani_status status = aniVm->DetachCurrentThread();
    if (status != ANI_OK) {
        HILOG_ERROR("worker:: DetachCurrentThread failed.");
    }
}
#endif

} // namespace Commonlibrary::Concurrent::WorkerModule
