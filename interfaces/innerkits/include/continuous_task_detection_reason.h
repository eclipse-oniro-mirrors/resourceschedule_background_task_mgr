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

#ifndef FOUNDATION_RESOURCESCHEDULE_BACKGROUND_TASK_MGR_INTERFACES_INNERKITS_INCLUDE_CONTINUOUSTASK_DETECTION_REASON_H
#define FOUNDATION_RESOURCESCHEDULE_BACKGROUND_TASK_MGR_INTERFACES_INNERKITS_INCLUDE_CONTINUOUSTASK_DETECTION_REASON_H

namespace OHOS {
namespace BackgroundTaskMgr {
class ContinuousTaskDetectionReason {
public:
    virtual ~ContinuousTaskDetectionReason() = default;
    enum Type : uint32_t {
        SUCCESS = 0,
        USB_NOT_CONNECTION = 1,
        USB_OCCUPIED = 2,
        END,
    };
};
}  // namespace BackgroundTaskMgr
}  // namespace OHOS
#endif