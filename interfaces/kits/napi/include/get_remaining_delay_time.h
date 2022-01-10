/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef FOUNDATION_RESOURCESCHEDULE_BACKGROUNDTASKMANAGER_INTERFACES_KITS_NAPI_INCLUDE_GET_REMAINING_DELAY_TIME_H
#define FOUNDATION_RESOURCESCHEDULE_BACKGROUNDTASKMANAGER_INTERFACES_KITS_NAPI_INCLUDE_GET_REMAINING_DELAY_TIME_H

#include <string>

#include "common.h"

namespace OHOS {
namespace BackgroundTaskMgr {
    napi_value GetRemainingDelayTime(napi_env env, napi_callback_info info);
} // namespace BackgroundTaskMgr
} // namespace OHOS
#endif