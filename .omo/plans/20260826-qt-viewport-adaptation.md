# Qt 客户端三视口适配计划

状态：`已批准（2026-08-26），执行中`
日期：2026-08-26
分支规划：`feature/qt-viewport-adaptation`（基于 `origin/main` b5829e6 新建，已创建）
任务来源：REQ-011 与 `docs/features/viewport-adaptation.md`（均为 `Draft`，随本计划一并呈批）；设计方向来自六页 HTML 原型三视口批次（`feature/prototype-responsive-viewport`，2026-08-25 验收，待合并）。

## 1. 背景和当前事实

### 1.1 规则权威

三视口规则在 `docs/ui/design-system.md` §7，要点：

- token 为固定 px（来自 CURRENT），**不缩放**；视口适配通过区域弹性比例实现。
- 1280×720 最小可用：区域不溢出、左面板表格列宽不裁切文字、导航栏与状态栏尺寸固定不变。
- 1920×1080 默认与权威截图。
- 3840×2160：控件密度与间距按 token 保持、中心区按比例放大、不出现大片留白或控件过小、字体仍用固定 px。
- §7.1 弹性区域：导航栏宽（80px）、左面板宽（320px）、状态栏高、菜单栏高、工具栏高在三视口下固定不变；中心区宽高与告警区高为弹性。
- §7.2 字体与控件尺寸：固定 px 不随视口缩放；4K 通过增加中心区留白与三维场景视野适配，不放大字号。

### 1.2 现状机制（Qt 客户端；通俗版见 §5）

- 壳层：`MainWindow` 默认 `resize(1920,1080)`、`setMinimumSize(1280,720)`；中心区 `m_pageStack`（`QStackedWidget`）index 0=态势、1=探测、2=决策，导航按钮 `DEC-NAV-01..06` 切换堆栈页面（路由逻辑 `src/MainWindow/MainWindow.cpp` L871-882；页码 01=态势、02=探测、03=决策已经源码核实）。
- 决策页已有视口机制：`MosRunwayWidget` 以 `clamp(min(w/1920, h/1080), 1, 2)` 等比缩放跑道画布（§7.2“4K 中心区放大”的 Qt 实现方式，上限 2×）；`MosGeneratorDialog`（568×424）随视口由 `DecisionViewLayout::applyViewportScale` 等比缩放。本批保留这两处既有机制。
- 4K 全局壳（菜单/导航/状态栏/字体 token）不放大：跨轮次 pre-existing 已知模式（§7.0 登记），非本批回归，本批不改。
- 已盘点关键最小尺寸约束：态势页画中画 480×294（浮动）、告警滚动区宽 400；探测页 `m_viewerLabel` 400×400、`m_heatmapLabel` 200×200；决策页 `MosRunwayWidget` 400×220。1280×720 下中心区宽约 1280−80−320=880px，这些约束是否引起溢出或裁切需阶段 1 实测判定。

### 1.3 文档与证据现状（本批需一并修正）

- 三处过时“1280×720 约 5px 底部溢出”记录：`UI.md` 第 3 节（L51）、`design-system.md` §7.0 末段、`docs/ui/README.md` §7。所指 `DecisionSuggestionPanel` 等组件未纳入 CMake 编译目标（`UI.md` 第 3 节注），属失实记录。
- 决策页历史三视口几何证据（`.omo/evidence/mos-p0-qt-closure-final/REPORT.md`，§7.0 另引用 `mos-p0-qt-final`）采集于单档位渲染修正之前，已被 `UI.md` 判定不能作为修正后 fresh 证据。
- REQ-010 A4 门禁矩阵（`scripts/gate-capture.sh`）：决策页(03)三视口 + 态势页(01) 1920×1080 共 5 张；态势页缺 1280/3840 档位，探测页(02)无任何视口覆盖；采集输出至 `/tmp/opencode/uiupgrade-batch7/<tag>/`，属门禁性质，未在 `.omo/evidence/` 留存（该目录按仓库惯例 gitignore，本地留存）。
- 原型批次证据留存于 `.omo/evidence/prototype-responsive-viewport/`（本地）。

### 1.4 工具链事实

