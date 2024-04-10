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

#include "app_state_observer.h"

#include "app_mgr_constants.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"

#include "bg_continuous_task_mgr.h"
#include "continuous_task_log.h"
#include "bg_efficiency_resources_mgr.h"

namespace OHOS {
namespace BackgroundTaskMgr {
namespace {
const std::string TASK_ON_PROCESS_DIED = "OnProcessDiedTask";
const std::string TASK_ON_ABILITY_STATE_CHANGED = "OnAbilityStateChangedTask";
}

void AppStateObserver::OnAbilityStateChanged(const AppExecFwk::AbilityStateData &abilityStateData)
{
    if (abilityStateData.abilityState != static_cast<int32_t>(AppExecFwk::AbilityState::ABILITY_STATE_TERMINATED)) {
        return;
    }
    BGTASK_LOGD("ability state changed, uid: %{public}d abilityName: %{public}s, abilityState: %{public}d, "
        "abilityId: %{public}d",
        abilityStateData.uid, abilityStateData.abilityName.c_str(), abilityStateData.abilityState,
        abilityStateData.abilityRecordId);
    int32_t uid = abilityStateData.uid;
    int32_t abilityId = abilityStateData.abilityRecordId;
    std::string abilityName = abilityStateData.abilityName;
    auto task = [uid, abilityName, abilityId]() {
        DelayedSingleton<BgContinuousTaskMgr>::GetInstance()->OnAbilityStateChanged(uid, abilityName, abilityId);
    };
    handler_->PostTask(task, TASK_ON_ABILITY_STATE_CHANGED);
}

void AppStateObserver::OnProcessDied(const AppExecFwk::ProcessData &processData)
{
    BGTASK_LOGD("process died, uid : %{public}d, pid : %{public}d", processData.uid, processData.pid);
    OnProcessDiedContinuousTask(processData);
    OnProcessDiedEfficiencyRes(processData);
}

void AppStateObserver::OnProcessDiedContinuousTask(const AppExecFwk::ProcessData &processData)
{
    auto task = [processData]() {
        DelayedSingleton<BgContinuousTaskMgr>::GetInstance()->OnProcessDied(processData.uid, processData.pid);
    };
    handler_->PostTask(task, TASK_ON_PROCESS_DIED);
}

void AppStateObserver::OnProcessDiedEfficiencyRes(const AppExecFwk::ProcessData &processData)
{
    DelayedSingleton<BgEfficiencyResourcesMgr>::GetInstance()->
        RemoveProcessRecord(processData.uid, processData.pid, processData.bundleName);
}

void AppStateObserver::OnAppStopped(const AppExecFwk::AppStateData &appStateData)
{
    if (!ValidateAppStateData(appStateData)) {
        BGTASK_LOGE("%{public}s : validate app state data failed!", __func__);
        return;
    }
    auto uid = appStateData.uid;
    auto bundleName = appStateData.bundleName;
    DelayedSingleton<BgEfficiencyResourcesMgr>::GetInstance()->RemoveAppRecord(uid, bundleName, false);
}

inline bool AppStateObserver::ValidateAppStateData(const AppExecFwk::AppStateData &appStateData)
{
    return appStateData.uid > 0 && appStateData.bundleName.size() > 0;
}

void AppStateObserver::SetEventHandler(const std::shared_ptr<AppExecFwk::EventHandler> &handler)
{
    handler_ = handler;
}
}  // namespace BackgroundTaskMgr
}  // namespace OHOS