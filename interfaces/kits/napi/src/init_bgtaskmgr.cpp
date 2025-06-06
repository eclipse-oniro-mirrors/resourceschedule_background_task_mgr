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

#include "init_bgtaskmgr.h"

#include "background_mode.h"
#include "background_sub_mode.h"
#include "bg_continuous_task_napi_module.h"
#include "cancel_suspend_delay.h"
#include "continuous_task_cancel_reason.h"
#include "continuous_task_suspend_reason.h"
#include "get_all_transient_tasks.h"
#include "get_remaining_delay_time.h"
#include "request_suspend_delay.h"
#include "transient_task_log.h"
#include "efficiency_resources_operation.h"
#include "resource_type.h"
#include "common.h"

namespace OHOS {
namespace BackgroundTaskMgr {
    static constexpr char BG_TASK_SUB_MODE_TYPE[] = "subMode";
EXTERN_C_START

napi_value BackgroundTaskMgrInit(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("requestSuspendDelay", RequestSuspendDelayThrow),
        DECLARE_NAPI_FUNCTION("cancelSuspendDelay", CancelSuspendDelayThrow),
        DECLARE_NAPI_FUNCTION("getRemainingDelayTime", GetRemainingDelayTimeThrow),
        DECLARE_NAPI_FUNCTION("getTransientTaskInfo", GetAllTransientTasksThrow),
        DECLARE_NAPI_FUNCTION("startBackgroundRunning", StartBackgroundRunningThrow),
        DECLARE_NAPI_FUNCTION("updateBackgroundRunning", UpdateBackgroundRunningThrow),
        DECLARE_NAPI_FUNCTION("stopBackgroundRunning", StopBackgroundRunningThrow),
        DECLARE_NAPI_FUNCTION("getAllContinuousTasks", GetAllContinuousTasksThrow),
        DECLARE_NAPI_FUNCTION("applyEfficiencyResources", ApplyEfficiencyResources),
        DECLARE_NAPI_FUNCTION("resetAllEfficiencyResources", ResetAllEfficiencyResources),
        DECLARE_NAPI_FUNCTION("getAllEfficiencyResources", GetAllEfficiencyResources),
        DECLARE_NAPI_FUNCTION("on", OnOnContinuousTaskCallback),
        DECLARE_NAPI_FUNCTION("off", OffOnContinuousTaskCallback),
    };

    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));

    return exports;
}

void SetNamedPropertyByInteger(napi_env env, napi_value dstObj, int32_t objName, const char *propName)
{
    napi_value prop = nullptr;
    if (napi_create_int32(env, objName, &prop) == napi_ok) {
        napi_set_named_property(env, dstObj, propName, prop);
    }
}

void SetNamedPropertyByString(napi_env env, napi_value dstObj, const char *objName, const char *propName)
{
    napi_value prop = nullptr;
    if (napi_create_string_utf8(env, objName, strlen(objName), &prop) == napi_ok) {
        napi_set_named_property(env, dstObj, propName, prop);
    }
}

