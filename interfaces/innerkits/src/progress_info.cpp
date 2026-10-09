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

#include "progress_info.h"
#include "bgtaskmgr_log_wrapper.h"

namespace OHOS {
namespace BackgroundTaskMgr {

std::string ProgressInfo::GetTitle() const
{
    return title_;
}

void ProgressInfo::SetTitle(const std::string &title)
{
    title_ = title;
}

std::string ProgressInfo::GetFileName() const
{
    return fileName_;
}

void ProgressInfo::SetFileName(const std::string &fileName)
{
    fileName_ = fileName;
}

int32_t ProgressInfo::GetProgressValue() const
{
    return progressValue_;
}

void ProgressInfo::SetProgressValue(int32_t progressValue)
{
    progressValue_ = progressValue;
}

bool ProgressInfo::IsMute() const
{
    return isMute_;
}

void ProgressInfo::SetIsMute(bool isMute)
{
    isMute_ = isMute;
}

bool ProgressInfo::Marshalling(Parcel& out) const
{
    if (!out.WriteString(title_)) {
        BGTASK_LOGE("Failed to write title");
        return false;
    }
    if (!out.WriteString(fileName_)) {
        BGTASK_LOGE("Failed to write fileName");
        return false;
    }
    if (!out.WriteInt32(progressValue_)) {
        BGTASK_LOGE("Failed to write progressValue");
        return false;
    }
    if (!out.WriteBool(isMute_)) {
        BGTASK_LOGE("Failed to write isMute");
        return false;
    }
    return true;
}

ProgressInfo* ProgressInfo::Unmarshalling(Parcel& in)
{
    ProgressInfo* info = new (std::nothrow) ProgressInfo();
    if (info && !info->ReadFromParcel(in)) {
        BGTASK_LOGE("read from parcel failed");
        delete info;
        info = nullptr;
    }
    return info;
}

bool ProgressInfo::ReadFromParcel(Parcel& in)
{
    if (!in.ReadString(title_)) {
        BGTASK_LOGE("read parcel title error");
        return false;
    }

    if (!in.ReadString(fileName_)) {
        BGTASK_LOGE("read parcel fileName error");
        return false;
    }

    if (!in.ReadInt32(progressValue_)) {
        BGTASK_LOGE("read parcel progressValue error");
        return false;
    }
    if (!in.ReadBool(isMute_)) {
        BGTASK_LOGE("read parcel isMute error");
        return false;
    }
    return true;
}
}  // namespace BackgroundTaskMgr
}  // namespace OHOS
