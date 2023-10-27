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

#ifndef JS_CONCURRENT_MODULE_COMMON_HELPER_ERROR_HELPER_H
#define JS_CONCURRENT_MODULE_COMMON_HELPER_ERROR_HELPER_H

#include <ctime>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>
#include <vector>

#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "napi_helper.h"
#include "native_engine/native_engine.h"

namespace Commonlibrary::Concurrent::Common::Helper {
class ErrorHelper {
public:
    ErrorHelper() = default;
    ~ErrorHelper() = default;

    static napi_value NewError(napi_env env, int32_t errCode, const char* errMessage = nullptr)
    {
        std::string errTitle = "";
        napi_value concurrentError = nullptr;

        napi_value code = nullptr;
        napi_create_uint32(env, errCode, &code);

        napi_value name = nullptr;
        std::string errName = "BusinessError";
        switch (errCode) {
            case ERR_WORKER_INITIALIZATION:
                errTitle = "Worker initialization failure, ";
                break;
            case ERR_WORKER_NOT_RUNNING:
                errTitle = "Worker instance is not running, ";
                break;
            case ERR_WORKER_UNSUPPORTED:
                errTitle = "The invoked API is not supported in workers, ";
                break;
            case ERR_WORKER_SERIALIZATION:
                errTitle = "An exception occurred during serialization, ";
                break;
            case ERR_WORKER_INVALID_FILEPATH:
                errTitle = "The worker file path is invalid path, ";
                break;
            case ERR_NOT_CONCURRENT_FUNCTION:
                errTitle = "The function is not mark as concurrent, ";
                break;
            case ERR_CANCEL_NONEXIST_TASK:
                errTitle = "The task does not exist when it is canceled";
                break;
            case ERR_CANCEL_NONEXIST_TASK_GROUP:
                errTitle = "The task group does not exist when it is canceled";
                break;
            case ERR_CANCEL_RUNNING_TASK:
                errTitle = "The task is executing when it is canceled";
                break;
            case ERR_TRIGGER_NONEXIST_EVENT:
                errTitle = "The triggered event does not exist.";
                break;
            case ERR_CALL_METHOD_ON_BINDING_OBJ:
                errTitle = "The method called on binding object does not exist or is not callable";
                break;
            case ERR_EXCEED_WAITING_LIMITATION:
                errTitle = "SyncCall waiting time has exceeded the time limitation: ";
                break;
            default:
                break;
        }
        napi_create_string_utf8(env, errName.c_str(), NAPI_AUTO_LENGTH, &name);
        napi_value msg = nullptr;
        if (errMessage == nullptr) {
            napi_create_string_utf8(env, errTitle.c_str(), NAPI_AUTO_LENGTH, &msg);
        } else {
            napi_create_string_utf8(env, (errTitle + std::string(errMessage)).c_str(), NAPI_AUTO_LENGTH, &msg);
        }

        napi_create_error(env, nullptr, msg, &concurrentError);
        napi_set_named_property(env, concurrentError, "code", code);
        napi_set_named_property(env, concurrentError, "name", name);
        return concurrentError;
    }

    static void ThrowError(napi_env env, int32_t errCode, const char* errMessage = nullptr)
    {
        napi_value concurrentError = NewError(env, errCode, errMessage);
        napi_throw(env, concurrentError);
    }

    static std::string GetCurrentTimeStamp()
    {
        auto now = std::chrono::system_clock::now();
        std::time_t currentTimeStamp = std::chrono::system_clock::to_time_t(now);
        std::tm* timeInfo = std::localtime(&currentTimeStamp);
        std::stringstream ss;
        ss << std::put_time(timeInfo, "%Y-%m-%d %X");
        return ss.str();
    }

    static std::string GetErrorFileInfo(const std::string& input)
    {
        std::regex pattern("\\((.*?)\\)");
        std::smatch match;
        if (std::regex_search(input, match, pattern)) {
            return match[1].str();
        }
        return "";
    }

    static std::vector<std::string> SplitErrorFileInfo(const std::string& input, char delimiter, int count)
    {
        std::vector<std::string> result;
        std::string rawErrorInfo = GetErrorFileInfo(input);
        if (rawErrorInfo.empty()) {
            return result;
        }

        auto pos = rawErrorInfo.rfind(delimiter);
        while (pos != std::string::npos && count > 0) {
            result.push_back(rawErrorInfo.substr(pos + 1));
            rawErrorInfo = rawErrorInfo.substr(0, pos);
            pos = rawErrorInfo.rfind(delimiter);
            count--;
        }
        result.push_back(rawErrorInfo);
        std::reverse(result.begin(), result.end());
        return result;
    }

