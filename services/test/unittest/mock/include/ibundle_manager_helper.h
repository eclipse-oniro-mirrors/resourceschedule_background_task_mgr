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

#ifndef FOUNDATION_RESOURCESCHEDULE_BACKGROUND_TASK_MGR_SERVICES_TEST_IBUNDLE_MANAGER_HELPER_H
#define FOUNDATION_RESOURCESCHEDULE_BACKGROUND_TASK_MGR_SERVICES_TEST_IBUNDLE_MANAGER_HELPER_H

namespace OHOS {
namespace BackgroundTaskMgr {
class IBundleManagerHelper {
public:
    virtual ~IBundleManagerHelper() = default;
    virtual bool GetApplicationInfo(const std::string &appName, const AppExecFwk::ApplicationFlag flag,
        const int userId, AppExecFwk::ApplicationInfo &appInfo) = 0;
};

void SetBundleManagerHelper(std::shared_ptr<IBundleManagerHelper> mock);
void CleanBundleManagerHelper();
}  // namespace BackgroundTaskMgr
}  // namespace OHOS
#endif  // FOUNDATION_RESOURCESCHEDULE_BACKGROUND_TASK_MGR_SERVICES_TEST_IBUNDLE_MANAGER_HELPER_H