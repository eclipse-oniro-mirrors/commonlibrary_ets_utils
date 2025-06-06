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

#include "commonlibrary/ets_utils/js_util_module/util/js_base64.h"
#include "commonlibrary/ets_utils/js_util_module/util/js_textdecoder.h"
#include "commonlibrary/ets_utils/js_util_module/util/js_textencoder.h"
#include "commonlibrary/ets_utils/js_util_module/util/js_types.h"
#include "commonlibrary/ets_utils/js_util_module/util/js_uuid.h"
#include "commonlibrary/ets_utils/js_util_module/util/js_stringdecoder.h"

#include "securec.h"
#include "tools/log.h"

#include "jni_helper.h"

extern const char _binary_util_js_js_start[];
extern const char _binary_util_js_js_end[];
extern const char _binary_util_abc_start[];
extern const char _binary_util_abc_end[];
static const std::vector<std::string> conventFormat = {"utf-8", "UTF-8", "gbk", "GBK", "GB2312", "gb2312",
                                                       "GB18030", "gb18030", "ibm866", "iso-8859-2", "iso-8859-3",
                                                       "iso-8859-4", "iso-8859-5", "iso-8859-6", "iso-8859-7",
                                                       "iso-8859-8", "iso-8859-8-i", "iso-8859-10", "iso-8859-13",
                                                       "iso-8859-14", "iso-8859-15", "koi8-r", "koi8-u", "macintosh",
                                                       "windows-874", "windows-1250", "windows-1251", "windows-1252",
                                                       "windows-1253", "windows-1254", "windows-1255", "windows-1256",
                                                       "windows-1257", "windows-1258", "big5", "euc-jp", "iso-2022-jp",
                                                       "shift_jis", "euc-kr", "x-mac-cyrillic", "utf-16be",
                                                       "utf-16le", "iso-8859-1"};

namespace OHOS::Util {
    using namespace Commonlibrary::Platform;
    static bool IsValidValue(napi_env env, napi_value value)
    {
        napi_value undefinedRef = nullptr;
        napi_value nullRef = nullptr;
        napi_get_undefined(env, &undefinedRef);
        napi_get_null(env, &nullRef);
        bool isUndefined = false;
        bool isNull = false;
        napi_strict_equals(env, value, undefinedRef, &isUndefined);
        napi_strict_equals(env, value, nullRef, &isNull);
        return !(isUndefined || isNull);
    }

    static char* ApplyMemory(const size_t length)
    {
        if (length == 0) {
            return nullptr;
        }
        char *type = new (std::nothrow) char[length + 1];
        if (type == nullptr) {
            HILOG_ERROR("Textdecoder:: memory allocation failed, type is nullptr");
            return nullptr;
        }
        if (memset_s(type, length + 1, '\0', length + 1) != EOK) {
            HILOG_ERROR("Textdecoder:: type memset_s failed");
            delete[] type;
            return nullptr;
        }
        return type;
    }

    static std::string temp = "cdfijoOs";
    static std::string DealWithPrintf(const std::string &format, const std::vector<std::string> &value)
    {
        size_t i = 0;
        size_t j = 0;
        std::string str;
        size_t formatSize = format.size();
        size_t valueSize = value.size();
        while (i < formatSize && j < valueSize) {
            if (format[i] == '%' && (i + 1 < formatSize && format[i + 1] == '%')) {
                str += '%';
                i += 2; // 2:The array goes back two digits.
            } else if (format[i] == '%' && (i + 1 < formatSize && (temp.find(format[i + 1])) != std::string::npos)) {
                if (format[i + 1] == 'c') {
                    j++;
                } else {
                    str += value[j++];
                }
                i += 2; // 2:The array goes back two digits.
            } else if (format[i] == '%' && (i + 1 < formatSize && (temp.find(format[i + 1])) == std::string::npos)) {
                str += '%';
                i++;
            }
            if (i < formatSize && format[i] != '%') {
                size_t pos = 0;
                if ((pos = format.find('%', i)) == std::string::npos) {
                    str += format.substr(i);
                    i = formatSize;
                    break;
                } else {
                    str += format.substr(i, pos - i);
                    i = pos;
                }
            }
        }
        while (j < valueSize) {
            str += " " + value[j++];
        }
        if (i < formatSize) {
            str += format.substr(i);
        }
        return str;
    }

    static napi_value ThrowError(napi_env env, const char* errMessage)
    {
        napi_value utilError = nullptr;
        napi_value code = nullptr;
        uint32_t errCode = 401;
        napi_create_uint32(env, errCode, &code);
        napi_value name = nullptr;
        std::string errName = "BusinessError";
        napi_value msg = nullptr;
        napi_create_string_utf8(env, errMessage, NAPI_AUTO_LENGTH, &msg);
        napi_create_string_utf8(env, errName.c_str(), NAPI_AUTO_LENGTH, &name);
        napi_create_error(env, nullptr, msg, &utilError);
        napi_set_named_property(env, utilError, "code", code);
        napi_set_named_property(env, utilError, "name", name);
        napi_throw(env, utilError);
        return nullptr;
    }

    static napi_value FormatString(napi_env env, std::string &str)
    {
        std::string res;
        size_t strSize = str.size();
        for (size_t i = 0; i < strSize; ++i) {
            if (str[i] == '%' && (i + 1 < strSize && temp.find(str[i + 1]) != std::string::npos)) {
                if (str[i + 1] == 'o') {
                    res += "o ";
                } else if (str[i + 1] == 'O') {
                    res += "O ";
                } else if (str[i + 1] == 'i') {
                    res += "i ";
                } else if (str[i + 1] == 'j') {
                    res += "j ";
                } else if (str[i + 1] == 'd') {
                    res += "d ";
                } else if (str[i + 1] == 's') {
                    res += "s ";
                } else if (str[i + 1] == 'f') {
                    res += "f ";
                } else if (str[i + 1] == 'c') {
                    res += "c ";
                }
                i++;
            } else if (str[i] == '%' && (i + 1 < strSize && str[i + 1] == '%')) {
                i++;
            }
        }
        if (!res.empty()) {
            res = res.substr(0, res.size() - 1);
        }
        napi_value result = nullptr;
        napi_create_string_utf8(env, res.c_str(), res.size(), &result);
        return result;
    }

    static void FreeMemory(napi_value *address)
    {
        delete[] address;
        address = nullptr;
    }

    static napi_value DealWithFormatString(napi_env env, napi_callback_info info)
    {
        size_t argc = 1;
        napi_value argv = nullptr;
        napi_get_cb_info(env, info, &argc, 0, nullptr, nullptr);

        napi_get_cb_info(env, info, &argc, &argv, nullptr, nullptr);
        std::string format = "";
        size_t formatsize = 0;
        if (napi_get_value_string_utf8(env, argv, nullptr, 0, &formatsize) != napi_ok) {
            HILOG_ERROR("DealWithFormatString:: can not get argv size");
            return nullptr;
        }
        format.reserve(formatsize + 1);
        format.resize(formatsize);
        if (napi_get_value_string_utf8(env, argv, format.data(), formatsize + 1, &formatsize) != napi_ok) {
            HILOG_ERROR("DealWithFormatString:: can not get argv value");
            return nullptr;
        }
        return FormatString(env, format);
    }

