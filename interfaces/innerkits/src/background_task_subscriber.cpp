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

#include "background_task_subscriber.h"

namespace OHOS {
namespace BackgroundTaskMgr {
BackgroundTaskSubscriber::BackgroundTaskSubscriber()
{
    impl_ = new (std::nothrow) BackgroundTaskSubscriberImpl(*this);
};

BackgroundTaskSubscriber::~BackgroundTaskSubscriber() {}

void BackgroundTaskSubscriber::OnConnected() {}

void BackgroundTaskSubscriber::OnDisconnected() {}

void BackgroundTaskSubscriber::OnTransientTaskStart(const std::shared_ptr<TransientTaskAppInfo>& info) {}

void BackgroundTaskSubscriber::OnTransientTaskEnd(const std::shared_ptr<TransientTaskAppInfo>& info) {}

void BackgroundTaskSubscriber::OnAppTransientTaskStart(const std::shared_ptr<TransientTaskAppInfo>& info) {}

void BackgroundTaskSubscriber::OnAppTransientTaskEnd(const std::shared_ptr<TransientTaskAppInfo>& info) {}

void BackgroundTaskSubscriber::OnContinuousTaskStart(
    const std::shared_ptr<ContinuousTaskCallbackInfo> &continuousTaskCallbackInfo) {}

void BackgroundTaskSubscriber::OnContinuousTaskUpdate(
    const std::shared_ptr<ContinuousTaskCallbackInfo> &continuousTaskCallbackInfo) {}

void BackgroundTaskSubscriber::OnContinuousTaskStop(
    const std::shared_ptr<ContinuousTaskCallbackInfo> &continuousTaskCallbackInfo) {}

void BackgroundTaskSubscriber::OnAppContinuousTaskStop(int32_t uid) {}

void BackgroundTaskSubscriber::OnRemoteDied(const wptr<IRemoteObject> &object) {}

void BackgroundTaskSubscriber::OnProcEfficiencyResourcesApply(
    const std::shared_ptr<ResourceCallbackInfo> &resourceInfo) {}

void BackgroundTaskSubscriber::OnProcEfficiencyResourcesReset(
    const std::shared_ptr<ResourceCallbackInfo> &resourceInfo) {}

void BackgroundTaskSubscriber::OnAppEfficiencyResourcesApply(
    const std::shared_ptr<ResourceCallbackInfo> &resourceInfo) {}

void BackgroundTaskSubscriber::OnAppEfficiencyResourcesReset(
    const std::shared_ptr<ResourceCallbackInfo> &resourceInfo) {}

const sptr<BackgroundTaskSubscriber::BackgroundTaskSubscriberImpl> BackgroundTaskSubscriber::GetImpl() const
{
    return impl_;
}

BackgroundTaskSubscriber::BackgroundTaskSubscriberImpl::BackgroundTaskSubscriberImpl(
    BackgroundTaskSubscriber &subscriber) : subscriber_(subscriber) {}

void BackgroundTaskSubscriber::BackgroundTaskSubscriberImpl::OnConnected() {}

void BackgroundTaskSubscriber::BackgroundTaskSubscriberImpl::OnDisconnected() {}

void BackgroundTaskSubscriber::BackgroundTaskSubscriberImpl::OnAppEfficiencyResourcesApply(
    const std::shared_ptr<ResourceCallbackInfo> &resourceInfo)
{
    subscriber_.OnAppEfficiencyResourcesApply(resourceInfo);
}

void BackgroundTaskSubscriber::BackgroundTaskSubscriberImpl::OnAppEfficiencyResourcesReset(
    const std::shared_ptr<ResourceCallbackInfo> &resourceInfo)
{
    subscriber_.OnAppEfficiencyResourcesReset(resourceInfo);
}

void BackgroundTaskSubscriber::BackgroundTaskSubscriberImpl::OnProcEfficiencyResourcesApply(
    const std::shared_ptr<ResourceCallbackInfo> &resourceInfo)
{
    subscriber_.OnProcEfficiencyResourcesApply(resourceInfo);
}

void BackgroundTaskSubscriber::BackgroundTaskSubscriberImpl::OnProcEfficiencyResourcesReset(
    const std::shared_ptr<ResourceCallbackInfo> &resourceInfo)
{
    subscriber_.OnProcEfficiencyResourcesReset(resourceInfo);
}

void BackgroundTaskSubscriber::BackgroundTaskSubscriberImpl::OnTransientTaskStart(
    const std::shared_ptr<TransientTaskAppInfo>& info)
{
    subscriber_.OnTransientTaskStart(info);
}

void BackgroundTaskSubscriber::BackgroundTaskSubscriberImpl::OnTransientTaskEnd(
    const std::shared_ptr<TransientTaskAppInfo>& info)
{
    subscriber_.OnTransientTaskEnd(info);
}

void BackgroundTaskSubscriber::BackgroundTaskSubscriberImpl::OnAppTransientTaskStart(
    const std::shared_ptr<TransientTaskAppInfo>& info)
{
    subscriber_.OnAppTransientTaskStart(info);
}

void BackgroundTaskSubscriber::BackgroundTaskSubscriberImpl::OnAppTransientTaskEnd(
    const std::shared_ptr<TransientTaskAppInfo>& info)
{
    subscriber_.OnAppTransientTaskEnd(info);
}

void BackgroundTaskSubscriber::BackgroundTaskSubscriberImpl::OnContinuousTaskStart(
    const std::shared_ptr<ContinuousTaskCallbackInfo> &continuousTaskCallbackInfo)
{
    subscriber_.OnContinuousTaskStart(continuousTaskCallbackInfo);
}

void BackgroundTaskSubscriber::BackgroundTaskSubscriberImpl::OnContinuousTaskUpdate(
    const std::shared_ptr<ContinuousTaskCallbackInfo> &continuousTaskCallbackInfo)
{
    subscriber_.OnContinuousTaskUpdate(continuousTaskCallbackInfo);
}

void BackgroundTaskSubscriber::BackgroundTaskSubscriberImpl::OnContinuousTaskStop(
    const std::shared_ptr<ContinuousTaskCallbackInfo> &continuousTaskCallbackInfo)
{
    subscriber_.OnContinuousTaskStop(continuousTaskCallbackInfo);
}

void BackgroundTaskSubscriber::BackgroundTaskSubscriberImpl::OnAppContinuousTaskStop(int32_t uid)
{
    subscriber_.OnAppContinuousTaskStop(uid);
}
}  // namespace BackgroundTaskMgr
}  // namespace OHOS