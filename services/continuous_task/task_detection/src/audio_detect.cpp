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

#include "audio_detect.h"

#include "bg_continuous_task_mgr.h"
#include "common_utils.h"
#include "continuous_task_log.h"

namespace OHOS {
namespace BackgroundTaskMgr {
namespace {
static constexpr int32_t UNSET_PID = -1;
static constexpr int32_t AUDIO_RUNNING_STATE = 2;
static constexpr uint32_t AUDIO_PLAYBACK_BGMODE_ID = 2;
static constexpr uint32_t AUDIO_RECORDING_BGMODE_ID = 3;
}

void AudioDetect::HandleAudioStreamInfo(
    const std::list<std::tuple<int32_t, int32_t, int32_t>> &streamInfos, const std::string &type)
{
    std::set<int32_t> uidRemoved;
    if (type == "player") {
        UpdateAudioRecord(streamInfos, audioPlayerInfos_, uidRemoved);
    } else {
        UpdateAudioRecord(streamInfos, audioRecorderInfos_, uidRemoved);
    }
    uint32_t bgModeId = (type == "player" ? AUDIO_PLAYBACK_BGMODE_ID : AUDIO_RECORDING_BGMODE_ID);
    for (int32_t uidToCheck : uidRemoved) {
        if (!CheckAudioCondition(uidToCheck, bgModeId)) {
            BgContinuousTaskMgr::GetInstance()->ReportTaskRunningStateUnmet(uidToCheck,
                UNSET_PID, bgModeId);
        }
    }
}

void AudioDetect::UpdateAudioRecord(const std::list<std::tuple<int32_t, int32_t, int32_t>> &streamInfos,
    std::list<std::shared_ptr<AudioInfo>> &records, std::set<int32_t> &uidRemoved)
{
    for (const auto &var : streamInfos) {
        int32_t uid;
        int32_t sessionId;
        int32_t state;
        std::tie(uid, sessionId, state) = var;
        BGTASK_LOGI("Get Audio info uid: %{public}d, sessionId: %{public}d, State: %{public}d",
            uid, sessionId, state);
        auto findRecord = [uid, sessionId](const auto &target) {
        return target->uid_ == uid && target->sessionId_ == sessionId;
        };
        auto findRecordIter = find_if(records.begin(), records.end(), findRecord);
        if (findRecordIter == records.end() && state == AUDIO_RUNNING_STATE) {
            auto recorderInfo = std::make_shared<AudioInfo>(uid, sessionId);
            records.emplace_back(recorderInfo);
        } else if (findRecordIter != records.end() && state != AUDIO_RUNNING_STATE) {
            records.erase(findRecordIter);
            uidRemoved.emplace(uid);
        }
    }
}

#ifdef AV_SESSION_PART_ENABLE
void AudioDetect::HandleAVSessionInfo(const AVSession::AVSessionDescriptor &descriptor,
    const std::string &action)
{
    std::string sessionId = descriptor.sessionId_;
    int32_t uid = descriptor.uid_;
    int32_t pid = descriptor.pid_;
    auto findRecord = [sessionId, pid, uid](const auto &target) {
        return target->sessionId_ == sessionId && target->pid_ == pid && target->uid_ == uid;
    };
    auto findRecordIter = find_if(avSessionInfos_.begin(), avSessionInfos_.end(), findRecord);
    if (action == "create" && findRecordIter == avSessionInfos_.end()) {
        avSessionInfos_.emplace_back(std::make_shared<AVSessionInfo>(uid, pid, sessionId));
    } else if (action == "release" && findRecordIter != avSessionInfos_.end()) {
        avSessionInfos_.erase(findRecordIter);
        if (!CheckAudioCondition(uid, AUDIO_PLAYBACK_BGMODE_ID)) {
            BgContinuousTaskMgr::GetInstance()->ReportTaskRunningStateUnmet(uid,
                UNSET_PID, AUDIO_PLAYBACK_BGMODE_ID);
        }
    }
}
#endif // AV_SESSION_PART_ENABLE

template<typename T>
void ParseAudioStreamInfoToStr(nlohmann::json &value, const std::string &type, const T &record)
{
    auto streamInfos = nlohmann::json::array();
    for (auto var : record) {
        nlohmann::json info;
        info["uid"] = var->uid_;
        info["sessionId"] = var->sessionId_;
        streamInfos.push_back(info);
    }
    value[type] = streamInfos;
}

void AudioDetect::ParseAudioRecordToStr(nlohmann::json &value)
{
    nlohmann::json playerInfo;
    ParseAudioStreamInfoToStr(playerInfo, "audioRenderInfos", audioPlayerInfos_);
    // Json::Value audioRenderInfos;
    // for (auto var : audioPlayerInfos_) {
    //     Json::Value info;
    //     info["uid"] = var->uid_;
    //     info["sessionId"] = var->sessionId_;
    //     audioRenderInfos.append(info);
    // }
    // playerInfo["audioRenderInfos"] = audioRenderInfos;

    auto avSessionInfos = nlohmann::json::array();
    for (auto var : avSessionInfos_) {
        nlohmann::json info;
        info["uid"] = var->uid_;
        info["pid"] = var->pid_;
        info["sessionId"] = var->sessionId_;
        avSessionInfos.push_back(info);
    }
    playerInfo["AVSessionInfos"] = avSessionInfos;
    
    value["audioPlayer"] = playerInfo;

    ParseAudioStreamInfoToStr(value, "audioRecorder", audioRecorderInfos_);

    // Json::Value RecorderInfo;
    // for (auto var : audioRecorderInfos_) {
    //     Json::Value info;
    //     info["uid"] = var->uid_;
    //     info["sessionId"] = var->sessionId_;
    //     RecorderInfo.append(info);
    // }
    // value["audioRecorder"] = RecorderInfo;
}

template<typename T>
void ParseAudioStreamInfoFromJson(const nlohmann::json &value, const std::string &type,
    std::set<int32_t> &uidSet, T &record)
{
    // nlohmann::json arrayObj = value[type];
    int32_t uid;
    int32_t sessionId;
    // for (uint32_t i = 0; i < arrayObj.size(); i++) {
    //     uid = arrayObj[i]["uid"].asInt();
    //     uidSet.emplace(uid);
    //     auto info = std::make_shared<AudioInfo>(uid, arrayObj[i]["sessionId"].asInt());
    //     record.emplace_back(info);
    // }

    // for (auto iter = arrayObj.begin(); iter != arrayObj.end(); iter++) {
    //     uid = (*iter).at("uid").get<int32_t>();
    //     sessionId = (*iter).at("sessionId").get<int32_t>();
    //     uidSet.emplace(uid);
    //     auto info = std::make_shared<AudioInfo>(uid, sessionId);
    //     record.emplace_back(info);
    // }

    for (auto &elem : value[type]) {
        uid = elem.at("uid").get<int32_t>();
        sessionId = elem.at("sessionId").get<int32_t>();
        uidSet.emplace(uid);
        auto info = std::make_shared<AudioInfo>(uid, sessionId);
        record.emplace_back(info);
    }
}

bool AudioDetect::ParseAudioRecordFromJson(const nlohmann::json &value, std::set<int32_t> &uidSet)
{
    // if (!value.isMember("audioPlayer") || !value.isMember("audioRecorder")) {
    //     return false;
    // }
    if (!CommonUtils::CheckJsonValue(root, { "audioPlayer", "audioRecorder" })) {
        return false;
    }
    nlohmann::json playerInfo = value["audioPlayer"];
    ParseAudioStreamInfoFromJson(playerInfo, "audioRenderInfos", uidSet, audioPlayerInfos_);
    // Json::Value arrayObj = playerInfo["audioRenderInfos"];
    // int32_t uid;
    // for (uint32_t i = 0; i < arrayObj.size(); i++) {
    //     uid = arrayObj[i]["uid"].asInt();
    //     uidSet.emplace(uid);
    //     auto record = std::make_shared<AudioInfo>(uid, arrayObj[i]["sessionId"].asInt());
    //     audioPlayerInfos_.emplace_back(record);
    // }

    // nlohmann::json = playerInfo["AVSessionInfos"];
    int32_t uid;
    int32_t pid;
    std::string sessionId;
    for (auto &item : playerInfo["AVSessionInfos"]) {
        uid = (*iter).at("uid").get<int32_t>();
        pid = (*iter).at("pid").get<int32_t>();
        sessionId = (*iter).at("sessionId").get<std::string>();
        uidSet.emplace(uid);
        auto record = std::make_shared<AVSessionInfo>(uid, pid, sessionId);
        avSessionInfos_.emplace_back(record);
    }

    // arrayObj = value["audioRecorder"];
    // for (uint32_t i = 0; i < arrayObj.size(); i++) {
    //     uid = arrayObj[i]["uid"].asInt();
    //     uidSet.emplace(uid);
    //     auto record = std::make_shared<AudioInfo>(uid, arrayObj[i]["sessionId"].asInt());
    //     audioRecorderInfos_.emplace_back(record);
    // }
    ParseAudioStreamInfoFromJson(value, "audioRecorder", uidSet, audioRecorderInfos_);
    return true;
}

bool AudioDetect::CheckAudioCondition(int32_t uid, uint32_t taskType)
{
    if (taskType == AUDIO_PLAYBACK_BGMODE_ID) {
#ifdef AV_SESSION_PART_ENABLE
        return CommonUtils::CheckIsUidExist(uid, audioPlayerInfos_)
            && CommonUtils::CheckIsUidExist(uid, avSessionInfos_);
#else
        return CommonUtils::CheckIsUidExist(uid, audioPlayerInfos_);
#endif
    } else {
        return CommonUtils::CheckIsUidExist(uid, audioRecorderInfos_);
    }
}

void AudioDetect::ClearData()
{
    audioPlayerInfos_.clear();
    audioRecorderInfos_.clear();
}
}
}