    static std::string PrintfString(const std::string &format, const std::vector<std::string> &value)
    {
        return DealWithPrintf(format, value);
    }

    static napi_value Printf(napi_env env, napi_callback_info info)
    {
        napi_value result = nullptr;
        size_t argc = 0;
        napi_get_cb_info(env, info, &argc, nullptr, nullptr, nullptr);
        napi_value *argv = nullptr;
        if (argc > 0) {
            argv = new (std::nothrow) napi_value[argc];
            if (argv == nullptr) {
                HILOG_ERROR("Printf:: memory allocation failed, argv is nullptr");
                return nullptr;
            }
            napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
            std::string format = "";
            size_t formatsize = 0;
            if (napi_get_value_string_utf8(env, argv[0], nullptr, 0, &formatsize) != napi_ok) {
                HILOG_ERROR("Printf:: can not get argv[0] size");
                FreeMemory(argv);
                return nullptr;
            }
            format.reserve(formatsize);
            format.resize(formatsize);
            if (napi_get_value_string_utf8(env, argv[0], format.data(), formatsize + 1, &formatsize) != napi_ok) {
                HILOG_ERROR("Printf:: can not get argv[0] value");
                FreeMemory(argv);
                return nullptr;
            }
            std::vector<std::string> value;
            for (size_t i = 1; i < argc; i++) {
                std::string valueString = "";
                size_t valuesize = 0;
                if (napi_get_value_string_utf8(env, argv[i], nullptr, 0, &valuesize) != napi_ok) {
                    HILOG_ERROR("Printf:: can not get argv[i] size");
                    FreeMemory(argv);
                    return nullptr;
                }
                valueString.reserve(valuesize);
                valueString.resize(valuesize);
                if (napi_get_value_string_utf8(env, argv[i], valueString.data(),
                                               valuesize + 1, &valuesize) != napi_ok) {
                    HILOG_ERROR("Printf:: can not get argv[i] value");
                    FreeMemory(argv);
                    return nullptr;
                }
                value.push_back(valueString.data());
            }
            std::string printInfo = PrintfString(format.data(), value);
            napi_create_string_utf8(env, printInfo.c_str(), printInfo.size(), &result);
            FreeMemory(argv);
            return result;
        }
        napi_value res = nullptr;
        NAPI_CALL(env, napi_get_undefined(env, &res));
        return res;
    }

