# 原型结构级同步计划：态势/探测/决策页面 100% 镜像 CURRENT

- 状态：Approved（用户已确认"原型结构 100% 镜像 CURRENT"方向）
- 日期：2026-08-24
- 工作区：/home/lin/UXO_v1-ui-visual-upgrade（分支 feature/prototype-baseline-sync，基于 origin/main 240f5bf）
- 关联需求：REQ-010（Implemented，本批为原型侧结构同步收尾）
- 关联功能设计：docs/features/ui-visual-upgrade.md（Approved）
- 前置计划：.omo/plans/20260824-prototype-baseline-sync.md（Approved，已执行完毕）

## 1. 背景与用户决策

前置计划完成六页视觉基线同步后，复核发现原型与 CURRENT Qt 客户端仍存在三处结构级偏差：

1. 态势页工具栏缺少 CURRENT `createMapToolbar` 的探测控制组（重置/开始/结束三按钮，f2f9/f04b/f04d）；
2. 三页导航中已实现页面（态势/探测/决策）之间仅为占位高亮切换，未真实互链（CURRENT `createConnections` 中导航点击 `setCurrentIndex` 实际路由）；
3. 探测页原型仍是旧版"搜索+筛选+证据 Tab"三栏布局，而 CURRENT `DetectionView` 已是"检测结果表 + 视频查看器 + 目标详情/异常热力图/状态时间线 + 底部操作条"结构。

**用户决策（2026-08-24）**：原型结构 100% 镜像 CURRENT——删除原型中 CURRENT 未实现的控件，补齐 CURRENT 已有而原型缺失的结构。

**超越前置计划说明**：前置计划 §2.1 曾以"已批准规格不因 CURRENT 扩张而回写"为由未挂载探测控制三按钮；本计划依据的用户新决策显式取代该条款。

**执行修正（2026-08-24）**：本计划 §2.1 初稿误记 `onResetDetection` "无告警写入"；执行期复核源码确认槽末尾 L581 调用 `addAlarm(QStringLiteral("探测已重置"))`（清空目标与探测结果之外同样写状态栏告警）。§3.1-4 据此修正：重置按钮行为从"清除动态告警条目"改为追加"探测已重置"告警条目，与开始/结束槽的告警演示保持一致。

## 2. CURRENT 事实（以源码为准）

### 2.1 主窗口壳（src/MainWindow/MainWindow.cpp）

- `createMainLayout` L205-302：QHBoxLayout = NavigationWidget(80px) + mainPageStack（QStackedWidget）；index 0 态势页（左面板 + 设备资源条 36px + 地图工具栏 32px + 地图舞台），index 1 DetectionView，index 2 DecisionView；无全局 QToolBar；探测页无工具栏。
- `createMapToolbar` L303-388：探测控制组 [重置 f2f9][开始 f04b][结束 f04d]（启用）+ spacing 8 + [视角复位 f065][图层 f5fd][测量 f545][坐标拾取 f601]（Qt 侧全部禁用，`onResetViewClicked` L933 不可达）+ 弹性 + [模拟] 标签。
- 探测槽：`onStartDetection` L541（播放视频 + 启动遥测模拟器 + 告警"探测已开始"）；`onStopDetection` L549（暂停/回 0s/停止 + 告警"探测已结束，视频回 0s"）；`onResetDetection` L558-583（暂停视频回 0s/复位遥测模拟器/清空目标与地图轨迹/清空探测结果并复位计数，末尾 L581 写告警"探测已重置"）。
- 状态栏 `createStatusBar` L389：绿色主色条（设备/最低电量/[模拟模式]/告警滚动/紧急停止恒禁用）。

### 2.2 探测页（src/MainWindow/DetectionView.cpp，650 行）

