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

#ifndef JS_CONCURRENT_MODULE_UTILS_JSON_MANAGER_H
#define JS_CONCURRENT_MODULE_UTILS_JSON_MANAGER_H

#include <string>

#include "helper/napi_helper.h"

namespace Commonlibrary::Concurrent {
class JsonManager {
public:
    static bool GeJsonFunction(napi_env env, napi_value global, const std::string &jsonName, napi_value &jsonFunction);
    static napi_value Init(napi_env env, napi_value exports);
};
}  // namespace Commonlibrary::Concurrent

#endif // JS_CONCURRENT_MODULE_UTILS_JSON_MANAGER_H
