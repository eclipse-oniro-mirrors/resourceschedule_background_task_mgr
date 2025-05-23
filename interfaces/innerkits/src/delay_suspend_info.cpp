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

#include "delay_suspend_info.h"

#include "ipc_util.h"

using namespace std;

namespace OHOS {
namespace BackgroundTaskMgr {
bool DelaySuspendInfo::Marshalling(Parcel& out) const
{
    WRITE_PARCEL_WITH_RET(out, Int32, requestId_, false);
    WRITE_PARCEL_WITH_RET(out, Int32, actualDelayTime_, false);
    return true;
}

DelaySuspendInfo* DelaySuspendInfo::Unmarshalling(Parcel &in)
{
    DelaySuspendInfo* info = new (std::nothrow) DelaySuspendInfo();
    if (info && !info->ReadFromParcel(in)) {
        BGTASK_LOGE("read from parcel failed");
        delete info;
        info = nullptr;
    }
    return info;
}

bool DelaySuspendInfo::ReadFromParcel(Parcel& in)
{
    READ_PARCEL_WITH_RET(in, Int32, requestId_, false);
    READ_PARCEL_WITH_RET(in, Int32, actualDelayTime_, false);
    return true;
}
}  // namespace BackgroundTaskMgr
}  // namespace OHOS