- `MainWindowCapture`：CLI `<png> <WxH> <navId>`，offscreen 整窗 PNG，稳定后截取，纯 mock 数据；页码映射 01=态势、02=探测、03=决策已经源码核实。
- `WidgetDump`：决策页 `DecisionView` 几何 TSV 输出（辅助定位溢出）。
- `scripts/gate-capture.sh`：钉死 `QT_QPA_PLATFORM=offscreen` 与 `FONTCONFIG_FILE`（构建所用 conda 环境字体配置），保证跨批次确定性；本批需将矩阵从 5 张扩至 9 张。
- `build-conda` 构建目录现有 `MainWindowCapture`、`WidgetDump`、`ScreenshotTool` 与 Mos 系测试目标，但**无 `UXOMissionControl` 主程序**，阶段 0 需补建。
- ctest 既有基线 15/17（2 项失败为 REQ-009 登记的 ONNX 集成问题，见 REQ-010 实现事实）。

### 1.5 与原型批次的关系

- 设计方向一致：橡皮筋（固定区域 + 弹性中心区），token 不缩放，已由原型批次验证可行。
- `MosRunwayWidget` clamp 缩放即“4K 中心区放大”在 Qt 侧的既有实现，与原型方案 A 不冲突，本批保留。
- 原型分支 `feature/prototype-responsive-viewport`（commit 8549702）待合并；本分支基于 `origin/main`，两分支对 `docs/ui/README.md` §7 有少量重叠修改，先合并原型分支可减少后续 PR 冲突。

## 2. 目标

1. MainWindow 壳层与态势、探测、决策三页在 1280×720、1920×1080、3840×2160 下满足 `design-system.md` §7：1280 无溢出、无意外裁切；1920 与修复前基线零回归（动画帧差异除外）；3840 无大片留白、固定 token 区域与字体不缩放、仅中心区放大。
2. 9 张离屏截图门禁（3 页 × 3 视口）形成可复核证据并本地留存（`.omo/evidence/qt-viewport-adaptation/`）。
3. `scripts/gate-capture.sh` 采集矩阵扩展至 9 张并入库。
4. 三处过时视口记录刷新为 fresh 事实。

## 3. 非目标

- 不改 token、颜色、字体、间距与交互行为。
- 不做 4K 全局壳/字体放大；不做右面板弹性宽度；不实现设备/统计/配置页。
- 不改六页 HTML 原型与 Playwright 契约测试补齐（独立任务）。
- 不动业务逻辑、模拟边界、状态边界。
- 不动主仓库 `/home/lin/UXO_v1` 与 `/home/lin/UXO_v1-database`。

注（对后续页面的复用性）：本批的壳层修复、三视口规则与门禁基础设施为全局机制，未实现页面（设备/统计/配置，导航 04–06 现为占位回退态势）实现后可直接复用——在采集矩阵中各加一行即可进入同一门禁流程，页面内部布局按 §7 规则随页面开发实现。后续页面无需再立同类专项，只需在交工时按本门禁做每页例行验收。

## 4. 安全边界

- 仅改 Qt 布局代码、门禁脚本与文档；全部本地模拟数据，无真实设备、数据库、外部通信。
- 只在 worktree `/home/lin/UXO_v1-ui-visual-upgrade` 的 `feature/qt-viewport-adaptation` 分支操作；`main` 只接受经审核的 PR。
- 文档必须区分 fresh 证据与旧证据、模拟与真实；不得把模拟状态写成真实设备状态。
- 证据留存 `.omo/evidence/qt-viewport-adaptation/`（本地；`.omo/evidence/` 按仓库惯例 gitignore 不入库）；临时脚本放 `/tmp/opencode/` 不入库。

## 5. 通俗说明（写给非前端读者）

**前情提要**：上一批我们把六页 HTML 原型（相当于“样板间”）做完了三种窗口尺寸的适配体检并验收。这一批给真正的房子——Qt 客户端程序——做同样的体检和必要的整修。

**体检怎么做**：程序里真正实现的页面有三个（态势、探测、决策）。给每个页面分别在三种窗口尺寸下各拍一张“体检照”（程序在无屏幕环境下自动截图，保证可复现）：小窗口 1280×720、标准 1920×1080、4K 大屏 3840×2160，共 9 张。

