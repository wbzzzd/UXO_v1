# docs 文档地图

本文是 `docs/` 目录的入口导航，说明核心基线、各子目录与来源资料的归属和权威性。模块细目、需求状态和文件索引由对应基线或子目录 README 维护，本文不重复。项目规则见 [`AGENTS.md`](../AGENTS.md)。

## 1. 当前核心基线

四份核心文档是默认开发入口。CURRENT 实现以源码、CMake 和实际构建为准，核心文档定义产品范围、目标架构、UI 规则和工程门禁。

| 文档 | 职责 |
|------|------|
| [`PRODUCT.md`](PRODUCT.md) | 产品目标、范围、需求注册表（第 9 节是 `REQ-NNN` ID 与状态的唯一权威）、路线 |
| [`ARCHITECTURE.md`](ARCHITECTURE.md) | 模块、状态所有权、依赖、运行流程 |
| [`UI.md`](UI.md) | 信息架构、交互、页面状态、设计系统契约 |
| [`DEVELOPMENT.md`](DEVELOPMENT.md) | 构建、测试、质量门禁、完成定义 |

## 2. 子目录

| 目录 | 用途 | 入口 |
|------|------|------|
| [`features/`](features/) | 进入评审的功能增量设计；只描述相对核心基线的变化，`Draft` 不得直接指导实现 | [`features/README.md`](features/README.md) |
| [`dev/`](dev/) | 重整评审未完成的开发草稿；五份旧基线已迁入归档，剩余文件按历史决策、UXR/MOS 草稿、UI 拥有文件分类，均为 `Draft`/来源/设计输入 | [`dev/README.md`](dev/README.md) |
| [`ddr/`](ddr/) | 开发决策记录（DDR）；历史决策依据，与核心基线冲突时以核心基线为准 | [`ddr/README.md`](ddr/README.md) |
| [`research/`](research/) | 研究和技术调研资料；仅作证据引用，不直接成为需求，不进入默认开发上下文 | [`research/README.md`](research/README.md) |
| [`archive/`](archive/) | 不再直接指导当前开发的历史资料；含已迁入的 [`archive/development/`](archive/development/) 五份旧基线；不删除内容、不破坏 Git 追溯 | [`archive/README.md`](archive/README.md) |
| [`ui/`](ui/) | 六页 UI 设计文档与 HTML 原型；状态 `TARGET / Draft / 设计评审原型 / 本地模拟`，不连接真实设备、不执行真实排爆动作 | [`ui/README.md`](ui/README.md) |

## 3. 来源资料（不作为当前实现）

以下资料是需求与设计来源，仅作历史依据和证据引用，不代表已实现模块，与核心基线冲突时不得指导实现。

### 3.1 仓库根目录 SRS/SDD

- [`../SRS排弹抢修指挥系统_v1.0.md`](../SRS排弹抢修指挥系统_v1.0.md)：软件需求规格说明书来源
- [`../SDD排弹抢修指挥系统_v1.0.md`](../SDD排弹抢修指挥系统_v1.0.md)：软件设计说明书来源

### 3.2 docs 目录硬件/产品/调研来源

- [`排弹抢修系统_产品形态设计说明书_v1.0.md`](排弹抢修系统_产品形态设计说明书_v1.0.md)：产品形态设计来源
- [`排弹抢修系统_硬件功能设计说明书_v2.2.md`](排弹抢修系统_硬件功能设计说明书_v2.2.md)：硬件功能设计来源
- [`硬件设备技术规格说明书_v1.0.md`](硬件设备技术规格说明书_v1.0.md)：硬件设备规格来源
- [`未爆弹排除技术详细调研报告_v2.0.md`](research/未爆弹排除技术详细调研报告_v2.0.md)：未爆弹排除技术调研来源

### 3.3 其他来源与工作流资料

- [`前端UI详细设计文档.md`](前端UI详细设计文档.md)：前端 UI 详细设计来源；UI 拥有，变更走 UI 流程
- [`OMO_ADAPTATION_PLAN.md`](OMO_ADAPTATION_PLAN.md)：opencode + oh-my-openagent 适配计划草案；当前工作流权威是 [`../AGENTS.md`](../AGENTS.md) 与 `.opencode/`、`.omo/rules/`

## 4. 实现前置条件

进入 `.omo/plans/` 执行计划前，关联的 `REQ-NNN` 在 [`PRODUCT.md`](PRODUCT.md) 第 9 节必须为 `Approved`，且对应 [`features/<feature>.md`](features/) 自身状态为 `Approved`。`Draft` 需求或 `Draft` 功能设计不得进入实现计划。