- 顶部："[AI] 自动检测" + 摘要"已分析 %1 帧 · 异常 %2"。
- 左栏"检测结果"表格 7 列：目标 ID/类型/威胁/置信度/时间/状态/探测源；正常帧目标 ID 与威胁显示"--"、类型"正常"、无分类时置信度"--"；异常帧威胁恒"高"。
- 中栏：视频查看器 + AI 分类标签。
- 右栏：目标详情（8 字段：目标 ID/类型/威胁等级/置信度/最大异常分/帧时间/推理耗时/探测源）+ 异常热力图（空态"无热力图"）+ 状态时间线。
- 底部操作条 44px：确认（主按钮）/拒绝/移除记录 + 状态标签（"当前目标： %1 · 状态： %2"；未选中"未选中结果"；空表"等待检测结果"）。
- 操作语义：确认→"已确认（人工）"、拒绝→"已拒绝（人工）"（时间线写入）；移除记录→删除行；空态"等待检测结果"。

## 3. 变更范围

### 3.1 docs/ui/prototypes/situation/index.html

1. 工具栏（L272-281）：新增探测控制组三按钮 `SIT-TB-DET-RESET`/`SIT-TB-DET-START`/`SIT-TB-DET-STOP`（f2f9/f04b/f04d，启用，tooltip 标注镜像语义），插入在视角复位之前；组间以 tb-separator 分隔；工具栏 CSS/HTML 注释改为"镜像 CURRENT createMapToolbar"。
2. 导航（L292-293）：`SIT-NAV-02`/`SIT-NAV-03` 改为 `<a>` 真实链接（`../detection/index.html`、`../decision/index.html`），移除占位 title；04/05/06 保持占位。
3. CSS：新增 `a.nav-item{text-decoration:none}`；工具栏图标注释补 f2f9/f04b/f04d。
4. JS：导航高亮选择器改为 `.nav-item:not([href])`（链接项原生路由，占位项保留仅高亮）；新增探测控制演示块——开始/结束向 SIT-SB-ALARM 滚动轨道追加动态告警条目（"探测已开始"/"探测已结束，视频回 0s"，上限 4 条），重置同样追加动态告警条目（"探测已重置"，上限 4 条）。

### 3.2 docs/ui/prototypes/detection/index.html（结构级重写）

- 顶部 det-topbar 32px：`DET-TB-AI`（"[AI] 自动检测"）+ `DET-TB-SUMMARY`（"已分析 5 帧 · 异常 3"）；删除旧工具栏（导出/刷新/计数）。
- 左栏 360px："检测结果" 7 列表 `DET-LP-TARGET-TABLE`；删除搜索/筛选/排序（CURRENT 无）。
- 中栏：视频查看器 `DET-CE-VIEWER`（异常帧渲染红色检测框+ID/类型/置信度标签，正常帧无框，空态"等待检测结果"）+ AI 分类标签 `DET-CE-CLASS`。
- 右栏 380px：`DET-RP-DETAIL`（8 字段）+ `DET-RP-HEATMAP`（异常热力图，空态"无热力图"）+ `DET-RP-TIMELINE`（状态时间线）。
- 底部 det-actionbar 44px：`DET-CE-CONFIRM`（确认，主按钮）/`DET-CE-REJECT`（拒绝，次级）/`DET-CE-REMOVE`（移除记录，次级）+ `DET-CE-STATUS`。
- 状态栏：与态势页同构绿色 22px（`DET-SB-DEVICE`/`DET-SB-BATTERY`/`DET-SB-SIM`/`DET-SB-ALARM` 滚动告警/`DET-SB-EMERGENCY` 恒禁用）。
- Mock 数据：3 异常帧（target-001 反跑道雷 86%、target-002 航弹 72%、target-003 火箭弹 91%，威胁均"高"）+ 2 正常帧（目标 ID 空→表格全"--"）；确认/拒绝实时写时间线（"HH:mm:ss 已确认（人工）"）；移除记录删行并自动选中相邻行，表空回到空态。
- 初始默认选中 target-001（演示三栏联动；CURRENT 初始为未选中空态，记为已知原型偏差并在 detection.md 标注）。
- ID 退役 21 个旧 ID（DET-LP-SEARCH/CLEAR/REFRESH/FILTER-TYPE/FILTER-THREAT/FILTER-STATUS/TABLE-COUNT/SORT-THREAT/SORT-CONF、DET-TB-EXPORT/REFRESH/COUNT、DET-CE-TARGET/TAB-RECOG/TAB-SOURCE/TAB-STATE/CONTENT/RECOG-1/RECOG-2/CONFIRM-INFO/REJECT-MSG）；保留 DET-NAV-*/DET-MENU-*/DET-LP-TARGET-TABLE/DET-CE-CONFIRM/REJECT/REMOVE/DET-RP-DETAIL/TIMELINE/DET-SB-*；新增 DET-TB-AI/DET-TB-SUMMARY/DET-CE-VIEWER/DET-CE-CLASS/DET-RP-HEATMAP/DET-CE-STATUS/DET-SB-BATTERY。
- 导航 01 态势/03 决策真实链接，02 当前页，04-06 占位。