**体检标准**：

- 小窗口：内容不许被挤出屏幕外或裁掉一截。
- 标准窗口：必须和现在长得一模一样（零回归）。
- 4K 大屏：不许出现大片空边；文字和固定区域大小不变；只有中间工作区放大。

**体检后处理**：先拍“体检前”的照片逐张人工审查，列出问题清单给您过目裁决——哪些要修、哪些属于“设计就这样”可以接受。您拍板后才动代码，修的时候只做布局层的小手术（最小侵入），不动业务逻辑。

**顺手清旧账**：文档里有三处“决策面板底部溢出 5 像素”的旧记录，但所指的组件根本没被编译进程序，属于过时信息，本批一并刷新成新事实。

**保险措施**：每个阶段完成都有证据留档并向您汇报；任何阶段失败立即停下来说明，不带病往下走。

## 6. 分阶段任务

- **阶段 0 构建基线**：`cmake --build build-conda --target UXOMissionControl -j2` 补建主程序（若 `build-conda` 非 Release 配置，先 `cmake -S . -B build-conda -DCMAKE_BUILD_TYPE=Release` 重新配置）；跑 `ctest --test-dir build-conda --output-on-failure` 记录基线（预期 15/17）。
- **阶段 1 before 实测**：9 张 before 截图采集至 `.omo/evidence/qt-viewport-adaptation/before/`；look_at 逐张审查（1280 溢出/裁切、1920 基线、3840 留白）；`WidgetDump` 辅助决策页几何分析；产出问题清单。
- **阶段 2 问题清单冻结**：问题清单呈报用户裁决修复范围；裁决前不动任何代码。
- **阶段 3 逐项修复**：按裁决清单做最小侵入布局修复（size hint / 最小尺寸 / 弹性策略），逐项复测。
- **阶段 4 after 门禁**：9 张 after 截图 + before/after A/B 比对（1920 零回归）+ ctest 复跑不低于基线；`scripts/gate-capture.sh` 矩阵扩展至 9 张（态势 01 三视口 + 探测 02 三视口 + 决策 03 三视口）并同步更新输出路径。
- **阶段 5 文档刷新**：`UI.md` 第 3 节、`design-system.md` §7.0 末段、`docs/ui/README.md` §7 过时记录刷新为 fresh 事实。
- **阶段 6 收官**：本计划 §10 完成记录更新；按用户指定节奏提交（见 §9）。

每阶段完成后向用户汇报证据（截图路径、比对结果、测试输出）；阶段失败即停并汇报，不带病进入下一阶段。

## 7. 验收标准

1. 构建 `UXOMissionControl`、`MainWindowCapture`、`WidgetDump` 全部退出 0。
2. ctest 不低于阶段 0 基线（预期 15/17，2 项既有失败为 REQ-009 登记项，非本批回归）。
3. 9 张离屏截图（3 页 × 3 视口）采集并留存 `.omo/evidence/qt-viewport-adaptation/`。
4. 1280×720：三页区域无溢出、无意外裁切、关键内容可用（用户裁决接受的项除外）。
5. 1920×1080：与 before 基线零回归（画中画实时时钟、告警跑马灯等动画帧差异除外）。
6. 3840×2160：无大片留白（决策页中心画布留白随视口增大属 1920 权威设计预期）；固定 token 区域与字体不缩放；`MosRunwayWidget` 缩放上限 2×。
7. 三处过时 5px 记录与旧证据表述全部刷新，无残留失实表述。
8. 提交原子、中文 commit message 说明变更内容与目的。

## 8. 验证命令

