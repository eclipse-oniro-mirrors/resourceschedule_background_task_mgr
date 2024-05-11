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

#include "background_task_mgr_service.h"

#include <functional>

#include "ability_manager_client.h"
#include "accesstoken_kit.h"
#include "bgtaskmgr_inner_errors.h"
#include "bundle_constants.h"
#include "common_event_manager.h"
#include "common_event_support.h"
#include "file_ex.h"
#include "ipc_skeleton.h"
#include "string_ex.h"

#include "bgtaskmgr_log_wrapper.h"
#include <parameters.h>

namespace OHOS {
namespace BackgroundTaskMgr {
namespace {
static constexpr int32_t NO_DUMP_PARAM_NUMS = 0;
const int32_t ENG_MODE = OHOS::system::GetIntParameter("const.debuggable", 0);
const std::string BGTASK_SERVICE_NAME = "BgtaskMgrService";
const bool REGISTER_RESULT = SystemAbility::MakeAndRegisterAbility(
    DelayedSingleton<BackgroundTaskMgrService>::GetInstance().get());
}

BackgroundTaskMgrService::BackgroundTaskMgrService()
    : SystemAbility(BACKGROUND_TASK_MANAGER_SERVICE_ID, true) {}

BackgroundTaskMgrService::~BackgroundTaskMgrService() {}

void BackgroundTaskMgrService::OnStart()
{
    if (state_ == ServiceRunningState::STATE_RUNNING) {
        BGTASK_LOGW("Service has already started.");
        return;
    }
    Init();
    AddSystemAbilityListener(APP_MGR_SERVICE_ID);
    AddSystemAbilityListener(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (!Publish(DelayedSingleton<BackgroundTaskMgrService>::GetInstance().get())) {
        BGTASK_LOGE("Service start failed!");
        return;
    }
    state_ = ServiceRunningState::STATE_RUNNING;
    BGTASK_LOGI("background task manager service start succeed!");
}

void BackgroundTaskMgrService::OnAddSystemAbility(int32_t systemAbilityId, const std::string& deviceId)
{
    DelayedSingleton<BgEfficiencyResourcesMgr>::GetInstance()->OnAddSystemAbility(systemAbilityId, deviceId);
}

void BackgroundTaskMgrService::OnRemoveSystemAbility(int32_t systemAbilityId, const std::string& deviceId)
{
    DelayedSingleton<BgEfficiencyResourcesMgr>::GetInstance()->OnRemoveSystemAbility(systemAbilityId, deviceId);
}

void BackgroundTaskMgrService::Init()
{
    runner_ = AppExecFwk::EventRunner::Create(BGTASK_SERVICE_NAME);
    DelayedSingleton<BgTransientTaskMgr>::GetInstance()->Init(runner_);
    DelayedSingleton<BgEfficiencyResourcesMgr>::GetInstance()->Init(runner_);
    BgContinuousTaskMgr::GetInstance()->Init(runner_);
}

void BackgroundTaskMgrService::OnStop()
{
    BgContinuousTaskMgr::GetInstance()->Clear();
    DelayedSingleton<BgEfficiencyResourcesMgr>::GetInstance()->Clear();
    state_ = ServiceRunningState::STATE_NOT_START;
    BGTASK_LOGI("background task manager stop");
}

bool BackgroundTaskMgrService::CheckCallingToken()
{
    Security::AccessToken::AccessTokenID tokenId = IPCSkeleton::GetCallingTokenID();
    auto tokenFlag = Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(tokenId);
    if (tokenFlag == Security::AccessToken::ATokenTypeEnum::TOKEN_NATIVE ||
        tokenFlag == Security::AccessToken::ATokenTypeEnum::TOKEN_SHELL) {
        return true;
    }
    return false;
}

ErrCode BackgroundTaskMgrService::RequestSuspendDelay(const std::u16string& reason,
    const sptr<IExpiredCallback>& callback, std::shared_ptr<DelaySuspendInfo> &delayInfo)
{
    return DelayedSingleton<BgTransientTaskMgr>::GetInstance()->RequestSuspendDelay(reason, callback, delayInfo);
}

ErrCode BackgroundTaskMgrService::CancelSuspendDelay(int32_t requestId)
{
    return DelayedSingleton<BgTransientTaskMgr>::GetInstance()->CancelSuspendDelay(requestId);
}

ErrCode BackgroundTaskMgrService::GetRemainingDelayTime(int32_t requestId, int32_t &delayTime)
{
    return DelayedSingleton<BgTransientTaskMgr>::GetInstance()->GetRemainingDelayTime(requestId, delayTime);
}

void BackgroundTaskMgrService::ForceCancelSuspendDelay(int32_t requestId)
{
    DelayedSingleton<BgTransientTaskMgr>::GetInstance()->ForceCancelSuspendDelay(requestId);
}

ErrCode BackgroundTaskMgrService::StartBackgroundRunning(const sptr<ContinuousTaskParam> &taskParam)
{
    return BgContinuousTaskMgr::GetInstance()->StartBackgroundRunning(taskParam);
}

ErrCode BackgroundTaskMgrService::UpdateBackgroundRunning(const sptr<ContinuousTaskParam> &taskParam)
{
    return BgContinuousTaskMgr::GetInstance()->UpdateBackgroundRunning(taskParam);
}

ErrCode BackgroundTaskMgrService::RequestBackgroundRunningForInner(const sptr<ContinuousTaskParamForInner> &taskParam)
{
    return BgContinuousTaskMgr::GetInstance()->RequestBackgroundRunningForInner(taskParam);
}

ErrCode BackgroundTaskMgrService::StopBackgroundRunning(const std::string &abilityName,
    const sptr<IRemoteObject> &abilityToken, int32_t abilityId)
{
    return BgContinuousTaskMgr::GetInstance()->StopBackgroundRunning(abilityName, abilityId);
}

ErrCode BackgroundTaskMgrService::GetTransientTaskApps(std::vector<std::shared_ptr<TransientTaskAppInfo>> &list)
{
    if (!CheckCallingToken()) {
        BGTASK_LOGW("GetTransientTaskApps not allowed");
        return ERR_BGTASK_PERMISSION_DENIED;
    }
    return DelayedSingleton<BgTransientTaskMgr>::GetInstance()->GetTransientTaskApps(list);
}

ErrCode BackgroundTaskMgrService::PauseTransientTaskTimeForInner(int32_t uid)
{
    return DelayedSingleton<BgTransientTaskMgr>::GetInstance()->PauseTransientTaskTimeForInner(uid);
}

ErrCode BackgroundTaskMgrService::StartTransientTaskTimeForInner(int32_t uid)
{
    return DelayedSingleton<BgTransientTaskMgr>::GetInstance()->StartTransientTaskTimeForInner(uid);
}

ErrCode BackgroundTaskMgrService::GetContinuousTaskApps(std::vector<std::shared_ptr<ContinuousTaskCallbackInfo>> &list)
{
    if (!CheckCallingToken()) {
        BGTASK_LOGW("GetContinuousTaskApps not allowed");
        return ERR_BGTASK_PERMISSION_DENIED;
    }
    return BgContinuousTaskMgr::GetInstance()->GetContinuousTaskApps(list);
}

ErrCode BackgroundTaskMgrService::SubscribeBackgroundTask(const sptr<IBackgroundTaskSubscriber>& subscriber)
{
    if (!CheckCallingToken()) {
        BGTASK_LOGW("SubscribeBackgroundTask not allowed");
        return ERR_BGTASK_PERMISSION_DENIED;
    }
    if (DelayedSingleton<BgTransientTaskMgr>::GetInstance()->SubscribeBackgroundTask(subscriber) == ERR_OK
        && DelayedSingleton<BgEfficiencyResourcesMgr>::GetInstance()->AddSubscriber(subscriber) == ERR_OK
        && BgContinuousTaskMgr::GetInstance()->AddSubscriber(subscriber) == ERR_OK) {
        BGTASK_LOGD("all bgtask subscribe success");
        return ERR_OK;
    } else {
        BGTASK_LOGW("subscribe background task failed");
    }
    return ERR_BGTASK_SYS_NOT_READY;
}

ErrCode BackgroundTaskMgrService::UnsubscribeBackgroundTask(const sptr<IBackgroundTaskSubscriber>& subscriber)
{
    if (!CheckCallingToken()) {
        BGTASK_LOGW("UnsubscribeBackgroundTask not allowed");
        return ERR_BGTASK_PERMISSION_DENIED;
    }
    if (DelayedSingleton<BgTransientTaskMgr>::GetInstance()->UnsubscribeBackgroundTask(subscriber) == ERR_OK
        && DelayedSingleton<BgEfficiencyResourcesMgr>::GetInstance()->RemoveSubscriber(subscriber) == ERR_OK
        && BgContinuousTaskMgr::GetInstance()->RemoveSubscriber(subscriber) == ERR_OK) {
        return ERR_OK;
    }
    return ERR_BGTASK_SYS_NOT_READY;
}

void BackgroundTaskMgrService::HandleRequestExpired(const int32_t requestId)
{
    DelayedSingleton<BgTransientTaskMgr>::GetInstance()->HandleRequestExpired(requestId);
}

void BackgroundTaskMgrService::HandleExpiredCallbackDeath(const wptr<IRemoteObject>& remote)
{
    DelayedSingleton<BgTransientTaskMgr>::GetInstance()->HandleExpiredCallbackDeath(remote);
}

void BackgroundTaskMgrService::HandleSubscriberDeath(const wptr<IRemoteObject>& remote)
{
    DelayedSingleton<BgTransientTaskMgr>::GetInstance()->HandleSubscriberDeath(remote);
}

ErrCode BackgroundTaskMgrService::ApplyEfficiencyResources(const sptr<EfficiencyResourceInfo> &resourceInfo)
{
    return DelayedSingleton<BgEfficiencyResourcesMgr>::GetInstance()->ApplyEfficiencyResources(resourceInfo);
}

ErrCode BackgroundTaskMgrService::ResetAllEfficiencyResources()
{
    return DelayedSingleton<BgEfficiencyResourcesMgr>::GetInstance()->ResetAllEfficiencyResources();
}

ErrCode BackgroundTaskMgrService::GetEfficiencyResourcesInfos(
    std::vector<std::shared_ptr<ResourceCallbackInfo>> &appList,
    std::vector<std::shared_ptr<ResourceCallbackInfo>> &procList)
{
    if (!CheckCallingToken()) {
        BGTASK_LOGW("GetEfficiencyResourcesInfos not allowed");
        return ERR_BGTASK_PERMISSION_DENIED;
    }
    return DelayedSingleton<BgEfficiencyResourcesMgr>::GetInstance()->GetEfficiencyResourcesInfos(appList, procList);
}

ErrCode BackgroundTaskMgrService::StopContinuousTask(int32_t uid, int32_t pid, uint32_t taskType,
    const std::string &key)
{
    if (!CheckCallingToken()) {
        BGTASK_LOGW("StopContinuousTask not allowed");
        return ERR_BGTASK_PERMISSION_DENIED;
    }
    BgContinuousTaskMgr::GetInstance()->StopContinuousTask(uid, pid, taskType, key);
    return ERR_OK;
}

bool BackgroundTaskMgrService::AllowDump()
{
    if (ENG_MODE == 0) {
        BGTASK_LOGE("Not eng mode");
        return false;
    }
    Security::AccessToken::AccessTokenID tokenId = IPCSkeleton::GetFirstTokenID();
    int32_t ret = Security::AccessToken::AccessTokenKit::VerifyAccessToken(tokenId, "ohos.permission.DUMP");
    if (ret != Security::AccessToken::PermissionState::PERMISSION_GRANTED) {
        BGTASK_LOGE("CheckPermission failed");
        return false;
    }
    return true;
}

int32_t BackgroundTaskMgrService::Dump(int32_t fd, const std::vector<std::u16string> &args)
{
    if (!AllowDump()) {
        return ERR_BGTASK_PERMISSION_DENIED;
    }
    std::vector<std::string> argsInStr;
    std::transform(args.begin(), args.end(), std::back_inserter(argsInStr),
        [](const std::u16string &arg) {
        return Str16ToStr8(arg);
    });
    std::string result;

    int32_t ret = ERR_OK;

    if (argsInStr.size() == NO_DUMP_PARAM_NUMS) {
        DumpUsage(result);
    } else {
        std::vector<std::string> infos;
        if (argsInStr[0] == "-h") {
            DumpUsage(result);
        } else if (argsInStr[0] == "-T") {
            ret = DelayedSingleton<BgTransientTaskMgr>::GetInstance()->ShellDump(argsInStr, infos);
        } else if (argsInStr[0] == "-C") {
            ret = BgContinuousTaskMgr::GetInstance()->ShellDump(argsInStr, infos);
        } else if (argsInStr[0] == "-E") {
            ret = DelayedSingleton<BgEfficiencyResourcesMgr>::GetInstance()->ShellDump(argsInStr, infos);
        } else {
            infos.emplace_back("Error params.\n");
            ret = ERR_BGTASK_INVALID_PARAM;
        }
        for (auto info : infos) {
            result.append(info);
        }
    }

    if (!SaveStringToFd(fd, result)) {
        BGTASK_LOGE("BackgroundTaskMgrService dump save string to fd failed!");
        ret = ERR_BGTASK_METHOD_CALLED_FAILED;
    }
    return ret;
}

void BackgroundTaskMgrService::DumpUsage(std::string &result)
{
    std::string dumpHelpMsg =
    "usage: bgtask dump [<options>]\n"
    "options list:\n"
    "    -h                                   help menu\n"
    "    -T                                   transient task commands:\n"
    "        BATTARY_LOW                          battary low mode\n"
    "        BATTARY_OKAY                         battary okay mode\n"
    "        DUMP_CANCEL                          cancel dump mode\n"
    "        All                                  list all request\n"
    "    -C                                   continuous task commands:\n"
    "        --all                                list all running continuous task infos\n"
    "        --cancel_all                         cancel all running continuous task\n"
    "        --cancel {continuous task key}       cancel one task by specifying task key\n"
    "    -E                                   efficiency resources commands;\n"
    "        --all                                list all efficiency resource aplications\n"
    "        --reset_all                          reset all efficiency resource aplications\n"
    "        --resetapp {uid} {resources}          reset one application of uid by specifying \n"
    "        --resetproc {pid} {resources}         reset one application of pid by specifying \n";

    result.append(dumpHelpMsg);
}  // namespace
}  // namespace BackgroundTaskMgr
}  // namespace OHOS
