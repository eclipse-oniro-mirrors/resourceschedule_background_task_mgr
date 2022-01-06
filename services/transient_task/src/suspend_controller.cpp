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

#include "suspend_controller.h"

#include "transient_task_log.h"

using namespace std;

namespace OHOS {
namespace BackgroundTaskMgr {
void SuspendController::RequestSuspendDelay(const std::shared_ptr<KeyInfo>& key)
{
    // do delayed suspend
    BGTASK_LOGI("Add delayed suspend app uid: %{public}d, name: %{public}s",key->GetUid(), key->GetPkg().c_str());
}

void SuspendController::CancelSuspendDelay(const std::shared_ptr<KeyInfo>& key)
{
    // cancel delayed suspend
    BGTASK_LOGI("Remove delayed suspend app uid: %{public}d, name: %{public}s",key->GetUid(), key->GetPkg().c_str());
}
}
}