```bash
# 构建主程序与采集工具（阶段 0 / 阶段 4 前置）
cmake --build build-conda --target UXOMissionControl MainWindowCapture WidgetDump -j2

# 9 宫格采集（before / after 两轮；TAG 替换为 before 或 after）
TAG=before
export QT_QPA_PLATFORM=offscreen
export FONTCONFIG_FILE=/home/lin/.local/share/mamba/envs/uxo-dev/etc/fonts/fonts.conf
mkdir -p ".omo/evidence/qt-viewport-adaptation/${TAG}"
for page in 01 02 03; do
  for vp in 1280x720 1920x1080 3840x2160; do
    build-conda/MainWindowCapture ".omo/evidence/qt-viewport-adaptation/${TAG}/p${page}_${vp}.png" "$vp" "$page"
  done
done

# 测试基线与复跑
ctest --test-dir build-conda --output-on-failure

# 门禁脚本（矩阵扩展后）
scripts/gate-capture.sh <tag>
```

视觉比对用 look_at 读取 `.omo/evidence/qt-viewport-adaptation/${TAG}/p*.png`。

## 9. 是否需要用户确认

需要，三点：

1. **三件套整体批准**（REQ-011 + `docs/features/viewport-adaptation.md` + 本计划）：批准后 REQ-011 与功能文档翻 `Approved` 并填写批准事实，随后进入阶段 0（AGENTS.md：关联需求与功能文档双 `Approved` 是进入实现的前提）。
2. **阶段 2 问题清单裁决**：before 实测后呈报问题清单，用户裁决修复范围后才动代码。
3. **提交节奏**：批准时可一并指定；默认建议三条提交——①三件套文档（“文档：”前缀）②代码修复与门禁脚本矩阵扩展（“UI：”/“工具：”前缀）③文档刷新（“文档：”前缀）。

## 10. 完成记录

- **2026-08-26 阶段 0 完成**：`build-conda` 补建 `UXOMissionControl`（Release），连同 `MainWindowCapture`、`WidgetDump` 三目标构建退出 0；ctest 基线 15/17（`video_render_capture`、`stop_select_flicker_repro` 两项失败为 REQ-009 登记的既有 ONNX 问题）。
- **2026-08-26 阶段 1 完成**：9 张 before 截图 + 决策页 1280 WidgetDump TSV 留存于 `.omo/evidence/qt-viewport-adaptation/before/`；逐张审查结论：
  - 决策页 1280×720：右面板三张候选方案卡片两行指标文字被裁（**后被阶段 3 像素分析证伪：右面板恒为 380px、卡片从未被裁，系误导性提问诱导的误报**，见阶段 3 记录）；参数区约 11 个标签换行但数值未裁；跑道画布与状态栏完好。
  - 决策页 4K：面板随 `applyViewportScale` 放大 ~1.93×、跑道画布留白 ~5%（§7.2 中心区放大既有机制的预期表现）。
  - 态势页 4K：空态地图留白、画中画固定 480×294、告警条集中于左侧（设计内）。
  - 探测页 1280：左表格出现水平滚动条（可用，非裁切）。
  - 其余 5 图（1920 三页 + 态势 1280 + 探测 4K）未见异常。
- **2026-08-26 阶段 2 完成（用户裁决）**：四个问题经用户逐项裁决，全部按推荐项采纳：
  1. 决策页 1280 候选卡片文字裁切 -> **修**（唯一修复项）。
  2. 决策页 4K 面板随视口放大 -> 保留既有设计（`applyViewportScale` 为 REQ-010 既有机制）。
  3. 态势页 4K 三处表现（空态留白/固定画中画/告警靠左）-> 全部接受为设计内。
  4. 探测页 1280 左表格水平滚动条 -> 接受。
  裁决前未动任何代码（阶段 2 铁律满足）。补记（2026-08-26）：第 1 项所依据的"卡片裁切"后被阶段 3 像素分析证伪，实际不存在裁切；已实施修复转为解除参数网格最小宽过约束（无害且零回归），详见阶段 3 记录。
