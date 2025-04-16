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

#ifndef FOUNDATION_RESOURCESCHEDULE_BACKGROUND_TASK_MGR_FRAMEWORKS_COMMON_INCLUDE_BGTASKMGR_INNER_ERRORS_H
#define FOUNDATION_RESOURCESCHEDULE_BACKGROUND_TASK_MGR_FRAMEWORKS_COMMON_INCLUDE_BGTASKMGR_INNER_ERRORS_H

#include "errors.h"
#include <map>

namespace OHOS {
namespace BackgroundTaskMgr {
/**
 * ErrCode layout
 *
 * +--+--+--+--+--+--+--+--+---+---+
 * |09|08|07|06|05|04|03|02| 01| 00|
 * +--+--+--+--+--+--+--+--+---+---+
 * | Syscap |      Code    |Subcode|
 * +--+--+--+--+--+--+--+--+---+---+
 */
const int OFFSET = 100;
const int THRESHOLD = 1000;
// Bgtask Common Error Code Defined.
enum : int32_t {
    // errcode for common
    ERR_BGTASK_PERMISSION_DENIED = 201,
    ERR_BGTASK_NOT_SYSTEM_APP = 202,
    ERR_BGTASK_INVALID_PARAM = 401,
    // errcode for Continuous Task
    ERR_BGTASK_NO_MEMORY = 980000101,
    ERR_BGTASK_PARCELABLE_FAILED = 980000201,
    ERR_BGTASK_TRANSACT_FAILED = 980000301,
    ERR_BGTASK_SYS_NOT_READY = 980000401,
    ERR_BGTASK_SERVICE_NOT_CONNECTED,
    ERR_BGTASK_OBJECT_EXISTS = 980000501,
    ERR_BGTASK_OBJECT_NOT_EXIST,
    ERR_BGTASK_KEEPING_TASK_VERIFY_ERR,
    ERR_BGTASK_INVALID_BGMODE,
    ERR_BGTASK_INVALID_UID,
    ERR_BGTASK_NOTIFICATION_VERIFY_FAILED = 980000601,
    ERR_BGTASK_NOTIFICATION_ERR,
    ERR_BGTASK_CHECK_TASK_PARAM,
    ERR_BGTASK_CREATE_FILE_ERR = 980000701,
    ERR_BGTASK_GET_ACTUAL_FILE_ERR,
    ERR_BGTASK_OPEN_FILE_ERR,
    // errcode for Transient Task
    ERR_BGTASK_INVALID_PID_OR_UID = 990000101,
    ERR_BGTASK_INVALID_BUNDLE_NAME,
    ERR_BGTASK_INVALID_REQUEST_ID,
    ERR_BGTASK_INVALID_CALLBACK = 990000201,
    ERR_BGTASK_CALLBACK_EXISTS,
    ERR_BGTASK_CALLBACK_NOT_EXIST,
    ERR_BGTASK_NOT_IN_PRESET_TIME,
    ERR_BGTASK_EXCEEDS_THRESHOLD,
    ERR_BGTASK_TIME_INSUFFICIENT,
    ERR_BGTASK_NOREQUEST_TASK,
    ERR_BGTASK_FOREGROUND,
    // errcode for Efficiency Resource
    ERR_BGTASK_RESOURCES_EXCEEDS_MAX = 1870000101,
    ERR_BGTASK_RESOURCES_INVALID_PID_OR_UID,
    // other inner errcode
    ERR_BGTASK_METHOD_CALLED_FAILED,
    ERR_BGTASK_DATA_STORAGE_ERR,
    ERR_BGTASK_SERVICE_INNER_ERROR,
    ERR_BGTASK_INVALID_PROCESS_NAME,
};

enum ParamErr: int32_t {
    ERR_PARAM_NUMBER_ERR = 9800401,
    ERR_REASON_NULL_OR_TYPE_ERR,
    ERR_CALLBACK_NULL_OR_TYPE_ERR,
    ERR_REQUESTID_NULL_OR_ID_TYPE_ERR,
    ERR_REQUESTID_ILLEGAL,
    ERR_CONTEXT_NULL_OR_TYPE_ERR,
    ERR_BGMODE_NULL_OR_TYPE_ERR,
    ERR_WANTAGENT_NULL_OR_TYPE_ERR,
    ERR_ABILITY_INFO_EMPTY,
    ERR_GET_TOKEN_ERR,
    ERR_BGMODE_RANGE_ERR,
    ERR_APP_NAME_EMPTY,
    ERR_RESOURCE_TYPES_INVALID,
    ERR_ISAPPLY_NULL_OR_TYPE_ERR,
    ERR_TIMEOUT_INVALID,
    ERR_ISPERSIST_NULL_OR_TYPE_ERR,
    ERR_ISPROCESS_NULL_OR_TYPE_ERR,
};
}  // namespace BackgroundTaskMgr
}  // namespace OHOS
#endif  // FOUNDATION_RESOURCESCHEDULE_BACKGROUND_TASK_MGR_FRAMEWORKS_COMMON_INCLUDE_BGTASKMGR_INNER_ERRORS_H