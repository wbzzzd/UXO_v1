# 需求注册表

本目录是项目唯一需求注册表与状态权威。需求 ID 采用追加式、无语义的 `REQ-NNN` 顺序编号，一经分配不复用；被取代的需求保留原 ID。其他文档只能引用需求 ID，不得创建或重编号需求，也不得复制或修改需求状态。

## 状态定义

| 状态 | 含义 |
|------|------|
| `Draft` | 未批准，不得进入实现 |
| `Approved` | 可进入功能设计与计划 |
| `Implemented` | 验收结果有可验证证据 |
| `Superseded` | 历史，不再活跃 |

## NEXT 候选范围

**目标**：把当前单目标状态演示扩展为完整本地模拟指挥环。NEXT 是已确认产品边界但尚未批准的需求草案，以下 ID 与语义保留。REQ-008（探测阶段动态演示，时间线驱动方案）已被 REQ-009 取代，状态为 `Superseded`。REQ-009（无人机探测态势演示）与 REQ-010（UI 视觉治理）为 `Implemented`，实现事实见各自需求文件。其余需求全部状态为 `Draft`，不得直接指导实现。

NEXT 切片包含 REQ-001~REQ-006（均为 `Draft`）。REQ-007 为 NEXT 切片之外的独立候选（P0 `Implemented`，P1/P2 `Draft`），不并入 NEXT 切片。REQ-010 为 NEXT 切片之外的独立候选（UI 视觉治理，`Implemented`），不并入 NEXT 切片。

## 需求索引

| ID | 能力 | 状态 | 文件 |
|----|------|------|------|
| REQ-001 | 统一模拟会话状态 | `Draft` | [REQ-001.md](REQ-001.md) |
| REQ-002 | 模拟设备指派 | `Draft` | [REQ-002.md](REQ-002.md) |
| REQ-003 | 模拟任务执行 | `Draft` | [REQ-003.md](REQ-003.md) |
| REQ-004 | 跨面板一致反馈 | `Draft` | [REQ-004.md](REQ-004.md) |
| REQ-005 | 可复现场景 | `Draft` | [REQ-005.md](REQ-005.md) |
| REQ-006 | 完整操作记录 | `Draft` | [REQ-006.md](REQ-006.md) |
| REQ-007 | 最小应急起降带（MOS）规划 | `Implemented`（仅 P0；P1/P2 `Draft`） | [REQ-007.md](REQ-007.md) |
| REQ-008 | ~~探测阶段动态演示~~（时间线驱动） | `Superseded` | [REQ-008.md](REQ-008.md) |
| REQ-009 | 无人机探测态势演示 | `Implemented` | [REQ-009.md](REQ-009.md) |
| REQ-010 | UI 视觉治理 | `Implemented` | [REQ-010.md](REQ-010.md) |

## 范围与批准门禁

**包含范围**：单一指挥席用户的本地模拟会话状态、设备指派、任务执行、跨面板反馈、可复现场景、完整操作记录，以及无人机探测态势演示（REQ-009：2D 战术地图 + 卫星底图 + 经纬度坐标 + 无人机遥测模拟 + 检测模拟 + 目标坐标推算 + 四区同步）。

**排除范围**：登录、角色切换、外部通信、持久化、真实设备控制、真实排弹或抢修动作、UXR、MOS。其中 MOS 作为独立候选 REQ-007 单独登记，其 P0 范围虽已获批并实现为独立候选，但不并入本 NEXT 切片。

**批准门禁**：NEXT 必须先通过架构与 UI 设计评审并获用户批准，才能进入实现计划。架构依据见 [ARCHITECTURE.md](../ARCHITECTURE.md)，UI 依据见 [UI.md](../UI.md)。

## 新增需求规则

新增需求时只需创建 `REQ-NNN.md` 文件并在上方索引表添加一行，无需修改 [`PRODUCT.md`](../PRODUCT.md)。功能开发与执行计划必须同时满足两项前提：关联的 `REQ-NNN` 在本目录中状态为 `Approved`，且对应 `docs/features/<feature>.md` 自身状态为 `Approved`；`Draft` 需求或 `Draft` 功能设计不得进入实现计划。
