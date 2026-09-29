# Spec 增量设计

> 本文档描述对长时任务规格的增量变更，使用 ADDED/MODIFIED/REMOVED 标记。
> 基础规格参考：`docs/spec/continuous_task.md`

---

## ADDED Requirements

### 5.1 startContinuousTask 接口

#### 5.1.1 业务规则

1. **接口声明**：新增 `startContinuousTask(context: Context, request: ContinuousTaskRequest): Promise<int>` 接口，从 API26 开始提供。
   - **验收条件**：应用调用 `startContinuousTask` → 返回 `Promise<int>`，返回值为有效的 `continuousTaskId`
   - **验收条件**：`startContinuousTask` 的功能行为（权限校验、模式验证、通知发送、订阅者通知、持久化、互斥检查）必须与 `startBackgroundRunning` 完全一致

2. **返回值规则**：`startContinuousTask` 返回 `int` 类型的 `continuousTaskId`，不返回 `notificationId`、`slotType`、`contentType`。
   - **验收条件**：调用成功 → `continuousTaskId` 为有效正整数
   - **验收条件**：调用失败 → Promise reject，携带对应错误码

3. **progressInfo 字段生效规则**：`ContinuousTaskRequest.progressInfo` 字段仅在 `startContinuousTask` 接口中合法生效。在 `startContinuousTask` 中，仅当任务包含 `DATA_TRANSFER` 后台模式时 progressInfo 生效；若任务不包含 `DATA_TRANSFER` 模式但传入了 progressInfo，接口正常调用，progressInfo 字段不生效（静默忽略）。
   - **验收条件**：通过 `startContinuousTask` 传入 `progressInfo` 且任务包含 `DATA_TRANSFER` 模式 → 正常处理，进度信息生效
   - **验收条件**：通过 `startContinuousTask` 传入 `progressInfo` 且任务不包含 `DATA_TRANSFER` 模式 → 正常处理，progressInfo 字段不生效（静默忽略）
   - **验收条件**：通过 `startContinuousTask` 不传入 `progressInfo`（字段为 null） → 正常处理，不影响任务申请

4. **停止与更新规则**：通过 `startContinuousTask` 申请的长时任务，停止操作复用 `stopBackgroundRunning`，更新操作复用 `updateBackgroundRunning`。
   - **验收条件**：`startContinuousTask` 申请的任务可通过 `stopBackgroundRunning` 正常停止
   - **验收条件**：`startContinuousTask` 申请的任务可通过 `updateBackgroundRunning` 正常更新

#### 5.1.2 交互流程

```mermaid
sequenceDiagram
    participant App as 应用
    participant NAPI as NAPI 接口
    participant Proxy as BackgroundTaskMgrProxy
    participant Stub as BackgroundTaskMgrStub
    participant Mgr as BgContinuousTaskMgr
    participant Notif as 通知子系统

    App->>NAPI: startContinuousTask(context, request)
    NAPI->>Proxy: StartContinuousTask(taskParam)
    Proxy->>Stub: IPC 调用
    Stub->>Mgr: StartContinuousTask()
    Mgr->>Mgr: 权限校验 + 模式验证
    Mgr->>Mgr: 创建 ContinuousTaskRecord
    Mgr->>Notif: 发送通知
    Mgr->>Mgr: 持久化记录
    Mgr-->>Stub: 返回 continuousTaskId
    Stub-->>Proxy: IPC 返回
    Proxy-->>NAPI: 返回 continuousTaskId
    NAPI-->>App: Promise<int>
```

#### 5.1.3 异常场景

1. **场景：progressInfo 在 startBackgroundRunning 中传入**
   - **触发条件**：应用调用 `startBackgroundRunning` 时 `ContinuousTaskRequest.progressInfo` 不为 null
   - **系统行为**：接口正常调用，progressInfo 字段不生效（静默忽略，置为 null）
   - **用户感知**：接口正常返回，progressInfo 不影响任务申请

2. **场景：服务未就绪**
   - **触发条件**：`startContinuousTask` 调用时服务未就绪（`isReady_=false`）
   - **系统行为**：拒绝请求，返回 `ERR_BGTASK_SYS_NOT_READY` 错误码
   - **用户感知**：Promise reject，错误码 `ERR_BGTASK_SYS_NOT_READY`

3. **场景：权限校验失败**
   - **触发条件**：应用未声明 `ohos.permission.KEEP_BACKGROUND_RUNNING` 权限
   - **系统行为**：拒绝请求，返回权限错误码
   - **用户感知**：Promise reject，权限错误码

---

## MODIFIED Requirements

### 5.2 startBackgroundRunning 接口（修改）

#### 5.2.1 业务规则

1. **progressInfo 字段校验规则**：`startBackgroundRunning` 接口入参 `ContinuousTaskRequest` 中的 `progressInfo` 字段不生效。若传入非 null 值，接口正常调用，progressInfo 被静默忽略（置为 null）。  ← (原为: `ContinuousTaskRequest.progressInfo` 字段无校验规则)
   - **验收条件**：`startBackgroundRunning` 传入 `progressInfo != null` → 正常处理，接口正常返回，progressInfo 不生效
   - **验收条件**：`startBackgroundRunning` 传入 `progressInfo == null`（或不设置） → 正常处理，行为不变

---

## REMOVED Requirements

无删除项。

---

## 数据约束变更

### ADDED

无新增数据模型。

### MODIFIED

#### 6.1 ContinuousTaskRequest

1. **progressInfo_** 字段约束修改：该字段仅在 `startContinuousTask` 接口中生效，且仅当任务包含 `DATA_TRANSFER` 后台模式时生效。在 `startBackgroundRunning` 接口中传入非 null 值时静默忽略（置为 null）。在 `startContinuousTask` 接口中传入非 null 值但任务不包含 `DATA_TRANSFER` 模式时也静默忽略。  ← (原为: `progressInfo_` 字段无使用场景约束)

---

## 术语变更

### ADDED

无新增术语。

---

## 合并检查清单

- [ ] ADDED 内容已添加到 `docs/spec/continuous_task.md` 对应章节
- [ ] MODIFIED 内容已替换 `docs/spec/continuous_task.md` 中 `startBackgroundRunning` 接口说明
- [ ] 接口说明表新增 `startContinuousTask` 行
- [ ] API 版本演进新增 API26 条目
- [ ] mermaid 图表可正常渲染
