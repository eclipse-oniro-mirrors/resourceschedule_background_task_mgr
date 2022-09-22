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

#include "resource_record_storage.h"

#include <fcntl.h>
#include <fstream>
#include <unistd.h>
#include <climits>
#include <sys/stat.h>

#include "errors.h"
#include "bundle_manager_helper.h"
#include "common_utils.h"

#include "bgtaskmgr_inner_errors.h"
#include "efficiency_resource_log.h"
#include "resource_application_record.h"

namespace OHOS {
namespace BackgroundTaskMgr {
namespace {
const std::string RESOURCE_RECORD_FILE_PATH = "/data/service/el1/public/background_task_mgr/resource_record";
const std::string APP_RESOURCE_RECORD = "appResourceRecord";
const std::string PROCESS_RESOURCE_RECORD = "processResourceRecord";
static constexpr int32_t MAX_BUFFER = 512;
}

ErrCode ResourceRecordStorage::RefreshResourceRecord(const ResourceRecordMap &appRecord,
    const ResourceRecordMap &processRecord)
{
    std::string record {""};
    ConvertMapToString(appRecord, processRecord, record);
    return WriteStringToFile(record, RESOURCE_RECORD_FILE_PATH);
}

ErrCode ResourceRecordStorage::RestoreResourceRecord(ResourceRecordMap &appRecord,
    ResourceRecordMap &processRecord)
{
    std::string recordString {""};
    if (ReadStringFromFile(recordString, RESOURCE_RECORD_FILE_PATH) != ERR_OK) {
        BGTASK_LOGD("can not read string form file: %{public}s", RESOURCE_RECORD_FILE_PATH.c_str());
        return ERR_BGTASK_DATA_STORAGE_ERR;
    }
    nlohmann::json root;
    return ConvertStringToMap(recordString, appRecord, processRecord);
}

void ResourceRecordStorage::ConvertMapToString(const ResourceRecordMap &appRecord,
    const ResourceRecordMap &processRecord, std::string &recordString)
{
    nlohmann::json root;
    nlohmann::json appValue;
    ConvertMapToJson(appRecord, appValue);
    root[APP_RESOURCE_RECORD] = appValue;
    nlohmann::json processValue;
    ConvertMapToJson(processRecord, processValue);
    root[PROCESS_RESOURCE_RECORD] = processValue;
    recordString = root.dump(CommonUtils::JSON_FORMAT);
}

void ResourceRecordStorage::ConvertMapToJson(const ResourceRecordMap &appRecord, nlohmann::json &root)
{
    for (const auto &iter : appRecord) {
        nlohmann::json value;
        iter.second->ParseToJson(value);
        root[std::to_string(iter.first)] = value;
    }
}

ErrCode ResourceRecordStorage::ConvertStringToMap(const std::string &recordString,
    ResourceRecordMap &appRecord, ResourceRecordMap &processRecord)
{
    nlohmann::json appRecordJson;
    nlohmann::json processrecordJson;
    if (ConvertStringToJson(recordString, appRecordJson, processrecordJson) != ERR_OK) {
        return ERR_BGTASK_DATA_STORAGE_ERR;
    }
    ConvertJsonToMap(appRecordJson, appRecord);
    ConvertJsonToMap(processrecordJson, processRecord);
    return ERR_OK;
}

void ResourceRecordStorage::ConvertJsonToMap(const nlohmann::json &value, ResourceRecordMap &recordMap)
{
    for (auto iter = value.begin(); iter != value.end(); iter++) {
        std::shared_ptr<ResourceApplicationRecord> recordPtr = std::make_shared<ResourceApplicationRecord>();
        recordPtr->ParseFromJson(iter.value());
        recordMap.emplace(static_cast<uint32_t>(std::stoi(iter.key())), recordPtr);
    }
}

ErrCode ResourceRecordStorage::ConvertStringToJson(const std::string &recordString,
    nlohmann::json &appRecord, nlohmann::json &processRecord)
{
    nlohmann::json root = nlohmann::json::parse(recordString, nullptr, false);
    if (root.is_null() || root.is_discarded()) {
        BGTASK_LOGE("failed due to data is discarded or record is null");
        return ERR_BGTASK_DATA_STORAGE_ERR;
    }
    appRecord = root.at(APP_RESOURCE_RECORD);
    processRecord = root.at(PROCESS_RESOURCE_RECORD);
    return ERR_OK;
}

bool ResourceRecordStorage::CreateNodeFile(const std::string &filePath)
{
    if (access(filePath.c_str(), F_OK) == ERR_OK) {
        BGTASK_LOGD("the file: %{public}s already exists", RESOURCE_RECORD_FILE_PATH.c_str());
        return true;
    }
    int32_t fd = open(filePath.c_str(), O_CREAT|O_RDWR, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if (fd < ERR_OK) {
        BGTASK_LOGE("Fail to open file: %{public}s", RESOURCE_RECORD_FILE_PATH.c_str());
        return false;
    }
    close(fd);
    return true;
}

bool ResourceRecordStorage::ConvertFullPath(const std::string& partialPath, std::string& fullPath)
{
    if (partialPath.empty() || partialPath.length() >= PATH_MAX) {
        return false;
    }
    char tmpPath[PATH_MAX] = {0};
    if (realpath(partialPath.c_str(), tmpPath) == nullptr) {
        return false;
    }
    fullPath = tmpPath;
    return true;
}

ErrCode ResourceRecordStorage::WriteStringToFile(const std::string &result, const std::string &filePath)
{
    if (!CreateNodeFile(filePath)) {
        BGTASK_LOGE("Create file failed");
        return ERR_BGTASK_DATA_STORAGE_ERR;
    }
    std::ofstream fout;
    std::string realPath;
    if (!ConvertFullPath(filePath, realPath)) {
        BGTASK_LOGE("Get real path failed");
        return ERR_BGTASK_DATA_STORAGE_ERR;
    }
    fout.open(realPath, std::ios::out);
    if (!fout.is_open()) {
        BGTASK_LOGE("Open file failed");
        return ERR_BGTASK_DATA_STORAGE_ERR;
    }
    fout << result.c_str() << std::endl;
    fout.close();
    return ERR_OK;
}

ErrCode ResourceRecordStorage::ReadStringFromFile(std::string &result, const std::string &filePath)
{
    std::ifstream fin;
    std::string realPath;
    if (!ConvertFullPath(filePath, realPath)) {
        BGTASK_LOGE("Get real path failed");
        return ERR_BGTASK_DATA_STORAGE_ERR;
    }
    fin.open(realPath, std::ios::in);
    if (!fin.is_open()) {
        BGTASK_LOGE("cannot open file %{public}s", realPath.c_str());
        return ERR_BGTASK_DATA_STORAGE_ERR;
    }
    char buffer[MAX_BUFFER];
    std::ostringstream os;
    while (!fin.eof()) {
        fin.getline(buffer, MAX_BUFFER);
        os << buffer;
    }
    result = os.str();
    fin.close();
    return ERR_OK;
}
}
}