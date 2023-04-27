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

#ifndef JS_CONCURRENT_MODULE_TASKPOOL_TASK_H_
#define JS_CONCURRENT_MODULE_TASKPOOL_TASK_H_

#include <mutex>
#include <queue>
#include <uv.h>

#include "napi/native_api.h"

namespace Commonlibrary::Concurrent::TaskPoolModule {
enum TaskState { NOT_FOUND, WAITING, RUNNING, TERMINATED, CANCELED };
enum Priority { HIGH, MEDIUM, LOW, NUMBER, DEFAULT = MEDIUM };

class Task {
public:
    static napi_value TaskConstructor(napi_env env, napi_callback_info cbinfo);

private:
    Task() = delete;
    ~Task() = delete;
    Task(const Task &) = delete;
    Task& operator=(const Task &) = delete;
    Task(Task &&) = delete;
    Task& operator=(Task &&) = delete;
};

struct TaskInfo {
    napi_env env = nullptr;
    napi_deferred deferred = nullptr;
    napi_value result = nullptr;
    napi_value serializationFunction = nullptr;
    napi_value serializationArguments = nullptr;
    uv_async_t *onResultSignal = nullptr;
    uint32_t taskId;
    uint32_t executeId;
    bool success = true;
    void *worker = nullptr;
};
} // namespace Commonlibrary::Concurrent::TaskPoolModule
#endif // JS_CONCURRENT_MODULE_TASKPOOL_TASK_H_