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

#ifndef PLATFORM_PROCESS_HELPER_H
#define PLATFORM_PROCESS_HELPER_H

#include <cstdlib>
#include <unistd.h>
#include "utils/log.h"

#define SYS_INFO_FAILED (-1)

namespace Commonlibrary::Platform {
void ProcessExit(int signal);
int GetSysConfig(int number);
double GetSysTimer();
int GetThreadId();
int GetThreadPRY(int tid);
} // namespace Commonlibrary::Platform
#endif // PLATFORM_PROCESS_HELPER_H