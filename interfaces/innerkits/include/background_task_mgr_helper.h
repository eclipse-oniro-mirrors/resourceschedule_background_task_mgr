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

#ifndef FOUNDATION_RESOURCESCHEDULE_BACKGROUND_TASK_MGR_INTERFACES_INNERKITS_INCLUDE_BACKGROUND_TASK_MGR_HELPER_H
#define FOUNDATION_RESOURCESCHEDULE_BACKGROUND_TASK_MGR_INTERFACES_INNERKITS_INCLUDE_BACKGROUND_TASK_MGR_HELPER_H

#include "background_task_subscriber.h"
#include "efficiency_resource_info.h"

namespace OHOS {
namespace BackgroundTaskMgr {
class BackgroundTaskMgrHelper {
public:
    /**
     * @brief Request service to keep running background
     *
     * @param taskParam Request params.
     * @return ERR_OK if success, else fail.
     */
    static ErrCode RequestStartBackgroundRunning(ContinuousTaskParam &taskParam);

    /**
     * @brief Request service to update running background
     *
     * @param taskParam Request params.
     * @return ERR_OK if success, else fail.
     */
    static ErrCode RequestUpdateBackgroundRunning(const ContinuousTaskParam &taskParam);

    /**
     * @brief Request service to keep or stop running background for inner ability.
     *
     * @param taskParam Request params.
     * @return ERR_OK if success, else fail.
     */
    static ErrCode RequestBackgroundRunningForInner(const ContinuousTaskParamForInner &taskParam);

    /**
     * @brief Request service to stop running background
     *
     * @param abilityName Ability name of the requester ability
     * @param abilityToken Ability token to mark an unique running ability instance
     * @param abilityId Ability identity
     * @return ERR_OK if success, else fail.
     */
    static ErrCode RequestStopBackgroundRunning(const std::string &abilityName,
        const sptr<IRemoteObject> &abilityToken, int32_t abilityId = -1);

    /**
     * @brief Subscribes background task event.
     *
     * @param subscriber Subscriber token.
     * @return ERR_OK if success, else fail.
     */
    static ErrCode SubscribeBackgroundTask(const BackgroundTaskSubscriber &subscriber);

    /**
     * @brief Unsubscribes background task event.
     *
     * @param subscriber Subscriber token.
     * @return ERR_OK if success, else fail.
     */
    static ErrCode UnsubscribeBackgroundTask(const BackgroundTaskSubscriber &subscriber);

    /**
     * @brief Get transient task applications.
     * @param list transient task apps.
     * @return Returns ERR_OK if success, else failure.
     */
    static ErrCode GetTransientTaskApps(std::vector<std::shared_ptr<TransientTaskAppInfo>> &list);
    
    /**
     * @brief Pause transient task time by uid for inner ability.
     * @param uid app uid.
     * @return Returns ERR_OK if success, else failure.
     */
    static ErrCode PauseTransientTaskTimeForInner(int32_t uid);

    /**
     * @brief Start transient task time by uid for inner ability.
     * @param uid app uid.
     * @return Returns ERR_OK if success, else failure.
     */
    static ErrCode StartTransientTaskTimeForInner(int32_t uid);

    /**
     * @brief Get all continuous task running infos.
     * @param list continuous task infos.
     * @return Returns ERR_OK if success, else failure.
     */
    static ErrCode GetContinuousTaskApps(std::vector<std::shared_ptr<ContinuousTaskCallbackInfo>> &list);

    /**
     * @brief Get all effficiency resources running infos.
     * @param appList EFficiency Resources infos of apps.
     * @param procList  EFficiency Resources infos of processes.
     * @return Returns ERR_OK on success, others on failure.
     */
    static ErrCode GetEfficiencyResourcesInfos(std::vector<std::shared_ptr<ResourceCallbackInfo>> &appList,
        std::vector<std::shared_ptr<ResourceCallbackInfo>> &procList);

    /**
     * @brief Apply or unapply efficiency resources.
     *
     * @param resourceInfo Request params.
     * @return Returns ERR_OK on success, others on failure.
     */
    static ErrCode ApplyEfficiencyResources(const EfficiencyResourceInfo &resourceInfo);

    /**
     * @brief Reset all efficiency resources.
     *
     * @return ERR_OK if success, else fail.
     */
    static ErrCode ResetAllEfficiencyResources();

    /**
     * @brief Request stop continuous task.
     * @param uid app uid.
     * @param pid app pid.
     * @param taskType continuous task type.
     * @return Returns ERR_OK if success, else failure.
     */
    static ErrCode StopContinuousTask(int32_t uid, int32_t pid, uint32_t taskType, const std::string &key);
};
}  // namespace BackgroundTaskMgr
}  // namespace OHOS
#endif  // FOUNDATION_RESOURCESCHEDULE_BACKGROUND_TASK_MGR_INTERFACES_INNERKITS_INCLUDE_BACKGROUND_TASK_MGR_HELPER_H