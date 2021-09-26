 /*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include "js_uri.h"
#include "utils/log.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"

extern const char _binary_js_uri_js_start[];
extern const char _binary_js_uri_js_end[];
extern const char _binary_uri_abc_start[];
extern const char _binary_uri_abc_end[];

namespace OHOS::Uri {
    static napi_value UriConstructor(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        void *data = nullptr;
        size_t argc = 1;
        napi_value argv[1] = { 0 };
        Uri *object = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, &data));
        napi_valuetype valuetype;
        NAPI_CALL(env, napi_typeof(env, argv[0], &valuetype));
        if (valuetype == napi_string) {
            char *type = nullptr;
            std::string input = "";
            size_t typelen = 0;
            NAPI_CALL(env, napi_get_value_string_utf8(env, argv[0], nullptr, 0, &typelen));
            if (typelen > 0) {
                type = new char[typelen + 1];
                NAPI_CALL(env, napi_get_value_string_utf8(env, argv[0], type, typelen + 1, &typelen));
                input = type;
                delete[] type;
            }
            object = new Uri(env, input);
        } else {
            napi_throw_error(env, nullptr, "parameter type is error");
        }
        NAPI_CALL(env, napi_wrap(env, thisVar, object,
            [](napi_env env, void *data, void *hint) {
            auto obj = (Uri*)data;
            if (obj != nullptr) {
                delete obj;
            }
        }, nullptr, nullptr));
        return thisVar;
    }

    static napi_value Normalize(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr));
        Uri *muri = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&muri));
        std::string normalizeUri = muri->Normalize();
        napi_value result = nullptr;
        size_t tempLen = normalizeUri.size();
        NAPI_CALL(env, napi_create_string_utf8(env, normalizeUri.c_str(), tempLen, &result));
        return result;
    }

    static napi_value Equals(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        napi_value result = nullptr;
        size_t argc = 1;
        napi_value argv[1] = { 0 };
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr));

        Uri *muri = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&muri));
        Uri *other = nullptr;
        NAPI_CALL(env, napi_unwrap(env, argv[0], (void**)&other));

        bool flag = muri->Equals(*other);
        NAPI_CALL(env, napi_get_boolean(env, flag, &result));
        return result;
    }

    static napi_value IsAbsolute(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        napi_value result = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr));
        Uri *muri = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&muri));
        bool flag = muri->IsAbsolute();
        NAPI_CALL(env, napi_get_boolean(env, flag, &result));
        return result;
    }

    static napi_value IsFailed(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        napi_value result = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr));
        Uri *muri = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&muri));
        std::string temp = muri->IsFailed();
        size_t templen = temp.size();
        NAPI_CALL(env, napi_create_string_utf8(env, temp.c_str(), templen, &result));
        return result;
    }

    static napi_value UriToString(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        napi_value result = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr));
        Uri *muri = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&muri));
        std::string temp = muri->ToString();
        size_t templen = temp.size();
        NAPI_CALL(env, napi_create_string_utf8(env, temp.c_str(), templen, &result));
        return result;
    }

    static napi_value GetScheme(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        napi_value result = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr));
        Uri *muri = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&muri));
        std::string temp = muri->GetScheme();
        size_t templen = temp.size();
        NAPI_CALL(env, napi_create_string_utf8(env, temp.c_str(), templen, &result));
        return result;
    }

    static napi_value GetAuthority(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        napi_value result = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr));
        Uri *muri = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&muri));
        std::string temp = muri->GetAuthority();
        size_t templen = temp.size();
        NAPI_CALL(env, napi_create_string_utf8(env, temp.c_str(), templen, &result));
        return result;
    }

    static napi_value GetSsp(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        napi_value result = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr));
        Uri *muri = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&muri));
        std::string temp = muri->GetSsp();
        size_t templen = temp.size();
        NAPI_CALL(env, napi_create_string_utf8(env, temp.c_str(), templen, &result));
        return result;
    }

    static napi_value GetUserinfo(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        napi_value result = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr));
        Uri *muri = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&muri));
        std::string temp = muri->GetUserinfo();
        size_t templen = temp.size();
        NAPI_CALL(env, napi_create_string_utf8(env, temp.c_str(), templen, &result));
        return result;
    }

    static napi_value GetHost(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        napi_value result = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr));
        Uri *muri = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&muri));
        std::string temp = muri->GetHost();
        size_t templen = temp.size();
        NAPI_CALL(env, napi_create_string_utf8(env, temp.c_str(), templen, &result));
        return result;
    }

    static napi_value GetPort(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        napi_value result = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr));
        Uri *muri = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&muri));
        std::string temp = muri->GetPort();
        size_t templen = temp.size();
        NAPI_CALL(env, napi_create_string_utf8(env, temp.c_str(), templen, &result));
        return result;
    }

    static napi_value GetPath(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        napi_value result = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr));
        Uri *muri = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&muri));
        std::string temp = muri->GetPath();
        size_t templen = temp.size();
        NAPI_CALL(env, napi_create_string_utf8(env, temp.c_str(), templen, &result));
        return result;
    }

    static napi_value GetQuery(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        napi_value result = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr));
        Uri *muri = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&muri));
        std::string temp = muri->GetQuery();
        size_t templen = temp.size();
        NAPI_CALL(env, napi_create_string_utf8(env, temp.c_str(), templen, &result));
        return result;
    }

    static napi_value GetFragment(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        napi_value result = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr));
        Uri *muri = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&muri));
        std::string temp = muri->GetFragment();
        size_t templen = temp.size();
        NAPI_CALL(env, napi_create_string_utf8(env, temp.c_str(), templen, &result));
        return result;
    }

    static napi_value UriInit(napi_env env, napi_value exports)
    {
        const char *uriClassName = "uri";
        napi_value uriClass = nullptr;
        static napi_property_descriptor uriDesc[] = {
            DECLARE_NAPI_FUNCTION("normalize", Normalize),
            DECLARE_NAPI_FUNCTION("equals", Equals),
            DECLARE_NAPI_FUNCTION("isAbsolute", IsAbsolute),
            DECLARE_NAPI_FUNCTION("toString", UriToString),
            DECLARE_NAPI_GETTER("scheme", GetScheme),
            DECLARE_NAPI_GETTER("authority", GetAuthority),
            DECLARE_NAPI_GETTER("ssp", GetSsp),
            DECLARE_NAPI_GETTER("userinfo", GetUserinfo),
            DECLARE_NAPI_GETTER("host", GetHost),
            DECLARE_NAPI_GETTER("port", GetPort),
            DECLARE_NAPI_GETTER("path", GetPath),
            DECLARE_NAPI_GETTER("query", GetQuery),
            DECLARE_NAPI_GETTER("fragment", GetFragment),
            DECLARE_NAPI_GETTER("isFailed", IsFailed),
        };
        NAPI_CALL(env, napi_define_class(env, uriClassName, strlen(uriClassName), UriConstructor,
                                         nullptr, sizeof(uriDesc) / sizeof(uriDesc[0]), uriDesc, &uriClass));
        static napi_property_descriptor desc[] = {
            DECLARE_NAPI_PROPERTY("Uri", uriClass)
        };
        napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
        return exports;
    }

    extern "C"
    __attribute__((visibility("default"))) void NAPI_uri_GetJSCode(const char **buf, int *bufLen)
    {
        if (buf != nullptr) {
            *buf = _binary_js_uri_js_start;
        }

        if (bufLen != nullptr) {
            *bufLen = _binary_js_uri_js_end - _binary_js_uri_js_start;
        }
    }
    extern "C"
    __attribute__((visibility("default"))) void NAPI_uri_GetABCCode(const char** buf, int* buflen)
    {
        if (buf != nullptr) {
            *buf = _binary_uri_abc_start;
        }
        if (buflen != nullptr) {
            *buflen = _binary_uri_abc_end - _binary_uri_abc_start;
        }
    }
    static napi_module UriModule = {
        .nm_version = 1,
        .nm_flags = 0,
        .nm_filename = nullptr,
        .nm_register_func = UriInit,
        .nm_modname = "uri",
        .nm_priv = ((void*)0),
        .reserved = {0},
    };
    extern "C" __attribute__((constructor)) void RegisterModule()
    {
        napi_module_register(&UriModule);
    }
} // namespace