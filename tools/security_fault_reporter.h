/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef COMMONLIBRARY_ETS_UTILS_TOOLS_SECURITY_FAULT_REPORTER_H
#define COMMONLIBRARY_ETS_UTILS_TOOLS_SECURITY_FAULT_REPORTER_H

#include <string>

#include "tools/log.h"

#ifdef ENABLE_ETS_UTILS_HISYSEVENT
#include "hisysevent.h"
#include <sys/syscall.h>
#include <unistd.h>
#endif

// Detection-only helper for security faults: prints an error log and reports the
// ARK_SECURITY_FAULT HiSysEvent. Never throws and never changes the caller's control
// flow - callers must fall through and keep the original behavior.
inline void ReportEtsUtilsSecurityFault(const std::string &funcName, const std::string &errorType,
                                        int32_t offset = -1, int32_t length = -1)
{
    HILOG_ERROR("EtsUtilsSecurityFault: %{public}s detected %{public}s, offset: %{public}d, length: %{public}d",
                funcName.c_str(), errorType.c_str(), offset, length);
#ifdef ENABLE_ETS_UTILS_HISYSEVENT
    HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::ARKTS_TASKPOOL,
        "ARK_SECURITY_FAULT", OHOS::HiviewDFX::HiSysEvent::EventType::FAULT,
        "PID", getpid(), "TID", static_cast<int32_t>(syscall(SYS_gettid)),
        "FUNCTION_NAME", funcName, "ERROR_TYPE", errorType, "OFFSET", offset, "LENGTH", length);
#endif
}

// Detection-only helper for incompatible API usage: prints an error log and reports the
// ARK_INCOMPATIBLE_EVENT HiSysEvent (STATISTIC). Never throws and never changes the caller's
// control flow - callers must fall through and keep the original behavior.
// Drop-in counterpart of ReportEtsUtilsSecurityFault (identical signature); differs only in
// event classification (STATISTIC incompatible event vs FAULT security event), modeled on
// ets_runtime's DFXHiSysEvent::SendRuntimeIncompatibleEvent (STATISTIC incompatible event).
inline void ReportEtsUtilsIncompatibleEvent(const std::string &funcName, const std::string &errorType,
                                            int32_t offset = -1, int32_t length = -1)
{
    HILOG_ERROR("EtsUtilsIncompatibleEvent: %{public}s detected %{public}s, offset: %{public}d, length: %{public}d",
                funcName.c_str(), errorType.c_str(), offset, length);
#ifdef ENABLE_ETS_UTILS_HISYSEVENT
    HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::ARKTS_TASKPOOL,
        "ARK_INCOMPATIBLE_EVENT", OHOS::HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "PID", getpid(), "TID", static_cast<int32_t>(syscall(SYS_gettid)),
        "FUNCTION_NAME", funcName, "ERROR_TYPE", errorType, "OFFSET", offset, "LENGTH", length);
#endif
}

#endif  // COMMONLIBRARY_ETS_UTILS_TOOLS_SECURITY_FAULT_REPORTER_H