### 3.3 docs/ui/prototypes/decision/index.html

- 导航 `DEC-NAV-01`/`DEC-NAV-02` 改 `<a>` 链接（`../situation/index.html`、`../detection/index.html`）；CSS 新增 `a.nav-btn{text-decoration:none}`；JS 高亮选择器改 `.nav-btn:not([href])`，注释更正为"真实链接路由（与 CURRENT 一致）"（现注释"不路由（与 CURRENT 一致）"为陈旧错误）。

### 3.4 文档同步（docs/ui/）

- `pages/situation.md`：§6.1 导航行/备注更新（02/03 真实路由）；§6.3 工具栏表新增探测控制 3 行并重编号后续行；§12 ID 索引补 3 行；清理 RESET 行陈旧的"与右面板 SIT-RP-RESET 等价"引用。
- `pages/detection.md`：按新结构整页重写（区域、ID、交互、CURRENT 映射、退役 ID 说明）。
- `application-shell.md`：§2 布局文字与 ASCII 图更正；§3.2 导航原型行为更正为真实路由；§5 工具栏清单重写（含探测控制组）；§7 重复 SIT-NAV-03 行去重；§9 行号引用修正。
- `pages/index.md`：探测 section 关键区域更新。
- `pages/decision.md`：仅当 grep 发现"仅切换高亮/不路由"类陈旧文字时修订导航行为描述。

## 4. 不变更项

- 前置计划已交付的 21 文件基线成果不动（本计划仅追加结构同步改动）。
- 工具栏位置（地图舞台上方 32px）、[模拟] 标签、状态栏结构不动。
- SIT-TB-RESET 保持启用 + 复位演示行为（已批准偏差，application-shell.md §5 记录；CURRENT 中视角复位为禁用）。
- PiP 悬浮窗、目标详情浮层等态势页既有演示不动。
- 设备/统计/配置三个占位页不动。

## 5. 验证计划

1. grep 断言：新 testid（SIT-TB-DET-*、DET-TB-AI 等）存在；21 个退役 ID 在 detection/index.html 中不存在；三页互链 `<a ... href="../{page}/index.html">`；`:not([href])` 选择器就位。
2. 截图：`node screenshot.js` 重新生成 situation/detection/decision 三页 overview-1920x1080.png；目视核查探测页三栏/操作条/状态栏与态势页新工具栏。
3. 证据落盘 `.omo/evidence/prototype-structure-resync/`（grep 输出 + 截图副本 + 验证记录）。
4. `git status --short` 仅出现预期文件变更。

## 6. 执行清单

- [x] 计划文档
- [x] situation/index.html 结构同步
- [x] detection/index.html 结构重写
- [x] decision/index.html 导航同步
- [x] 文档同步（situation.md/detection.md/application-shell.md/pages/index.md/decision.md）
- [x] 验证（grep 断言 + 截图 + 证据落盘）
- [x] 完成状态更新与总结

## 7. 完成状态与总结（2026-08-24）

**状态：已完成（HTML + 文档 + 验证全部通过）；git 提交待用户确认后进行。**

### 7.1 执行结果

