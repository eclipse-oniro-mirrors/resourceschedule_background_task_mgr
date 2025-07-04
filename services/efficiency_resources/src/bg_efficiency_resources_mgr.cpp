/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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

#include "bg_efficiency_resources_mgr.h"
#include "background_task_mgr_service.h"

#include <set>
#include <algorithm>
#include <vector>
#include <dlfcn.h>

#include "event_runner.h"
#include "system_ability_definition.h"
#include "iservice_registry.h"
#include "bgtask_hitrace_chain.h"
#include "bgtaskmgr_inner_errors.h"

#include "resource_type.h"
#include "time_provider.h"
#include "bundle_manager_helper.h"
#include "efficiency_resource_log.h"
#include "tokenid_kit.h"
#include "extension_ability_info.h"
#include "hisysevent.h"
#include "hitrace_meter.h"

namespace OHOS {
namespace BackgroundTaskMgr {
namespace {
    const std::string DUMP_PARAM_LIST_ALL = "--all";
    const std::string DUMP_PARAM_RESET_ALL = "--reset_all";
    const std::string DUMP_PARAM_RESET_APP = "--resetapp";
    const std::string DUMP_PARAM_RESET_PROC = "--resetproc";
    const std::string DUMP_PARAM_SET_CPU_QUOTA = "--setquota";
    const std::string DUMP_PARAM_RESET_CPU_QUOTA = "--resetquota";
    const std::string DUMP_PARAM_GET_CPU_QUOTA = "--getquota";
    const int32_t MAX_DUMP_PARAM_NUMS = 4;
    const uint32_t APP_MGR_READY = 1;
    const uint32_t BUNDLE_MGR_READY = 2;
    const uint32_t ALL_DEPENDS_READY = 3;
    const uint32_t FREEZE_ALL_RESOURCES = 0;
    const uint32_t MAX_RESOURCES_TYPE_NUM = ResourceTypeName.size();
    const uint32_t MAX_RESOURCE_MASK = (1 << ResourceTypeName.size()) - 1;
    const char *RESOURCE_QUOTA_MANAGER_LIB = "libresource_quota_manager.z.so";
}
BgEfficiencyResourcesMgr::BgEfficiencyResourcesMgr() {}

BgEfficiencyResourcesMgr::~BgEfficiencyResourcesMgr() {}

void BgEfficiencyResourcesMgr::LoadResourceQuotaMgrLib()
{
    resourceQuotaMgrHandle_ = dlopen(RESOURCE_QUOTA_MANAGER_LIB, RTLD_NOW);
    if (!resourceQuotaMgrHandle_) {
        BGTASK_LOGE("Not find resource_quota_manager lib.");
    }

    BGTASK_LOGI("Load resource_quota_manager lib success.");
}

bool BgEfficiencyResourcesMgr::Init(const std::shared_ptr<AppExecFwk::EventRunner>& runner)
{
    BGTASK_LOGI("BgEfficiencyResourcesMgr service init start");
    subscriberMgr_ = DelayedSingleton<ResourcesSubscriberMgr>::GetInstance();
    if (runner == nullptr) {
        BGTASK_LOGE("efficiency resources mgr runner create failed!");
        return false;
    }
    handler_ = std::make_shared<AppExecFwk::EventHandler>(runner);
    if (!handler_) {
        BGTASK_LOGE("efficiency resources mgr handler create failed!");
        return false;
    }
    BGTASK_LOGI("efficiency resources mgr finish Init");

    LoadResourceQuotaMgrLib();
    return true;
}

void BgEfficiencyResourcesMgr::InitNecessaryState()
{
    if (isSysReady_.load()) {
        BGTASK_LOGW("Efficiency resources manager is ready");
        return;
    }
    HandlePersistenceData();
    BGTASK_LOGD("app resource record size: %{public}d, process  resource record size:  %{public}d!",
        static_cast<int32_t>(appResourceApplyMap_.size()), static_cast<int32_t>(procResourceApplyMap_.size()));
    isSysReady_.store(true);
    DelayedSingleton<BackgroundTaskMgrService>::GetInstance()->SetReady(
        ServiceReadyState::EFFICIENCY_RESOURCES_SERVICE_READY);
    BGTASK_LOGI("SetReady EFFICIENCY_RESOURCES_SERVICE_READY");
}

void BgEfficiencyResourcesMgr::OnAddSystemAbility(int32_t systemAbilityId, const std::string& deviceId)
{
    BGTASK_LOGI("add system ability, systemAbilityId : %{public}d", systemAbilityId);
    std::lock_guard<std::mutex> lock(sysAbilityLock_);
    switch (systemAbilityId) {
        case APP_MGR_SERVICE_ID:
            BGTASK_LOGI("app mgr service is ready!");
            dependsReady_ |= APP_MGR_READY;
            break;
        case BUNDLE_MGR_SERVICE_SYS_ABILITY_ID:
            BGTASK_LOGI("bundle mgr service is ready!");
            dependsReady_ |= BUNDLE_MGR_READY;
            break;
        default:
            break;
    }
    if (dependsReady_ == ALL_DEPENDS_READY) {
        BGTASK_LOGI("necessary system service has been satisfied!");
        auto task = [weak = weak_from_this()]() {
            auto self = weak.lock();
            if (!self) {
                BGTASK_LOGE("weak.lock return null");
                return;
            }
            self->InitNecessaryState();
        };
        handler_->PostSyncTask(task);
    }
}

void BgEfficiencyResourcesMgr::OnRemoveSystemAbility(int32_t systemAbilityId, const std::string& deviceId)
{
    BGTASK_LOGI("remove system ability, systemAbilityId : %{public}d", systemAbilityId);
    std::lock_guard<std::mutex> lock(sysAbilityLock_);
    switch (systemAbilityId) {
        case APP_MGR_SERVICE_ID:
            BGTASK_LOGI("app mgr service is removed!");
            dependsReady_ &= (~APP_MGR_READY);
            break;
        case BUNDLE_MGR_SERVICE_SYS_ABILITY_ID:
            BGTASK_LOGI("bundle mgr service is removed!");
            dependsReady_ &= (~BUNDLE_MGR_READY);
            break;
        default:
            break;
    }
    if (dependsReady_ != ALL_DEPENDS_READY) {
        BGTASK_LOGI("necessary system service has been unsatisfied");
        isSysReady_.store(false);
    }
}

void BgEfficiencyResourcesMgr::HandlePersistenceData()
{
    BGTASK_LOGD("ResourceRecordStorage service restart, restore data");

    if (appMgrClient_ == nullptr) {
        appMgrClient_ = std::make_unique<AppExecFwk::AppMgrClient>();
        if (!appMgrClient_) {
            BGTASK_LOGE("ResourceRecordStorage connect to app mgr service failed");
            DelayedSingleton<DataStorageHelper>::GetInstance()->RefreshResourceRecord(
                appResourceApplyMap_, procResourceApplyMap_);
            return;
        }
    }
    std::vector<AppExecFwk::RunningProcessInfo> allAppProcessInfos;
    if (appMgrClient_->GetAllRunningProcesses(allAppProcessInfos) != ERR_OK) {
        BGTASK_LOGE("get all running processes failed");
        return;
    }
    BGTASK_LOGI("start to recovery delayed task of apps and processes");
    DelayedSingleton<DataStorageHelper>::GetInstance()->RestoreResourceRecord(
        appResourceApplyMap_, procResourceApplyMap_);
    CheckPersistenceData(allAppProcessInfos);
    RecoverDelayedTask(true, procResourceApplyMap_);
    RecoverDelayedTask(false, appResourceApplyMap_);
    DelayedSingleton<DataStorageHelper>::GetInstance()->RefreshResourceRecord(
        appResourceApplyMap_, procResourceApplyMap_);
}

void BgEfficiencyResourcesMgr::EraseRecordIf(ResourceRecordMap &infoMap,
    const std::function<bool(ResourceRecordPair)> &fun)
{
    for (auto iter = infoMap.begin(); iter != infoMap.end();) {
        if (fun(*iter)) {
            iter = infoMap.erase(iter);
        } else {
            iter++;
        }
    }
}

void BgEfficiencyResourcesMgr::CheckPersistenceData(const std::vector<AppExecFwk::RunningProcessInfo> &allProcesses)
{
    BGTASK_LOGI("efficiency resources check existing uid and pid");
    std::set<int32_t> runningUid;
    std::set<int32_t> runningPid;
    std::for_each(allProcesses.begin(), allProcesses.end(), [&runningUid, &runningPid](const auto &iter) {
        runningUid.emplace(iter.uid_);
        runningPid.emplace(iter.pid_);
    });
    auto removeUid = [&runningUid](const auto &iter) {
        std::shared_ptr<ResourceApplicationRecord> record = iter.second;
        if ((record->GetResourceNumber() & ResourceType::WORK_SCHEDULER) != 0 ||
            (record->GetResourceNumber() & ResourceType::TIMER) != 0) {
                return false;
            }
        return runningUid.find(iter.first) == runningUid.end();
    };
    EraseRecordIf(appResourceApplyMap_, removeUid);
    auto removePid = [&runningPid](const auto &iter)  { return runningPid.find(iter.first) == runningPid.end(); };
    EraseRecordIf(procResourceApplyMap_, removePid);
}

__attribute__((no_sanitize("cfi"))) void BgEfficiencyResourcesMgr::RecoverDelayedTask(bool isProcess,
    ResourceRecordMap& infoMap)
{
    BGTASK_LOGD("start to recovery delayed task");
    const auto &mgr = shared_from_this();
    for (auto iter = infoMap.begin(); iter != infoMap.end(); iter ++) {
        auto &resourceList = iter->second->resourceUnitList_;
        int32_t mapKey = iter->first;
        for (auto resourceIter = resourceList.begin(); resourceIter != resourceList.end(); resourceIter++) {
            if (resourceIter->isPersist_) {
                continue;
            }
            auto task = [mgr, mapKey, isProcess] () {
                mgr->ResetTimeOutResource(mapKey, isProcess);
            };
            int32_t timeOut = static_cast<int32_t>(resourceIter->endTime_ - TimeProvider::GetCurrentTime());
            handler_->PostTask(task, std::max(0, timeOut));
        }
    }
}

ErrCode BgEfficiencyResourcesMgr::RemoveAppRecord(int32_t uid, const std::string &bundleName, bool resetAll)
{
    if (!isSysReady_.load()) {
        BGTASK_LOGW("Efficiency resources manager is not ready, RemoveAppRecord failed");
        return ERR_BGTASK_SYS_NOT_READY;
    }
    BGTASK_LOGD("app died, uid: %{public}d, bundleName: %{public}s", uid, bundleName.c_str());
    handler_->PostTask([this, uid, bundleName, resetAll]() {
        int resourceNumber = resetAll ? MAX_RESOURCE_MASK : (MAX_RESOURCE_MASK ^ ResourceType::WORK_SCHEDULER ^
            ResourceType::TIMER);
        std::shared_ptr<ResourceCallbackInfo> callbackInfo = std::make_shared<ResourceCallbackInfo>(uid,
            0, resourceNumber, bundleName);
        this->ResetEfficiencyResourcesInner(callbackInfo, false, CancelReason::APP_DIED);
    });
    return ERR_OK;
}

ErrCode BgEfficiencyResourcesMgr::RemoveProcessRecord(int32_t uid, int32_t pid, const std::string &bundleName)
{
    if (!isSysReady_.load()) {
        BGTASK_LOGW("Efficiency resources manager is not ready, remove process record failed");
        return ERR_BGTASK_SYS_NOT_READY;
    }
    BGTASK_LOGD("process died, uid: %{public}d, pid: %{public}d, bundleName: %{public}s",
        uid, pid, bundleName.c_str());
    handler_->PostTask([this, uid, pid, bundleName]() {
        std::shared_ptr<ResourceCallbackInfo> callbackInfo = std::make_shared<ResourceCallbackInfo>(uid,
            pid, MAX_RESOURCE_MASK ^ ResourceType::WORK_SCHEDULER ^ ResourceType::TIMER, bundleName);
        this->ResetEfficiencyResourcesInner(callbackInfo, true, CancelReason::PROCESS_DIED);
    });
    return ERR_OK;
}

void BgEfficiencyResourcesMgr::Clear()
{
}

bool CheckResourceInfo(const sptr<EfficiencyResourceInfo> &resourceInfo)
{
    if (!resourceInfo) {
        BGTASK_LOGE("apply efficiency resource request params is null!");
        return false;
    }
    if (resourceInfo->GetResourceNumber() == 0 || resourceInfo->GetResourceNumber() > MAX_RESOURCE_MASK
        || (resourceInfo->IsApply() && !resourceInfo->IsPersist() && resourceInfo->GetTimeOut() == 0)) {
        BGTASK_LOGE("efficiency resources params invalid!");
        return false;
    }
    return true;
}

bool BgEfficiencyResourcesMgr::IsServiceExtensionType(const pid_t pid)
{
    if (!appMgrClient_) {
        BGTASK_LOGE("ApplyEfficiencyResources connect to app mgr service failed");
        return false;
    }

    int32_t proId = static_cast<int32_t>(pid);
    std::vector<AppExecFwk::RunningProcessInfo> allRunningProcessInfos;
    if (appMgrClient_->GetAllRunningProcesses(allRunningProcessInfos) != ERR_OK) {
        BGTASK_LOGE("get all running processes failed");
        return false;
    }
    for (const auto &info : allRunningProcessInfos) {
        if (info.pid_ == proId && info.extensionType_ == OHOS::AppExecFwk::ExtensionAbilityType::SERVICE) {
            return true;
        }
    }
    return false;
}

bool BgEfficiencyResourcesMgr::CheckOrUpdateCpuApplyQuota(int32_t uid, const std::string &bundleName,
    const sptr<EfficiencyResourceInfo> &resourceInfo)
{
    if (resourceInfo->IsProcess()) {
        BGTASK_LOGD("Not app resource, return.");
        return true;
    }

    if ((resourceInfo->GetResourceNumber() & ResourceType::CPU) == 0) {
        BGTASK_LOGD("App resource but not include CPU type, return.");
        return true;
    }

    if (resourceInfo->IsApply() && resourceInfo->IsPersist()) {
        BGTASK_LOGD("Apply but persist, return.");
        return true;
    }

    if (resourceQuotaMgrHandle_ == nullptr) {
        BGTASK_LOGD("ResourceQuotaMgrHandle_ is nullptr.");
        return true;
    }

    BGTASK_LOGD("CheckOrUpdateCpuApplyQuota start, uid: %{public}d.", uid);

    using HandleQuotaFunc = bool (*)(int32_t, const std::string &, const sptr<EfficiencyResourceInfo> &);
    auto handleQuotaFunc = reinterpret_cast<HandleQuotaFunc>(
        dlsym(resourceQuotaMgrHandle_, "HandleCpuApplyQuotaProcess"));
    if (!handleQuotaFunc) {
        BGTASK_LOGE("Get handleQuotaFunc failed.");
        return true;
    }

    return handleQuotaFunc(uid, bundleName, resourceInfo);
}

ErrCode BgEfficiencyResourcesMgr::ApplyEfficiencyResources(
    const sptr<EfficiencyResourceInfo> &resourceInfo)
{
    BgTaskHiTraceChain traceChain(__func__);
    HitraceScoped traceScoped(HITRACE_TAG_OHOS,
        "BackgroundTaskManager::EfficiencyResource::Service::ApplyEfficiencyResources");

    BGTASK_LOGD("start bgtaskefficiency");
    if (!isSysReady_.load()) {
        BGTASK_LOGW("Efficiency resources manager is not ready");
        return ERR_BGTASK_SYS_NOT_READY;
    }

    if (!CheckResourceInfo(resourceInfo)) {
        return ERR_BGTASK_RESOURCES_EXCEEDS_MAX;
    }

    auto uid = IPCSkeleton::GetCallingUid();
    auto pid = IPCSkeleton::GetCallingPid();
    std::string bundleName = "";
    if (!IsCallingInfoLegal(uid, pid, bundleName)) {
        BGTASK_LOGI("apply efficiency resources failed, calling info is illegal");
        return ERR_BGTASK_INVALID_PID_OR_UID;
    }

    uint64_t tokenId = IPCSkeleton::GetCallingFullTokenID();
    if (!BundleManagerHelper::GetInstance()->IsSystemApp(tokenId) && !IsServiceExtensionType(pid)) {
        BGTASK_LOGE("apply efficiency resources failed, %{public}s is not system app and service extension type",
            bundleName.c_str());
        return ERR_BGTASK_NOT_SYSTEM_APP;
    }

    auto exemptedResourceType = GetExemptedResourceType(resourceInfo->GetResourceNumber(), uid, bundleName);
    resourceInfo->SetResourceNumber(exemptedResourceType);
    if (exemptedResourceType == 0) {
        BGTASK_LOGE("apply efficiency resources failed, no permitted resource type");
        return ERR_BGTASK_PERMISSION_DENIED;
    }

    if (!CheckOrUpdateCpuApplyQuota(uid, bundleName, resourceInfo)) {
        return ERR_BGTASK_PERMISSION_DENIED;
    }

    ApplyResourceForPkgAndProc(uid, pid, bundleName, resourceInfo);
    return ERR_OK;
}

void BgEfficiencyResourcesMgr::ApplyResourceForPkgAndProc(int32_t uid, int32_t pid, const std::string &bundleName,
    const sptr<EfficiencyResourceInfo> &resourceInfo)
{
    if (!resourceInfo->IsProcess()) {
        SendResourceApplyTask(uid, pid, bundleName, resourceInfo);
        return;
    }
    // Only cpu can apply for process
    if ((resourceInfo->GetResourceNumber() & ResourceType::CPU) !=0) {
        sptr<EfficiencyResourceInfo> procResourceInfo = new (std::nothrow) EfficiencyResourceInfo(*resourceInfo);
        if (procResourceInfo == nullptr) {
            return;
        }
        procResourceInfo->SetResourceNumber(ResourceType::CPU);
        SendResourceApplyTask(uid, pid, bundleName, procResourceInfo);
    }
    int resourceNumber = resourceInfo->GetResourceNumber() & (~ResourceType::CPU);
    if (resourceNumber != 0) {
        sptr<EfficiencyResourceInfo> appResourceInfo = new (std::nothrow) EfficiencyResourceInfo(*resourceInfo);
        if (appResourceInfo == nullptr) {
            return;
        }
        appResourceInfo->SetResourceNumber(resourceNumber);
        appResourceInfo->SetProcess(false);
        SendResourceApplyTask(uid, pid, bundleName, appResourceInfo);
    }
}

void BgEfficiencyResourcesMgr::SendResourceApplyTask(int32_t uid, int32_t pid, const std::string &bundleName,
    const sptr<EfficiencyResourceInfo> &resourceInfo)
{
    std::shared_ptr<ResourceCallbackInfo> callbackInfo = std::make_shared<ResourceCallbackInfo>(uid,
        pid, resourceInfo->GetResourceNumber(), bundleName);
    if (resourceInfo->IsApply()) {
        handler_->PostTask([this, callbackInfo, resourceInfo]() {
            this->ApplyEfficiencyResourcesInner(callbackInfo, resourceInfo);
        });
    } else {
        handler_->PostTask([this, callbackInfo, resourceInfo]() {
            this->ResetEfficiencyResourcesInner(callbackInfo, resourceInfo->IsProcess(),
                CancelReason::APPLY_INTERFACE);
        });
    }
}

void BgEfficiencyResourcesMgr::ApplyEfficiencyResourcesInner(std::shared_ptr<ResourceCallbackInfo>
    callbackInfo, const sptr<EfficiencyResourceInfo> &resourceInfo)
{
    BGTASK_LOGI("apply efficiency resources, uid:%{public}d, pid %{public}d, resource number: %{public}u,"\
        "isPersist: %{public}d, timeOut: %{public}u, isProcess: %{public}d", callbackInfo->GetUid(),
        callbackInfo->GetPid(), resourceInfo->GetResourceNumber(), resourceInfo->IsPersist(),
        resourceInfo->GetTimeOut(), resourceInfo->IsProcess());
    int32_t mapKey = resourceInfo->IsProcess() ? callbackInfo->GetPid() : callbackInfo->GetUid();
    auto &infoMap = resourceInfo->IsProcess() ? procResourceApplyMap_ : appResourceApplyMap_;
    uint32_t preResourceNumber = 0;
    auto iter = infoMap.find(mapKey);
    if (iter == infoMap.end()) {
        infoMap.emplace(mapKey, std::make_shared<ResourceApplicationRecord>(callbackInfo->GetUid(),
            callbackInfo->GetPid(), callbackInfo->GetResourceNumber(), callbackInfo->GetBundleName()));
        iter = infoMap.find(mapKey);
    } else {
        preResourceNumber = iter->second->resourceNumber_;
        iter->second->resourceNumber_ |= callbackInfo->GetResourceNumber();
    }
    BGTASK_LOGD("start to update resources end time");
    UpdateResourcesEndtime(callbackInfo, iter->second, resourceInfo);
    ReportHisysEvent(EfficiencyResourceEventTriggerType::EFFICIENCY_RESOURCE_APPLY, resourceInfo, callbackInfo,
        EfficiencyResourcesEventType::APP_RESOURCE_APPLY);
    uint32_t diffResourceNumber = iter->second->resourceNumber_ ^ (preResourceNumber & iter->second->resourceNumber_);
    if (diffResourceNumber == 0) {
        BGTASK_LOGD("after update end time, diff between resourcesNumbers is zero");
        DelayedSingleton<DataStorageHelper>::GetInstance()->RefreshResourceRecord(
            appResourceApplyMap_, procResourceApplyMap_);
        return;
    }

    callbackInfo->SetResourceNumber(diffResourceNumber);
    BGTASK_LOGD("after update end time, callbackInfo resource number is %{public}u,"\
        " uid: %{public}d, bundle name: %{public}s", callbackInfo->GetResourceNumber(),
        callbackInfo->GetUid(), callbackInfo->GetBundleName().c_str());
    if (resourceInfo->IsProcess()) {
        subscriberMgr_->OnResourceChanged(callbackInfo, EfficiencyResourcesEventType::RESOURCE_APPLY);
    } else {
        subscriberMgr_->OnResourceChanged(callbackInfo, EfficiencyResourcesEventType::APP_RESOURCE_APPLY);
    }
    DelayedSingleton<DataStorageHelper>::GetInstance()->RefreshResourceRecord(
        appResourceApplyMap_, procResourceApplyMap_);
}

__attribute__((no_sanitize("cfi"))) void BgEfficiencyResourcesMgr::UpdateResourcesEndtime(
    const std::shared_ptr<ResourceCallbackInfo> &callbackInfo, std::shared_ptr<ResourceApplicationRecord> &record,
    const sptr<EfficiencyResourceInfo> &resourceInfo)
{
    for (uint32_t resourceIndex = 0; resourceIndex < MAX_RESOURCES_TYPE_NUM; ++resourceIndex) {
        if ((callbackInfo->GetResourceNumber() & (1 << resourceIndex)) == 0) {
            continue;
        }
        auto task = [resourceIndex](const auto &it) {
            return it.resourceIndex_ == resourceIndex;
        };
        auto resourceUnitIter = std::find_if(record->resourceUnitList_.begin(),
            record->resourceUnitList_.end(), task);
        int64_t endtime = TimeProvider::GetCurrentTime() + static_cast<int64_t>(resourceInfo->GetTimeOut());
        int64_t timeOut = static_cast<int64_t>(resourceInfo->GetTimeOut());
        if (resourceUnitIter == record->resourceUnitList_.end()) {
            if (resourceInfo->IsPersist()) {
                endtime = 0;
            }
            record->resourceUnitList_.emplace_back(PersistTime {resourceIndex, resourceInfo->IsPersist(),
                endtime, resourceInfo->GetReason(), timeOut});
        } else {
            resourceUnitIter->reason_ = resourceInfo->GetReason();
            resourceUnitIter->isPersist_ = resourceUnitIter->isPersist_ || resourceInfo->IsPersist();
            if (resourceUnitIter->isPersist_) {
                resourceUnitIter->endTime_ = 0;
            } else {
                resourceUnitIter->endTime_ = std::max(resourceUnitIter->endTime_,
                    endtime);
            }
        }
    }
    BGTASK_LOGD("update end time of resource");
    if (resourceInfo->IsPersist()) {
        return;
    }
    const bool isProcess = resourceInfo->IsProcess();
    int32_t mapKey = isProcess ? callbackInfo->GetPid() : callbackInfo->GetUid();
    const auto& mgr = shared_from_this();
    auto task = [mgr, mapKey, isProcess] () {
        mgr->ResetTimeOutResource(mapKey, isProcess);
    };
    handler_->PostTask(task, resourceInfo->GetTimeOut());
}

void BgEfficiencyResourcesMgr::UpdateQuotaIfCpuReset(
    EfficiencyResourcesEventType type, int32_t uid, uint32_t resourceNumber)
{
    if (type != EfficiencyResourcesEventType::APP_RESOURCE_RESET) {
        BGTASK_LOGD("Not app resource reset, return.");
        return;
    }

    if ((resourceNumber & ResourceType::CPU) == 0) {
        BGTASK_LOGD("Not CPU resource reset, return.");
        return;
    }

    if (resourceQuotaMgrHandle_ == nullptr) {
        BGTASK_LOGD("ResourceQuotaMgrHandle_ is nullptr.");
        return;
    }

    using UpdateQuotaFunc = void (*)(int32_t);
    auto updateQuotaFunc = reinterpret_cast<UpdateQuotaFunc>(
        dlsym(resourceQuotaMgrHandle_, "UpdateCpuApplyQuotaProcess"));
    if (!updateQuotaFunc) {
        BGTASK_LOGE("Get updateQuotaFunc failed.");
        return;
    }

    updateQuotaFunc(uid);
    BGTASK_LOGD("Time's out, update cpu resource quota, uid: %{public}d.", uid);
}

void BgEfficiencyResourcesMgr::ResetTimeOutResource(int32_t mapKey, bool isProcess)
{
    BGTASK_LOGD("ResetTimeOutResource reset efficiency rsources, mapkey: %{public}d",
        mapKey);
    auto &infoMap = isProcess ? procResourceApplyMap_ : appResourceApplyMap_;
    auto type = isProcess ? EfficiencyResourcesEventType::RESOURCE_RESET :
        EfficiencyResourcesEventType::APP_RESOURCE_RESET;
    auto iter = infoMap.find(mapKey);
    if (iter == infoMap.end()) {
        BGTASK_LOGI("efficiency resource does not exist");
        return;
    }
    auto &resourceRecord = iter->second;
    uint32_t eraseBit = 0;
    for (auto recordIter = resourceRecord->resourceUnitList_.begin();
        recordIter != resourceRecord->resourceUnitList_.end(); ++recordIter) {
        if (recordIter->isPersist_) {
            continue;
        }
        auto endTime = recordIter->endTime_;
        if (TimeProvider::GetCurrentTime() >= endTime) {
            eraseBit |= 1 << recordIter->resourceIndex_;
        }
    }
    BGTASK_LOGD("ResetTimeOutResource eraseBit: %{public}u, resourceNumber: %{public}u, result: %{public}u",
        eraseBit, resourceRecord->resourceNumber_, resourceRecord->resourceNumber_ ^ eraseBit);
    if (eraseBit == 0) {
        BGTASK_LOGD("try to reset time out resources, but find nothing to reset");
        return;
    }
    resourceRecord->resourceNumber_ ^= eraseBit;
    RemoveListRecord(resourceRecord->resourceUnitList_, eraseBit);
    auto callbackInfo = std::make_shared<ResourceCallbackInfo>(resourceRecord->uid_, resourceRecord->pid_, eraseBit,
        resourceRecord->bundleName_);

    // update quota after time reset if CPU resource apply
    UpdateQuotaIfCpuReset(type, callbackInfo->GetUid(), callbackInfo->GetResourceNumber());

    BGTASK_LOGI("after reset time out resources, callbackInfo resource number is %{public}u,"\
        " uid: %{public}d, bundle name: %{public}s", callbackInfo->GetResourceNumber(),
        callbackInfo->GetUid(), callbackInfo->GetBundleName().c_str());
    EfficiencyResourcesEventType processType = EfficiencyResourcesEventType::APP_RESOURCE_RESET;
    if (isProcess) {
        processType = EfficiencyResourcesEventType::RESOURCE_RESET;
    }
    ReportHisysEvent(EfficiencyResourceEventTriggerType::EFFICIENCY_RESOURCE_RESET, nullptr, callbackInfo,
        processType);
    subscriberMgr_->OnResourceChanged(callbackInfo, type);
    if (resourceRecord->resourceNumber_ == 0) {
        infoMap.erase(iter);
    }
}

ErrCode BgEfficiencyResourcesMgr::ResetAllEfficiencyResources()
{
    HitraceScoped traceScoped(HITRACE_TAG_OHOS,
        "BackgroundTaskManager::EfficiencyResource::Service::ResetAllEfficiencyResources");

    BGTASK_LOGD("start to reset all efficiency resources");
    if (!isSysReady_.load()) {
        BGTASK_LOGW("efficiency resources manager is not ready");
        return ERR_BGTASK_SYS_NOT_READY;
    }

    auto uid = IPCSkeleton::GetCallingUid();
    auto pid = IPCSkeleton::GetCallingPid();
    std::string bundleName = "";
    if (!IsCallingInfoLegal(uid, pid, bundleName)) {
        BGTASK_LOGE("reset efficiency resources failed, calling info is illegal");
        return ERR_BGTASK_INVALID_PID_OR_UID;
    }
    uint64_t tokenId = IPCSkeleton::GetCallingFullTokenID();
    if (!BundleManagerHelper::GetInstance()->IsSystemApp(tokenId) && !IsServiceExtensionType(pid)) {
        BGTASK_LOGE("reset efficiency resources failed, %{public}s is not system app and service extension type",
            bundleName.c_str());
        return ERR_BGTASK_NOT_SYSTEM_APP;
    }

    auto exemptedResourceType = GetExemptedResourceType(MAX_RESOURCE_MASK, uid, bundleName);
    if (exemptedResourceType == 0) {
        BGTASK_LOGE("reset efficiency resources failed, no permitted resource type");
        return ERR_BGTASK_PERMISSION_DENIED;
    }

    handler_->PostTask([this, exemptedResourceType, uid, pid, bundleName]() {
        std::shared_ptr<ResourceCallbackInfo> callbackInfo = std::make_shared<ResourceCallbackInfo>(uid,
            pid, exemptedResourceType, bundleName);
        this->ResetEfficiencyResourcesInner(callbackInfo, false, CancelReason::RESET_INTERFACE);
    });
    return ERR_OK;
}

void BgEfficiencyResourcesMgr::RemoveRelativeProcessRecord(int32_t uid, uint32_t resourceNumber)
{
    for (auto iter = procResourceApplyMap_.begin(); iter != procResourceApplyMap_.end(); iter++) {
        if (iter->second->uid_ == uid && (resourceNumber & iter->second->resourceNumber_) != 0) {
            uint32_t eraseBit = (resourceNumber & iter->second->resourceNumber_);
            std::shared_ptr<ResourceCallbackInfo> callbackInfo = std::make_shared<ResourceCallbackInfo>(uid,
                iter->second->pid_, eraseBit, iter->second->bundleName_);
            handler_->PostTask([this, callbackInfo]() {
                this->ResetEfficiencyResourcesInner(callbackInfo, true, CancelReason::APP_DIDE_TRIGGER_PROCESS_DIED);
            });
        }
    }
}

void BgEfficiencyResourcesMgr::ResetEfficiencyResourcesInner(
    const std::shared_ptr<ResourceCallbackInfo> &callbackInfo, bool isProcess, CancelReason cancelType)
{
    HitraceScoped traceScoped(HITRACE_TAG_OHOS,
        "BackgroundTaskManager::EfficiencyResource::Service::ResetEfficiencyResourcesInner");

    BGTASK_LOGD("reset efficiency resources inner,  uid:%{public}d, pid %{public}d,"\
        " resource number: %{public}u, isProcess: %{public}d", callbackInfo->GetUid(),
        callbackInfo->GetPid(), callbackInfo->GetResourceNumber(), isProcess);
    if (isProcess) {
        RemoveTargetResourceRecord(procResourceApplyMap_, callbackInfo->GetPid(),
            callbackInfo->GetResourceNumber(), EfficiencyResourcesEventType::RESOURCE_RESET, cancelType);
    } else {
        RemoveTargetResourceRecord(appResourceApplyMap_, callbackInfo->GetUid(),
            callbackInfo->GetResourceNumber(), EfficiencyResourcesEventType::APP_RESOURCE_RESET, cancelType);
        RemoveRelativeProcessRecord(callbackInfo->GetUid(), callbackInfo->GetResourceNumber());
    }
    DelayedSingleton<DataStorageHelper>::GetInstance()->RefreshResourceRecord(
        appResourceApplyMap_, procResourceApplyMap_);
}

bool BgEfficiencyResourcesMgr::IsCallingInfoLegal(int32_t uid, int32_t pid, std::string &bundleName)
{
    if (uid < 0 || pid < 0) {
        BGTASK_LOGE("pid or uid is invalid");
        return false;
    }
    bundleName = BundleManagerHelper::GetInstance()->GetClientBundleName(uid);
    return true;
}

ErrCode BgEfficiencyResourcesMgr::AddSubscriber(const sptr<IBackgroundTaskSubscriber> &subscriber)
{
    BGTASK_LOGD("add subscriber to efficiency resources succeed");
    handler_->PostSyncTask([this, &subscriber]() {
        subscriberMgr_->AddSubscriber(subscriber);
    });
    return ERR_OK;
}

ErrCode BgEfficiencyResourcesMgr::RemoveSubscriber(const sptr<IBackgroundTaskSubscriber> &subscriber)
{
    BGTASK_LOGD("remove subscriber to efficiency resources succeed");
    ErrCode result {};
    handler_->PostSyncTask([this, &result, &subscriber]() {
        result = subscriberMgr_->RemoveSubscriber(subscriber);
    });
    return result;
}

ErrCode BgEfficiencyResourcesMgr::ShellDump(const std::vector<std::string> &dumpOption,
    std::vector<std::string> &dumpInfo)
{
    if (!isSysReady_.load()) {
        BGTASK_LOGE("manager is not ready");
        return ERR_BGTASK_SYS_NOT_READY;
    }
    handler_->PostSyncTask([&]() {
        this->ShellDumpInner(dumpOption, dumpInfo);
    });
    return ERR_OK;
}

ErrCode BgEfficiencyResourcesMgr::ShellDumpInner(const std::vector<std::string> &dumpOption,
    std::vector<std::string> &dumpInfo)
{
    const std::unordered_map<std::string, std::function<void()>> dumpProcMap = {
        { DUMP_PARAM_LIST_ALL, [&] { DumpAllApplicationInfo(dumpInfo); }},
        { DUMP_PARAM_RESET_ALL, [&] { DumpResetAllResource(dumpInfo); }},
        { DUMP_PARAM_RESET_APP, [&] { DumpResetResource(dumpOption, true, false); }},
        { DUMP_PARAM_RESET_PROC, [&] { DumpResetResource(dumpOption, false, false); }},
        { DUMP_PARAM_SET_CPU_QUOTA, [&] { DumpSetCpuQuota(dumpOption); }},
        { DUMP_PARAM_RESET_CPU_QUOTA, [&] { DumpResetCpuQuotaUsage(dumpOption); }},
        { DUMP_PARAM_GET_CPU_QUOTA, [&] { DumpGetCpuQuota(dumpOption, dumpInfo); }}
    };

    auto iter = dumpProcMap.find(dumpOption[1]);
    if (iter != dumpProcMap.end()) {
        iter->second();
    }

    DelayedSingleton<DataStorageHelper>::GetInstance()->RefreshResourceRecord(
        appResourceApplyMap_, procResourceApplyMap_);
    return ERR_OK;
}

void BgEfficiencyResourcesMgr::DumpGetCpuQuota(const std::vector<std::string> &dumpOption,
    std::vector<std::string> &dumpInfo)
{
    if (resourceQuotaMgrHandle_ == nullptr) {
        BGTASK_LOGD("ResourceQuotaMgrHandle_ is nullptr.");
        return;
    }

    using GetQuotaFunc = void (*)(int32_t, std::vector<std::string> &);
    auto getQuotaFunc = reinterpret_cast<GetQuotaFunc>(
        dlsym(resourceQuotaMgrHandle_, "GetCpuApplyQuotaProcess"));
    if (!getQuotaFunc) {
        BGTASK_LOGE("Get getQuotaFunc failed.");
        return;
    }

    constexpr uint16_t VALID_PARAM_NUM = 3;
    if (dumpOption.size() != VALID_PARAM_NUM) {
        BGTASK_LOGW("Invalid dump param number, must have 1 param for uid, "
            "get all info when uid is 0.");
        return;
    }

    int32_t uid = std::atoi(dumpOption[2].c_str());
    getQuotaFunc(uid, dumpInfo);
}

void BgEfficiencyResourcesMgr::DumpSetCpuQuota(const std::vector<std::string> &dumpOption)
{
    if (resourceQuotaMgrHandle_ == nullptr) {
        BGTASK_LOGD("ResourceQuotaMgrHandle_ is nullptr.");
        return;
    }

    using SetQuotaFunc = void (*)(int32_t, uint32_t, uint32_t);
    auto setQuotaFunc = reinterpret_cast<SetQuotaFunc>(
        dlsym(resourceQuotaMgrHandle_, "SetCpuApplyQuotaProcess"));
    if (!setQuotaFunc) {
        BGTASK_LOGE("Get setQuotaFunc failed.");
        return;
    }

    constexpr uint16_t VALID_PARAM_NUM = 5;
    if (dumpOption.size() != VALID_PARAM_NUM) {
        BGTASK_LOGW("Invalid dump param number, "
            "must have 3 param for { uid, m, quotaPerDay }, "
            "set default quota when uid is 0.");
        return;
    }

    int32_t uid = std::atoi(dumpOption[2].c_str());
    uint32_t quotaPerRequest = static_cast<uint32_t>(std::atoi(dumpOption[3].c_str()));
    uint32_t quotaPerDay = static_cast<uint32_t>(std::atoi(dumpOption[4].c_str()));

    setQuotaFunc(uid, quotaPerRequest, quotaPerDay);
    BGTASK_LOGI("Set cpu apply quota, "
        "uid: %{public}d, : %{public}u, quotaPerDay: %{public}u.",
        uid, quotaPerRequest, quotaPerDay);
}

void BgEfficiencyResourcesMgr::DumpResetCpuQuotaUsage(const std::vector<std::string> &dumpOption)
{
    if (resourceQuotaMgrHandle_ == nullptr) {
        BGTASK_LOGD("ResourceQuotaMgrHandle_ is nullptr.");
        return;
    }

    using ResetQuotaFunc = void (*)(int32_t);
    auto resetQuotaFunc = reinterpret_cast<ResetQuotaFunc>(
        dlsym(resourceQuotaMgrHandle_, "ResetCpuApplyQuotaUsageProcess"));
    if (!resetQuotaFunc) {
        BGTASK_LOGE("Get resetQuotaFunc failed.");
        return;
    }

    constexpr uint16_t VALID_PARAM_NUM = 3;
    if (dumpOption.size() != VALID_PARAM_NUM) {
        BGTASK_LOGW("Invalid dump param number, must have 1 param for uid, "
            "reset all when uid is 0.");
        return;
    }

    int32_t uid = std::atoi(dumpOption[2].c_str());
    resetQuotaFunc(uid);
    BGTASK_LOGI("Reset cpu apply quota, uid: %{public}d.", uid);
}

void BgEfficiencyResourcesMgr::DumpAllApplicationInfo(std::vector<std::string> &dumpInfo)
{
    std::stringstream stream;
    if (appResourceApplyMap_.empty() && procResourceApplyMap_.empty()) {
        dumpInfo.emplace_back("No running efficiency resources\n");
        return;
    }
    DumpApplicationInfoMap(appResourceApplyMap_, dumpInfo, stream, "app efficiency resource: \n");
    DumpApplicationInfoMap(procResourceApplyMap_, dumpInfo, stream, "process efficiency resource: \n");
}

void BgEfficiencyResourcesMgr::DumpApplicationInfoMap(std::unordered_map<int32_t,
    std::shared_ptr<ResourceApplicationRecord>> &infoMap, std::vector<std::string> &dumpInfo,
    std::stringstream &stream, const char *headInfo)
{
    uint32_t index = 1;
    stream << headInfo << "\n";
    for (auto iter = infoMap.begin(); iter != infoMap.end(); iter++) {
        stream << "No." << index << "\n";
        stream << "\tefficiencyResourceKey: " << iter->first << "\n";
        stream << "\tefficiencyResourceValue:" << "\n";
        stream << "\t\tbundleName: " << iter->second->GetBundleName() << "\n";
        stream << "\t\tuid: " << iter->second->GetUid() << "\n";
        stream << "\t\tpid: " << iter->second->GetPid() << "\n";
        stream << "\t\tresourceNumber: " << iter->second->GetResourceNumber() << "\n";
        int64_t curTime = TimeProvider::GetCurrentTime();
        auto &resourceUnitList = iter->second->resourceUnitList_;
        for (auto unitIter = resourceUnitList.begin();
            unitIter != resourceUnitList.end(); ++unitIter) {
            stream << "\t\t\tresource type: " << ResourceTypeName[unitIter->resourceIndex_] << "\n";
            stream << "\t\t\tisPersist: " << (unitIter->isPersist_ ? "true" : "false") << "\n";
            if (!unitIter->isPersist_) {
                stream << "\t\t\tremainTime: " << unitIter->endTime_ - curTime << "\n";
            }
            stream << "\t\t\treason: " << unitIter->reason_ << "\n";
        }
        stream << "\n";
        dumpInfo.emplace_back(stream.str());
        stream.str("");
        stream.clear();
        index++;
    }
}

void BgEfficiencyResourcesMgr::DumpResetAllResource(const std::vector<std::string> &dumpOption)
{
    DumpResetResource(dumpOption, true, true);
    DumpResetResource(dumpOption, false, true);
}

void BgEfficiencyResourcesMgr::DumpResetResource(const std::vector<std::string> &dumpOption,
    bool cleanApp, bool cleanAll)
{
    auto &infoMap = cleanApp ? appResourceApplyMap_ : procResourceApplyMap_;
    auto type = cleanApp ? EfficiencyResourcesEventType::APP_RESOURCE_RESET
        : EfficiencyResourcesEventType::RESOURCE_RESET;

    if (cleanAll) {
        for (auto iter = infoMap.begin(); iter != infoMap.end(); ++iter) {
            std::shared_ptr<ResourceCallbackInfo> callbackInfo = std::make_shared<ResourceCallbackInfo>
                (iter->second->GetUid(), iter->second->GetPid(), iter->second->GetResourceNumber(),
                iter->second->GetBundleName());
            subscriberMgr_->OnResourceChanged(callbackInfo, type);
        }
        infoMap.clear();
    } else {
        if (dumpOption.size() < MAX_DUMP_PARAM_NUMS) {
            BGTASK_LOGW("invalid dump param");
            return;
        }
        int32_t mapKey = std::atoi(dumpOption[2].c_str());
        uint32_t cleanResource = static_cast<uint32_t>(std::atoi(dumpOption[3].c_str()));
        RemoveTargetResourceRecord(infoMap, mapKey, cleanResource, type, CancelReason::DUMPER);
    }
}

bool BgEfficiencyResourcesMgr::RemoveTargetResourceRecord(std::unordered_map<int32_t,
    std::shared_ptr<ResourceApplicationRecord>> &infoMap, int32_t mapKey, uint32_t cleanResource,
    EfficiencyResourcesEventType type, CancelReason cancelType)
{
    BGTASK_LOGD("resource record key: %{public}d, resource record size(): %{public}d",
        mapKey, static_cast<int32_t>(infoMap.size()));
    auto iter = infoMap.find(mapKey);
    if (iter == infoMap.end() || (iter->second->resourceNumber_ & cleanResource) == 0) {
        BGTASK_LOGD("remove single resource record failure, no matched task: %{public}d", mapKey);
        return false;
    }
    uint32_t eraseBit = (iter->second->resourceNumber_ & cleanResource);
    iter->second->resourceNumber_ ^= eraseBit;
    RemoveListRecord(iter->second->resourceUnitList_, eraseBit);
    auto callbackInfo = std::make_shared<ResourceCallbackInfo>(iter->second->GetUid(),
        iter->second->GetPid(), eraseBit, iter->second->GetBundleName());

    // update the left quota of cpu efficiency resource when reset
    UpdateQuotaIfCpuReset(type, callbackInfo->GetUid(), callbackInfo->GetResourceNumber());
    BGTASK_LOGI("remove record from info map, mapkey %{public}d, uid: %{public}d, bundle name: %{public}s "
        "erasebit %{public}d, reason: %{public}u", mapKey, callbackInfo->GetUid(),
        callbackInfo->GetBundleName().c_str(), eraseBit, cancelType);
    ReportHisysEvent(EfficiencyResourceEventTriggerType::EFFICIENCY_RESOURCE_RESET, nullptr, callbackInfo, type);
    subscriberMgr_->OnResourceChanged(callbackInfo, type);
    if (iter->second->resourceNumber_ == 0) {
        infoMap.erase(iter);
    }
    return true;
}

void BgEfficiencyResourcesMgr::RemoveListRecord(std::list<PersistTime> &resourceUnitList, uint32_t eraseBit)
{
    BGTASK_LOGD("start remove record from list, eraseBit: %{public}d", eraseBit);
    if (eraseBit == 0) {
        return;
    }
    for (auto it = resourceUnitList.begin(); it != resourceUnitList.end();) {
        if (((1 << it->resourceIndex_) & eraseBit) != 0) {
            it = resourceUnitList.erase(it);
        } else {
            ++it;
        }
    }
}

ErrCode BgEfficiencyResourcesMgr::GetEfficiencyResourcesInfos(std::vector<std::shared_ptr<
    ResourceCallbackInfo>> &appList, std::vector<std::shared_ptr<ResourceCallbackInfo>> &procList)
{
    handler_->PostSyncTask([this, &appList, &procList]() {
        this->GetEfficiencyResourcesInfosInner(appResourceApplyMap_, appList);
        this->GetEfficiencyResourcesInfosInner(procResourceApplyMap_, procList);
        }, AppExecFwk::EventQueue::Priority::HIGH);

    return ERR_OK;
}

void BgEfficiencyResourcesMgr::GetEfficiencyResourcesInfosInner(const ResourceRecordMap &infoMap,
    std::vector<std::shared_ptr<ResourceCallbackInfo>> &list)
{
    if (infoMap.empty()) {
        return;
    }
    BGTASK_LOGD("get efficiency resources info inner function, resources record size(): %{public}d ",
        static_cast<int32_t>(infoMap.size()));
    for (auto &record : infoMap) {
        auto appInfo = std::make_shared<ResourceCallbackInfo>(record.second->uid_, record.second->pid_,
            record.second->resourceNumber_, record.second->bundleName_);
        list.push_back(appInfo);
    }
}

uint32_t BgEfficiencyResourcesMgr::GetExemptedResourceType(uint32_t resourceNumber, const int32_t uid,
    const std::string &bundleName)
{
    const std::vector<int32_t>& resourcesApply = QueryRunningResourcesApply(uid, bundleName);

    uint32_t exemptedResources = 0;
    if (resourcesApply.empty()) {
        return exemptedResources;
    }

    // application is permitted to use all type of resources if FREEZE_ALL_RESOURCES in resourceApply
    if (std::find(resourcesApply.begin(), resourcesApply.end(), FREEZE_ALL_RESOURCES) != resourcesApply.end()) {
        return resourceNumber;
    }

    // traverse resourcesApply and get exempted resource type
    for (const auto resourceType : resourcesApply) {
        if (resourceType < 0 || resourceType > static_cast<int32_t>(MAX_RESOURCES_TYPE_NUM)) {
            continue;
        }
        // (1 << (resourceType - 1)) map number in resourceApply to resourceType defined in resource_type.h
        exemptedResources |= (1 << (resourceType - 1));
    }
    exemptedResources &= resourceNumber;
    BGTASK_LOGD("after filter, uid: %{public}d, bundleName: %{public}s, origin resource number: %{public}u, return "\
        "resource number: %{public}u", uid, bundleName.c_str(), resourceNumber, exemptedResources);

    return exemptedResources;
}

// meaning of number in resourcesApply list: 0 - all type of resources, 1 - cpu, 2 - COMMON_EVENT, 3 - TIMER,
// 4 - WORK_SCHEDULER, 5 - BLUETOOTH, 6 - GPS, 7 - AUDIO, 8 - RUNNING_LOCK, 9 - SENSOR
std::vector<int32_t> BgEfficiencyResourcesMgr::QueryRunningResourcesApply(const int32_t uid,
    const std::string &bundleName)
{
    AppExecFwk::ApplicationInfo applicationInfo;
    if (!BundleManagerHelper::GetInstance()->GetApplicationInfo(bundleName,
        AppExecFwk::ApplicationFlag::GET_BASIC_APPLICATION_INFO, GetUserIdByUid(uid), applicationInfo)) {
        BGTASK_LOGE("failed to get applicationInfo from AppExecFwk, bundleName is %{public}s", bundleName.c_str());
        return {};
    }
    BGTASK_LOGD("size of applicationInfo.resourcesApply is %{public}d",
        static_cast<int32_t>(applicationInfo.resourcesApply.size()));
    return applicationInfo.resourcesApply;
}

int32_t BgEfficiencyResourcesMgr::GetUserIdByUid(int32_t uid)
{
    const int32_t BASE_USER_RANGE = 200000;
    return uid / BASE_USER_RANGE;
}

ErrCode BgEfficiencyResourcesMgr::GetAllEfficiencyResources(
    std::vector<std::shared_ptr<EfficiencyResourceInfo>> &resourceInfoList)
{
    HitraceScoped traceScoped(HITRACE_TAG_OHOS,
        "BackgroundTaskManager::EfficiencyResource::Service::GetAllEfficiencyResources");

    BGTASK_LOGD("start to get all efficiency resources");
    if (!isSysReady_.load()) {
        BGTASK_LOGW("efficiency resources is not ready");
        return ERR_BGTASK_RESOURCES_SYS_NOT_READY;
    }

    auto uid = IPCSkeleton::GetCallingUid();
    auto pid = IPCSkeleton::GetCallingPid();
    std::string bundleName = "";
    if (!IsCallingInfoLegal(uid, pid, bundleName)) {
        BGTASK_LOGE("get all efficiency resources failed, calling info is illegal");
        return ERR_BGTASK_RESOURCES_INVALID_PID_OR_UID;
    }
    uint64_t tokenId = IPCSkeleton::GetCallingFullTokenID();
    if (!BundleManagerHelper::GetInstance()->IsSystemApp(tokenId) && !IsServiceExtensionType(pid)) {
        BGTASK_LOGE("get all efficiency resources failed, %{public}s is not system app and service extension type",
            bundleName.c_str());
        return ERR_BGTASK_NOT_SYSTEM_APP;
    }

    handler_->PostSyncTask([this, &resourceInfoList, uid, pid]() {
            this->GetAllEfficiencyResourcesInner(appResourceApplyMap_, resourceInfoList, uid, pid, false);
            this->GetAllEfficiencyResourcesInner(procResourceApplyMap_, resourceInfoList, uid, pid, true);
        });
    return ERR_OK;
}

void BgEfficiencyResourcesMgr::GetAllEfficiencyResourcesInner(const ResourceRecordMap &infoMap,
    std::vector<std::shared_ptr<EfficiencyResourceInfo>> &resourceInfoList, const int32_t uid, const int32_t pid,
    const bool isProcess)
{
    if (infoMap.empty()) {
        BGTASK_LOGD("infoMap is empty");
        return;
    }
    BGTASK_LOGD("get efficiency resources info inner function, resources record size(): %{public}d",
        static_cast<int32_t>(infoMap.size()));

    for (auto iter = infoMap.begin(); iter != infoMap.end(); iter++) {
        if (iter->second == nullptr) {
            continue;
        }
        if (iter->second->uid_ != uid) {
            continue;
        }
        auto &resourceList = iter->second->resourceUnitList_;
        for (auto recordIter = resourceList.begin(); recordIter != resourceList.end(); recordIter++) {
            int32_t resourceNumber = 1;
            if (recordIter->resourceIndex_ != 0) {
                resourceNumber = recordIter->resourceIndex_ << 1;
            }
            auto appInfo = std::make_shared<EfficiencyResourceInfo>(resourceNumber, false,
                recordIter->timeOut_, recordIter->reason_, recordIter->isPersist_, isProcess);
            appInfo->SetPid(pid);
            appInfo->SetUid(uid);
            resourceInfoList.push_back(appInfo);
        }
    }
}

void BgEfficiencyResourcesMgr::ReportHisysEvent(EfficiencyResourceEventTriggerType operationType,
    const sptr<EfficiencyResourceInfo> &resourceInfo, const std::shared_ptr<ResourceCallbackInfo> &callbackInfo,
    EfficiencyResourcesEventType type)
{
    switch (operationType) {
        case EfficiencyResourceEventTriggerType::EFFICIENCY_RESOURCE_APPLY:
            if (!applyEventData_.AddData(resourceInfo, callbackInfo)) {
                HiSysEventWrite(HiviewDFX::HiSysEvent::Domain::BACKGROUND_TASK, "EFFICIENCY_RESOURCE_APPLY",
                    HiviewDFX::HiSysEvent::EventType::STATISTIC,
                    "APP_UID", applyEventData_.appUid_, "APP_PID", applyEventData_.appPid_,
                    "APP_NAME", applyEventData_.appName_, "UIABILITY_IDENTITY", applyEventData_.uiabilityIdentity_,
                    "RESOURCE_TYPE", applyEventData_.resourceType_, "TIMEOUT", applyEventData_.timeout_,
                    "PERSIST", applyEventData_.persist_, "PROCESS", applyEventData_.process_);
                applyEventData_.ClearData();
                applyEventData_.AddData(resourceInfo, callbackInfo);
            }
            break;
        case EfficiencyResourceEventTriggerType::EFFICIENCY_RESOURCE_RESET:
            if (!resetEventData_.AddData(callbackInfo, type)) {
                HiSysEventWrite(HiviewDFX::HiSysEvent::Domain::BACKGROUND_TASK, "EFFICIENCY_RESOURCE_RESET",
                    HiviewDFX::HiSysEvent::EventType::STATISTIC,
                    "APP_UID", resetEventData_.appUid_, "APP_PID", resetEventData_.appPid_,
                    "APP_NAME", resetEventData_.appName_, "UIABILITY_IDENTITY", resetEventData_.uiabilityIdentity_,
                    "RESOURCE_TYPE", resetEventData_.resourceType_, "PROCESS", resetEventData_.process_,
                    "QUOTA", resetEventData_.quota_, "ALL_QUOTA", resetEventData_.allQuota_);
                resetEventData_.ClearData();
                resetEventData_.AddData(callbackInfo, type);
            }
            break;
    }
}
}  // namespace BackgroundTaskMgr
}  // namespace OHOS
