 /*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "native_module_convertxml.h"
#include "tools/ets_error.h"
#include "tools/log.h"
#include "js_convertxml.h"

extern const char _binary_js_convertxml_js_start[];
extern const char _binary_js_convertxml_js_end[];
extern const char _binary_convertxml_abc_start[];
extern const char _binary_convertxml_abc_end[];

namespace OHOS::Xml {
using namespace OHOS::Tools;
static const int32_t ERROR_CODE = 401; // 401 : the parameter type is incorrect

    static napi_value ConvertXmlConstructor(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        void *data = nullptr;
        napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, &data);
        auto objectInfo = new (std::nothrow) ConvertXml(env);
        if (objectInfo == nullptr) {
            HILOG_ERROR("ConvertXmlConstructor:: memory allocation failed, objectInfo is nullptr");
            return nullptr;
        }
        napi_status status = napi_wrap(env, thisVar, objectInfo,
            [](napi_env environment, void *data, void *hint) {
                auto obj = reinterpret_cast<ConvertXml*>(data);
                if (obj != nullptr) {
                    delete obj;
                    obj = nullptr;
                }
            }, nullptr, nullptr);
        if (status != napi_ok) {
            HILOG_ERROR("ConvertXmlConstructor:: napi_wrap failed");
            delete objectInfo;
            objectInfo = nullptr;
        }
        return thisVar;
    }

    static napi_value Convert(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t requireMaxArgc = 2; // 2:MaxArgc
        size_t requireMinArgc = 1;
        size_t argc = 2; // 2:The number of parameters is 2
        napi_value args[2] = { nullptr };
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc <= requireMaxArgc, "Wrong number of arguments(Over)");
        NAPI_ASSERT(env, argc >= requireMinArgc, "Wrong number of arguments(Less)");
        std::string strXml;
        napi_valuetype valuetype;
        ConvertXml *object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, reinterpret_cast<void**>(&object)));

        NAPI_CALL(env, napi_typeof(env, args[0], &valuetype));
        NAPI_ASSERT(env, valuetype == napi_string, "Wrong argument type: string expected.");
        object->DealNapiStrValue(env, args[0], strXml);
        if (argc > 1) {
            object->DealOptions(env, args[1], true);
        }
        napi_value result = object->Convert(env, strXml, true);
        return result;
    }

    static napi_value FastConvert(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t argc = 2; // 2:The number of parameters
        napi_value args[2] = { nullptr }; // 2:The number of parameters
        napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr);
        std::string strXml;
        ConvertXml *convertxml = nullptr;
        napi_unwrap(env, thisVar, reinterpret_cast<void**>(&convertxml));
        if (convertxml == nullptr) {
            ErrorHelper::ThrowError(env, ERROR_CODE, "Parameter error. Parameter verification failed.");
            return nullptr;
        }
        convertxml->DealNapiStrValue(env, args[0], strXml);
        if (argc > 1) {
            convertxml->DealOptions(env, args[1], false);
        }
        return convertxml->Convert(env, strXml, false);
    }

    napi_value ConvertXmlInit(napi_env env, napi_value exports)
    {
        const char *convertXmlClassName = "ConvertXml";
        napi_value convertXmlClass = nullptr;
        napi_property_descriptor convertXmlDesc[] = {
            DECLARE_NAPI_FUNCTION("convert", Convert),
            DECLARE_NAPI_FUNCTION("fastConvertToJSObject", FastConvert)
        };
        NAPI_CALL(env, napi_define_class(env, convertXmlClassName, strlen(convertXmlClassName), ConvertXmlConstructor,
                                         nullptr, sizeof(convertXmlDesc) / sizeof(convertXmlDesc[0]), convertXmlDesc,
                                         &convertXmlClass));
        napi_property_descriptor desc[] = {
            DECLARE_NAPI_PROPERTY("ConvertXml", convertXmlClass)
        };
        NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
        return exports;
    }

    extern "C"
    __attribute__((visibility("default"))) void NAPI_convertxml_GetJSCode(const char **buf, int *bufLen)
    {
        if (buf != nullptr) {
            *buf = _binary_js_convertxml_js_start;
        }
        if (bufLen != nullptr) {
            *bufLen = _binary_js_convertxml_js_end - _binary_js_convertxml_js_start;
        }
    }
    extern "C"
    __attribute__((visibility("default"))) void NAPI_convertxml_GetABCCode(const char** buf, int* buflen)
    {
        if (buf != nullptr) {
            *buf = _binary_convertxml_abc_start;
        }
        if (buflen != nullptr) {
            *buflen = _binary_convertxml_abc_end - _binary_convertxml_abc_start;
        }
    }

    static napi_module_with_js convertXmlModule = {
        .nm_version = 1,
        .nm_flags = 0,
        .nm_filename = nullptr,
        .nm_register_func = ConvertXmlInit,
        .nm_modname = "convertxml",
        .nm_priv = reinterpret_cast<void*>(0),
        .nm_get_abc_code = NAPI_convertxml_GetABCCode,
        .nm_get_js_code = NAPI_convertxml_GetJSCode,
    };

    extern "C" __attribute__((constructor)) void ConvertXMLRegisterModule(void)
    {
        napi_module_with_js_register(&convertXmlModule);
    }
} // namespace OHOS::Xml