- **situation/index.html**（806 行）：工具栏新增探测控制组三按钮 `SIT-TB-DET-RESET`/`START`/`STOP`（f2f9/f04b/f04d，启用，插入视角复位之前，组间 tb-separator 分隔，tooltip 标注镜像语义）；`SIT-NAV-02`/`03` 改 `<a>` 真实链接（../detection、../decision），04-06 保持占位；JS 高亮选择器改 `.nav-item:not([href])`；三键点击向 `SIT-SB-ALARM` 追加模拟告警（"探测已开始"/"探测已结束，视频回 0s"/"探测已重置"，上限 4 条）。
- **detection/index.html**（554 行，结构级重写）：顶栏 AI 开关 + 帧摘要；左栏 360px 七列结果表；中栏视频查看器（异常帧红框 + ID/类型/置信度，正常帧无框，空态"等待检测结果"）+ AI 分类条；右栏 380px 目标详情（8 字段）+ 异常热力图 + 状态时间线；底部 44px 操作条（确认/拒绝/移除记录 + 状态标签）；与态势页同构状态栏。退役 21 个旧 ID、新增 7 个、共 28 个 ID；本地 fixture 3 异常帧 + 2 正常帧；初始默认选中 target-001（CURRENT 初始为未选中空态，已知原型偏差已在 detection.md 标注）。
- **decision/index.html**（1222 行）：`DEC-NAV-01`/`02` 改 `<a>` 真实链接；`:not([href])` 选择器；L1195 注释更正为"真实链接路由（与 CURRENT 一致）"。
- **文档同步**：
  - situation.md（15 处编辑，751 行）：§6.1 导航行/备注（02/03 真实路由）、§6.3 工具栏表新增探测控制 3 行、§12 ID 索引补 3 行、RESET 行陈旧 SIT-RP-RESET 引用清理等。
  - detection.md 整页重写（198 行）：区域、28 个 ID、交互、CURRENT 映射、退役 ID 说明。
  - application-shell.md（E1-E16 全部编辑点，263 行）：§2 布局与 m_pageStack 三页结构、§2.1 出处列漂移注、§3.2 导航原型行为、§5.1 工具栏清单（10 行）、§5.2 交互与 CURRENT 映射、§7 去重 SIT-NAV-03 并修正 04/05/06 行为行、§9 行号引用全部对齐源码。
  - pages/index.md（135 行）：探测 section 重写（5 行关键区域表、28 ID 声明、198 行引用）+ situation 行数引用更新（751 行）。
  - decision.md：grep 核查无陈旧导航文字，无需修订（§3.4 条件不触发）。
- **计划外连带更新（由 ui/README §9.1 同步规则强制）**：design-system.md L365 图标挂载状态更新为 14 枚字形全部挂载（含本批接入的态势工具栏 7 枚）；application-shell.md §6.2 菜单 JS 行为单元格陈旧文字修正。

### 7.2 验证（证据：.omo/evidence/prototype-structure-resync/VERIFICATION.md）

- grep 断言全部通过：21 个退役 ID 双文件 0 命中；7 个新 ID 齐备；detection.md ID 清单恰为 28 个；SIT-TB-DET 三 testid、三页互链 `<a>`、`:not([href])` 全部就位；application-shell.md 陈旧引用清零（唯一 "QLabel 占位" 命中为有意保留的历史说明句）。
- 三页 overview-1920x1080.png 重新生成（19:01，晚于 HTML 最后修改）并目视核查通过（态势 7 按钮工具栏 + [模拟] 角标、探测三栏 + 操作条 + 状态栏、图标无 tofu）。
- git status 仅预期文件变更；无运行中子任务/构建/测试/GUI 进程。

### 7.3 遗留与已知偏差

- situation 原型右面板无 SIT-RP-* testid（浮层实现），粒度与逐控件契约存在偏差（沿前置计划已知项）。
- situation.md §11 引用的 loadMockData 行号可能滞后，留待下次文档校对批次。
- 1280×720 与 3840×2160 两视口截图仍未交付（ui/README §8 后续任务，非本计划范围）。

**更新（2026-08-25）**：上述三项遗留已由 `.omo/plans/20260825-situation-rp-testids-and-viewports.md` 批次全部解决——SIT-RP-* 8 枚 testid 已挂载并通过 29 项 Playwright 运行时断言；§11 行号已按源码校准；六页三视口截图已交付，README §7 已记录固定画布现状。