napi_value BackgroundModeInit(napi_env env, napi_value exports)
{
    napi_value obj = nullptr;
    napi_create_object(env, &obj);

    SetNamedPropertyByInteger(env, obj, static_cast<uint32_t>(BackgroundMode::DATA_TRANSFER), "DATA_TRANSFER");
    SetNamedPropertyByInteger(env, obj, static_cast<uint32_t>(BackgroundMode::AUDIO_PLAYBACK), "AUDIO_PLAYBACK");
    SetNamedPropertyByInteger(env, obj, static_cast<uint32_t>(BackgroundMode::AUDIO_RECORDING),
        "AUDIO_RECORDING");
    SetNamedPropertyByInteger(env, obj, static_cast<uint32_t>(BackgroundMode::LOCATION),
        "LOCATION");
    SetNamedPropertyByInteger(env, obj, static_cast<uint32_t>(BackgroundMode::BLUETOOTH_INTERACTION),
        "BLUETOOTH_INTERACTION");
    SetNamedPropertyByInteger(env, obj, static_cast<uint32_t>(BackgroundMode::MULTI_DEVICE_CONNECTION),
        "MULTI_DEVICE_CONNECTION");
    SetNamedPropertyByInteger(env, obj, static_cast<uint32_t>(BackgroundMode::WIFI_INTERACTION), "WIFI_INTERACTION");
    SetNamedPropertyByInteger(env, obj, static_cast<uint32_t>(BackgroundMode::VOIP), "VOIP");
    SetNamedPropertyByInteger(env, obj, static_cast<uint32_t>(BackgroundMode::TASK_KEEPING), "TASK_KEEPING");

    SetNamedPropertyByInteger(env, obj, static_cast<uint32_t>(ResourceType::CPU), "CPU");
    SetNamedPropertyByInteger(env, obj, static_cast<uint32_t>(ResourceType::COMMON_EVENT), "COMMON_EVENT");
    SetNamedPropertyByInteger(env, obj, static_cast<uint32_t>(ResourceType::TIMER), "TIMER");
    SetNamedPropertyByInteger(env, obj, static_cast<uint32_t>(ResourceType::WORK_SCHEDULER), "WORK_SCHEDULER");
    SetNamedPropertyByInteger(env, obj, static_cast<uint32_t>(ResourceType::BLUETOOTH), "BLUETOOTH");
    SetNamedPropertyByInteger(env, obj, static_cast<uint32_t>(ResourceType::GPS), "GPS");
    SetNamedPropertyByInteger(env, obj, static_cast<uint32_t>(ResourceType::AUDIO), "AUDIO");
    SetNamedPropertyByInteger(env, obj, static_cast<uint32_t>(ResourceType::RUNNING_LOCK), "RUNNING_LOCK");
    SetNamedPropertyByInteger(env, obj, static_cast<uint32_t>(ResourceType::SENSOR), "SENSOR");

    SetNamedPropertyByInteger(env, obj, static_cast<uint32_t>(BackgroundSubMode::CAR_KEY), "CAR_KEY");
    SetNamedPropertyByString(env, obj, BG_TASK_SUB_MODE_TYPE, "SUB_MODE");

    napi_property_descriptor exportFuncs[] = {
        DECLARE_NAPI_PROPERTY("BackgroundMode", obj),
        DECLARE_NAPI_PROPERTY("ResourceType", obj),
        DECLARE_NAPI_PROPERTY("BackgroundSubMode", obj),
        DECLARE_NAPI_PROPERTY("BackgroundModeType", obj),
    };

    napi_define_properties(env, exports, sizeof(exportFuncs) / sizeof(*exportFuncs), exportFuncs);
    return exports;
}

napi_value ContinuousTaskCancelReasonInit(napi_env env, napi_value exports)
{
    napi_value obj = nullptr;
    napi_create_object(env, &obj);
    SetNamedPropertyByInteger(env, obj, static_cast<uint32_t>(ContinuousTaskCancelReason::USER_CANCEL), "USER_CANCEL");
    SetNamedPropertyByInteger(env, obj, static_cast<uint32_t>(ContinuousTaskCancelReason::SYSTEM_CANCEL),
        "SYSTEM_CANCEL");
    SetNamedPropertyByInteger(env, obj, static_cast<uint32_t>(
        ContinuousTaskCancelReason::USER_CANCEL_REMOVE_NOTIFICATION), "USER_CANCEL_REMOVE_NOTIFICATION");
    SetNamedPropertyByInteger(env, obj, static_cast<uint32_t>(
        ContinuousTaskCancelReason::SYSTEM_CANCEL_DATA_TRANSFER_LOW_SPEED), "SYSTEM_CANCEL_DATA_TRANSFER_LOW_SPEED");
    SetNamedPropertyByInteger(env, obj, static_cast<uint32_t>(
        ContinuousTaskCancelReason::SYSTEM_CANCEL_AUDIO_PLAYBACK_NOT_USE_AVSESSION),
        "SYSTEM_CANCEL_AUDIO_PLAYBACK_NOT_USE_AVSESSION");
    SetNamedPropertyByInteger(env, obj, static_cast<uint32_t>(
        ContinuousTaskCancelReason::SYSTEM_CANCEL_AUDIO_PLAYBACK_NOT_RUNNING),
        "SYSTEM_CANCEL_AUDIO_PLAYBACK_NOT_RUNNING");
    SetNamedPropertyByInteger(env, obj, static_cast<uint32_t>(
        ContinuousTaskCancelReason::SYSTEM_CANCEL_AUDIO_RECORDING_NOT_RUNNING),
        "SYSTEM_CANCEL_AUDIO_RECORDING_NOT_RUNNING");
    SetNamedPropertyByInteger(env, obj, static_cast<uint32_t>(
        ContinuousTaskCancelReason::SYSTEM_CANCEL_NOT_USE_LOCATION), "SYSTEM_CANCEL_NOT_USE_LOCATION");
    SetNamedPropertyByInteger(env, obj, static_cast<uint32_t>(
        ContinuousTaskCancelReason::SYSTEM_CANCEL_NOT_USE_BLUETOOTH), "SYSTEM_CANCEL_NOT_USE_BLUETOOTH");
    SetNamedPropertyByInteger(env, obj, static_cast<uint32_t>(
        ContinuousTaskCancelReason::SYSTEM_CANCEL_NOT_USE_MULTI_DEVICE), "SYSTEM_CANCEL_NOT_USE_MULTI_DEVICE");
    SetNamedPropertyByInteger(env, obj, static_cast<uint32_t>(
        ContinuousTaskCancelReason::SYSTEM_CANCEL_USE_ILLEGALLY), "SYSTEM_CANCEL_USE_ILLEGALLY");
    napi_property_descriptor exportFuncs[] = {
        DECLARE_NAPI_PROPERTY("ContinuousTaskCancelReason", obj),
    };
    napi_define_properties(env, exports, sizeof(exportFuncs) / sizeof(*exportFuncs), exportFuncs);
    return exports;
}