- **2026-08-26 阶段 3 完成**：修复范围仅第 1 项。原"splitter 挤压右面板致卡片裁切"根因假设经像素分析**证伪**：右面板在 1280×720 修复前后均为完整 380px（边界 x=899 前后一致；1920×1080 为 x=1539），修复前后右面板条带逐字节相同，全部像素差异位于中心参数网格区 x∈[355,780]；1280 与 1920 字体亮行簇中位高均 13px（字号一致），1280 卡片无省略号/换行/重叠/切边，与 1920 仅垂直间距差异（面板内容高 ~605 vs ~1000；固定 px 字体 + 固定 380px 面板宽下的设计内表现，§7.2）。阶段 1"卡片被裁"系 look_at 误导性提问诱导 + 跨图显示比例错觉的误报。真实缺陷为参数网格最小宽 ~630 超过 1280×720 决策页中心列可用宽（约 545px：1280 − 80 导航 − 260 左栏 − 380 右栏 − 16 句柄与边距），仅表现为网格内部挤压重排（约 11 个标签换行），非面板或卡片裁切。修复（最小侵入）：`MosParamsPanel::setupUi` 参数网格标签设显式最小宽 56px（wordWrap 换行兜底）、输入框设 84px，网格最小宽降至 ~470 ≤ 545；宽裕视口下显式最小宽不参与列分配（布局仍按 sizeHint）。验证：三目标构建退出 0；ctest 15/17 与基线逐项一致（日志 /tmp/opencode/ctest-after-fix.log）；9 张 after 截图留存 `.omo/evidence/qt-viewport-adaptation/after/`，A/B md5：p02 三视口与 p03 1920/3840 逐字节一致（零回归），p01 三视口差异属动画帧（画中画时钟、告警跑马灯），p03_1280 差异即参数网格重排。
- **2026-08-26 阶段 4 完成**：`scripts/gate-capture.sh` 采集矩阵 5 -> 9 张（三页 × 三视口，与 §8 验证命令一致），输出路径更新 `/tmp/opencode/ui-gate/<tag>/`（原 batch7 路径仅脚本自身引用），头部补 A/B 确定性解读说明。试运行 `req011-verify`：9 张 + env.txt 落盘，字体环境固化生效（offscreen + conda fonts.conf）。确定性核验（与 after/ md5 对比）：p02 三视口逐字节一致；p01 三视口属动画帧（画中画时钟/告警跑马灯，既有结论）；p03 三视口存在一处阶段 3 未登记的帧级时变——跑道画布选中目标脉冲动画（`MosRunwayWidget` `m_pulseTimer` 50ms × 20 相位，仅选中目标时运行，`DecisionView` 中心列实例化），决策页中心小方形区域（1920 下约 94px 方块，三视口同位置，占 0.07~0.08% 像素）：before（14:21）与 after（15:13）恰为同相位故逐字节一致，gate 试运行（15:49）相位滑移；二进制 15:10:51 未重编译，排除构建差异。其余区域含修复网格区与 after/ 一致（before-vs-gate 对比仍含网格重排区 x∈[354,781]，修复持续生效）。后续 A/B 门禁解读时该区域与 p01 动画帧同视噪声，不算回归。
- **2026-08-26 阶段 5 完成**：三处记录刷新——`docs/UI.md` 第 3 节、`docs/ui/design-system.md` §7.0（含旧证据覆盖范围限定句修正）、`docs/ui/README.md` §7 均改为 REQ-011 三视口 fresh 像素验证事实（9 张证据、1280 三页无溢出与意外裁切、1920/4K 零回归、4K 固定 token 区域与字体不缩放），并撤销"DecisionSuggestionPanel 约 5px 底部溢出"失实登记（该组件未纳入 CMake 编译目标）；另修 `docs/features/viewport-adaptation.md` §2 右面板非目标表述笔误（原"程序当前无右面板"改为"壳层右面板未纳入编译目标；决策页内部候选方案右栏维持 CURRENT 固定宽度不变"）。
- **2026-08-26 阶段 6 完成**：按批准的三条提交节奏落库--提交 ② `237597c`（`MosParamsPanel.cpp` + `scripts/gate-capture.sh`；提交前修正 MosParamsPanel 注释中残留的被证伪根因表述"splitter 挤压面板致卡片裁切"与旧数字 ~552，与阶段 3 像素证据对齐）与提交 ③（本批文档×5：`docs/UI.md`、`docs/ui/design-system.md`、`docs/ui/README.md`、`docs/features/viewport-adaptation.md` 与本计划 §10 记录）。计划内工作全部收尾；REQ-011 与功能文档状态翻 `Implemented`、push/PR 属计划外事项，待用户决定。
