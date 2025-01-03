/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef JS_CONCURRENT_MODULE_TASKPOOL_ASYNC_RUNNER_MANAGER_H
#define JS_CONCURRENT_MODULE_TASKPOOL_ASYNC_RUNNER_MANAGER_H

#include <mutex>
#include <unordered_map>

#include "async_runner.h"
#include "napi/native_api.h"
#include "task.h"

namespace Commonlibrary::Concurrent::TaskPoolModule {
using namespace Commonlibrary::Concurrent::Common::Helper;

class AsyncRunnerManager {
public:
    AsyncRunnerManager() = default;
    ~AsyncRunnerManager() = default;

    static AsyncRunnerManager &GetInstance();
    AsyncRunner* CreateOrGetGlobalRunner(napi_env env, napi_value thisVar, const std::string &name,
                                         uint32_t runningCapacity, uint32_t waitingCapacity);
    bool TriggerAsyncRunner(napi_env env, Task* lastTask);
    void StoreAsyncRunner(uint64_t asyncRunnerId, AsyncRunner* asyncRunner);
    void RemoveAsyncRunner(uint64_t asyncRunnerId);
    AsyncRunner* GetAsyncRunner(uint64_t asyncRunnerId);
    void RemoveGlobalAsyncRunnerRef(napi_env env, AsyncRunner* asyncRunner);
    void RemoveGlobalAsyncRunner(const std::string &name);
    void GlobalAsyncRunnerDestructor(napi_env env, AsyncRunner* asyncRunner);
    void CancelAsyncRunnerTask(napi_env env, Task *task);

private:
    AsyncRunnerManager(const AsyncRunnerManager &) = delete;
    AsyncRunnerManager& operator=(const AsyncRunnerManager &) = delete;
    AsyncRunnerManager(AsyncRunnerManager &&) = delete;
    AsyncRunnerManager& operator=(AsyncRunnerManager &&) = delete;

    // <asyncRunnerId, AsyncRunner>
    std::unordered_map<uint64_t, AsyncRunner*> asyncRunners_ {};
    std::mutex asyncRunnersMutex_;
    // <<name1, AsyncRunner>, <name2, AsyncRunner>, ...>
    std::unordered_map<std::string, AsyncRunner*> globalAsyncRunner_ {};
    std::mutex globalAsyncRunnerMutex_;
};
} // namespace Commonlibrary::Concurrent::TaskPoolModule
#endif // JS_CONCURRENT_MODULE_TASKPOOL_ASYNC_RUNNER_MANAGER_H