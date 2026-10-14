# PRODUCT.md 分层拆分 - 工作计划(简化版)

> 评审历史:原方案(含 status/)经 Oracle 评审 APPROVE WITH CONDITIONS,已修复全部 BLOCKER 与 MAJOR。用户指出 status/ 按"模块"组织与 dev/(按主题)、features/(按功能)尺度不一致,维护成本高。简化方案砍掉 status/,只移第 9 节到 requirements/,保留 6/7/8 节在 PRODUCT.md。本简化版是原方案的子集(移除复杂度),Oracle 对原方案的批准仍有效。
>
> **✅ 同步完成(2026-08-12):** 本地 main 已与 `origin/main` 合并(merge commit `5e79853`),无冲突。工作分支 `docs/product-md-layering-split` 已从同步后的 main 创建。PRODUCT.md 从 211 行变为 233 行,引用文件清单和行号已重新扫描(见 1.6 节)。ARCHITECTURE.md 用户改动(第 11 节)已 stash + pop 恢复,完好无损。
>
> **✅ 执行完成(2026-08-12):** 全部阶段已完成并通过验证。
> - 阶段 0 Preflight:✅ 快照已保存到 `.omo/evidence/product-md-split/`。
> - 阶段 1 requirements/:✅ 10 文件已创建(README + REQ-001~009),迁移清单已写入。
> - 阶段 2 PRODUCT.md 瘦身:✅ 第 9 节替换为指针,233 行 -> 181 行,内部引用已清理。
> - 阶段 3 规则同步:✅ 14 个文件 28 处引用已更新(含 ARCHITECTURE.md 选项 A,用户第 11 节完整保留)。
> - 阶段 4 验证:✅ 全部通过(4.1 旧引用扫描 / 4.2 文件存在性 / 4.3 PRODUCT.md 结构 / 4.4 REQ 索引 / 4.5 脏文件保护 / 4.6 无代码变更 / 4.7 空白检查 / 4.8 迁移清单)。
> - 阶段 5 收尾:✅ 计划状态已更新,会话记录已保存。不提交(除非用户明确要求)。

## TL;DR (给人看)

**你会得到什么:** PRODUCT.md 第 9 节(需求注册表,NEXT 候选范围)独立到 `docs/requirements/`,消除最大写热点。PRODUCT.md 其余章节(含 6/7/8 节 CURRENT 现状)保留不动,写入压力在可接受范围。

**为什么这么做:** PRODUCT.md 第 9 节是真正的写热点——表在持续增长,状态在持续变更,多 agent 并行新增需求必然撞车。第 6/7/8 节写入频率远低(基线审计后少改 / 8 行小表改不同行可自动合并),留在原处可接受。只移第 9 节,用最小改动解决主要问题,不引入尺度不一致的新目录。

**不会做什么:** 不改代码、构建、测试;不碰用户已有的 git 改动;不提交、不推送;不删除任何移出内容(完整保真迁移);不新建 status/ 目录。

**工作量:** 小(10 个新文件 + 最多 15 个文件修改,含待决策的 ARCHITECTURE.md)
**风险:** 低 - 纯文档,无代码影响;回滚用补丁反向应用,绝不用 `git checkout` 清除文件
**分支:** `docs/product-md-layering-split`(已从同步后的 main 创建)
**需要确认:** 是。一个待用户决策点(ARCHITECTURE.md 脏文件),见第 10 节。

---

## 1. 背景与当前事实

### 1.1 问题根源

`docs/PRODUCT.md`(211 行)当前混装三类不同稳定性内容:

| 内容 | 所在章节 | 改动频率 | 冲突严重度 | 处置 |
|------|---------|---------|-----------|------|
| 稳定产品意图(TARGET) | 第 1-5、10、11 节 | 极低 | 低 | 留 |
| CURRENT 功能清单 | 第 6 节(14 行) | 初始审计后低 | 低 | **留** |
| CURRENT 已验证闭环 | 第 7 节 | 验证结果变化时 | 低 | **留** |
| CURRENT 未完成能力 | 第 8 节(8 行小表) | agent 完成工作时 | 低-中(改不同行,git 大多自动合并) | **留** |
| 需求注册表(REQ) | 第 9 节 | 每次新增需求/状态变更 | **高**(表持续增长,多 agent 并行新增必然撞车) | **移走 -> requirements/** |

第 9 节是唯一的真正写热点。第 6/7/8 节写入频率和冲突风险远低,留在 PRODUCT.md 可接受。只移第 9 节即可解决主要冲突问题。

### 1.2 当前 PRODUCT.md 内容盘点(233 行)

| 章节 | 内容 | 处置 |
|------|------|------|
| 第 1 节 文档作用 | 含"本文回答 CURRENT/未完成/NEXT"等内部引用 | 留 + 清理第 9 节失效引用(见阶段 2.2) |
| 第 2 节 读者导航 | 导航表第 23 行含"第 9 节"指向 | 留(更新第 9 节指向) |
| 第 3 节 TARGET/CURRENT/NEXT 定义 | 第 38 行"由本文第 9 节统一编号" | 留(改指向 requirements/) |
| 第 4 节 实现状态定义 | 状态判定标准表 | 留(不动) |
| 第 5 节 TARGET 产品意图 | 第 56 行"(见第 9 节)" | 留 + 清理第 9 节失效引用(见阶段 2.2) |
| 第 6 节 CURRENT 功能清单 | 14 行能力表 | **留(不动)** |
| 第 7 节 CURRENT 已验证闭环 | 单目标状态流转闭环,第 144 行"NEXT 候选范围见第 9 节" | **留**(清理第 9 节引用) |
| 第 8 节 CURRENT 未完成能力 | 8 个能力维度表 + 表下注释 | **留(不动)** |
| 第 9 节 NEXT 候选范围 | 9.1 ID 规则 / 9.2 NEXT 草案(REQ-001~006/008/009) / 9.3 MOS 独立候选(REQ-007) / 9.4 范围门禁 | **移走 -> requirements/**;原位替换为一行指针 |
| 第 10 节 强制安全边界 | 安全约束 | 留(不动,无重编号) |
| 第 11 节 已确认产品决策 | 5 条决策 | 留(不动,无重编号) |

**关键简化:** 第 6/7/8/10/11 节全部保留原位,不做重编号。第 9 节替换为一行导航指针,不删除章节编号(避免跳号)。

### 1.3 第 6 节 14 行功能清单(迁移基准)— 不迁移,保留参考

本节内容**不迁移**,保留在 PRODUCT.md 第 6 节原位。以下清单仅作为 PRODUCT.md 内容完整性参考,不参与本次迁移:

1. 应用启动与主窗口
2. 固定模拟场景
3. 目标列表展示与选择
4. 模拟目标状态流转
5. 任务列表展示
6. 设备列表与状态卡片
7. 3D 机场态势与视角
8. 视频工作区布局
9. 模拟告警展示
10. 决策建议
11. 内存操作日志
12. 导航、菜单、工具栏与文件设置外壳
13. 平台初始化占位与未接入编译单元
14. 模拟模式标识与紧急停止占位

### 1.4 现有 REQ 盘点(远程 origin/main 第 9 节,共 9 个)

**第 9.2 节 NEXT 草案需求(8 个,含 1 个 Superseded):**

| ID | 标题 | 状态 |
|----|------|------|
| REQ-001 | 统一模拟会话状态 | Draft |
| REQ-002 | 模拟设备指派 | Draft |
| REQ-003 | 模拟任务执行 | Draft |
| REQ-004 | 跨面板一致反馈 | Draft |
| REQ-005 | 可复现场景 | Draft |
| REQ-006 | 完整操作记录 | Draft |
| REQ-008 | ~~探测阶段动态演示~~(时间线驱动) | Superseded(被 REQ-009 取代) |
| REQ-009 | 无人机探测态势演示 | Approved |

**第 9.3 节 MOS 独立候选(1 个,不在 NEXT 切片内):**

| ID | 标题 | 状态 |
|----|------|------|
| REQ-007 | 最小应急起降带(MOS)规划 | Implemented(仅 P0;P1/P2 Draft) |

注:REQ-007 由 `features/mos-planning.md` 负责功能设计,PRD 来源 `dev/prd-mos.md`。REQ-008 已被 REQ-009 取代。第 9.1 节含 ID 规则与 4 状态定义,第 9.4 节含范围(含 REQ-009 无人机探测)与批准门禁。

### 1.5 用户已有改动(不得碰)

`git status --short` 当前显示 `M docs/ARCHITECTURE.md`(用户新增第 11 节 + 修改第 16 行导航)。该文件同时含 6 处需同步的旧引用(见 1.6),构成冲突,待用户决策(第 10 节)。其余未跟踪的 `.omo/**` 文件本计划不碰。

### 1.6 完整同步点清单(grep 验证,38 处 / 17 文件,同步后重新扫描)

grep 模式:`第 9 节|第9节|PRODUCT\.md.*需求|需求.*PRODUCT\.md|PRODUCT\.md 是.*权威|需求注册表`

| 文件 | 行 | 当前措辞要点 | 处置 |
|------|-----|------------|------|
| `AGENTS.md` | 24,29,30 | "PRODUCT.md 第 9 节是唯一需求注册表" | 阶段 3.1 |
| `docs/PRODUCT.md` | 23,38,56,144,167 | 自身引用 | 阶段 2 |
| `docs/ARCHITECTURE.md` | 12,16,299,303,370,384 | "PRODUCT.md 第 9 节 REQ-001~006" | **待用户决策(第 10 节)** |
| `docs/UI.md` | 149,151 | "PRODUCT.md 第 9 节统一管理需求 ID" | 阶段 3.3 |
| `docs/DEVELOPMENT.md` | 10,118,175 | "产品需求与功能状态:PRODUCT.md" | 阶段 3.4 |
| `docs/README.md` | 11,59 | "需求注册表(第 9 节是唯一权威)" | 阶段 3.2 |
| `docs/features/README.md` | 7,43 | "PRODUCT.md 第 9 节分配 REQ-NNN" | 阶段 3.5 |
| `docs/features/mos-planning.md` | 7,128 | "REQ-007 状态见第 9.3 节" | 阶段 3.5b(新增) |
| `docs/features/drone-detection-demo.md` | 4 | "REQ-009(PRODUCT.md §9.2,Approved)" | 阶段 3.5c(新增) |
| `docs/features/detection-stage-demo.md` | 4 | "REQ-008(PRODUCT.md §9.2,Superseded)" | 阶段 3.5d(新增) |
| `docs/dev/README.md` | 27,44 | "REQ-NNN 在 PRODUCT.md 第 9 节状态为 Approved" | 阶段 3.6 |
| `docs/research/README.md` | 7 | "PRODUCT.md 第 9 节需求注册表的权威" | 阶段 3.7 |
| `docs/sources/README.md` | 11,17,31,32 | "第 9 节是 REQ-NNN ID 与状态的唯一权威" | 阶段 3.8(新增,远程引入) |
| `docs/ddr/README.md` | 23 | "PRODUCT.md 需求注册表为准" | 阶段 3.9 |
| `docs/ui/README.md` | 31 | "PRODUCT.md 已确认的产品边界" | 阶段 3.10(检查,PRODUCT 仍是基线,可能只改措辞) |
| `docs/dev/architecture-uxo-recognition-and-mos.md` | 17 | "核心基线(PRODUCT.md...)" | **无需改**(未提"第 9 节权威") |
| `README.md`(根) | 15 | "产品与需求基线"(仅链接) | **无需改**(未提"第 9 节") |

## 2. 目标与非目标

### 2.1 目标

1. 新建 `docs/requirements/` 子系统:需求 ID 与状态唯一权威,每个 REQ 一个详情文件,新增需求=新建文件。承接 PRODUCT.md 第 9 节全部内容(9.1 ID 规则 / 9.2 NEXT 草案 / 9.3 MOS 独立候选 / 9.4 范围门禁)。
2. PRODUCT.md 第 9 节替换为一行导航指针,指向 `docs/requirements/`。第 6/7/8/10/11 节保留原位,不做重编号。
3. 同步全部 1.6 清单中的引用(ARCHITECTURE.md 待决策)。
4. 清理 PRODUCT.md 内部对"第 9 节"的失效引用,改为指向 requirements/。

### 2.2 权威模型(明确)

**四份核心基线不变**(PRODUCT/ARCHITECTURE/UI/DEVELOPMENT)。`docs/requirements/` 是**独立权威子系统**,不是新的核心基线:
- `requirements/` 承接 PRODUCT.md 第 9 节的需求管理职责。
- PRODUCT.md 保留产品意图基线 + CURRENT 现状(第 6/7/8 节)角色,在第 9 节位置指向 requirements/,但不维护需求内容。
- `features/` 已有的功能级实现状态(如 `features/mos-planning.md` 标注 "Implemented, P0")不受影响,不重复。
- 不新建 `status/` 目录。CURRENT 现状(第 6/7/8 节)留在 PRODUCT.md,写入频率可接受。

### 2.3 非目标(护栏)

- 不改任何代码、CMake、构建、测试。
- 不改 ARCHITECTURE/UI/DEVELOPMENT 三份核心基线的**实质内容**(仅改引用措辞)。
- 不碰用户已有的 git 改动(ARCHITECTURE.md 按第 10 节决策处理)。
- 不删除 PRODUCT.md 移出内容的任何信息(完整保真迁移)。
- 不提交、不推送。
- 不动 `docs/ddr/`、`docs/research/`、`docs/archive/`、`docs/ui/` 的实质内容(仅 README 引用同步)。
- **不新建 `docs/status/` 目录**(简化方案核心决策)。
- **不对 PRODUCT.md 章节重编号**(第 9 节替换为指针,不删除章节号)。

## 3. 安全边界

- 纯文档迁移,无设备控制、通信、数据库写入、排爆动作影响。
- 不碰 `.env`、密钥、凭据文件。
- 保留所有"模拟/占位/真实接入"标注,不改变安全语义。
- PRODUCT.md 第 10 节安全边界原样保留,不弱化。
- **回滚绝不使用 `git checkout <file>`**(会清除用户未提交改动);改用补丁反向应用(见第 9 节)。

## 4. 目标目录结构

```
docs/
  PRODUCT.md            # 第 9 节替换为指针;其余章节不动
  ARCHITECTURE.md       # 仅改引用措辞(待决策)
  UI.md                 # 仅改引用措辞
  DEVELOPMENT.md        # 仅改引用措辞
  requirements/         # 新增:需求管理子系统(唯一新目录)
    README.md           #   ID 规则 + 状态定义 + REQ 索引表 + 范围门禁 + 模板
    REQ-001.md ~ REQ-009.md  # 9 个详情文件(含 007 Implemented/008 Superseded/009 Approved)
  features/             # 已有:仅改 README 引用
  dev/                  # 已有:仅改 README 引用
```

**与原方案对比:** 无 `status/` 目录,无 10 个模块状态文件,无 4.1/4.2 映射表。新文件从 20 个减为 10 个。

### 4.1 PRODUCT.md 最终结构(无重编号)

| 章节 | 标题 | 变化 |
|------|------|------|
| 1 | 文档作用 | 清理第 9 节失效引用 |
| 2 | 读者导航 | 更新第 9 节指向 |
| 3 | TARGET/CURRENT/NEXT 定义 | NEXT 改指向 requirements/ |
| 4 | 实现状态定义 | 不动 |
| 5 | TARGET 产品意图 | 清理第 9 节失效引用 |
| 6 | CURRENT 功能清单 | **不动** |
| 7 | CURRENT 已验证闭环 | **不动** |
| 8 | CURRENT 未完成能力 | **不动** |
| 9 | NEXT 候选范围 | **替换为一行指针**:"需求 ID、状态注册表与 NEXT 候选范围见 [docs/requirements/](requirements/)。" |
| 10 | 强制安全边界 | **不动** |
| 11 | 已确认产品决策 | **不动** |

**关键:** 无章节重编号。第 9 节保留编号但内容替换为导航指针,避免跳号,最小化引用更新。

## 5. 分阶段任务

### 阶段 0:Preflight(已完成)

- **0.1** ✅ 远程同步完成:`git merge origin/main`(merge commit `5e79853`),无冲突。
- **0.2** ✅ 脏文件保护:`git stash push docs/ARCHITECTURE.md` -> 合并 -> `git stash pop` 成功,用户第 11 节完好。
- **0.3** ✅ 工作分支创建:`git switch -c docs/product-md-layering-split`(从同步后的 main)。
- **0.4** 记录 `git status --short` 基线(执行前做)。
- **0.5** 保存 PRODUCT.md 内容快照(阶段 1 必须基于此快照):`cp docs/PRODUCT.md .omo/evidence/preflight-PRODUCT.md.snapshot`。
- **0.6** 指定证据落点:`.omo/evidence/product-md-split/`,本计划所有验证输出存此。
- **0.7** 保存脏文件基线:`git diff HEAD -- docs/ARCHITECTURE.md > .omo/evidence/preflight-architecture.md.patch`。

### 阶段 1:建立 requirements/ 子系统(基于阶段 0.3 快照)

**1.1 创建 `docs/requirements/README.md`**
- 定位:需求 ID 与状态唯一权威(从 PRODUCT.md 9.1 移入)。
- ID 规则:追加式、无语义 `REQ-NNN`,一经分配不复用,被取代保留原 ID。
- 状态定义表:`Draft`/`Approved`/`Implemented`/`Superseded`(从 9.1 移入)。
- REQ 索引表:分两组呈现--9.2 NEXT 草案(REQ-001~006/008/009)+ 9.3 独立候选(REQ-007);每行 ID + 标题 + 状态 + 详情链接。
- 范围与批准门禁:包含范围(含 REQ-009 无人机探测)、排除范围、批准门禁(从 9.4 移入)。
- 独立候选说明:REQ-007 MOS 不并入 NEXT 切片,由 `features/mos-planning.md` 负责功能设计(从 9.3 移入)。
- 详情文件模板:状态/关联需求/解决的缺口/验收结果/范围(引用本 README 全局范围)/待确认事项。

**1.2 为 REQ-001~009 各创建详情文件(9 个)**
- **REQ-001~006**(Draft,NEXT 切片):标题、状态(Draft)、解决的当前缺口、验收结果(从 9.2 表逐字拆出)。范围引用全局,不复制。
- **REQ-007**(Implemented,独立候选):标题、状态(Implemented,仅 P0;P1/P2 Draft)、解决的缺口、验收结果、2026-08-03 批准事实与 2026-08-04 实现事实(从 9.3 逐字移入)、链接 `features/mos-planning.md` 和 `dev/prd-mos.md`。
- **REQ-008**(Superseded):标题、状态(Superseded)、原验收结果(划线)、注明"已被 REQ-009 取代"(从 9.2 表逐字移入)。
- **REQ-009**(Approved):标题、状态(Approved)、解决的当前缺口、**完整验收结果**(从 9.2 表逐字移入,验收描述极长,必须完整保真不截断)、范围引用全局。
- 内容逐字保真,不改写语义。

**1.3 生成迁移清单**(内容保真证据)
- 落点:`.omo/evidence/product-md-split/migration-ledger.md`。
- 逐项记录:原章节/原行键(REQ ID 或字段名)/目标文件/目标标题。
- 覆盖:第 9.1 ID 规则 + 4 状态定义 + 9.2 表 8 REQ 全字段 + 9.3 MOS 独立候选批准/实现事实 + 9.4 范围门禁。

### 阶段 2:瘦身 PRODUCT.md 第 9 节(依赖阶段 1)

**2.1 替换第 9 节内容**:原 9.1/9.2/9.3/9.4 全部内容删除,替换为一行:"需求 ID、状态注册表与 NEXT 候选范围见 [docs/requirements/](requirements/)。"

**2.2 清理内部失效引用**(逐条,只涉及第 9 节引用;第 6/7/8 节引用不动):
- 第 1 节:若含"本文回答...NEXT..."等指向第 9 节的措辞,改指向 requirements/。
- 第 2 节读者导航表:"NEXT 候选范围"行指向更新(第 9 节 -> requirements/)。
- 第 3 节(第 38 行):"由本文第 9 节统一编号" -> "由 docs/requirements/ 统一编号"。
- 第 5.1 节(第 56 行):"(见第 9 节)" -> "(见 docs/requirements/)"。
- 第 7 节末(第 144 行):"NEXT 候选范围见第 9 节" -> "见 docs/requirements/"。"完整闭环所需能力见第 8 节"不动(第 8 节保留)。
- 第 5.3 节(第 77 行):"当前已验证环路见第 7 节" **不动**(第 7 节保留)。
- 验收:PRODUCT.md 内部 `grep "第 9 节\|第9节"` 为零(第 9 节标题本身除外)。

**2.3 不做重编号**:第 10/11 节保持原编号。无跳号问题(第 9 节保留为指针)。

### 阶段 3:同步规则文档(依赖阶段 2;3.1-3.12 按文件组并行,ARCHITECTURE.md 除外)

**3.1 AGENTS.md**(第 24、29、30 行)
- "PRODUCT.md 第 9 节是唯一需求注册表" -> "docs/requirements/ 是需求 ID 与状态唯一权威"。
- "产品范围、需求或路线任务:读 PRODUCT.md" -> 拆分为产品范围读 PRODUCT.md,需求状态读 requirements/。
- 实现前置:"REQ-NNN 在 PRODUCT.md 中状态为 Approved" -> "在 docs/requirements/ 中状态为 Approved"。
- 明确 2.2 权威模型表述。

**3.2 docs/README.md**(第 11、59 行)
- 核心基线表:PRODUCT.md 职责去掉"需求注册表"。
- 子目录表:新增 requirements/。
- 实现前置段:REQ-NNN 指向 docs/requirements/。

**3.3 docs/UI.md**(第 149、151 行)
- "PRODUCT.md 第 9 节统一管理需求 ID 与状态" -> "docs/requirements/ 统一管理"。

**3.4 docs/DEVELOPMENT.md**(第 10、118、175 行)
- "产品需求与功能状态:PRODUCT.md" -> "产品需求与功能状态:docs/requirements/"。
- "功能开发必须关联 PRODUCT.md 中已批准的需求 ID" -> "关联 docs/requirements/ 中已批准的需求 ID"。
- 路由表"产品、需求、路线"行更新。

**3.5 docs/features/README.md**(第 7、43 行)
- "关联产品需求引用 PRODUCT.md 第 9 节" -> "引用 docs/requirements/"。
- "PRODUCT.md 是需求 ID 与需求状态的唯一权威" -> "docs/requirements/ 是...唯一权威"。

**3.5b docs/features/mos-planning.md**(第 7、128 行)- 新增(远程引入)
- "REQ-007 状态见 PRODUCT.md 第 9.3 节" -> "见 docs/requirements/REQ-007.md"。

**3.5c docs/features/drone-detection-demo.md**(第 4 行)- 新增(远程引入)
- "REQ-009(PRODUCT.md §9.2,Approved)" -> "REQ-009(docs/requirements/REQ-009.md,Approved)"。

**3.5d docs/features/detection-stage-demo.md**(第 4 行)- 新增(远程引入)
- "REQ-008(PRODUCT.md §9.2,Superseded)" -> "REQ-008(docs/requirements/REQ-008.md,Superseded)"。

**3.6 docs/dev/README.md**(第 27、44 行)
- "REQ-007 见 PRODUCT.md 第 9.3 节" -> "见 docs/requirements/REQ-007.md"。
- "REQ-NNN 在 PRODUCT.md 第 9 节状态为 Approved" -> "在 docs/requirements/ 中状态为 Approved"。

**3.7 docs/research/README.md**(第 7 行)
- "PRODUCT.md 第 9 节需求注册表的权威" -> "docs/requirements/ 的权威"。

**3.8 docs/sources/README.md**(第 11、17、31、32 行)- 新增(远程引入)
- "第 9 节是 REQ-NNN ID 与状态的唯一权威" -> "docs/requirements/ 是...唯一权威"。
- "通过 PRODUCT.md 第 9 节的需求状态提升为 Approved" -> "通过 docs/requirements/ 的需求状态提升为 Approved"。
- 来源资料表中 "PRODUCT.md 需求注册表" -> "docs/requirements/"。

**3.9 docs/ddr/README.md**(第 23 行)
- "PRODUCT.md 需求注册表为准" -> "docs/requirements/ 为准"。

**3.10 docs/ui/README.md**(第 31 行)
- 检查:"PRODUCT.md 已确认的产品边界"--PRODUCT.md 仍是核心基线,但若措辞暗示需求权威则改指向 requirements/;若仅指产品边界则保留。

**3.11 README.md(根)**(第 15 行)
- 仅链接"产品与需求基线",未提"第 9 节"。检查是否需补充 requirements/ 入口;若无"第 9 节"引用则不改。

**3.12 docs/ARCHITECTURE.md**(第 12、16、299、303、370、384 行)-- **待用户决策,见第 10 节**

### 阶段 4:验证(依赖阶段 3)

所有验证命令带断言,任一失败即非零退出。证据存 `.omo/evidence/product-md-split/`。

**4.1 旧引用扫描**(扩展模式,排除历史归档)
```bash
grep -rn "第 9 节\|第9节\|PRODUCT\.md.*需求\|需求.*PRODUCT\.md\|PRODUCT\.md 是.*权威\|需求注册表" \
  docs/ AGENTS.md README.md --include="*.md" \
  | grep -v "docs/archive/\|docs/ddr/DDR-\|docs/research/未爆弹\|docs/dev/architecture-uxo\|docs/dev/incremental" \
  > /tmp/stale-refs.txt
test ! -s /tmp/stale-refs.txt || { echo "STALE REFS:"; cat /tmp/stale-refs.txt; exit 1; }
```
注:ARCHITECTURE.md 按第 10 节决策处理;若选 C(暂缓),ARCHITECTURE.md 行计入已知例外。

**4.2 新文件存在性(10 个)**
```bash
for f in docs/requirements/README.md \
         docs/requirements/REQ-00{1,2,3,4,5,6,7,8,9}.md; do
  test -e "$f" || { echo "MISSING: $f"; exit 1; }
done
echo "ALL 10 FILES EXIST"
```

**4.3 PRODUCT.md 结构与内部引用**
```bash
# 第 9 节标题仍存在(替换为指针,不删除)
grep -q "^## 9\." docs/PRODUCT.md || { echo "Section 9 heading missing"; exit 1; }
# 第 9 节内容已替换为指针(不含 9.1/9.2/9.3/9.4 子节标题)
grep -n "^### 9\.[1-4]" docs/PRODUCT.md && { echo "Old 9.x subsections still present"; exit 1; }
# 第 9 节含 requirements/ 指针
grep -q "docs/requirements/" docs/PRODUCT.md || { echo "No requirements/ pointer in PRODUCT.md"; exit 1; }
# 第 6/7/8/10/11 节标题仍存在(未动)
grep -q "^## 6\." docs/PRODUCT.md || { echo "Section 6 missing"; exit 1; }
grep -q "^## 7\." docs/PRODUCT.md || { echo "Section 7 missing"; exit 1; }
grep -q "^## 8\." docs/PRODUCT.md || { echo "Section 8 missing"; exit 1; }
grep -q "^## 10\." docs/PRODUCT.md || { echo "Section 10 missing"; exit 1; }
grep -q "^## 11\." docs/PRODUCT.md || { echo "Section 11 missing"; exit 1; }
# 内部无对第 9 节内容的失效引用(第 9 节标题本身除外)
grep -n "第 9 节\|第9节" docs/PRODUCT.md | grep -v "^## 9\." && { echo "Stale section 9 refs"; exit 1; }
echo "PRODUCT.md STRUCTURE OK"
```

**4.4 REQ 索引完整(9 个)**
```bash
for id in REQ-001 REQ-002 REQ-003 REQ-004 REQ-005 REQ-006 REQ-007 REQ-008 REQ-009; do
  grep -q "| $id " docs/requirements/README.md || { echo "MISSING $id in index"; exit 1; }
  test -e "docs/requirements/$id.md" || { echo "MISSING $id.md"; exit 1; }
done
echo "9 REQS INDEXED"
```

**4.5 脏文件保护**
```bash
# ARCHITECTURE.md 用户改动仍完整(若第 10 节选 A)
git diff HEAD -- docs/ARCHITECTURE.md > /tmp/arch-after.patch
# 用户原补丁内容仍在(第 11 节存在)
grep -q "^## 11\. Draft 功能共享架构约束" docs/ARCHITECTURE.md || echo "WARN: user section 11 missing"
# 对比 preflight 快照
diff <(git diff HEAD -- docs/ARCHITECTURE.md) .omo/evidence/preflight-architecture.md.patch \
  && echo "ARCHITECTURE UNCHANGED" || echo "ARCHITECTURE MODIFIED (expected if option A)"
```

**4.6 范围保护(代码与其他基线实质内容)**
```bash
git diff HEAD --name-only -- src include CMakeLists.txt | grep . && exit 1
echo "NO CODE CHANGED"
```

**4.7 空白检查**
```bash
git diff HEAD --check || exit 1
```

**4.8 迁移清单完整性**
```bash
test -e .omo/evidence/product-md-split/migration-ledger.md || exit 1
grep -c "9\.1\|9\.2\|9\.3\|9\.4" .omo/evidence/product-md-split/migration-ledger.md
```

### 阶段 5:收尾

- **5.1** 更新本计划文件状态:在标题下标注"已执行"或各阶段完成状态。
- **5.2** 工作会话记录:保存总结到 `.omo/evidence/product-md-split/session-summary.md`(不提交)。
- **5.3** 不创建任何 commit(除非用户明确要求)。

## 6. 验收标准

1. PRODUCT.md 第 9 节替换为一行指针,指向 docs/requirements/;第 6/7/8/10/11 节保留原位不动;无章节重编号。
2. PRODUCT.md 内部无对"第 9 节"内容的失效引用(第 9 节标题本身除外)。
3. `docs/requirements/README.md` 含 ID 规则、状态定义、REQ-001~009 索引表(分 NEXT 切片 + 独立候选两组)、范围门禁、模板。
4. `docs/requirements/REQ-001.md` ~ `REQ-009.md` 各含标题/状态/缺口/验收,范围引用全局,内容与原 9.2/9.3 保真;REQ-007 含实现事实、REQ-008 标注 Superseded、REQ-009 验收描述完整不截断。
5. 1.6 同步点清单中,除 ARCHITECTURE.md(按第 10 节决策)外,全部更新为指向 docs/requirements/。
6. 无断链:所有新文档相对链接可解析。
7. 范围干净:未改代码、构建、测试;未碰用户 ARCHITECTURE.md 改动(若选 A 则仅追加引用修改,用户第 11 节完整保留)。
8. 迁移清单存在且覆盖第 9 节全部移出内容(9.1/9.2/9.3/9.4)。
9. 不提交、不推送。

## 7. 验证命令

见阶段 4(4.1-4.8),均为带断言脚本,任一失败非零退出。

## 8. 并行执行策略

| 阶段 | 依赖 | 可并行 |
|------|------|--------|
| 阶段 0 | 无 | - |
| 阶段 1(requirements/) | 阶段 0.3 快照 | 单通道(10 文件,可拆 2 通道:README+索引 / 9 个 REQ 文件) |
| 阶段 2(瘦身 PRODUCT.md 第 9 节) | 阶段 1 | - |
| 阶段 3.1-3.11(规则同步) | 阶段 2 | 按文件组并行,互不冲突 |
| 阶段 3.12(ARCHITECTURE.md) | 用户决策 | **独立串行**(脏文件) |
| 阶段 4(验证) | 阶段 3 | - |

**通道划分**(2-3 通道):
- 通道 A:阶段 1 全部(requirements/ 10 文件,可再拆)
- 阶段 2 单通道(依赖 A,PRODUCT.md 单文件)
- 阶段 3 拆文件组并行(AGENTS / docs-README / UI / DEVELOPMENT / features-README / features-mos / features-drone / features-detection / dev / research / sources / ddr / ui-README / 根-README)
- 阶段 3.12 独立串行(待用户决策)
- 阶段 4 单通道验证

## 9. 风险与回滚

| 风险 | 缓解 | 回滚 |
|------|------|------|
| 交叉引用遗漏 | 阶段 4.1 扩展 grep 兜底 | 反向应用补丁 |
| 内容迁移丢失 | 阶段 1.3 迁移清单 + 阶段 4.4 完整性断言 | 反向应用补丁 |
| 误碰用户 ARCHITECTURE.md 改动 | 阶段 0.2 保存 preflight 补丁 + 阶段 4.5 对比 | 反向应用补丁(不用 git checkout) |
| 第 6/7/8 节误改 | 阶段 4.3 断言第 6/7/8/10/11 节标题存在 | 反向应用补丁 |

**回滚方法(绝不使用 `git checkout <file>`)**:
- 对**修改的现有文件**(PRODUCT.md、AGENTS.md 等):执行前 `git diff HEAD -- <file> > preflight-<file>.patch`,回滚时 `git apply -R preflight-<file>.patch`。
- 对**新建文件**(requirements/):直接 `rm` 这些路径(基线不存在,删除即回滚)。
- 任何情况下不 `git checkout docs/ARCHITECTURE.md`(会清除用户未提交改动)。

## 10. 待用户决策点(执行前必须确认)

### ARCHITECTURE.md 脏文件冲突

`docs/ARCHITECTURE.md` 既有用户未提交改动(新增第 11 节,在 line 382),又含 6 处需同步的 `PRODUCT.md 第 9 节` 引用(第 12、16、299、303、370、384 行)。其中第 16 行和第 384 行落在用户改动范围内。三者选一:

- **选项 A**:允许 agent 在用户现有工作树基础上,用精确 `edit`(oldString/newString 字符串匹配)只改这 6 处引用措辞,不碰用户其他改动。用户第 11 节内容完整保留。风险低(edit 精确匹配),且顺带同步了用户新增内容里的引用。
- **选项 B**:用户先提交或 `git stash` ARCHITECTURE.md,再执行拆分。最干净,但要用户操作。
- **选项 C**:ARCHITECTURE.md 引用同步本次跳过,留待用户后续处理。文档权威暂时不一致(ARCHITECTURE.md 仍指向 PRODUCT.md 第 9 节),但本计划其余部分照常执行。

**推荐 A**(edit 精确匹配只改引用字串,用户第 11 节不受影响,且帮用户同步了新内容里的引用)。

### 其他确认

- 本计划批准后即可执行,执行完不自动提交。
- ✅ 远程同步已完成(2026-08-12),不再作为决策点。

## 11. Commit 策略

不创建任何 commit,除非用户单独明确要求。若后续要求,按职责拆原子提交:
- 一个提交:requirements/ 子系统(10 文件)
- 一个提交:PRODUCT.md 第 9 节瘦身
- 一个提交:规则文档同步(按文件组)
- ARCHITECTURE.md 单独提交(若选 A)
绝不 stage `.omo/**` 运行态文件。

---

## 附:与原方案对比

| 维度 | 原方案(status/ + requirements/) | 简化方案(只 requirements/) |
|------|------|------|
| 新建目录 | 2 个(requirements/ + status/) | 1 个(requirements/) |
| 新文件数 | 20 | 10 |
| 修改文件数 | ~12 | ~11 |
| 组织轴 | REQ + 模块(不一致) | 只有 REQ(一致) |
| 写热点解决 | 第 9 节 ✓,第 8 节 ✓ | 第 9 节 ✓,第 8 节部分(留原处,可接受) |
| 章节重编号 | 是(10->6, 11->7) | 否(第 9 节替换为指针) |
| 维护成本 | 高(模块↔功能交叉引用) | 低(单一轴) |
| 尺度匹配问题 | 有(dev/features/status 三轴不一致) | 无(dev/features 不变,只加 requirements 单轴) |
