/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <ani.h>
#include <array>
#include <iostream>
#include "api/ani_textdecoder.h"
#include "api/ani_textencoder.h"
#include "ohos/init_data.h"

namespace OHOS::ETSUtil {
static std::string ANIStringToStdString(ani_env *env, ani_string ani_str)
{
    ani_size  strSize;
    env->String_GetUTF8Size(ani_str, &strSize);

    std::vector<char> buffer(strSize + 1); // +1 for null terminator
    char* utf8Buffer = buffer.data();

    ani_size bytes_written = 0;
    env->String_GetUTF8(ani_str, utf8Buffer, strSize + 1, &bytes_written);

    utf8Buffer[bytes_written] = '\0';
    std::string content = std::string(utf8Buffer);
    return content;
}

struct ArrayBufferInfo {
    void* data;
    size_t length;
    ani_status retCode;
};

static ArrayBufferInfo GetArrayInfo([[maybe_unused]] ani_env *env, ani_arraybuffer buffer)
{
    void* data;
    size_t length;
    ani_status retCode = env->ArrayBuffer_GetInfo(buffer, &data, &length);
    return {data, length, retCode};
}

static void GetUint8ArrayInfo(ani_env *env, ani_object array, void* &data, int32_t &byteLength, int32_t &byteOffset)
{
    ani_ref buffer;
    if (auto retCode = env->Object_GetFieldByName_Ref(array, "buffer", &buffer); retCode != ANI_OK) {
        HILOG_ERROR("TextDecoder:: env->Object_GetFieldByName_Ref() failed");
        return;
    }
    auto bufferInfo = GetArrayInfo(env, static_cast<ani_arraybuffer>(buffer));
    if (bufferInfo.retCode != ANI_OK) {
        HILOG_ERROR("TextDecoder:: env->ArrayBuffer_GetInfo() failed");
        return;
    }

    std::vector<int32_t> vec;
    for (const char* propName: {"byteLength", "byteOffset"}) {
        ani_int value;
        std::string fieldName = std::string{propName} + "Int";
        ani_status retCode = env->Object_GetFieldByName_Int(array, fieldName.c_str(), &value);
        if (retCode != ANI_OK) {
            HILOG_ERROR("TextDecoder:: env->Object_GetFieldByName_Int fieldName: %s\n", fieldName.c_str());
            return;
        }
        vec.push_back(value);
    }
    byteLength = vec[0];
    byteOffset = vec[1];
    data = bufferInfo.data;
}

static TextDecoder *unwrapp(ani_env *env, ani_object object)
{
    ani_long textDecoder;
    if (ANI_OK != env->Object_GetFieldByName_Long(object, "nativeDecoder_", &textDecoder)) {
        return nullptr;
    }
    return reinterpret_cast<TextDecoder *>(textDecoder);
}

static ani_string Decode(ani_env *env, ani_object object, ani_object typedArray, ani_boolean stream)
{
    void* data;
    int32_t byteLength;
    int32_t byteOffset;
    GetUint8ArrayInfo(env, typedArray, data, byteLength, byteOffset);
    bool iflag = stream == ANI_TRUE ? true : false;
    auto textDecoder = unwrapp(env, object);
    if (textDecoder != nullptr) {
        return textDecoder->DecodeToString(env, data, byteOffset, byteLength, iflag);
    }
    return nullptr;
}

static void BindNativeDecoder(ani_env *env, ani_object object, ani_string ani_encoding, ani_int flags)
{
    std::string stringEncoding = ANIStringToStdString(env, ani_encoding);
    auto nativeTextDecoder = new TextDecoder(stringEncoding, flags);
    env->Object_SetFieldByName_Long(object, "nativeDecoder_", reinterpret_cast<ani_long>(nativeTextDecoder));
}

static ani_status BindTextDecoder(ani_env *env)
{
    static const char *className = "L@ohos/util/util/TextDecoder;";
    ani_class cls;
    if (ANI_OK != env->FindClass(className, &cls)) {
        std::cerr << "Not found '" << className << "'" << std::endl;
        return ANI_ERROR;
    }

    std::array methods = {
        ani_native_function {"bindNativeDecoder", "Lstd/core/String;I:V", reinterpret_cast<void *>(BindNativeDecoder)},
        ani_native_function {"decode", "Lescompat/Uint8Array;Z:Lstd/core/String;", reinterpret_cast<void *>(Decode)},
    };

    if (ANI_OK != env->Class_BindNativeMethods(cls, methods.data(), methods.size())) {
        HILOG_ERROR("TextDecoder:: Cannot bind native methods to className : %s\n", className);
        return ANI_ERROR;
    }
    return ANI_OK;
}

[[maybe_unused]] static ani_status BindTextEncoder(ani_env *env)
{
    ani_class cls;
    const char *className = "L@ohos/util/util/TextEncoder;";
    if (ANI_OK != env->FindClass(className, &cls)) {
        HILOG_ERROR("Cannot find class '%s'.", className);
        return ANI_ERROR;
    }
    std::array barMethods = {
        ani_native_function {
            "doEncodeInto",
            "Lstd/core/String;Lstd/core/String;:Lescompat/Uint8Array;",
            reinterpret_cast<void*>(DoEncodeInto),
        },
        ani_native_function{
            "doEncodeInfoUint8Array",
            "Lstd/core/String;Lstd/core/String;Lescompat/Uint8Array;:L@ohos/util/util/EncodeIntoUint8ArrayInfo;",
            reinterpret_cast<void*>(DoEncodeIntoUint8Array),
        },
    };
    if (ANI_OK != env->Class_BindNativeMethods(cls, barMethods.data(), barMethods.size())) {
        HILOG_ERROR("Cannot bind native methods to '%s'.", className);
        return ANI_ERROR;
    };
    SetOhosIcuDirectory();
    return ANI_OK;
}

extern "C" {
ANI_EXPORT ani_status ANI_Constructor(ani_vm *vm, uint32_t *result)
{
    ani_env *env;
    if (ANI_OK != vm->GetEnv(ANI_VERSION_1, &env)) {
        std::cerr << "Unsupported ANI_VERSION_1" << std::endl;
        HILOG_ERROR("Unsupported ANI_VERSION_1");
        return (ani_status)ANI_ERROR;
    }
    ani_status status = BindTextDecoder(env);
    if (status != ANI_OK) {
        HILOG_ERROR("BindTextDecoder Failed");
        return status;
    }
    status = BindTextEncoder(env);
    if (status != ANI_OK) {
        HILOG_ERROR("BindTextDecoder Failed");
        return status;
    }
    *result = ANI_VERSION_1;
    return ANI_OK;
}
}
}