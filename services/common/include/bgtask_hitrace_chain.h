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

#ifndef FOUNDATION_RESOURCESCHEDULE_BACKGROUND_TASK_MGR_SERVICES_COMMON_BGTASK_HITRACE_CHAIN_H
#define FOUNDATION_RESOURCESCHEDULE_BACKGROUND_TASK_MGR_SERVICES_COMMON_BGTASK_HITRACE_CHAIN_H

#include "hitracechainc.h"

namespace OHOS {
namespace BackgroundTaskMgr {
class BgTaskHiTraceChain {
public:
    BgTaskHiTraceChain(const bool isClearId, const char *name);
    explicit BgTaskHiTraceChain(const char *name, const int32_t flags = -1);
    ~BgTaskHiTraceChain();

private:
    bool isBegin_ = false;
    HiTraceIdStruct traceId_ = {0};
};
}  // namespace BackgroundTaskMgr
}  // namespace OHOS
#endif // FOUNDATION_RESOURCESCHEDULE_BACKGROUND_TASK_MGR_SERVICES_COMMON_BGTASK_HITRACE_CHAIN_H