    static napi_value GetErrorString(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        napi_value result = nullptr;
        std::string errInfo;
        size_t argc = 1;
        napi_value argv = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &argv, &thisVar, nullptr));
        int32_t err = 0;
        NAPI_CALL(env, napi_get_value_int32(env, argv, &err));
        errInfo = uv_strerror(err);
        NAPI_CALL(env, napi_create_string_utf8(env, errInfo.c_str(), errInfo.size(), &result));
        return result;
    }

    static napi_value RandomUUID(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t requireArgc = 1;
        size_t argc = 1;
        napi_value args[1] = { 0 };
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");
        bool flag = false;
        napi_get_value_bool(env, args[0], &flag);
        std::string uuidString = OHOS::Util::GetStringUUID(env, flag);
        napi_value result = nullptr;
        size_t tempLen = uuidString.size();
        napi_create_string_utf8(env, uuidString.c_str(), tempLen, &result);
        return result;
    }

    static napi_value RandomBinaryUUID(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t requireArgc = 1;
        size_t argc = 1;
        napi_value args[1] = { 0 };
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");
        bool flag = false;
        napi_get_value_bool(env, args[0], &flag);
        napi_value result = OHOS::Util::GetBinaryUUID(env, flag);
        return result;
    }

    static napi_value ParseUUID(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t requireArgc = 1;
        size_t argc = 1;
        napi_value args[1] = { nullptr };
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");
        napi_valuetype valuetype;
        NAPI_CALL(env, napi_typeof(env, args[0], &valuetype));
        NAPI_ASSERT(env, valuetype == napi_string, "Wrong argument type. String expected.");
        napi_value result = OHOS::Util::DoParseUUID(env, args[0]);
        return result;
    }

    static napi_value GetHash(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t requireArgc = 1;
        size_t argc = 1;
        napi_value args[1] = { nullptr };
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr));
        if (argc < requireArgc) {
            napi_throw_error(env, "-1", "Expected 1 parameter, actually not included in the parameter.");
            return nullptr;
        }
        napi_valuetype valuetype;
        NAPI_CALL(env, napi_typeof(env, args[0], &valuetype));
        if (valuetype != napi_object) {
            return ThrowError(env, "Parameter error. The type of Parameter must be object.");
        }
        NativeEngine *engine = reinterpret_cast<NativeEngine*>(env);
        int32_t value = engine->GetObjectHash(env, args[0]);
        napi_value result = nullptr;
        napi_create_uint32(env, value, &result);
        return result;
    }

    static napi_value GetMainThreadStackTrace(napi_env env, [[maybe_unused]] napi_callback_info info)
    {
        NativeEngine *engine = reinterpret_cast<NativeEngine*>(env);
        std::string stackTraceStr;
        engine->GetMainThreadStackTrace(env, stackTraceStr);
        napi_value result = nullptr;
        size_t tempLen = stackTraceStr.size();
        napi_create_string_utf8(env, stackTraceStr.c_str(), tempLen, &result);
        return result;
    }

    static napi_value TextdecoderConstructor(napi_env env, napi_callback_info info)
    {
        size_t tempArgc = 0;
        napi_value thisVar = nullptr;
        napi_get_cb_info(env, info, &tempArgc, nullptr, &thisVar, nullptr);
        size_t argc = 0;
        void *data = nullptr;
        char *type = nullptr;
        size_t typeLen = 0;
        int32_t flags = 0;
        std::vector<int> paraVec(2, 0); // 2: Specifies the size of the container to be applied for.
        if (tempArgc == 1) {
            argc = 1;
            napi_value argv = nullptr;
            napi_get_cb_info(env, info, &argc, &argv, nullptr, &data);
            napi_get_value_string_utf8(env, argv, nullptr, 0, &typeLen);
            if (typeLen > 0) {
                type = ApplyMemory(typeLen);
            }
            napi_get_value_string_utf8(env, argv, type, typeLen + 1, &typeLen);
        } else if (tempArgc == 2) { // 2: The number of parameters is 2.
            argc = 2; // 2: The number of parameters is 2.
            napi_value argvArr[2] = { 0 }; // 2:The number of parameters is 2
            napi_get_cb_info(env, info, &argc, argvArr, nullptr, &data);
            // first para
            napi_get_value_string_utf8(env, argvArr[0], nullptr, 0, &typeLen);
            if (typeLen > 0) {
                type = ApplyMemory(typeLen);
            }
            napi_get_value_string_utf8(env, argvArr[0], type, typeLen + 1, &typeLen);
            napi_get_value_int32(env, argvArr[1], &flags);
        }
        std::string enconding = "utf-8";
        if (type != nullptr) {
            enconding = type;
        }
        delete []type;
        type = nullptr;
        auto objectInfo = new (std::nothrow) TextDecoder(enconding, flags);
        if (objectInfo == nullptr) {
            HILOG_ERROR("TextDecoder:: memory allocation failed, objectInfo is nullptr");
            return nullptr;
        }
        NAPI_CALL(env, napi_wrap(
            env, thisVar, objectInfo,
            [](napi_env environment, void *data, void *hint) {
                auto objInfo = reinterpret_cast<TextDecoder*>(data);
                if (objInfo != nullptr) {
                    delete objInfo;
                    objInfo = nullptr;
                }
            },
            nullptr, nullptr));
        return thisVar;
    }

    static napi_value DecodeToString(napi_env env, napi_callback_info info)
    {
        size_t tempArgc = 2; // 2:The number of parameters is 2
        napi_value thisVar = nullptr;
        napi_get_cb_info(env, info, &tempArgc, nullptr, &thisVar, nullptr);
        size_t argc = 0;
        void *dataPara = nullptr;
        napi_typedarray_type type;
        size_t length = 0;
        void *data = nullptr;
        napi_value arraybuffer = nullptr;
        size_t byteOffset = 0;
        bool iStream = false;
        TextDecoder *textDecoder = nullptr;
        napi_unwrap(env, thisVar, (void**)&textDecoder);
        if (textDecoder == nullptr) {
            HILOG_ERROR("DecodeToString:: textDecoder is nullptr");
            return nullptr;
        }
        napi_value valStr = nullptr;
        if (tempArgc == 1) {
            argc = 1;
            napi_value argv = nullptr;
            napi_get_cb_info(env, info, &argc, &argv, nullptr, &dataPara);
            napi_get_typedarray_info(env, argv, &type, &length, &data, &arraybuffer, &byteOffset);
            if (type != napi_uint8_array) {
                return ThrowError(env, "Parameter error. The type of Parameter must be Uint8Array.");
            }
            valStr = textDecoder->DecodeToString(env, argv, iStream);
        } else if (tempArgc == 2) { // 2: The number of parameters is 2.
            argc = 2; // 2: The number of parameters is 2.
            napi_value argvArr[2] = { 0 }; // 2:The number of parameters is 2
            napi_get_cb_info(env, info, &argc, argvArr, nullptr, &dataPara);
            napi_get_typedarray_info(env, argvArr[0], &type, &length, &data, &arraybuffer, &byteOffset);
            if (type != napi_uint8_array) {
                return ThrowError(env, "Parameter error. The type of first Parameter must be Uint8Array.");
            }
            napi_valuetype valueType;
            napi_typeof(env, argvArr[1], &valueType);
            if (valueType != napi_undefined && valueType != napi_null) {
                if (valueType != napi_object) {
                    return ThrowError(env, "Parameter error. The type of second Parameter must be object.");
                }
                const char *messageKeyStrStream = "stream";
                napi_value resultStream = nullptr;
                napi_get_named_property(env, argvArr[1], messageKeyStrStream, &resultStream);
                napi_get_value_bool(env, resultStream, &iStream);
            }
            valStr = textDecoder->DecodeToString(env, argvArr[0], iStream);
        }
        return valStr;
    }

    static napi_value TextdecoderDecode(napi_env env, napi_callback_info info)
    {
        size_t tempArgc = 2; // 2:The number of parameters is 2
        napi_value thisVar = nullptr;
        napi_get_cb_info(env, info, &tempArgc, nullptr, &thisVar, nullptr);
        size_t argc = 0;
        void *dataPara = nullptr;
        napi_typedarray_type type;
        size_t length = 0;
        void *data = nullptr;
        napi_value arraybuffer = nullptr;
        size_t byteOffset = 0;
        bool iStream = false;
        TextDecoder *textDecoder = nullptr;
        napi_unwrap(env, thisVar, (void**)&textDecoder);
        if (textDecoder == nullptr) {
            return nullptr;
        }
        napi_value valStr = nullptr;
        if (tempArgc == 1) {
            argc = 1;
            napi_value argv = nullptr;
            napi_get_cb_info(env, info, &argc, &argv, nullptr, &dataPara);
            // first para
            napi_get_typedarray_info(env, argv, &type, &length, &data, &arraybuffer, &byteOffset);
            if (type != napi_uint8_array) {
                return ThrowError(env, "Parameter error. The type of Parameter must be Uint8Array.");
            }
            valStr = textDecoder->Decode(env, argv, iStream);
        } else if (tempArgc == 2) { // 2: The number of parameters is 2.
            argc = 2; // 2: The number of parameters is 2.
            napi_value argvArr[2] = { 0 }; // 2:The number of parameters is 2
            napi_get_cb_info(env, info, &argc, argvArr, nullptr, &dataPara);
            // first para
            napi_get_typedarray_info(env, argvArr[0], &type, &length, &data, &arraybuffer, &byteOffset);
            // second para
            if (type != napi_uint8_array) {
                return ThrowError(env, "Parameter error. The type of Parameter must be string.");
            }
            napi_valuetype valueType1;
            napi_typeof(env, argvArr[1], &valueType1);
            if (valueType1 != napi_undefined && valueType1 != napi_null) {
                if (valueType1 != napi_object) {
                    return ThrowError(env, "Parameter error. The type of Parameter must be object.");
                }
                napi_value messageKeyStream = nullptr;
                const char *messageKeyStrStream = "stream";

                napi_value resultStream = nullptr;
                NAPI_CALL(env, napi_create_string_utf8(env, messageKeyStrStream, strlen(messageKeyStrStream),
                    &messageKeyStream));
                NAPI_CALL(env, napi_get_property(env, argvArr[1], messageKeyStream, &resultStream));
                NAPI_CALL(env, napi_get_value_bool(env, resultStream, &iStream));
            }
            valStr = textDecoder->Decode(env, argvArr[0], iStream);
        }
        return valStr;
    }

    static bool CheckEncodingFormat(const std::string &encoding)
    {
        for (const auto& format : conventFormat) {
            if (format == encoding) {
                return true;
            }
        }
        return false;
    }

    static napi_value InitTextEncoder(napi_env env, napi_value thisVar, std::string encoding, std::string orgEncoding)
    {
        auto object = new (std::nothrow) TextEncoder(encoding);
        if (object == nullptr) {
            HILOG_ERROR("TextEncoder:: memory allocation failed, object is nullptr");
            return nullptr;
        }
        object->SetOrgEncoding(orgEncoding);
        napi_status status = napi_wrap(env, thisVar, object,
            [](napi_env environment, void *data, void *hint) {
                auto obj = reinterpret_cast<TextEncoder*>(data);
                if (obj != nullptr) {
                    delete obj;
                    obj = nullptr;
                }
            }, nullptr, nullptr);
        if (status != napi_ok) {
            delete object;
            object = nullptr;
        }
        return thisVar;
    }

    // Encoder
    static napi_value TextEncoderConstructor(napi_env env, napi_callback_info info)
    {
        size_t argc = 0;
        napi_value thisVar = nullptr;
        napi_value src = nullptr;
        napi_get_cb_info(env, info, &argc, &src, &thisVar, nullptr);
        std::string encoding = "utf-8";
        std::string orgEncoding = encoding;
        if (argc == 1) {
            napi_get_cb_info(env, info, &argc, &src, nullptr, nullptr);
            napi_valuetype valuetype;
            napi_typeof(env, src, &valuetype);
            if (valuetype != napi_undefined && valuetype != napi_null) {
                if (valuetype != napi_string) {
                    return ThrowError(env, "Parameter error. The type of Parameter must be string.");
                }
                size_t bufferSize = 0;
                if (napi_get_value_string_utf8(env, src, nullptr, 0, &bufferSize) != napi_ok) {
                    HILOG_ERROR("TextEncoder:: can not get src size");
                    return nullptr;
                }
                std::string buffer(bufferSize, '\0');
                if (napi_get_value_string_utf8(env, src, buffer.data(), bufferSize + 1, &bufferSize) != napi_ok) {
                    HILOG_ERROR("TextEncoder:: can not get src value");
                    return nullptr;
                }
                orgEncoding = buffer;
                for (char &temp : buffer) {
                    temp = std::tolower(static_cast<unsigned char>(temp));
                }
                NAPI_ASSERT(env, CheckEncodingFormat(buffer),
                            "Wrong encoding format, the current encoding format is not support");
                encoding = buffer;
            }
        }
        return InitTextEncoder(env, thisVar, encoding, orgEncoding);
    }

    static napi_value GetEncoding(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr));

        TextEncoder *object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));

        return object->GetEncoding(env);
    }

    static napi_value Encode(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t requireArgc = 1;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc <= requireArgc, "Wrong number of arguments");

        napi_value result;
        if (argc == 1) {
            napi_valuetype valuetype;
            NAPI_CALL(env, napi_typeof(env, args, &valuetype));
            if (!IsValidValue(env, args)) {
                napi_get_undefined(env, &result);
                return result;
            }
            NAPI_ASSERT(env, valuetype == napi_string, "Wrong argument type. String expected.");
        } else {
            napi_get_undefined(env, &result);
            return result;
        }
        TextEncoder *object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));

        result = object->Encode(env, args);

        return result;
    }

    static napi_value EncodeIntoOne(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        // EncodeIntoOne is invoked by EncodeIntoArgs, argc can be 0 or 1
        // if argc is 0, typeof args is undefined.
        size_t argc = 1;
        napi_value args = nullptr;
        napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr);
        napi_value result = nullptr;
        napi_valuetype valuetype;
        NAPI_CALL(env, napi_typeof(env, args, &valuetype));

        if (valuetype == napi_null || valuetype == napi_undefined) {
            napi_get_undefined(env, &result);
            return result;
        }
        if (valuetype != napi_string) {
            return ThrowError(env, "Parameter error. The type of Parameter must be string.");
        }

        TextEncoder *object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void **)&object));
        result = object->Encode(env, args);
        return result;
    }

    static napi_value EncodeIntoTwo(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t requireArgc = 2; // 2:The number of parameters is 2
        size_t argc = 2; // 2:The number of parameters is 2
        napi_value args[2] = { nullptr }; // 2:The number of parameters is 2
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr));

        NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");

        napi_valuetype valuetype0;
        NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

        napi_typedarray_type valuetype1;
        size_t length = 0;
        void *data = nullptr;
        napi_value arraybuffer = nullptr;
        size_t byteOffset = 0;
        NAPI_CALL(env, napi_get_typedarray_info(env, args[1], &valuetype1, &length, &data, &arraybuffer, &byteOffset));

        NAPI_ASSERT(env, valuetype0 == napi_string, "Wrong argument type. String expected.");
        NAPI_ASSERT(env, valuetype1 == napi_uint8_array, "Wrong argument type. napi_uint8_array expected.");

        TextEncoder *object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));

        napi_value result = object->EncodeInto(env, args[0], args[1]);

        return result;
    }

    static napi_value EncodeIntoUint8Array(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t argc = 2; // 2:The number of parameters is 2
        napi_value args[2] = { nullptr }; // 2:The number of parameters is 2
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr));
        napi_valuetype valuetype0;
        NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));
        napi_typedarray_type valuetype1;
        size_t length = 0;
        void *data = nullptr;
        napi_value arraybuffer = nullptr;
        size_t byteOffset = 0;
        napi_get_typedarray_info(env, args[1], &valuetype1, &length, &data, &arraybuffer, &byteOffset);
        if (valuetype0 != napi_string) {
            return ThrowError(env, "Parameter error. The type of Parameter must be string.");
        }
        if (valuetype1 != napi_uint8_array) {
            return ThrowError(env, "Parameter error. The type of Parameter must be Uint8Array.");
        }
        TextEncoder *object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value result = object->EncodeInto(env, args[0], args[1]);
        return result;
    }

    static napi_value EncodeIntoArgs(napi_env env, napi_callback_info info)
    {
        size_t argc = 0;
        napi_get_cb_info(env, info, &argc, nullptr, nullptr, nullptr);
        if (argc >= 2) { // 2:The number of parameters is 2
            return EncodeIntoTwo(env, info);
        }
        return EncodeIntoOne(env, info);
    }

    static napi_value Create(napi_env env, napi_callback_info info)
    {
        napi_value textEncoderObj = TextEncoderConstructor(env, info);
        napi_property_descriptor textEncoderDesc[] = {
            DECLARE_NAPI_GETTER("encoding", GetEncoding),
            DECLARE_NAPI_FUNCTION("encode", Encode),
            DECLARE_NAPI_FUNCTION("encodeInto", EncodeIntoArgs),
            DECLARE_NAPI_FUNCTION("encodeIntoUint8Array", EncodeIntoUint8Array),
        };
        napi_define_properties(env, textEncoderObj, sizeof(textEncoderDesc) / sizeof(textEncoderDesc[0]),
                               textEncoderDesc);
        return textEncoderObj;
    }

    static napi_value TextcoderInit(napi_env env, napi_value exports)
    {
        const char *textEncoderClassName = "TextEncoder";
        napi_value textEncoderClass = nullptr;
        napi_property_descriptor textEncoderDesc[] = {
            DECLARE_NAPI_GETTER("encoding", GetEncoding),
            DECLARE_NAPI_FUNCTION("encode", Encode),
            DECLARE_NAPI_FUNCTION("encodeInto", EncodeIntoArgs),
            DECLARE_NAPI_FUNCTION("encodeIntoUint8Array", EncodeIntoUint8Array),
            DECLARE_NAPI_STATIC_FUNCTION("create", Create),

        };
        NAPI_CALL(env, napi_define_class(env, textEncoderClassName, strlen(textEncoderClassName),
                                         TextEncoderConstructor, nullptr,
                                         sizeof(textEncoderDesc) / sizeof(textEncoderDesc[0]),
                                         textEncoderDesc, &textEncoderClass));

        const char *textDecoderClassName = "TextDecoder";
        napi_value textDecoderClass = nullptr;
        napi_property_descriptor textdecoderDesc[] = {
            DECLARE_NAPI_FUNCTION("decodeToString", DecodeToString),
            DECLARE_NAPI_FUNCTION("decode", TextdecoderDecode),
            DECLARE_NAPI_FUNCTION("decodeWithStream", TextdecoderDecode),
        };
        NAPI_CALL(env, napi_define_class(env, textDecoderClassName, strlen(textDecoderClassName),
                                         TextdecoderConstructor, nullptr,
                                         sizeof(textdecoderDesc) / sizeof(textdecoderDesc[0]),
                                         textdecoderDesc, &textDecoderClass));
        napi_property_descriptor desc[] = {
            DECLARE_NAPI_PROPERTY("TextEncoder", textEncoderClass),
            DECLARE_NAPI_PROPERTY("TextDecoder", textDecoderClass),
        };

        NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));

        napi_value global = nullptr;
        NAPI_CALL(env, napi_get_global(env, &global));
        NAPI_CALL(env, napi_set_named_property(env, global, "TextDecoderCreate_", textDecoderClass));

        return exports;
    }

    static napi_value Base64Constructor(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        void *data = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, &data));
        auto objectInfo = new Base64();
        napi_status status = napi_wrap(env, thisVar, objectInfo,
            [](napi_env environment, void *data, void *hint) {
                auto objInfo = reinterpret_cast<Base64*>(data);
                if (objInfo != nullptr) {
                    delete objInfo;
                    objInfo = nullptr;
                }
            }, nullptr, nullptr);
        if (status != napi_ok && objectInfo != nullptr) {
            HILOG_ERROR("Base64Constructor:: napi_wrap failed");
            delete objectInfo;
            objectInfo = nullptr;
        }
        return thisVar;
    }

    static napi_value EncodeBase64(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t requireArgc = 1;
        size_t argc = 1;
        napi_value args[1] = { nullptr };
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");
        napi_typedarray_type valuetype0;
        size_t length = 0;
        void *data = nullptr;
        napi_value arraybuffer = nullptr;
        size_t byteOffset = 0;
        NAPI_CALL(env, napi_get_typedarray_info(env, args[0], &valuetype0, &length, &data, &arraybuffer, &byteOffset));
        NAPI_ASSERT(env, valuetype0 == napi_uint8_array, "Wrong argument type. napi_uint8_array expected.");
        Base64 *object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value result = object->EncodeSync(env, args[0], Type::BASIC);
        return result;
    }

    static napi_value EncodeToString(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t requireArgc = 1;
        size_t argc = 1;
        napi_value args[1] = { nullptr };
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");
        napi_typedarray_type valuetype0;
        size_t length = 0;
        void *data = nullptr;
        napi_value arraybuffer = nullptr;
        size_t byteOffset = 0;
        NAPI_CALL(env, napi_get_typedarray_info(env, args[0], &valuetype0, &length, &data, &arraybuffer, &byteOffset));
        NAPI_ASSERT(env, valuetype0 == napi_uint8_array, "Wrong argument type. napi_uint8_array expected.");
        Base64 *object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value result = object->EncodeToStringSync(env, args[0], Type::BASIC);
        return result;
    }

    static napi_value DecodeBase64(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t requireArgc = 1;
        size_t argc = 1;
        napi_value args[1] = { nullptr };
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");
        napi_typedarray_type valuetype0;
        napi_valuetype valuetype1;
        size_t length = 0;
        void *data = nullptr;
        napi_value arraybuffer = nullptr;
        size_t byteOffset = 0;
        NAPI_CALL(env, napi_typeof(env, args[0], &valuetype1));
        if (valuetype1 != napi_valuetype::napi_string) {
            NAPI_CALL(env, napi_get_typedarray_info(env, args[0], &valuetype0, &length,
                                                    &data, &arraybuffer, &byteOffset));
        }
        if ((valuetype1 != napi_valuetype::napi_string) && (valuetype0 != napi_typedarray_type::napi_uint8_array)) {
            napi_throw_error(env, nullptr, "The parameter type is incorrect");
            return nullptr;
        }
        Base64 *object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value result = object->DecodeSync(env, args[0], Type::BASIC);
        return result;
    }

    static napi_value EncodeAsync(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t requireArgc = 1;
        size_t argc = 1;
        napi_value args[1] = { nullptr };
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");
        napi_typedarray_type valuetype0;
        size_t length = 0;
        void *data = nullptr;
        napi_value arraybuffer = nullptr;
        size_t byteOffset = 0;
        NAPI_CALL(env, napi_get_typedarray_info(env, args[0], &valuetype0, &length, &data, &arraybuffer, &byteOffset));
        NAPI_ASSERT(env, valuetype0 == napi_uint8_array, "Wrong argument type. napi_uint8_array expected.");
        Base64 *object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value result = object->Encode(env, args[0], Type::BASIC);
        return result;
    }

    static napi_value EncodeToStringAsync(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t requireArgc = 1;
        size_t argc = 1;
        napi_value args[1] = { nullptr };
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");
        napi_typedarray_type valuetype0;
        size_t length = 0;
        void *data = nullptr;
        napi_value arraybuffer = nullptr;
        size_t byteOffset = 0;
        NAPI_CALL(env, napi_get_typedarray_info(env, args[0], &valuetype0, &length, &data, &arraybuffer, &byteOffset));
        NAPI_ASSERT(env, valuetype0 == napi_uint8_array, "Wrong argument type. napi_uint8_array expected.");
        Base64 *object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value result = object->EncodeToString(env, args[0], Type::BASIC);
        return result;
    }
    static napi_value DecodeAsync(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t requireArgc = 1;
        size_t argc = 1;
        napi_value args[1] = { nullptr };
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");
        napi_typedarray_type valuetype0;
        napi_valuetype valuetype1;
        size_t length = 0;
        void *data = nullptr;
        napi_value arraybuffer = nullptr;
        size_t byteOffset = 0;
        NAPI_CALL(env, napi_typeof(env, args[0], &valuetype1));
        if (valuetype1 != napi_valuetype::napi_string) {
            NAPI_CALL(env, napi_get_typedarray_info(env, args[0], &valuetype0,
                                                    &length, &data, &arraybuffer, &byteOffset));
        }
        if ((valuetype1 != napi_valuetype::napi_string) && (valuetype0 != napi_typedarray_type::napi_uint8_array)) {
            napi_throw_error(env, nullptr, "The parameter type is incorrect");
            return nullptr;
        }
        Base64 *object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value result = object->Decode(env, args[0], Type::BASIC);
        return result;
    }

    static napi_value EncodeToStringHelper(napi_env env, napi_callback_info info)
    {
        size_t argc = 2; // 2:The number of parameters is 2
        napi_value args[2] = { nullptr };
        napi_value thisVar = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr));
        int32_t encode = 0;
        NAPI_CALL(env, napi_get_value_int32(env, args[1], &encode));
        Type typeValue = static_cast<Type>(encode);
        if (typeValue < Type::TYPED_FIRST || typeValue > Type::TYPED_LAST) {
            return ThrowError(env,
                              "Parameter error. The target encoding type option must be one of the Type enumerations.");
        }
        Base64 *object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        return object->EncodeToStringSync(env, args[0], typeValue);
    }

    static napi_value EncodeBase64Helper(napi_env env, napi_callback_info info)
    {
        size_t argc = 2; // 2:The number of parameters is 2
        napi_value args[2] = { nullptr };
        napi_value thisVar = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr));
        int32_t encode = 0;
        NAPI_CALL(env, napi_get_value_int32(env, args[1], &encode));
        Type typeValue = static_cast<Type>(encode);
        if (typeValue != Type::BASIC && typeValue != Type::BASIC_URL_SAFE) {
            return ThrowError(env, "Parameter error. The target encoding type option must be BASIC or BASIC_URL_SAFE.");
        }
        Base64 *object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        return object->EncodeSync(env, args[0], typeValue);
    }

    static napi_value EncodeAsyncHelper(napi_env env, napi_callback_info info)
    {
        size_t argc = 2; // 2:The number of parameters is 2
        napi_value args[2] = { nullptr };
        napi_value thisVar = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr));
        int32_t encode = 0;
        NAPI_CALL(env, napi_get_value_int32(env, args[1], &encode));
        Type typeValue = static_cast<Type>(encode);
        Base64 *object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        return object->Encode(env, args[0], typeValue);
    }

    static napi_value EncodeToStringAsyncHelper(napi_env env, napi_callback_info info)
    {
        size_t argc = 2; // 2:The number of parameters is 2
        napi_value args[2] = { nullptr };
        napi_value thisVar = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr));
        int32_t encode = 0;
        NAPI_CALL(env, napi_get_value_int32(env, args[1], &encode));
        Type typeValue = static_cast<Type>(encode);
        Base64 *object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value result = object->EncodeToString(env, args[0], typeValue);
        return result;
    }

    static napi_value DecodeBase64Helper(napi_env env, napi_callback_info info)
    {
        size_t argc = 2; // 2:The number of parameters is 2
        napi_value args[2] = { nullptr };
        napi_value thisVar = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr));
        int32_t encode = 0;
        NAPI_CALL(env, napi_get_value_int32(env, args[1], &encode));
        Type typeValue = static_cast<Type>(encode);
        if (typeValue < Type::TYPED_FIRST || typeValue > Type::TYPED_LAST) {
            return ThrowError(env,
                "Parameter error. The target encoding type option must be one of the Type enumerations.");
        }
        Base64 *object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        return object->DecodeSync(env, args[0], typeValue);
    }

    static napi_value DecodeAsyncHelper(napi_env env, napi_callback_info info)
    {
        size_t argc = 2; // 2:The number of parameters is 2
        napi_value args[2] = { nullptr };
        napi_value thisVar = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr));
        int32_t encode = 0;
        NAPI_CALL(env, napi_get_value_int32(env, args[1], &encode));
        Type typeValue = static_cast<Type>(encode);
        Base64 *object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        return object->Decode(env, args[0], typeValue);
    }

    // Types
    static napi_value TypesConstructor(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        void* data = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, &data));
        auto objectInfo = new Types();
        napi_status status = napi_wrap(env, thisVar, objectInfo,
            [](napi_env environment, void* data, void* hint) {
                auto objectInformation = reinterpret_cast<Types*>(data);
                if (objectInformation != nullptr) {
                    delete objectInformation;
                    objectInformation = nullptr;
                }
            }, nullptr, nullptr);
        if (status != napi_ok && objectInfo != nullptr) {
            delete objectInfo;
            objectInfo = nullptr;
        }
        return thisVar;
    }

    static napi_value IsAnyArrayBuffer(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        Types* object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value rst = object->IsAnyArrayBuffer(env, args);
        return rst;
    }

    static napi_value IsArrayBufferView(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        Types* object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value rst = object->IsArrayBufferView(env, args);
        return rst;
    }

    static napi_value IsArgumentsObject(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        Types* object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value rst = object->IsArgumentsObject(env, args);
        return rst;
    }

    static napi_value IsArrayBuffer(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        Types* object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value rst = object->IsArrayBuffer(env, args);
        return rst;
    }

    static napi_value IsAsyncFunction(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        Types* object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value rst = object->IsAsyncFunction(env, args);
        return rst;
    }

    static napi_value IsBigInt64Array(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        Types* object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value rst = object->IsBigInt64Array(env, args);
        return rst;
    }

    static napi_value IsBigUint64Array(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        Types* object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value rst = object->IsBigUint64Array(env, args);
        return rst;
    }

    static napi_value IsBooleanObject(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        Types* object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value rst = object->IsBooleanObject(env, args);
        return rst;
    }

    static napi_value IsBoxedPrimitive(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        Types* object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value rst = object->IsBoxedPrimitive(env, args);
        return rst;
    }

    static napi_value IsDataView(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        Types* object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value rst = object->IsDataView(env, args);
        return rst;
    }

    static napi_value IsDate(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        Types* object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value rst = object->IsDate(env, args);
        return rst;
    }

    static napi_value IsExternal(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        Types* object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value rst = object->IsExternal(env, args);
        return rst;
    }

    static napi_value IsFloat32Array(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        Types* object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value rst = object->IsFloat32Array(env, args);
        return rst;
    }

    static napi_value IsFloat64Array(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        Types* object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value rst = object->IsFloat64Array(env, args);
        return rst;
    }

    static napi_value IsGeneratorFunction(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        Types* object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value rst = object->IsGeneratorFunction(env, args);
        return rst;
    }

    static napi_value IsGeneratorObject(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t requireArgc = 1;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");
        Types* object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value result = object->IsGeneratorObject(env, args);
        return result;
    }

    static napi_value IsInt8Array(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t requireArgc = 1;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");
        Types* object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value result = object->IsInt8Array(env, args);
        return result;
    }

    static napi_value IsInt16Array(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t requireArgc = 1;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");
        Types* object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value result = object->IsInt16Array(env, args);
        return result;
    }

    static napi_value IsInt32Array(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t requireArgc = 1;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");
        Types* object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value result = object->IsInt32Array(env, args);
        return result;
    }

    static napi_value IsMap(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t requireArgc = 1;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");
        Types* object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value result = object->IsMap(env, args);
        return result;
    }

    static napi_value IsMapIterator(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t requireArgc = 1;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");
        Types* object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value result = object->IsMapIterator(env, args);
        return result;
    }

    static napi_value IsModuleNamespaceObject(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t requireArgc = 1;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");
        Types* object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value result = object->IsModuleNamespaceObject(env, args);
        return result;
    }

    static napi_value IsNativeError(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t requireArgc = 1;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");
        Types* object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value result = object->IsNativeError(env, args);
        return result;
    }

    static napi_value IsNumberObject(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t requireArgc = 1;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");
        Types* object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value result = object->IsNumberObject(env, args);
        return result;
    }

    static napi_value IsPromise(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t requireArgc = 1;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");
        Types* object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value result = object->IsPromise(env, args);
        return result;
    }

    static napi_value IsProxy(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t requireArgc = 1;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");
        Types* object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value result = object->IsProxy(env, args);
        return result;
    }

    static napi_value IsRegExp(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t requireArgc = 1;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");
        Types* object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value result = object->IsRegExp(env, args);
        return result;
    }

    static napi_value IsSet(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t requireArgc = 1;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");
        Types* object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value result = object->IsSet(env, args);
        return result;
    }

    static napi_value IsSetIterator(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t requireArgc = 1;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");
        Types* object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value result = object->IsSetIterator(env, args);
        return result;
    }

    static napi_value IsSharedArrayBuffer(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t requireArgc = 1;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");
        Types* object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value result = object->IsSharedArrayBuffer(env, args);
        return result;
    }

    static napi_value IsStringObject(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t requireArgc = 1;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");
        Types* object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value result = object->IsStringObject(env, args);
        return result;
    }

    static napi_value IsSymbolObject(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t requireArgc = 1;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");
        Types* object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value result = object->IsSymbolObject(env, args);
        return result;
    }

    static napi_value IsTypedArray(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t requireArgc = 1;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");
        Types* object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value result = object->IsTypedArray(env, args);
        return result;
    }

    static napi_value IsUint8Array(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t requireArgc = 1;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");
        Types* object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value result = object->IsUint8Array(env, args);
        return result;
    }

    static napi_value IsUint8ClampedArray(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t requireArgc = 1;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");
        Types* object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value result = object->IsUint8ClampedArray(env, args);
        return result;
    }

    static napi_value IsUint16Array(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t requireArgc = 1;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");
        Types* object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value result = object->IsUint16Array(env, args);
        return result;
    }

    static napi_value IsUint32Array(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t requireArgc = 1;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");
        Types* object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value result = object->IsUint32Array(env, args);
        return result;
    }

    static napi_value IsWeakMap(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t requireArgc = 1;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");
        Types* object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value result = object->IsWeakMap(env, args);
        return result;
    }

    static napi_value IsWeakSet(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t requireArgc = 1;
        size_t argc = 1;
        napi_value args = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &args, &thisVar, nullptr));
        NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");
        Types* object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        napi_value result = object->IsWeakSet(env, args);
        return result;
    }

    // StringDecoder
    static napi_value StringDecoderConstructor(napi_env env, napi_callback_info info)
    {
        size_t argc = 1;
        napi_value thisVar = nullptr;
        napi_value argv = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &argv, &thisVar, nullptr));
        std::string enconding = "utf-8";
        if (argc == 1) {
            napi_valuetype valuetype;
            NAPI_CALL(env, napi_typeof(env, argv, &valuetype));
            if (valuetype == napi_string) {
                size_t bufferSize = 0;
                if (napi_get_value_string_utf8(env, argv, nullptr, 0, &bufferSize) != napi_ok) {
                    HILOG_ERROR("StringDecoder:: can not get argv size");
                    return nullptr;
                }
                std::string buffer = "";
                buffer.reserve(bufferSize);
                buffer.resize(bufferSize);
                if (napi_get_value_string_utf8(env, argv, buffer.data(), bufferSize + 1, &bufferSize) != napi_ok) {
                    HILOG_ERROR("StringDecoder:: can not get argv value");
                    return nullptr;
                }
                if (!CheckEncodingFormat(buffer)) {
                    napi_throw_error(env, "401",
                        "Parameter error. Wrong encoding format, the current encoding format is not support.");
                    return nullptr;
                }
                enconding = buffer;
            }
        }
        auto objectInfo = new StringDecoder(enconding);
        napi_status status = napi_wrap(env, thisVar, objectInfo,
            [](napi_env environment, void* data, void* hint) {
                auto obj = reinterpret_cast<StringDecoder*>(data);
                if (obj != nullptr) {
                    delete obj;
                    obj = nullptr;
                }
            }, nullptr, nullptr);
        if (status != napi_ok && objectInfo != nullptr) {
            HILOG_ERROR("StringDecoderConstructor:: napi_wrap failed.");
            delete objectInfo;
            objectInfo = nullptr;
        }
        return thisVar;
    }

    static napi_value Write(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t argc = 1;
        napi_value argv = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &argv, &thisVar, nullptr));
        napi_valuetype valuetype;
        NAPI_CALL(env, napi_typeof(env, argv, &valuetype));
        if (valuetype == napi_string) {
            return argv;
        }
        StringDecoder *object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        return object->Write(env, argv);
    }

    static napi_value End(napi_env env, napi_callback_info info)
    {
        napi_value thisVar = nullptr;
        size_t argc = 1;
        napi_value argv = nullptr;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &argv, &thisVar, nullptr));
        StringDecoder *object = nullptr;
        NAPI_CALL(env, napi_unwrap(env, thisVar, (void**)&object));
        if (argc == 0) {
            return object->End(env);
        }
        napi_valuetype valuetype;
        NAPI_CALL(env, napi_typeof(env, argv, &valuetype));
        if (valuetype == napi_string) {
            return argv;
        }
        if (valuetype == napi_undefined) {
            return object->End(env);
        }
        return object->End(env, argv);
    }

    static napi_value TypeofInit(napi_env env, napi_value exports)
    {
        const char* typeofClassName = "Types";
        napi_value typeofClass = nullptr;
        napi_property_descriptor typeofDesc[] = {
            DECLARE_NAPI_FUNCTION("isBigInt64Array", IsBigInt64Array),
            DECLARE_NAPI_FUNCTION("isBigUint64Array", IsBigUint64Array),
            DECLARE_NAPI_FUNCTION("isBooleanObject", IsBooleanObject),
            DECLARE_NAPI_FUNCTION("isBoxedPrimitive", IsBoxedPrimitive),
            DECLARE_NAPI_FUNCTION("isAnyArrayBuffer", IsAnyArrayBuffer),
            DECLARE_NAPI_FUNCTION("isArrayBufferView", IsArrayBufferView),
            DECLARE_NAPI_FUNCTION("isArgumentsObject", IsArgumentsObject),
            DECLARE_NAPI_FUNCTION("isArrayBuffer", IsArrayBuffer),
            DECLARE_NAPI_FUNCTION("isDataView", IsDataView),
            DECLARE_NAPI_FUNCTION("isDate", IsDate),
            DECLARE_NAPI_FUNCTION("isExternal", IsExternal),
            DECLARE_NAPI_FUNCTION("isFloat32Array", IsFloat32Array),
            DECLARE_NAPI_FUNCTION("isFloat64Array", IsFloat64Array),
            DECLARE_NAPI_FUNCTION("isGeneratorFunction", IsGeneratorFunction),
            DECLARE_NAPI_FUNCTION("isGeneratorObject", IsGeneratorObject),
            DECLARE_NAPI_FUNCTION("isInt8Array", IsInt8Array),
            DECLARE_NAPI_FUNCTION("isInt16Array", IsInt16Array),
            DECLARE_NAPI_FUNCTION("isInt32Array", IsInt32Array),
            DECLARE_NAPI_FUNCTION("isMap", IsMap),
            DECLARE_NAPI_FUNCTION("isMapIterator", IsMapIterator),
            DECLARE_NAPI_FUNCTION("isModuleNamespaceObject", IsModuleNamespaceObject),
            DECLARE_NAPI_FUNCTION("isNativeError", IsNativeError),
            DECLARE_NAPI_FUNCTION("isNumberObject", IsNumberObject),
            DECLARE_NAPI_FUNCTION("isPromise", IsPromise),
            DECLARE_NAPI_FUNCTION("isProxy", IsProxy),
            DECLARE_NAPI_FUNCTION("isRegExp", IsRegExp),
            DECLARE_NAPI_FUNCTION("isSet", IsSet),
            DECLARE_NAPI_FUNCTION("isSetIterator", IsSetIterator),
            DECLARE_NAPI_FUNCTION("isSharedArrayBuffer", IsSharedArrayBuffer),
            DECLARE_NAPI_FUNCTION("isStringObject", IsStringObject),
            DECLARE_NAPI_FUNCTION("isSymbolObject", IsSymbolObject),
            DECLARE_NAPI_FUNCTION("isTypedArray", IsTypedArray),
            DECLARE_NAPI_FUNCTION("isUint8Array", IsUint8Array),
            DECLARE_NAPI_FUNCTION("isUint8ClampedArray", IsUint8ClampedArray),
            DECLARE_NAPI_FUNCTION("isUint16Array", IsUint16Array),
            DECLARE_NAPI_FUNCTION("isUint32Array", IsUint32Array),
            DECLARE_NAPI_FUNCTION("isWeakMap", IsWeakMap),
            DECLARE_NAPI_FUNCTION("isWeakSet", IsWeakSet),
            DECLARE_NAPI_FUNCTION("isAsyncFunction", IsAsyncFunction),
        };
        NAPI_CALL(env, napi_define_class(env, typeofClassName, strlen(typeofClassName), TypesConstructor,
                                         nullptr, sizeof(typeofDesc) / sizeof(typeofDesc[0]), typeofDesc,
                                         &typeofClass));
        napi_property_descriptor desc[] = { DECLARE_NAPI_PROPERTY("Types", typeofClass) };
        NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
        return exports;
    }

    static napi_value Base64Init(napi_env env, napi_value exports)
    {
        const char *base64ClassName = "Base64";
        napi_value base64Class = nullptr;
        napi_property_descriptor base64Desc[] = {
            DECLARE_NAPI_FUNCTION("encodeSync", EncodeBase64),
            DECLARE_NAPI_FUNCTION("encodeToStringSync", EncodeToString),
            DECLARE_NAPI_FUNCTION("decodeSync", DecodeBase64),
            DECLARE_NAPI_FUNCTION("encode", EncodeAsync),
            DECLARE_NAPI_FUNCTION("encodeToString", EncodeToStringAsync),
            DECLARE_NAPI_FUNCTION("decode", DecodeAsync),
        };
        NAPI_CALL(env, napi_define_class(env, base64ClassName, strlen(base64ClassName), Base64Constructor,
                                         nullptr, sizeof(base64Desc) / sizeof(base64Desc[0]), base64Desc,
                                         &base64Class));
        napi_property_descriptor desc[] = {
            DECLARE_NAPI_PROPERTY("Base64", base64Class)
        };
        NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
        return exports;
    }

    static napi_value Base64HelperInit(napi_env env, napi_value exports)
    {
        const char *base64HelperClassName = "Base64Helper";
        napi_value Base64HelperClass = nullptr;
        napi_property_descriptor Base64HelperDesc[] = {
            DECLARE_NAPI_FUNCTION("encodeSync", EncodeBase64Helper),
            DECLARE_NAPI_FUNCTION("encodeToStringSync", EncodeToStringHelper),
            DECLARE_NAPI_FUNCTION("decodeSync", DecodeBase64Helper),
            DECLARE_NAPI_FUNCTION("encode", EncodeAsyncHelper),
            DECLARE_NAPI_FUNCTION("encodeToString", EncodeToStringAsyncHelper),
            DECLARE_NAPI_FUNCTION("decode", DecodeAsyncHelper),
        };
        NAPI_CALL(env, napi_define_class(env, base64HelperClassName, strlen(base64HelperClassName), Base64Constructor,
                                         nullptr, sizeof(Base64HelperDesc) / sizeof(Base64HelperDesc[0]),
                                         Base64HelperDesc, &Base64HelperClass));
        napi_property_descriptor desc[] = {
            DECLARE_NAPI_PROPERTY("Base64Helper", Base64HelperClass)
        };
        NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
        return exports;
    }

    static napi_value StringDecoderInit(napi_env env, napi_value exports)
    {
        const char *stringDecoderClassName = "StringDecoder";
        napi_value StringDecoderClass = nullptr;
        napi_property_descriptor StringDecoderDesc[] = {
            DECLARE_NAPI_FUNCTION("write", Write),
            DECLARE_NAPI_FUNCTION("end", End),
        };
        NAPI_CALL(env, napi_define_class(env, stringDecoderClassName, strlen(stringDecoderClassName),
                                         StringDecoderConstructor, nullptr,
                                         sizeof(StringDecoderDesc) / sizeof(StringDecoderDesc[0]),
                                         StringDecoderDesc, &StringDecoderClass));
        napi_property_descriptor desc[] = {
            DECLARE_NAPI_PROPERTY("StringDecoder", StringDecoderClass)
        };
        NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
        return exports;
    }

    static napi_value UtilInit(napi_env env, napi_value exports)
    {
        napi_property_descriptor desc[] = {
            DECLARE_NAPI_FUNCTION("printf", Printf),
            DECLARE_NAPI_FUNCTION("format", Printf),
            DECLARE_NAPI_FUNCTION("geterrorstring", GetErrorString),
            DECLARE_NAPI_FUNCTION("errnoToString", GetErrorString),
            DECLARE_NAPI_FUNCTION("dealwithformatstring", DealWithFormatString),
            DECLARE_NAPI_FUNCTION("randomUUID", RandomUUID),
            DECLARE_NAPI_FUNCTION("randomBinaryUUID", RandomBinaryUUID),
            DECLARE_NAPI_FUNCTION("parseUUID", ParseUUID),
            DECLARE_NAPI_FUNCTION("getHash", GetHash),
            DECLARE_NAPI_FUNCTION("getMainThreadStackTrace", GetMainThreadStackTrace)
        };
        NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
        TextcoderInit(env, exports);
        Base64Init(env, exports);
        Base64HelperInit(env, exports);
        TypeofInit(env, exports);
        StringDecoderInit(env, exports);
        return exports;
    }

    // util JS register
    extern "C"
    __attribute__((visibility("default"))) void NAPI_util_GetJSCode(const char **buf, int *buflen)
    {
        if (buf != nullptr) {
            *buf = _binary_util_js_js_start;
        }
        if (buflen != nullptr) {
            *buflen = _binary_util_js_js_end - _binary_util_js_js_start;
        }
    }
    extern "C"
    __attribute__((visibility("default"))) void NAPI_util_GetABCCode(const char** buf, int* buflen)
    {
        if (buf != nullptr) {
            *buf = _binary_util_abc_start;
        }
        if (buflen != nullptr) {
            *buflen = _binary_util_abc_end - _binary_util_abc_start;
        }
    }

    // util module define
    static napi_module_with_js utilModule = {
        .nm_version = 1,
        .nm_flags = 0,
        .nm_filename = nullptr,
        .nm_register_func = UtilInit,
        .nm_modname = "util",
        .nm_priv = ((void*)0),
        .nm_get_abc_code = NAPI_util_GetABCCode,
        .nm_get_js_code = NAPI_util_GetJSCode,
    };
    // util module register
    extern "C"
    __attribute__((constructor)) void UtilRegisterModule()
    {
        napi_module_with_js_register(&utilModule);
        UtilPluginJniRegister();
    }
}
