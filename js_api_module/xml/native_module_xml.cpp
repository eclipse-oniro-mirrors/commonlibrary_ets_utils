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

#include "native_module_xml.h"
#include "js_xml.h"
#include "tools/ets_error.h"

extern const char _binary_js_xml_js_start[];
extern const char _binary_js_xml_js_end[];
extern const char _binary_xml_abc_start[];
extern const char _binary_xml_abc_end[];

namespace OHOS::xml {
using namespace OHOS::Tools;
static const int32_t ERROR_CODE = 401; // 401 : the parameter type is incorrect
const int32_t ARGC_ONE = 1;

    static napi_value XmlSerializerConstructor(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        void *data = nullptr;
        size_t argc = 2;
        napi_value args[2] = { nullptr }; // 2:The number of parameters is 2
        XmlSerializer *object = nullptr;
        size_t iLength = 0;
        size_t offPos = 0;
        napi_value arraybuffer = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, &data));
        NAPI_ASSERT(env, argc == 1 || argc == 2, "Wrong number of arguments"); // 2: number of args

        bool bFlag = false;
        napi_is_arraybuffer(env, args[0], &bFlag);
        if (bFlag) {
            napi_get_arraybuffer_info(env, args[0], &data, &iLength);
        } else {
            napi_is_dataview(env, args[0], &bFlag);
            if (bFlag) {
                napi_get_dataview_info(env, args[0], &iLength, &data, &arraybuffer, &offPos);
            } else {
                napi_throw_error(env, "401", "Parameter error. The type of Parameter must be ArrayBuffer or DataView.");
                return nullptr;
            }
        }

        if (argc == 1) {
            object = new (std::nothrow) XmlSerializer(reinterpret_cast<char*>(data), iLength);
            if (object == nullptr) {
                HILOG_ERROR("XmlSerializerConstructor:: memory allocation failed, object is nullptr");
                return nullptr;
            }
        } else if (argc == 2) { // 2:When the input parameter is set to 2
            std::string encoding = "";
            napi_valuetype valuetype;
            NAPI_CALL(env, napi_typeof(env, args[1], &valuetype));
            NAPI_ASSERT(env, valuetype == napi_string, "Wrong argument type: string expected.");
            napi_status status = napi_ok;
            status = XmlSerializer::DealNapiStrValue(env, args[1], encoding);
            if (status == napi_ok) {
                object = new (std::nothrow) XmlSerializer(reinterpret_cast<char*>(data), iLength, encoding);
                if (object == nullptr) {
                    HILOG_ERROR("XmlSerializerConstructor:: memory allocation failed, object is nullptr");
                    return nullptr;
                }
            }
        }
        napi_status status = napi_wrap(env, thisVar, object,
            [](napi_env environment, void *data, void *hint) {
                auto obj = reinterpret_cast<XmlSerializer*>(data);
                if (obj != nullptr) {
                    delete obj;
                    obj = nullptr;
                }
            }, nullptr, nullptr);
        if (status != napi_ok && object != nullptr) {
            HILOG_ERROR("XmlSerializerConstructor:: napi_wrap failed");
            delete object;
            object = nullptr;
        }
        return thisVar;
    }

    static napi_value XmlPullParserConstructor(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        void *data = nullptr;
        size_t argc = 2;
        napi_value args[2] = { nullptr }; // 2:two args
        XmlPullParser *object = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, &data));
        NAPI_ASSERT(env, argc == 1 || argc == 2, "Wrong number of arguments"); // 2:two args
        napi_valuetype valuetype = napi_null;
        NAPI_CALL(env, napi_typeof(env, args[0], &valuetype));
        NAPI_ASSERT(env, valuetype == napi_object, "Wrong argument type: DataView or ArrayBuffer expected.");
        bool bFlag = false;
        size_t len = 0;
        size_t offPos = 0;
        napi_value arraybuffer = nullptr;
        napi_is_arraybuffer(env, args[0], &bFlag);
        if (bFlag) {
            napi_get_arraybuffer_info(env, args[0], &data, &len);
        } else {
            napi_is_dataview(env, args[0], &bFlag);
            if (bFlag) {
                napi_get_dataview_info(env, args[0], &len, &data, &arraybuffer, &offPos);
            } else {
                napi_throw_error(env, "401", "Parameter error. The type of Parameter must be ArrayBuffer or DataView.");
                return nullptr;
            }
        }
        if (data) {
            std::string strEnd(reinterpret_cast<char*>(data), len);
            if (argc == 1) {
                object = new (std::nothrow) XmlPullParser(env, strEnd, "utf-8");
                if (object == nullptr) {
                    HILOG_ERROR("XmlPullParserConstructor:: memory allocation failed, object is nullptr");
                    return nullptr;
                }
            } else if (argc == 2) { // 2:When the input parameter is set to 2
                NAPI_CALL(env, napi_typeof(env, args[1], &valuetype));
                NAPI_ASSERT(env, valuetype == napi_string, "Wrong argument type: string expected.");
                std::string strEncoding = "";
                XmlSerializer::DealNapiStrValue(env, args[1], strEncoding);
                object = new (std::nothrow) XmlPullParser(env, strEnd, strEncoding);
                if (object == nullptr) {
                    HILOG_ERROR("XmlPullParserConstructor:: memory allocation failed, object is nullptr");
                    return nullptr;
                }
            }
        }
        napi_status status = napi_wrap(env, thisVar, object,
            [](napi_env env, void *data, void *hint) {
                auto obj = reinterpret_cast<XmlPullParser*>(data);
                if (obj != nullptr) {
                    delete obj;
                    obj = nullptr;
                }
            }, nullptr, nullptr);
        if (status != napi_ok && object != nullptr) {
            HILOG_ERROR("XmlPullParserConstructor:: napi_wrap failed");
            delete object;
            object = nullptr;
        }
        return thisVar;
    }

    static napi_value SetAttributes(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t argc = 2;
        napi_value args[2] = { nullptr }; // 2:two args
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc == 2, "Wrong number of arguments"); // 2: number of args
        napi_valuetype valuetype = napi_null;
        NAPI_CALL(env, napi_typeof(env, args[0], &valuetype));
        NAPI_ASSERT(env, valuetype == napi_string, "Wrong argument type. string expected.");
        NAPI_CALL(env, napi_typeof(env, args[1], &valuetype));
        NAPI_ASSERT(env, valuetype == napi_string, "Wrong argument type. string expected.");
        XmlSerializer *object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, reinterpret_cast<void**>(&object)));
        std::string name;
        std::string value;
        XmlSerializer::DealNapiStrValue(env, args[0], name);
        XmlSerializer::DealNapiStrValue(env, args[1], value);
        object->SetAttributes(name, value);
        int32_t errorCode = object->XmlSerializerErrorCode();
        if (errorCode == ErrorCodeEnum::BUFFER_OVERFLOW) {
            return ErrorHelper::ThrowError(env, errorCode, object->XmlSerializerError().c_str());
        }
        napi_value result = nullptr;
        NAPI_CALL(env, napi_get_undefined(env, &result));
        return result;
    }

    static napi_value AddEmptyElement(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t argc = 1;
        napi_value args[1] = { 0 };
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc == 1, "Wrong number of arguments.");

        napi_valuetype valuetype;
        NAPI_CALL(env, napi_typeof(env, args[0], &valuetype));
        NAPI_ASSERT(env, valuetype == napi_string, "Wrong argument type: string expected.");
        XmlSerializer *object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, reinterpret_cast<void**>(&object)));
        std::string name;
        object->DealNapiStrValue(env, args[0], name);
        object->AddEmptyElement(name);
        int32_t errorCode = object->XmlSerializerErrorCode();
        if (errorCode == ErrorCodeEnum::BUFFER_OVERFLOW) {
            return ErrorHelper::ThrowError(env, errorCode, object->XmlSerializerError().c_str());
        }
        napi_value result = nullptr;
        NAPI_CALL(env, napi_get_undefined(env, &result));
        return result;
    }

    static napi_value SetDeclaration(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr));
        XmlSerializer *object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, reinterpret_cast<void**>(&object)));
        object->SetDeclaration();
        int32_t errorCode = object->XmlSerializerErrorCode();
        if (errorCode == ErrorCodeEnum::BUFFER_OVERFLOW) {
            return ErrorHelper::ThrowError(env, errorCode, object->XmlSerializerError().c_str());
        }
        napi_value result = nullptr;
        NAPI_CALL(env, napi_get_undefined(env, &result));
        return result;
    }

    static napi_value StartElement(napi_env env, napi_callback_info info)
    {
        size_t argc = 1;
        napi_value args[1] = { nullptr };
        napi_value thisVar = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc == 1, "Wrong number of arguments");
        XmlSerializer *object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, reinterpret_cast<void**>(&object)));
        std::string name;
        object->DealNapiStrValue(env, args[0], name);
        object->StartElement(name);
        int32_t errorCode = object->XmlSerializerErrorCode();
        if (errorCode == ErrorCodeEnum::BUFFER_OVERFLOW) {
            return ErrorHelper::ThrowError(env, errorCode, object->XmlSerializerError().c_str());
        }
        napi_value result = nullptr;
        NAPI_CALL(env, napi_get_undefined(env, &result));
        return result;
    }

    static napi_value EndElement(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr));
        XmlSerializer *object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, reinterpret_cast<void**>(&object)));
        object->EndElement();
        int32_t errorCode = object->XmlSerializerErrorCode();
        if (errorCode != ErrorCodeEnum::NO_ERROR) {
            return ErrorHelper::ThrowError(env, errorCode, object->XmlSerializerError().c_str());
        }
        napi_value result = nullptr;
        NAPI_CALL(env, napi_get_undefined(env, &result));
        return result;
    }

    static napi_value SetNamespace(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t argc = 2;
        napi_value args[2] = { nullptr }; // 2:two args
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc == 2, "Wrong number of arguments"); // 2:two args
        napi_valuetype valuetype = napi_null;
        NAPI_CALL(env, napi_typeof(env, args[0], &valuetype));
        NAPI_ASSERT(env, valuetype == napi_string, "Wrong argument type: string expected.");
        NAPI_CALL(env, napi_typeof(env, args[1], &valuetype));
        NAPI_ASSERT(env, valuetype == napi_string, "Wrong argument type: string expected.");
        XmlSerializer *object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, reinterpret_cast<void**>(&object)));
        std::string prefix;
        std::string nsTemp;
        XmlSerializer::DealNapiStrValue(env, args[0], prefix);
        XmlSerializer::DealNapiStrValue(env, args[1], nsTemp);
        object->SetNamespace(prefix, nsTemp);
        int32_t errorCode = object->XmlSerializerErrorCode();
        if (errorCode == ErrorCodeEnum::BUFFER_OVERFLOW) {
            return ErrorHelper::ThrowError(env, errorCode, object->XmlSerializerError().c_str());
        }
        napi_value result = nullptr;
        NAPI_CALL(env, napi_get_undefined(env, &result));
        return result;
    }

    static napi_value SetComment(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t argc = 1;
        napi_value args[1] = { nullptr };
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc == 1, "Wrong number of arguments.");
        napi_valuetype valuetype = napi_null;
        NAPI_CALL(env, napi_typeof(env, args[0], &valuetype));
        NAPI_ASSERT(env, valuetype == napi_string, "Wrong argument type: string expected.");
        XmlSerializer *object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, reinterpret_cast<void**>(&object)));
        std::string comment;
        object->DealNapiStrValue(env, args[0], comment);
        object->SetComment(comment);
        int32_t errorCode = object->XmlSerializerErrorCode();
        if (errorCode == ErrorCodeEnum::BUFFER_OVERFLOW) {
            return ErrorHelper::ThrowError(env, errorCode, object->XmlSerializerError().c_str());
        }
        napi_value result = nullptr;
        NAPI_CALL(env, napi_get_undefined(env, &result));
        return result;
    }

    static napi_value SetCData(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t argc = 1;
        napi_value args[1] = { nullptr };
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc == 1, "Wrong number of arguments");
        napi_valuetype valuetype = napi_null;
        NAPI_CALL(env, napi_typeof(env, args[0], &valuetype));
        NAPI_ASSERT(env, valuetype == napi_string, "Wrong argument type: string expected.");
        XmlSerializer *object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, reinterpret_cast<void**>(&object)));
        std::string data;
        XmlSerializer::DealNapiStrValue(env, args[0], data);
        object->SetCData(data);
        int32_t errorCode = object->XmlSerializerErrorCode();
        if (errorCode == ErrorCodeEnum::BUFFER_OVERFLOW) {
            return ErrorHelper::ThrowError(env, errorCode, object->XmlSerializerError().c_str());
        }
        napi_value result = nullptr;
        NAPI_CALL(env, napi_get_undefined(env, &result));
        return result;
    }

    static napi_value SetText(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t argc = 1;
        napi_value args[1] = { nullptr };
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc == 1, "Wrong number of arguments");
        napi_valuetype valuetype = napi_null;
        NAPI_CALL(env, napi_typeof(env, args[0], &valuetype));
        NAPI_ASSERT(env, valuetype == napi_string, "Wrong argument type: string expected.");
        XmlSerializer *object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, reinterpret_cast<void**>(&object)));
        std::string text;
        XmlSerializer::DealNapiStrValue(env, args[0], text);
        object->SetText(text);
        int32_t errorCode = object->XmlSerializerErrorCode();
        if (errorCode == ErrorCodeEnum::BUFFER_OVERFLOW) {
            return ErrorHelper::ThrowError(env, errorCode, object->XmlSerializerError().c_str());
        }
        napi_value result = nullptr;
        NAPI_CALL(env, napi_get_undefined(env, &result));
        return result;
    }

    static napi_value SetDocType(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t argc = 1;
        napi_value args[1] = { nullptr };
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc == 1, "Wrong number of arguments");
        napi_valuetype valuetype = napi_null;
        NAPI_CALL(env, napi_typeof(env, args[0], &valuetype));
        NAPI_ASSERT(env, valuetype == napi_string, "Wrong argument type: string expected.");
        XmlSerializer *object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, reinterpret_cast<void**>(&object)));
        std::string text;
        XmlSerializer::DealNapiStrValue(env, args[0], text);
        object->SetDocType(text);
        int32_t errorCode = object->XmlSerializerErrorCode();
        if (errorCode == ErrorCodeEnum::BUFFER_OVERFLOW) {
            return ErrorHelper::ThrowError(env, errorCode, object->XmlSerializerError().c_str());
        }
        napi_value result = nullptr;
        NAPI_CALL(env, napi_get_undefined(env, &result));
        return result;
    }
    static napi_value XmlSerializerError(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        napi_value result = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr));
        XmlSerializer *object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, reinterpret_cast<void**>(&object)));
        std::string temp = object->XmlSerializerError();
        size_t templen = temp.size();
        NAPI_CALL(env, napi_create_string_utf8(env, temp.c_str(), templen, &result));
        return result;
    }

    static napi_value GetOutput(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr));
        XmlSerializer *object = nullptr;
        napi_status status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&object));
        if (status != napi_ok) {
            HILOG_ERROR("XmlSerializer:: napi_unwrap failed!");
            return nullptr;
        }
        napi_value arrBuffer = nullptr;
        void* arrBufferPtr = nullptr;
        size_t arrBufferSize = object->GetXmlBufferLength();
        status = napi_create_arraybuffer(env, arrBufferSize * sizeof(size_t), &arrBufferPtr, &arrBuffer);
        if (status != napi_ok) {
            HILOG_ERROR("XmlSerializer:: create arraybuffer failed!");
            return nullptr;
        }
        object->GetXmlBuffer(arrBufferPtr, arrBufferSize);
        return arrBuffer;
    }

    static napi_value CreateDynamic(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        void *data = nullptr;
        size_t argc = ARGC_ONE;
        napi_value args[1] = { nullptr }; // 1:The number of parameters is 1
        XmlSerializer *object = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, &data));
        std::string encoding;
        napi_status status = XmlSerializer::DealNapiStrValue(env, args[0], encoding);
        if (status != napi_ok) {
            return ErrorHelper::ThrowError(env, ERROR_CODE, "Parameter error. The type of Parameter must be string.");
        }
        object = new (std::nothrow) XmlSerializer(encoding);
        if (object == nullptr) {
            HILOG_ERROR("XmlSerializerConstructor:: memory allocation failed, object is nullptr");
            return nullptr;
        }
        status = napi_wrap(env, thisVar, object,
            [](napi_env environment, void *data, void *hint) {
                auto obj = reinterpret_cast<XmlSerializer*>(data);
                if (obj != nullptr) {
                    delete obj;
                    obj = nullptr;
                }
            }, nullptr, nullptr);
        if (status != napi_ok && object != nullptr) {
            HILOG_ERROR("XmlSerializerConstructor::napi_wrap failed");
            delete object;
            object = nullptr;
        }
        napi_property_descriptor xmlDesc[] = {
            DECLARE_NAPI_FUNCTION("setAttributes", SetAttributes),
            DECLARE_NAPI_FUNCTION("addEmptyElement", AddEmptyElement),
            DECLARE_NAPI_FUNCTION("setDeclaration", SetDeclaration),
            DECLARE_NAPI_FUNCTION("startElement", StartElement),
            DECLARE_NAPI_FUNCTION("endElement", EndElement),
            DECLARE_NAPI_FUNCTION("setNamespace", SetNamespace),
            DECLARE_NAPI_FUNCTION("setComment", SetComment),
            DECLARE_NAPI_FUNCTION("setCDATA", SetCData),
            DECLARE_NAPI_FUNCTION("setText", SetText),
            DECLARE_NAPI_FUNCTION("setDocType", SetDocType),
            DECLARE_NAPI_FUNCTION("XmlSerializerError", XmlSerializerError),
            DECLARE_NAPI_FUNCTION("getOutput", GetOutput),
        };
        napi_define_properties(env, thisVar, sizeof(xmlDesc) / sizeof(xmlDesc[0]), xmlDesc);
        return thisVar;
    }

    napi_value XmlSerializerInit(napi_env env, napi_value exports)
    {
        const char *xmlSerializerClass = "XmlSerializer";
        napi_value xmlClass = nullptr;
        napi_property_descriptor xmlDesc[] = {
            DECLARE_NAPI_FUNCTION("setAttributes", SetAttributes),
            DECLARE_NAPI_FUNCTION("addEmptyElement", AddEmptyElement),
            DECLARE_NAPI_FUNCTION("setDeclaration", SetDeclaration),
            DECLARE_NAPI_FUNCTION("startElement", StartElement),
            DECLARE_NAPI_FUNCTION("endElement", EndElement),
            DECLARE_NAPI_FUNCTION("setNamespace", SetNamespace),
            DECLARE_NAPI_FUNCTION("setComment", SetComment),
            DECLARE_NAPI_FUNCTION("setCDATA", SetCData),
            DECLARE_NAPI_FUNCTION("setText", SetText),
            DECLARE_NAPI_FUNCTION("setDocType", SetDocType),
            DECLARE_NAPI_FUNCTION("XmlSerializerError", XmlSerializerError),
            DECLARE_NAPI_FUNCTION("getOutput", GetOutput),
            DECLARE_NAPI_STATIC_FUNCTION("createDynamic", CreateDynamic)
        };
        NAPI_CALL(env, napi_define_class(env, xmlSerializerClass, strlen(xmlSerializerClass), XmlSerializerConstructor,
                                         nullptr, sizeof(xmlDesc) / sizeof(xmlDesc[0]), xmlDesc, &xmlClass));
        napi_property_descriptor desc[] = {
            DECLARE_NAPI_PROPERTY("XmlSerializer", xmlClass)
        };
        napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
        return exports;
    }

    static napi_value Parse(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t argc = 1;
        napi_value args[1] = { nullptr };
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc == 1, "Wrong number of arguments, one expected.");
        napi_valuetype valuetype;
        NAPI_CALL(env, napi_typeof(env, args[0], &valuetype));
        NAPI_ASSERT(env, valuetype == napi_object, "Wrong argument type: object expected.");
        XmlPullParser *object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, reinterpret_cast<void**>(&object)));
        object->DealOptionInfo(env, args[0]);
        object->Parse(env, thisVar, true);
        napi_value result = nullptr;
        NAPI_CALL(env, napi_get_undefined(env, &result));
        return result;
    }

    static napi_value ParseXml(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t argc = 1;
        napi_value args[1] = { nullptr };
        napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr);
        XmlPullParser *xmlPullParser = nullptr;
        napi_unwrap(env, thisVar, reinterpret_cast<void**>(&xmlPullParser));
        napi_value result = nullptr;
        if (xmlPullParser == nullptr) {
            ErrorHelper::ThrowError(env, ERROR_CODE, "Parameter error. Parameter verification failed.");
            napi_get_boolean(env, false, &result);
            return result;
        }
        xmlPullParser->DealOptionInfo(env, args[0]);
        xmlPullParser->Parse(env, thisVar, false);
        std::string errStr = xmlPullParser->XmlPullParserError();
        napi_get_boolean(env, !errStr.empty(), &result);
        return result;
    }

    static napi_value XmlPullParserError(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        napi_value result = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr));
        XmlPullParser *object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, reinterpret_cast<void**>(&object)));
        std::string temp = object->XmlPullParserError();
        size_t templen = temp.size();
        NAPI_CALL(env, napi_create_string_utf8(env, temp.c_str(), templen, &result));
        return result;
    }

    napi_value XmlPullParserInit(napi_env env, napi_value exports)
    {
        const char *xmlPullParserClass = "XmlPullParser";
        napi_value xmlClass = nullptr;
        napi_property_descriptor xmlDesc[] = {
            DECLARE_NAPI_FUNCTION("parse", Parse),
            DECLARE_NAPI_FUNCTION("parseXml", ParseXml),
            DECLARE_NAPI_FUNCTION("XmlPullParserError", XmlPullParserError)
        };
        NAPI_CALL(env, napi_define_class(env, xmlPullParserClass, strlen(xmlPullParserClass),
            XmlPullParserConstructor, nullptr, sizeof(xmlDesc) / sizeof(xmlDesc[0]),
            xmlDesc, &xmlClass));
        napi_property_descriptor desc[] = {
            DECLARE_NAPI_PROPERTY("XmlPullParser", xmlClass)
        };
        napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
        return exports;
    };

    static napi_value Init(napi_env env, napi_value exports)
    {
        XmlSerializerInit(env, exports);
        XmlPullParserInit(env, exports);
        return exports;
    }

    extern "C"
    __attribute__((visibility("default"))) void NAPI_xml_GetJSCode(const char **buf, int *bufLen)
    {
        if (buf != nullptr) {
            *buf = _binary_js_xml_js_start;
        }

        if (bufLen != nullptr) {
            *bufLen = _binary_js_xml_js_end - _binary_js_xml_js_start;
        }
    }
    extern "C"
    __attribute__((visibility("default"))) void NAPI_xml_GetABCCode(const char** buf, int* buflen)
    {
        if (buf != nullptr) {
            *buf = _binary_xml_abc_start;
        }
        if (buflen != nullptr) {
            *buflen = _binary_xml_abc_end - _binary_xml_abc_start;
        }
    }


    static napi_module_with_js xmlModule = {
        .nm_version = 1,
        .nm_flags = 0,
        .nm_filename = nullptr,
        .nm_register_func = Init,
        .nm_modname = "xml",
        .nm_priv = reinterpret_cast<void*>(0),
        .nm_get_abc_code = NAPI_xml_GetABCCode,
        .nm_get_js_code = NAPI_xml_GetJSCode,
    };
    extern "C" __attribute__((constructor)) void XmlRegisterModule()
    {
        napi_module_with_js_register(&xmlModule);
    }
} // namespace OHOS::Xml
