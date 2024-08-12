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

#include "test.h"

#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "taskpool.h"

namespace Commonlibrary::Concurrent::TaskPoolModule {
napi_value SendableUtils::CreateSendableClass(napi_env env)
{
    auto constructor = [](napi_env env, napi_callback_info info) -> napi_value {
        napi_value thisVar = nullptr;
        napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr);
        return thisVar;
    };

    napi_property_descriptor props[] = {
        DECLARE_NAPI_FUNCTION("foo", Foo),
        DECLARE_NAPI_FUNCTION("bar", Bar),
    };

    napi_value sendableClass = nullptr;
    napi_define_sendable_class(env, "SendableClass", NAPI_AUTO_LENGTH, constructor, nullptr,
                               sizeof(props) / sizeof(props[0]), props, nullptr, &sendableClass);
    return sendableClass;
}

napi_value SendableUtils::CreateSendableInstance(napi_env env)
{
    napi_value cls = SendableUtils::CreateSendableClass(env);
    napi_value instance = nullptr;
    napi_new_instance(env, cls, 0, nullptr, &instance);
    return instance;
}

napi_value SendableUtils::Foo(napi_env env, napi_callback_info info)
{
    return nullptr;
}

napi_value SendableUtils::Bar(napi_env env, napi_callback_info info)
{
    return nullptr;
}

napi_value NativeEngineTest::IsConcurrent(napi_env env, napi_value argv[], size_t argc)
{
    ExceptionScope scope(env);
    std::string funcName = "IsConcurrent";
    napi_value cb = nullptr;
    napi_value result = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), TaskPool::IsConcurrent, nullptr, &cb);
    napi_call_function(env, nullptr, cb, argc, argv, &result);
    return result;
}

napi_value NativeEngineTest::GetTaskPoolInfo(napi_env env, napi_value argv[], size_t argc)
{
    ExceptionScope scope(env);
    std::string funcName = "GetTaskPoolInfo";
    napi_value cb = nullptr;
    napi_value result = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), TaskPool::GetTaskPoolInfo, nullptr, &cb);
    napi_call_function(env, nullptr, cb, argc, argv, &result);
    return result;
}

napi_value NativeEngineTest::TerminateTask(napi_env env, napi_value argv[], size_t argc)
{
    ExceptionScope scope(env);
    std::string funcName = "TerminateTask";
    napi_value cb = nullptr;
    napi_value result = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), TaskPool::TerminateTask, nullptr, &cb);
    napi_call_function(env, nullptr, cb, argc, argv, &result);
    return result;
}

napi_value NativeEngineTest::Execute(napi_env env, napi_value argv[], size_t argc)
{
    ExceptionScope scope(env);
    std::string funcName = "Execute";
    napi_value cb = nullptr;
    napi_value result = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), TaskPool::Execute, nullptr, &cb);
    napi_call_function(env, nullptr, cb, argc, argv, &result);
    return result;
}

napi_value NativeEngineTest::ExecuteDelayed(napi_env env, napi_value argv[], size_t argc)
{
    ExceptionScope scope(env);
    std::string funcName = "ExecuteDelayed";
    napi_value cb = nullptr;
    napi_value result = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), TaskPool::ExecuteDelayed, nullptr, &cb);
    napi_call_function(env, nullptr, cb, argc, argv, &result);
    return result;
}
} // namespace Commonlibrary::Concurrent::TaskPoolModule