    static napi_value TranslateErrorEvent(napi_env env, napi_value error)
    {
        napi_value obj = NapiHelper::CreateObject(env);

        // add message
        napi_value msgValue = nullptr;
        napi_coerce_to_string(env, error, &msgValue);
        napi_set_named_property(env, obj, "message", msgValue);

        // add backtrace
        napi_value stack = NapiHelper::GetNameProperty(env, error, "stack");
        napi_set_named_property(env, obj, "backtrace", stack);

        // add timeStamp
        std::string current = GetCurrentTimeStamp();
        napi_value timeStamp = nullptr;
        napi_create_string_utf8(env, current.c_str(), NAPI_AUTO_LENGTH, &timeStamp);
        napi_set_named_property(env, obj, "timeStamp", timeStamp);
        char* stackValue = NapiHelper::GetString(env, stack);
        std::string rawStack = std::string(stackValue);
        delete[] stackValue;
        std::vector<std::string> result = SplitErrorFileInfo(rawStack, ':', 2); // 2 : the last two :
        if (result.size() == 3) { // 3 : the rawStack is divided into three parts by last two :
            // add filename
            napi_value filenameValue = nullptr;
            napi_create_string_utf8(env, result[0].c_str(), NAPI_AUTO_LENGTH, &filenameValue); // 0 : filename
            napi_set_named_property(env, obj, "filename", filenameValue);

            // add lineno
            napi_value lineno = nullptr;
            napi_create_string_utf8(env, result[1].c_str(), NAPI_AUTO_LENGTH, &lineno); // 1 : lineno
            napi_set_named_property(env, obj, "lineno", lineno);

            // add colno
            napi_value colno = nullptr;
            napi_create_string_utf8(env, result[2].c_str(), NAPI_AUTO_LENGTH, &colno); // 2 : colno
            napi_set_named_property(env, obj, "colno", colno);
        }

        // add type
        napi_value eventType = nullptr;
        napi_create_string_utf8(env, "ErrorEvent", NAPI_AUTO_LENGTH, &eventType);
        napi_set_named_property(env, obj, "type", eventType);

        // add error
        napi_set_named_property(env, obj, "error", error);

        return obj;
    }

    static const int32_t TYPE_ERROR = 401; // 401 : the parameter type is incorrect
    static const int32_t ERR_WORKER_INITIALIZATION = 10200003; // 10200003 : worker initialization failure
    static const int32_t ERR_WORKER_NOT_RUNNING = 10200004; // 10200004 : worker instance is not running
    static const int32_t ERR_WORKER_UNSUPPORTED = 10200005; // 10200005 : the invoked API is not supported in worker
    static const int32_t ERR_WORKER_SERIALIZATION = 10200006; // 10200006 : serialize an uncaught exception failed
    static const int32_t ERR_WORKER_INVALID_FILEPATH = 10200007; // 10200007 : the worker file path is invalid path
    static const int32_t ERR_NOT_CONCURRENT_FUNCTION = 10200014; // 10200014 : the function is not mark as concurrent
    static const int32_t ERR_CANCEL_NONEXIST_TASK = 10200015; // 10200015 : the task does not exist when it is canceled
    static const int32_t ERR_CANCEL_RUNNING_TASK = 10200016; // 10200016 : the task is executing when it is canceled
    static const int32_t ERR_CANCEL_NONEXIST_TASK_GROUP = 10200018; // 10200018 : cancel nonexist task group
    static const int32_t ERR_TRIGGER_NONEXIST_EVENT = 10200019; // 10200019 : The triggered event does not exist
    static const int32_t ERR_CALL_METHOD_ON_BINDING_OBJ = 10200020; // 10200020 : call method on binding obj failed
    static const int32_t ERR_EXCEED_WAITING_LIMITATION = 10200021; // 10200021 : SyncCall exceed waiting time limitation
    // add for inner implementation
    static const int32_t ERR_DURING_SYNC_CALL = 10200022; // 10200022 : SyncCall encountered exception during calling
};
} // namespace Commonlibrary::Concurrent::Common::Helper
#endif // JS_CONCURRENT_MODULE_COMMON_HELPER_ERROR_HELPER_H