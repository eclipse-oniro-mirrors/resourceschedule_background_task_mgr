/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <unordered_map>

#include "background_mode.h"

namespace OHOS {
namespace BackgroundTaskMgr {
const std::unordered_map<uint32_t, std::string> PARAM_BACKGROUND_MODE_STR_MAP = {
    {BackgroundMode::DATA_TRANSFER, "dataTransfer"},
    {BackgroundMode::AUDIO_PLAYBACK, "audioPlayback"},
    {BackgroundMode::AUDIO_RECORDING, "audioRecording"},
    {BackgroundMode::LOCATION, "location"},
    {BackgroundMode::BLUETOOTH_INTERACTION, "bluetoothInteraction"},
    {BackgroundMode::MULTI_DEVICE_CONNECTION, "multiDeviceConnection"},
    {BackgroundMode::WIFI_INTERACTION, "wifiInteraction"},
    {BackgroundMode::VOIP, "voip"},
    {BackgroundMode::TASK_KEEPING, "taskKeeping"},
    {BackgroundMode::WORKOUT, "workout"},
    {BackgroundMode::END, "end"}
};

std::string BackgroundMode::GetBackgroundModeStr(uint32_t mode)
{
    auto iter = PARAM_BACKGROUND_MODE_STR_MAP.find(mode);
    if (iter != PARAM_BACKGROUND_MODE_STR_MAP.end()) {
        return iter->second.c_str();
    }
    return "default";
}
}  // namespace BackgroundTaskMgr
}  // namespace OHOS