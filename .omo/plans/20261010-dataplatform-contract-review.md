# 数据平台 API 契约 v1 评审结论（UXO_v1 侧）

日期：2026-10-10
评审对象：`UXO_DataPlatform/docs/api/v1-contract.md` v1.0.3（评审导读：同目录 `v1-contract-review-guide.md`）
评审人：UXO_v1 agent 会话（待用户确认）
工作树：`/home/lin/UXO_v1-dataplatform`（分支 `feature/data-platform-integration`，基于 main 6800091）

> 注意：契约中的 "REQ-002" 是 **UXO_DataPlatform 仓** 的需求 ID，与本仓 `docs/requirements/REQ-002.md`（模拟设备指派，Draft）同名不同物，后续引用必须区分仓库。

## 结论

**通过（附非阻断意见）**。契约语义足以支撑客户端适配器实现，无需修改项（无"必须改"意见）。

依据：

1. 契约内部自洽：端点总表、字段表、错误码表、状态码表交叉一致；词表 9 表 61 项与表 4-1 code 区间算术闭合（12+5+7+5+9+8+7+5+3=61）。
2. 与本仓领域模型（`include/Core/Data/Types.h`）逐项对齐：八个枚举的值域大小与契约词表项数全部一致（TargetType 12、ThreatLevel 5、TargetStatus 7、MissionType 5、MissionStatus 9、DeviceType 8、DeviceStatus 7、AlarmLevel 5）；五实体字段与客户端结构体字段一一对应，无缺项。
3. 检测链路对齐已实证：客户端 `assets/models/yolov8_cls_224.onnx` 元数据的 9 个模型类（aircraft-bombs/background/fuzes/grenades/landmines/mortars/projectiles/rockets/submunitions）与契约表 5-1 取值域完全一致；PatchCore 异常分 + YOLO 分类双阶段输出可直填 detection-records 的 `has_anomaly`/`class_name`/`confidence`/`raw_result`。
4. 服务端已实现并经 154 项测试锁定（含响应键集快照），契约语义已被实现验证，非纸面契约。

## 导读 §4 九项核对逐项表态

| # | 核对项 | 表态 | 理由 |
|---|--------|------|------|
| 1 | UUID 输入两形式、输出 36 位小写 | 同意 | `QUuid::toString()` 花括号形式可直送，对 Qt 客户端友好；适配器统一以规范形式存储 |
| 2 | id 客户端生成 | 同意 | `QUuid::createUuid()` 满足；单席位低频下冲突风险可忽略，409 兜底语义明确 |
| 3 | PUT 全量更新 | 同意 | 客户端内存中持有完整结构体（TargetInfo 等），天然具备全量表示；检测复核等局部更新场景由适配器先 GET 合并，属适配器职责 |
| 4 | create_time 保留 / update_time 服务端刷新 | 同意 | 客户端无需维护这两类时间戳，减少时钟一致性负担 |
| 5 | 恰三个可缺省字段 | 同意 | 与字段表一致；POST 构造路径清晰 |
| 6 | 错误判断以 field+code 为准 | 同意 | 标准做法；适配器将 code 映射为内部错误枚举，message 仅展示 |
| 7 | 无并发控制、无删除端点 | 同意 | 与单席位低频假设一致；目标软删除由 target_status=FalseAlarm 覆盖 |
| 8 | 精确匹配 + AND 起步过滤集 | 同意（附意见 1） | 起步集覆盖当前列表页需求；时间范围过滤的缺少见意见 1 |
| 9 | 400 聚合全部字段错误 | 同意 | 适配器一次映射全部错误字段，表单友好 |

另表态：§6.0 业务状态机不在服务层强制——同意。状态流转合法性由客户端 Domain 层负责，与本仓 ARCHITECTURE 状态机归属一致；契约 §1.9 审计留痕提供事后追溯。

## 意见清单（按导读 §6 格式：章节 / 问题 / 建议）

**意见 1**｜§7 分页与过滤 / 起步过滤集无时间范围过滤（如 `detect_time` 区间、`create_time` 区间）。当前客户端列表页无按时间段查询需求，不阻断适配器实现；但后续历史/审计类视图几乎必然需要，而过滤集扩展须经契约评审。建议：**需讨论**——作为已知演进点记录于双仓 backlog，本期不变更。

**意见 2**｜§6.4 alarms / `type` 为自由文本（契约自注"词表待定"）。自由文本长期会发散，跨端统计与过滤可靠性差。建议：**需讨论**——后续版本补 alarm_type 词表（v1 加法演进可兼容）；本期客户端内部以常量集自律。

**意见 3**｜§4/§5 词表与映射明细不入契约（正典单源）。客户端枚举序数与词表 code 的对应关系目前只有示例级证据（code 1 = AirBomb 与本仓 TargetType::AirBomb=1 一致），61 项明细只能在运行时经端点核验。建议：**建议改（客户端侧采纳即可，不要求契约变更）**——适配器以词表端点 `name_en` 为映射锚点，不硬编码序数；集成测试锁定全量对应关系。

无"必须改"意见。

## 适配器实现注意事项（非契约意见，开发备忘录）

1. **Qt JSON 数字为 double**：契约类型严格（§1.3 整数必须是整数字面量）；序列化整数字段须用整型 QVariant，解析整数须校验，禁止直接 `toInt()` 静默截断浮点。
2. **battery_level 精度**：契约整数 0–100，本仓 `DeviceInfo::batteryLevel` 为 double；适配器负责四舍五入与值域校验。
3. **missions.result 空值语义**：契约可空（null = 无结果），本仓 `MissionInfo::result` 为 int 默认 0；适配器需定义 null↔0 映射，避免把"无结果"写成 0 产生歧义。
4. **detection-records.raw_result 必填**：本仓 `DetectionResult` 仅 type/videoRect/confidence 三字段，上报检测记录需要引擎侧提供更完整原始载荷（ONNX 输出）；空载荷场景至少送 `{}`。
5. **health 的 503 响应体与其他端点不同**（§3 专属形状 vs §8 错误体）：适配器健康探测路径单独解析。
6. **模拟数据不得写入真实库**：本仓模拟链（SimulationWorkflow/DemoScenarioProvider）产出的 TargetType 中 Unknown/ClusterBomb/CruiseMissile/IED/Other 等无 model_class 映射行；适配器默认只接真实检测链路上报，模拟上报通道须显式标注且只连本地实例。
7. **服务端默认仅回环监听 127.0.0.1:8080**：适配器基址经 `UXO_API_HOST`/`UXO_API_PORT` 配置，默认回环；符合本仓安全边界（不连生产库）。

## 本仓门禁状态与后续步骤

当前**不具备**进入实现的门禁条件（AGENTS.md / docs/requirements/README.md）：

- 本仓需求注册表无"数据平台接入"需求；NEXT 切片明确排除"外部通信、持久化"。需新注册 REQ（预计 REQ-012）并置 `Approved`。
- 需对应 `docs/features/<feature>.md` 状态 `Approved`。
- 安全边界：AGENTS.md 默认禁止数据库写入；本次接入已由用户明确授权，但实现范围应限定为"客户端 ⇄ 本地数据平台实例"，不连生产库、不接真实设备。

建议步骤：

1. 用户确认本评审结论 → 结论由数据平台仓记入契约 §9.3（本仓不写数据库侧文件）。
2. 本仓注册 REQ-012（数据平台接入）+ 功能设计草案 → 用户批准。
3. 在本工作树（`feature/data-platform-integration`）实现适配器，架构归属 TARGET `Integration` 库（ARCHITECTURE §5，按需增量引入）。