napi_value ContinuousTaskSuspendReasonInit(napi_env env, napi_value exports)
{
    napi_value obj = nullptr;
    napi_create_object(env, &obj);
    SetNamedPropertyByInteger(env, obj, static_cast<uint32_t>(
        ContinuousTaskSuspendReason::SYSTEM_SUSPEND_DATA_TRANSFER_LOW_SPEED),
        "SYSTEM_SUSPEND_DATA_TRANSFER_LOW_SPEED");
    SetNamedPropertyByInteger(env, obj, static_cast<uint32_t>(
        ContinuousTaskSuspendReason::SYSTEM_SUSPEND_AUDIO_PLAYBACK_NOT_USE_AVSESSION),
        "SYSTEM_SUSPEND_AUDIO_PLAYBACK_NOT_USE_AVSESSION");
    SetNamedPropertyByInteger(env, obj, static_cast<uint32_t>(
        ContinuousTaskSuspendReason::SYSTEM_SUSPEND_AUDIO_PLAYBACK_NOT_RUNNING),
        "SYSTEM_SUSPEND_AUDIO_PLAYBACK_NOT_RUNNING");
    SetNamedPropertyByInteger(env, obj, static_cast<uint32_t>(
        ContinuousTaskSuspendReason::SYSTEM_SUSPEND_AUDIO_RECORDING_NOT_RUNNING),
        "SYSTEM_SUSPEND_AUDIO_RECORDING_NOT_RUNNING");
    SetNamedPropertyByInteger(env, obj, static_cast<uint32_t>(
        ContinuousTaskSuspendReason::SYSTEM_SUSPEND_LOCATION_NOT_USED), "SYSTEM_SUSPEND_LOCATION_NOT_USED");
    SetNamedPropertyByInteger(env, obj, static_cast<uint32_t>(
        ContinuousTaskSuspendReason::SYSTEM_SUSPEND_BLUETOOTH_NOT_USED), "SYSTEM_SUSPEND_BLUETOOTH_NOT_USED");
    SetNamedPropertyByInteger(env, obj, static_cast<uint32_t>(
        ContinuousTaskSuspendReason::SYSTEM_SUSPEND_MULTI_DEVICE_NOT_USED), "SYSTEM_SUSPEND_MULTI_DEVICE_NOT_USED");
    SetNamedPropertyByInteger(env, obj, static_cast<uint32_t>(
        ContinuousTaskSuspendReason::SYSTEM_SUSPEND_USED_ILLEGALLY), "SYSTEM_SUSPEND_USED_ILLEGALLY");
    SetNamedPropertyByInteger(env, obj, static_cast<uint32_t>(
        ContinuousTaskSuspendReason::SYSTEM_SUSPEND_SYSTEM_LOAD_WARNING), "SYSTEM_SUSPEND_SYSTEM_LOAD_WARNING");
    napi_property_descriptor exportFuncs[] = {
        DECLARE_NAPI_PROPERTY("ContinuousTaskSuspendReason", obj),
    };
    napi_define_properties(env, exports, sizeof(exportFuncs) / sizeof(*exportFuncs), exportFuncs);
    return exports;
}

/*
 * Module export function
 */
static napi_value InitApi(napi_env env, napi_value exports)
{
    /*
     * Properties define
     */
    BackgroundTaskMgrInit(env, exports);
    BackgroundModeInit(env, exports);
    ContinuousTaskCancelReasonInit(env, exports);
    ContinuousTaskSuspendReasonInit(env, exports);
    return exports;
}

/*
 * Module register function
 */
__attribute__((constructor)) void RegisterModule(void)
{
    napi_module_register(&g_apiModule);
}
EXTERN_C_END
}  // namespace BackgroundTaskMgr
}  // namespace OHOS
