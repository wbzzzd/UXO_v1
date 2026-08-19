# 指挥席客户端 UI 视觉升级计划

状态：Approved
计划所属 worktree：`/home/lin/UXO_v1-ui-visual-upgrade`
计划所属分支：`feature/ui-visual-upgrade`
创建日期：2026-08-13
修订日期：2026-08-14（修订 Momus 评审指出的 3 个 blocking issues 与事实修正）
关联需求：REQ-010（`docs/requirements/REQ-010.md`，`Approved`）
关联功能设计：`docs/features/ui-visual-upgrade.md`（`Approved`）

## 1. 背景与当前事实

### 1.1 应用场景定位

本仓库交付军事排弹抢修指控桌面客户端 `UXOMissionControl`。指控软件的视觉风格决策（暗色主题、高信息密度、克制装饰、威胁语义色阶、等宽遥测字体）是**合理的领域选择**，本计划不改风格方向。

用户反馈"界面像二十年前"。经源码与文档调查，根因不是风格选错，而是**执行未到位**：设计 token 体系名存实亡，平台原生控件外露，可读性指标偏低，几乎无图标辅助态势识别。

### 1.2 CURRENT 事实证据

| 维度 | 现状 | 证据 |
|------|------|------|
| 样式治理 | `GlobalStyle.cpp`(546 行)定义了覆盖 18 种控件的完整 QSS + token 体系，但 `src/` 有 **215 处内联 `setStyleSheet` 散落 22 个文件**绕开 token（全库含 tests 为 219/24） | explore 调查 + `design-system.md` 第 8 节 |
| 平台风格 | **未调用 `setStyle("Fusion")`**，依赖平台默认 style；QSS 只覆盖背景/前景色，控件几何与渲染细节仍由平台决定 | grep `setStyle\|QStyleFactory\|Fusion` 仅命中 `setStyleHint(QFont::Monospace)` |
| 字体执行 | `Fonts::Family`（`GlobalStyle.h:84`）是逗号分隔 QString（`"Microsoft YaHei, Source Han Sans SC, SimHei, sans-serif"`），若用 `QFont(Family, Size)` 构造会把整串当单一 family；`BodySize` 等 int 常量（`:86-88`）被当 pt，而设计文档用 px | `GlobalStyle.h:84,86-88` |
| 图标 | 几乎无图标，按钮纯文字，导航栏用 `◎` 文字符号占位；无 `.qrc` 资源、无 SVG 图标库 | grep 图标相关仅 14 处，集中在 `TacticalMapWidget` 绘图 |
| 可读性 | body 14px / caption 12px / 紧急停止 11px / 决策页参数栏标签 11px；`--color-text-secondary=#AAAAAA` on `#1E1E1E` 对比度 4.8:1（临界值） | `design-system.md` §2、§1.4 |
| GlobalStyle 缺陷 | 纵向 `QScrollBar:handle:hover` 颜色写错（`:211` 与 `:216-218` 与默认同值）；**横向 scrollbar `:227-231` 根本没有 hover 规则**（不止颜色错，是规则缺失）；`QDockWidget` 引用 `url(close.png)`（`:320`）但项目无 `.qrc`，图标加载失败 | `GlobalStyle.cpp:211,216-218,227-231,320` |
| 依赖 | 仅 Qt5 标准模块（Widgets/Gui/3D/Multimedia），**无任何现代 UI 库**（无 Qt Material、无 QtAwesome） | `CMakeLists.txt` find_package / target_link_libraries |
| 设计 token | `design-system.md` 已登记完整 token 体系（颜色/字体/尺寸/动画/组件原语/状态规则/三视口），方向正确 | `docs/ui/design-system.md` 全文 |
| 入口 | `main()` 在 `src/App/main.cpp:6` | 源码确认 |
| 截图工具 | `ScreenshotTool`（`CMakeLists.txt:309`）只截 1280×720 的 `DecisionView`（`screenshot_tool.cpp:43`），不能验证完整主窗口/三视口/跨平台/DPR；离屏渲染不构成像素保真证据 | 源码确认 + `DEVELOPMENT.md:98` |

### 1.3 治理边界说明

本计划是**视觉与工程治理改进**，非新功能开发，已纳入治理门禁：

- **关联需求**：REQ-010（UI 视觉治理，`Approved`，独立候选不并入 NEXT 切片），见 `docs/requirements/REQ-010.md`。
- **关联功能设计**：`docs/features/ui-visual-upgrade.md`（`Approved`）。
- 本计划不新增产品功能、不改状态边界、不改业务逻辑、不改模拟边界。
- 视觉 token 变更按 `docs/ui/README.md` §9 维护规则同步更新：修改 token 同步 `design-system.md` token 表（§9.1）；如涉及原型布局变化，同步 `pages/<page>.md` 与原型，并运行 `node prototypes/screenshot.js <page>` 重新生成截图（§9.2 步骤 5）。

## 2. 目标

### 2.1 必须达成

1. **token 体系重新生效**：消除绕开 `GlobalStyle` 的内联样式，所有视觉值取自 token。
2. **跨平台外观一致**：全局应用 Fusion 风格 + 强制字体，消除平台原生 3D 凸起感。
3. **可读性达标**：字号与对比度满足军事指控长时间值守的可读性要求。
4. **图标体系建立**：导航栏、工具栏、关键按钮配矢量图标，辅助态势快速识别。
5. **修复 GlobalStyle 缺陷**：scrollbar hover（纵向颜色 + 横向补规则）、close.png 等已知 bug。

### 2.2 非目标（明确排除）

- **不清理占位控件**（用户明确排除）。工具栏 QLabel 占位、未实现导航页、菜单占位项保持现状。
- 不改设计风格方向（暗色/克制/高密度保留）。
- 不重写为 QML/Qt Quick（与现有 Qt3D 集成冲突，成本过高）。
- 不引入干扰决策的花哨动效（波纹、卡片翻转、大范围位移动画）。
- 不做卡片化布局重构（军事指控高密度表格行更合适）。
- 不动业务逻辑、状态所有权、数据流、模拟边界。
- 不引入网络、持久化、设备控制、真实排爆能力。

## 3. 安全边界

- 只改视觉层：QSS、字体、图标、Fusion 风格、`GlobalStyle`、`design-system.md` token。
- 不改 `src/Core/`、`src/MainWindow/` 中的业务槽函数、信号连接、状态机、模拟数据。
- 模拟标注（"[模拟模式]"等）保留，不被视觉调整遮盖或弱化。
- 现有自动测试（`simulation_workflow_ui`、`mos_decision_ui` 等）必须继续通过，不退化。
- 不覆盖、回退或提交用户在 main worktree 的未跟踪文件。
- 所有代码变更写中文注释，commit message 说明变更内容与目的。

## 4. 分阶段任务

每个阶段结束设门禁：构建通过 + 现有测试全绿 + 视觉检查 + **用户确认后才进下一阶段**。

### 阶段 1（P0 治理与基础）- 让 token 生效、统一外观

> 目标：消除"二十年前感"的最大元凶（平台原生控件外露 + 样式碎片化），性价比最高。

- [ ] **1.1 全局应用 Fusion 风格与强制字体**
  - 在 `main()`（`src/App/main.cpp:6`）调用 `QApplication::setStyle(QStyleFactory::create("Fusion"))`。
  - 字体设置修正（`Fonts::Family` 是逗号分隔串，不能用 `QFont(Family, Size)` 构造）：
    - 在 `GlobalStyle.h` 新增 `Fonts::FamilyList`（`QStringList{"Microsoft YaHei", "Source Han Sans SC", "SimHei", "sans-serif"}`）。
    - main.cpp 中：`QFont font; font.setFamilies(GlobalStyle::Fonts::FamilyList); font.setPixelSize(GlobalStyle::Fonts::BodySize); QApplication::setFont(font);`（用 `setPixelSize` 而非 pointSize，因设计文档用 px）。
    - `setFamilies` 需 Qt >= 5.13，执行时确认项目 Qt 版本满足。
  - 验证 Fusion 在目标部署平台（Linux/Windows）可用；不可用时回退并记录。
  - 验收：同一份构建在 Linux xcb 与 Windows 上控件外观一致；按钮/滚动条/下拉箭头不再有平台 3D 凸起。

- [ ] **1.2 收敛内联 `setStyleSheet` 到 `GlobalStyle` getter**
  - 全量扫描 `src/` 的 215 处 `setStyleSheet` 调用（22 个文件），分类：
    - (a) 重复 `GlobalStyle` 已有 QSS 的 -> 删除内联，依赖全局样式。
    - (b) `GlobalStyle` 未覆盖的局部样式 -> 评估是否新增 token 与 getter，或保留为局部样式但取 token 值。
    - (c) 业务临时态（如运行时动态变色）-> 保留，但颜色取 `Colors::*` 而非字面量。
  - 禁止业务代码硬编码颜色/字号字面量；新增视觉值必须先在 `design-system.md` 登记。
  - 验收：`rg "setStyleSheet" src/` 数量 < 50，仅保留运行时动态态；`rg "#[0-9A-Fa-f]{6}" src/ -g '!**/GlobalStyle.cpp'` 在 `.cpp` 中的字面量清零或附说明。（2026-08-19 用户裁决修订：阈值调整为 ≤90，且剩余调用均为登记保留的运行时动态态/完整复合样式/非等值 hex；批次6 深转换后实测 89，基线 215。）

- [ ] **1.3 修复 `GlobalStyle.cpp` 已知缺陷**
  - 纵向 `QScrollBar:handle:vertical:hover`（`:211`、`:216-218`）颜色改为更亮值（`Colors::TextPrimary` 或新增 `ScrollbarHandleHover` token）。
  - **横向 scrollbar `:227-231` 补加 `:horizontal:hover` 规则**（当前完全缺失，不止颜色错）。
  - `QDockWidget` 的 `titlebar-close-icon: url(close.png)`（`:320`）资源缺失：要么引入 `.qrc` 图标资源，要么移除该声明走 QSS 默认。
  - 验收：纵向与横向 scrollbar hover 视觉均可区分；`QDockWidget` 标题栏无资源加载警告。

**阶段 1 门禁**：
- `cmake --build build --target UXOMissionControl -j2` 退出 0。
- `ctest` 现有测试全绿。
- 视觉验证（**ScreenshotTool 局限说明**：`screenshot_tool.cpp:43` 只截 1280×720 `DecisionView`，不能验证完整主窗口/三视口/跨平台/DPR；离屏渲染不构成像素保真证据，见 `DEVELOPMENT.md:98`）：
  - 采用以下之一：(a) 扩展 ScreenshotTool 支持完整主窗口三视口截图（1280/1920/4K + DPR 记录），输出 PNG + 几何 TSV（`overflow_rows=0`）；(b) 在 xcb 环境运行 `UXOMissionControl` + 脚本/手动截完整主窗口，保存 PNG 并记录 DPR。
  - 明确记录：截图环境（Linux xcb / Windows）、DPR、几何阈值（`overflow_rows=0`）、预期结果（控件外观统一，无平台原生残留）。
- **暂停，等用户确认效果后进入阶段 2。**

### 阶段 2（P1 可读性与图标）- 军事硬指标

> 目标：可读性与态势识别能力达标，这是军事指控的硬指标。

- [ ] **2.1 提升 token 可读性（同步更新 `design-system.md`）**
  - 字号：`--font-size-body` 14->15px；`--font-size-caption` 12->13px；`--font-size-title` 16->17px。
  - 对比度：`--color-text-secondary` `#AAAAAA`->`#B8B8B8`（对比度 4.8:1 -> ~6.3:1，达 WCAG AA）。
  - 同步更新 `design-system.md` §1.4、§2 token 表与 `GlobalStyle.h` 常量（`Fonts::BodySize` 等 int 值 + `Colors::TextSecondary`）。
  - 按 `docs/ui/README.md` §9.1，token 变更须同步 `design-system.md` token 表；如涉及原型，运行 `node prototypes/screenshot.js <page>` 重新生成截图（§9.2 步骤 5）。
  - 验收：三视口下辅助文本可读；`design-system.md` token 表与源码一致。

- [ ] **2.2 引入图标体系**
  - **选型决策（需用户确认）**，候选：
    - 选项 A：`QtAwesome`（Font Awesome 字体图标，MIT，CMake 可集成，矢量缩放清晰）- 推荐
    - 选项 B：自绘 SVG 图标集 + `.qrc`（完全可控，工作量大）
    - 选项 C：QSS `image: url(...)` + PNG 资源（最简，但非矢量，缩放发虚）
  - 为导航栏 6 项、工具栏可点击项、关键按钮（确认/处置/完成/紧急停止/重置）配图标。
  - 图标颜色遵循语义：导航选中态主色、危险操作红色、默认辅助色。
  - 验收：图标在三视口下清晰无发虚；图标与文字双行排版对齐；色盲友好（图标形状可区分，不仅靠颜色）。

- [ ] **2.3 圆角与间距微调（同步 token）**
  - `--radius-control` 4->6px（控件圆角，更现代但仍克制）。
  - 按钮/输入框内边距适度增加（`--space-button-pad-y` 6->7px，保持比例）。
  - 面板间分隔从紧贴 1px 边框改为可呼吸的间距（评估是否引入 `--space-panel-gap` token）。
  - 验收：三视口下无拥挤感，信息密度仍保持军事指控风格。

**阶段 2 门禁**：
- 构建通过 + 现有测试全绿。
- 三视口视觉检查：可读性显著提升，图标清晰。
- `design-system.md` 与 `GlobalStyle` token 一致性校验通过。
- **暂停，等用户确认效果后进入阶段 3。**

### 阶段 3（P2 现代化，可选）- 需评审

> 目标：在军事克制风格内的精致度提升。本阶段整体可选，每项可独立取舍。

- [ ] **3.1 补 elevation/阴影 token**
  - 在 `design-system.md` 新增 `--elevation-*` token（如浮动面板 `--elevation-overlay: 0 4px 12px rgba(0,0,0,0.4)`）。
  - 应用到浮动面板：视频 PiP、目标详情浮层、`MosGeneratorDialog` 模态。
  - **注意**：QSS 不支持 `box-shadow`，需用 `QGraphicsDropShadowEffect` 实现。
  - 验收：浮动面板有明确纵深，不喧宾夺主。

- [ ] **3.2 克制过渡动画**
  - **QSS 不支持 CSS `transition`**，改用 `QPropertyAnimation` 实现有限的 hover 颜色/几何过渡（150ms，复用 `Animation::DurationShort`）。
  - 或直接取消本项（军事场景本就克制动画，取消合理）。
  - 不加干扰决策的大范围位移动画。
  - 验收：交互反馈柔顺（若实施），决策界面无干扰。

- [ ] **3.3 QSS 主题框架评估（需用户决定是否引入）**
  - 评估 `Qt-Material`（MIT）或类似库是否能一次性提供现代化效果，对比自绘 QSS 的成本/可控性。
  - **不擅自引入第三方依赖**，出评估报告由用户决定。
  - 若决定不引入，本项取消，沿用阶段 1-2 的自绘 QSS。

**阶段 3 门禁**：
- 每项独立验收，独立确认。
- 不引入未评审的第三方依赖。

## 5. 验收标准（整体）

| 编号 | 标准 |
|------|------|
| A1 | `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --target UXOMissionControl -j2` 退出 0 |
| A2 | `ctest` 现有测试全绿，无新增失败 |
| A3 | `rg "setStyleSheet" src/` 数量 < 50（基线 215），且无业务代码硬编码颜色字面量 |
| A4 | 三视口（1280/1920/4K）视觉检查 `overflow_rows=0`、无裁切、无平台原生残留；截图环境与 DPR 已记录 |
| A5 | `design-system.md` token 表与 `GlobalStyle.h`/`GlobalStyle.cpp` 完全一致 |
| A6 | 图标在三视口下清晰，色盲友好（形状可区分） |
| A7 | 对比度达 WCAG AA（辅助文本 >= 4.5:1） |
| A8 | 模拟标注保留且可读 |
| A9 | `git diff --check` 退出 0，改动范围限定在视觉层：`GlobalStyle.*`、`include/Common/GlobalStyle.h`、`src/App/main.cpp`、图标资源（`.qrc`）、`docs/ui/design-system.md`、`docs/ui/README.md`（若同步规则要求），以及 `src/MainWindow/*.cpp` 中含内联 `setStyleSheet` 的控件文件（收敛 1.2 所必需，逐文件清单在阶段 1 执行时确认并记录于此） |

## 6. 验证命令

```bash
# 构建
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target UXOMissionControl -j2

# 测试
ctest --test-dir build --output-on-failure

# 治理校验
rg "setStyleSheet" src/                     # 内联样式计数（基线 215，目标 < 50）
rg "#[0-9A-Fa-f]{6}" src/ -g '!**/GlobalStyle.cpp'  # 业务代码颜色字面量
rg "setStyle|QStyleFactory" src/App/       # Fusion 应用确认

# 视觉验证
# ScreenshotTool 局限：只截 1280×720 DecisionView，不能验证完整主窗口。
# 阶段 1 执行时确定方案：(a) 扩展 ScreenshotTool 支持完整主窗口三视口；或 (b) xcb 手动截图。
# 无论哪种，记录截图环境、DPR、几何阈值（overflow_rows=0）。

# 文档一致性
diff <(rg "Colors::|Fonts::|Sizes::" include/Common/GlobalStyle.h) <(rg "\| `#|`[0-9]+px`" docs/ui/design-system.md) || true
```

## 7. 是否需要用户确认

| 节点 | 确认内容 |
|------|----------|
| 计划评审 | REQ-010 与 features 文档的 Approved 状态（当前草案） |
| 阶段 1 后 | Fusion + 收敛内联样式 的实际视觉效果 |
| 阶段 2 前 | 图标库选型（QtAwesome / 自绘 SVG / PNG） |
| 阶段 3 前 | 是否做阶段 3，以及 3.3 是否引入第三方 QSS 框架 |
| 合并前 | PR 审核（按 `DEVELOPMENT.md` §6.1，PR 是进入 main 的唯一入口） |

## 8. 风险与回退

| 风险 | 缓解 |
|------|------|
| 收敛 215 处内联样式工作量大、易引入回归 | 分文件分批，每批后跑测试；保留业务动态态 |
| Fusion 风格在某些部署平台不可用 | 执行时探测，不可用回退并记录 |
| 图标库引入增加依赖 | 阶段 2 才引入，且选型经用户确认；QtAwesome 仅 header + 字体文件 |
| token 改动影响 HTML 原型一致性 | 按 `docs/ui/README.md` §9 同步更新 `design-system.md`、原型与截图 |
| QSS 能力限制（不支持 transition/box-shadow） | 3.1 用 `QGraphicsDropShadowEffect`，3.2 用 `QPropertyAnimation` 或取消 |
| `setFamilies` 需 Qt >= 5.13 | 执行时确认 Qt 版本，不满足则拆分主 family + 手动 fallback |
| 视觉改动被用户否决 | 每阶段独立 commit，可独立回退；阶段 1 是基础，阶段 2/3 可独立取舍 |

## 9. 完成条件

阶段 1-2 完成且验收 A1-A9 全部通过，本计划标记为 Implemented。阶段 3 为可选增量，不阻塞主交付。完成后由用户决定是否走 PR 合入 main。

## 10. 执行记录（阶段 2 进行中）

- **批次1（已完成，已验证）**：NavigationWidget（4→0）、LeftPanelWidget（13→2）、RightPanelWidget（5→1）内联样式收敛至全局 QSS 词汇（containerBg / btnVariant / labelRole / textColor / cardRadius / edgeBorder 等）。build 100% + ctest 16/16 + 6 页像素比对噪声级。证据：`/tmp/opencode/uiupgrade-batch1/`。
- **批次2（已完成，已验证，2026-08-17）**：MainWindow 地图工具栏容器、7 个工具栏按钮、simTag 收敛 + GlobalStyle 新增 `btnVariant="flat"` 变体。期间出现工具栏底色回归（46,646px #2D2D2D→#1E1E1E，y66-97×x400-1920 恰为工具栏几何）；根因：`containerBg` 属性在 `addWidget` 之后设置导致 QSS 规则不匹配。修复：属性移至创建点设置（property 先于 attribute、先于 addWidget，与 statusTabs 工作模式一致）。build 100% + ctest 16/16 + 6 页比对 0.24%-0.26% 噪声级（p3 决策页仅 0.002%）。证据：`/tmp/opencode/uiupgrade-batch2/`。
- **批次3（已完成，已验证，2026-08-18）**：态势页族 14 文件内联样式收敛（AlertPanel/BatchOperationBar/DecisionView/DetectionControlPanel/DeviceResourceBar/DeviceStatusPanel/MosGeneratorDialog/PlanCardWidget/SituationView/StatusBarWidget/TargetCardWidget/TargetDetailOverlay/TargetDetailPanel/VideoStreamPanel）。首轮后 p3 决策页噪声级（2581, 0.124%）但态势页族 5 页 ~9.7k-10.3k FAIL；像素取证定位 4 热点并统一根因：基线容器裸样式表（无选择器）的 background/border 通过 QSS 级联被子孙 QLabel 原生绘制，属性化改造后级联消失（DRB 卡标签=centralWidget #1E1E1E 级联、视频标题=VideoStreamPanel 级联、状态栏三标签顶边框=StatusBarWidget 级联、x79 竖线=NavigationWidget border-right 级联）。修复：GlobalStyle 词汇新增 `labelBg="main"`（置于 textColor 之后：同特异度后声明者胜，background-color 长属性覆盖 background:transparent 简写）+ StatusBarWidget 三标签 labelBg/edgeBorder（deviceStatusLabel 另 textColor="white"）、DRB 三卡标签 labelBg、VideoStreamPanel 标题 labelBg、导航 LOGO edgeBorder="right"。附带证实 QSS 盒（背景+边框）影响 QLabel sizeHint：恢复后状态栏分隔线由 x113/229/321 回到基线 x120/243/343。build 100% + ctest 16/16 + 6 页像素比对 632-1638px（0.030%-0.079%）全部噪声级。证据：`/tmp/opencode/uiupgrade-batch3/`。
- **批次4（已完成，已验证，2026-08-18）**：决策页族收敛 + 全局词汇扩展。GlobalStyle：`textColor="disabled"`（恢复风险标签降级色语义）、`labelBg="panel"/"toolbar"/"chip"`、`stateBanner` 7 变体（idle/planning/loading/ok/error/empty/nofeasible），全部复用现有 %1-%26 占位符零新增 .arg。DecisionSuggestionPanel 15 处编辑；MosParamsPanel 8 处编辑（chip 计数盒、monoLabel、planStateBanner 首帧 labelBg、setPlanState 全 switch 改 stateBanner 属性 + repolish、revalidate ok/error）。DSP 为死代码（见后续发现），其收敛像素惰性。build 100% + ctest 16/16。
  像素门禁：基础设施因系统重启丢失（批次1-3 证据目录 `/tmp/opencode/uiupgrade-batch{1,2,3}/` 已不存在）；本次以 0d8fab8 基线 worktree（`/tmp/opencode/uiupgrade-baseline-wt/`）重建离屏 A/B 管线（QT_QPA_PLATFORM=offscreen 1920x1080，DEC-NAV-01..06 六页导航 + 事件循环 settle），工具与证据：`/tmp/opencode/uiupgrade-batch4/`。A/B 结果：p01/p02/p04/p05/p06 噪声级 PASS（0.0057%-0.0166%）；p03 14.028%（290,885px）FAIL。取证分解（自噪声对照 p03 全图 0 差异=完全确定性；几何 dump 两侧逐行一致）：(1) 左列目标列表 92.07%（222,148px）= **批次3 遗留回归**——批次3 删除 m_targetList 内联 `QListWidget{background:#1E1E1E}`（注释误称“已由全局 QSS 覆盖”），实际父级 m_leftPanel 裸样式表 `background-color:PanelBackground` 的级联在 Qt 合并顺序中晚于应用级全局规则、压过 `QListWidget{background:%1=Background}`，列表底色 #1E1E1E→#252526，item 徽章色系 +6 为同底色 alpha 合成跟随；批次3 门禁记录 p3 0.124% 与此矛盾，当时参考图疑似污染或比对缺陷（证据已丢失无法复核），修复方向待用户裁决；(2) MPP 区 19.63%（67,140px，集中 y869-996 状态横幅/按钮/validation）= 批次4 预期 stateBanner/labelBg token 收敛，参数输入区 y741-868 零差异；(3) 跑道区 0.077%（1,597px，x833-927×y246-340 红色系 alpha 合成微移）低于 0.2% 阈值且非批次4 触及文件，判亚阈值良性；(4) 右列 0.00%。注：p03 的 0.2% 页阈仅适用于非预期差异；批次4 预期收敛（MPP 区）单独构成约 3.2% 页占比，门禁判定应按“预期区域外达噪声级”执行。
- **进度指标（2026-08-17）**：src/ `setStyleSheet` 计数 215→183（目标 <50）；MainWindow.cpp 硬编码色 0。
- **进度指标（2026-08-18）**：src/ `setStyleSheet` 计数 183→171（目标 <50）；MainWindow.cpp 硬编码色 0。
- **进度指标（2026-08-18，批次4）**：src/ `setStyleSheet` 计数 171→153（目标 <50）；MainWindow.cpp 硬编码色 0。
- **发现**：RightPanelWidget 经 grep 确认未被任何代码实例化（死代码），其样式收敛为像素中性操作。
- **剩余文件清单（批次3/4 候选）**：DecisionView 28、TargetDetailPanel 20、TargetDetailOverlay 19、DecisionSuggestionPanel 14、SituationView 10、DeviceStatusPanel 10、TargetCardWidget 9、AlertPanel 9、StatusBarWidget 9、VideoStreamPanel 9、PlanCardWidget 9、DetectionControlPanel 8、DeviceResourceBar 7、MosParamsPanel 7、MosGeneratorDialog 4、BatchOperationBar 4、DecisionViewLayout 2、DecisionViewTier 1（部分属运行时动态态，转换时逐条判定可保留）。
- **p03 左列回归修复（已完成，已验证，2026-08-18）**：用户裁决“恢复原底色”。DecisionView.cpp m_targetList 恢复内联 token 规则（`QListWidget{background-color:Background;border:none}` 与 item:selected 透明），中文注释标明 Qt 级联约束：父级 m_leftPanel 裸样式表的 PanelBackground 声明在 Qt 合并顺序中晚于应用级全局 QListWidget 规则，省略本规则即回归 #252526。build 100% + ctest 16/16；门禁复验（current3 对 基线）：左列 0/241,280px（0.00%，区域 top 色构成与基线逐字节一致，底色恢复 #1E1E1E），p03 残差 3.316%（68,760px）= MPP 批次4 预期收敛 67,140px + 跑道亚阈值微移 1,620px，其余 5 页 0.0115%-0.0226% 噪声级 PASS。采集工具重链段错误根因：手工链接偏离 cmake 配方，须 conda 工具链驱动 + Qt 全路径 .so + QT_NO_DEBUG/QT_*_LIB 定义 + 静态库顺序（MainWindow/Core/Common），已固化 `/tmp/opencode/uiupgrade-batch4/build_gate.sh`。证据：`/tmp/opencode/uiupgrade-batch4/current3/`。
- **1.1/1.3 核实闭环（2026-08-18，分支 feature/ui-stage1-batch5）**：经源码与 git 取证，1.1 与 1.3 均已随 2d665fa 落地，此前计划清单未同步。1.1：Application.cpp:21-27 已 setStyle(QStyleFactory::create("Fusion")) 并 setFamilies(Fonts::FamilyList)+setPixelSize(BodySize)（落在 Application 构造函数而非计划所写 main.cpp，效果等同）；Linux xcb 下 Qt5 默认风格即 Fusion，current3 门禁即 Fusion 生效下采集、与基线像素一致属预期，Windows 外观一致性留待用户侧 Windows 构建确认。1.3：GlobalStyle.cpp 纵横滚动条 handle hover 规则均在（:216/:232，hover 用 %3=TextPrimary #FFFFFF，常态 %10=TextDisabled #888888，视觉可区分）；QDockWidget 段无 url(close.png)（全文件无 url() 引用），资源加载警告不可能发生。
- **批次5（已完成，已验证，2026-08-18，分支 feature/ui-stage1-batch5）**：QLabel 属性词汇收敛批次，`setStyleSheet` 计数 142->107。GlobalStyle.cpp 词汇区新增 `QLabel[fontSize="9"/"10"/"11"/"13"/"14"]`、`QLabel[fontWeight="bold"]`、`QLabel[fontFamily="mono"]` 属性规则（L386-401），design-system.md 同步登记（9/10/11/13 为存量字号收敛登记）。转换 35 处：SBW×4、DSP×3、PCW×3、AP×3、BOB×1、DV-suggest×1、DCP×1、SV×2、TCW×5、DRB×3、VSP×1、TDO×5（含 threatValue 运行期改 textColor 键 + repolish，补 `#include <QStyle>`）、TDP×3。hex 治理：LeftPanelWidget 威胁 switch、PlanCardWidget L21/L201、MosRunwayWidget 2 处等值替换为 `Colors::` token；附说明保留 6 处（BOB #333333、DV #FFEB3B、MRW #1a2a1a/#555555×2/#777777，均无等值 token 或语义不同）。剩余 107 处 = 已注释保留（运行时动态态、完整复合样式、非等值 hex）+ DecisionView 28 处令牌化动态卡（需批次4 级深转换，见待办）。验证：build 100% + ctest 16/16 + 双层像素门禁：current4 对基线 p01/02/04/05/06 噪声级（<=0.0226%）PASS、p03 3.3160%（68,760px = 批次4 接受残差 67,140 + 跑道亚阈值 1,620）；current4 对 current3（批次4 接受态）GATE PASS，p03 diff=0/2073600 逐位相同，其余页 <=0.026% 时钟噪声。证据：`/tmp/opencode/uiupgrade-batch4/current4/`。
- **进度指标（2026-08-18，批次5）**：src/ `setStyleSheet` 计数 142->107（目标 <50）；MainWindow 业务代码无未附注颜色字面量（6 处附说明保留）。
- **A9 改动白名单核对（批次5）**：`git diff --check` 退出 0；改动 19 文件 = GlobalStyle.cpp + design-system.md + 本计划 + 16 个 MainWindow cpp。其中 MosRunwayWidget.cpp 为 QPainter hex 清理（本批后 setStyleSheet 计数 0），其余 15 个均含内联 setStyleSheet（DecisionView 28、TargetDetailPanel 12、SituationView 8、TargetDetailOverlay 8、VideoStreamPanel 8、DeviceStatusPanel 7、PlanCardWidget 6、StatusBarWidget 6、AlertPanel 4、DetectionControlPanel 4、TargetCardWidget 4、DeviceResourceBar 3、BatchOperationBar 3、LeftPanelWidget 2、DecisionSuggestionPanel 1），属 1.2 收敛必需。src/Core/（AirportSceneFactory 3D 材质色、AirportData 数据字段色）不属 A9 白名单，未改动。
- **A1-A9 自检（2026-08-18，阶段1 视角）**：A1 PASS（构建退出 0）；A2 PASS（ctest 16/16）；A3 部分--hex 治理达标，计数 107 未达 <50（其中 DecisionView 28 处需深转换，待用户裁决）；A4 未执行（门禁仅 1920x1080 offscreen，三视口需扩 ScreenshotTool 或 xcb 手动）；A5 对账通过--design-system.md 28 行颜色 token 与 GlobalStyle.h 逐值一致，header 另有 9 个未入表 token（Priority 别名/HoverBackground/CardSelectedBorder/BorderLight/ButtonGray/TextDim/ViewportDark/RowSelected/VideoBackground），完备性缺口留阶段2 补登记；A6/A7 属阶段2 范围未检；A8 证据=门禁 6 页截图含模拟标注（1920 offscreen），三视口可读性随 A4；A9 PASS（见上条）。
- **批次6（已完成，已验证，2026-08-19，分支 feature/ui-stage1-batch5）**：DecisionView 令牌化动态卡深转换批次（用户裁决 A3="深转换+阈值修订"、A4="扩展门禁工具"），`setStyleSheet` 计数 107->89（基线 215；修订阈 ≤90 达标，1.2 验收行已同步修订）。转换：DecisionView/DecisionViewLayout 内联 QSS 收敛为属性词汇（sectionTitle/chipStyle/statusDot/slotStyle/btnVariant=zoom/toolBg、QSplitter containerBg，登记 design-system.md §5.12），均为存量基线值等价迁移。门禁工具扩展（A4）：新增 tests/main_window_capture.cpp（CLI `<out.png> [WxH] [navSuffix]`，offscreen 独立主窗口采集，settle 2000ms）+ CMakeLists.txt 接线 + tests/screenshot_tool.cpp 复用改造；WidgetDump 树转储用于控件取证。像素门禁（三视口 1280/1920/4K offscreen DPR=1 + map01/02 五页矩阵，对 0d8fab8 基线）：初始 34 簇 -> 11 处收敛修复 -> 2 项登记良性。修复清单：GlobalStyle 词汇区新增 `btnVariant="zoomReset"`（缩放行复位钮，同 zoom 免 min-width）与 `btnVariant="mainBg"`（主区底色按钮保底）规则（已补登记 §5.12）；DecisionView m_zoomReset；TargetCardWidget 5 标签 labelBg="panel"；MosParamsPanel m_resetBtn/monoLabel；PlanCardWidget m_thumbnail labelBg="panel"。档位簇根因（最小复现程序 disabled_pixmap_test.cpp 四场景闭环实证）：disabled QLabel pixmap 淡化渲染与标签自身背景色混合，基线背景由 DEC-RP-PLANS 祖先裸样式 #252526 级联提供，批次6 移除裸样式后回落 app 级 transparent，致唯一 disabled 档位卡缩略图 48x24 色差；补 labelBg=panel 恢复基线混合底色，有效卡 pixmap 不透明全覆盖不受影响。最终矩阵：1280x720 48px/0.0052% 与 1920x1080 48px/0.0023%（0 簇；48px 为跑道画布竖向边界线 AA 边缘，肉眼不可见，两视口同位等量确定性）；3840x2160 6538px/0.0788%（单簇 x[1612,1815]y[552,755]，pre6 同区 6241px 确定性复现，根因 MosRunwayWidget 分数线宽+辉光环小数笔位 AA、Δ≤10/255 亚感知，批次4 已判同区亚阈值良性，登记不修）；map01/02 250/261px（≈0.012%，时钟时间依赖噪声 x[813,870]y[1009,1018]，批次4/5 既定容忍）。验证：build 100% + ctest 16/16 + `git diff --check` 退出 0。A9 白名单外改动记录：CMakeLists.txt、tests/main_window_capture.cpp（新增）、tests/screenshot_tool.cpp 系 A4 门禁工具扩展（用户裁决）产物，非视觉层收敛，特此登记。证据：`/tmp/opencode/uiupgrade-batch6/`（captures-mw/ 五页 base/pre6/cur+热力图+pre-tier-fix 过程态、analyze2.py、compare.py、disabled_pixmap_test.cpp 复现程序、pre6/cur 树转储）。
- **批次6 A1-A9 自检（2026-08-19）**：A1 PASS（构建退出 0）；A2 PASS（ctest 16/16）；A3 PASS（89/修订阈 90，剩余均登记保留）；A4 已执行（三视口+map 五页像素门禁，Linux offscreen/DPR=1 已记录；overflow_rows=0 沿 `.omo/evidence/mos-p0-qt-final/` 几何证据；xcb/Windows 真机复核留用户侧）；A5 PASS（§5.12 补登 zoomReset/mainBg；header 9 个未入表 token 完备性缺口仍留阶段2）；A6/A7 属阶段2 未检；A8 证据=五页截图含模拟标注；A9 PASS（含白名单外门禁工具改动记录）。
- **批次7（已完成，已验证，2026-08-19，分支 feature/ui-stage1-batch5）**：2.1 token 可读性批次（阶段2 首批）。改动：`GlobalStyle.h` 四 token（TextSecondary #AAAAAA->#B8B8B8 对 #1E1E1E 底对比度 4.8:1->约 6.3:1 达 WCAG AA；TitleSize 16->17、BodySize 14->15、CaptionSize 12->13），全部经 `.arg()` 词汇自动跟随无需改 QSS；11 处引用旧值的注释同步（7 个 .cpp）；design-system.md §1.4 色表/§2 字体表/档位等值注改写（12/14 档位不再等值 caption/body 13/15，属预期层级变化，小字场景保持原密度）；application-shell.md L163 字面量同步；六页原型 `:root` token 同步（var() 消费者自动跟随；decision/situation 字面量 14/16px 判为图标字形（✓/×/缩放/罗盘）、12px 判为紧凑密度登记，均按档位政策保留）。门禁（阶段2 语义=预期变更区域验证，非像素等价）：新增 `scripts/gate-capture.sh` 落实用户"字体环境固化到门禁脚本"裁决（QT_QPA_PLATFORM=offscreen + FONTCONFIG_FILE 钉死 conda uxo-dev etc/fonts/fonts.conf，环境证据 env.txt 落盘；系统 CJK 字体仅 1 款，钉死配置消除宿主字体变动影响）；BEFORE（改值前二进制）/AFTER（重建后）矩阵 p03 三视口+态势页：变更 13.13%/6.74%/2.21%/1.09%（1280/1920/4K/p01@1920），全部弥散文本级分布（16x9 网格密度 ≤02H 无块状结构区）、密度随视口增大稀释符合字号像素占比规律；视觉复核（look_at 双图对照）：字号可见增大、辅助文本提亮、无文本截断/重叠/面板错位、结构一致（存量坐标截断非本次引入）。构建 100% + ctest 16/16。原型截图再生成（§9.2 步骤5）：npm install playwright（node_modules/package-lock 已被 .gitignore 覆盖）+ LD_LIBRARY_PATH=uxo-dev/lib（系统缺 libnspr4.so 致 chromium 启动 exitCode=127，用 conda 环境内库修复）后六页全部成功，`docs/ui/images/<page>/overview-1920x1080.png` 已更新。证据：`/tmp/opencode/uiupgrade-batch7/`（before/、after/、env.txt、compare.py）。
- **批次8（已完成，已验证，2026-08-19，分支 feature/ui-stage1-batch5）**：2.3 圆角间距批次。改动：GlobalStyle.cpp `--radius-control` 4->6px 共 10 处（getMainWindowStyle 基础 QPushButton / 输入框组 QLineEdit/QTextEdit/QPlainTextEdit/QSpinBox/QDoubleSpinBox / QComboBox，词汇变体 btnVariant="subtle"/"zoom"/"zoomReset" 与 QLineEdit[fieldVariant="compact"]，独立 getter getButtonStyle/getLineEditStyle/getComboBoxStyle），`--space-button-pad-y` 6->7px 共 2 处（getMainWindowStyle 与 getButtonStyle 的 QPushButton padding，按钮全高 +2px；`--space-input-pad-y` 维持 6px）；保留 4px：cardRadius、labelBg="toolbar"、statusDot 圆点族（8px 点配 4px 半径=正圆几何，非控件语义）；保留 5px：滚动条手柄（`--radius-scroll-handle` 独立令牌）与 device 10px 点。panel-gap 评估结论：**暂不引入** `--space-panel-gap`——外壳为零间距贴边格局（各布局 setSpacing(0)/setContentsMargins(0)+1px 边框分隔），引入属布局级变更、三视口几何回归面大，与"信息密度保持军事指控风格"验收约束冲突；结论已落 design-system.md §3.3 注记。文档与原型：design-system.md §3.3 token 表两行值+批次标注；六页原型 `:root` 同步（--radius-control 6 页、--space-button-pad-y detection 页，其余页未定义该 token 无需改）。门禁：BEFORE=批次7 AFTER（二进制未再变）对 batch8-after：p03 三视口 1.15%/0.78%/0.47% + p01@1920 0.02%，全部弥散（16x9 网格密度 ≤01H 无块状结构区，低于结构阈值）；视觉复核：6px 圆角可见无变形、按钮 +2px 无文字裁切/控件重叠/行溢出（含 1280 紧凑视口）、结构一致；1920 右侧面板下部空白为既有密度观察项（像素弥散证明结构未变，非本次回归，留阶段3 评审参考）。构建 100% + ctest 16/16。原型截图六页再生成成功（§9.2 步骤5）。证据：`/tmp/opencode/uiupgrade-batch7/batch8-after/`、compare8.py。
- **批次9（已完成，已验证，2026-08-19，分支 feature/ui-stage1-batch5）**：2.2 图标体系批次（QtAwesome；至此阶段2 三个子任务 2.1/2.2/2.3 全部落地，按门禁暂停待用户确认效果后进入阶段3）。改动：引入 vendored `third_party/QtAwesome`（静态库，来源与裁剪说明见其目录内 CMakeLists）+ 新增 `UiIcons` 助手（`init()` 于 QApplication 后幂等初始化、失败降级纯文本不阻断；`navGlyph()` 导航字形表；`icon()` 统一 color/color-active/color-disabled 选项），挂载 14 枚图标：导航 6 项（NavigationWidget 由 QPushButton 改 `QToolButton`+`Qt::ToolButtonTextUnderIcon`，16px，objectName `DEC-NAV-01..06`，`applyNavIcon()` 按选中态重建图标色）、地图工具栏 7 钮（MainWindow `flatIcon` 12px：fa_rotate_right/fa_play/fa_stop/fa_expand/fa_layer_group/fa_ruler/fa_location_crosshairs）、状态栏紧急停止 1 枚（fa_hand 12px，恒禁用渲染 TextDisabled）；GlobalStyle navBtn 规则选择器同步 QPushButton->QToolButton（3 条）并调垂直 padding 12px->8px 容纳双行布局。CMake 集成：顶层 `add_subdirectory(third_party/QtAwesome)` + link_directories 前置 conda uxo-dev lib（环境 ICU 78 依赖 `__cxa_call_terminate@CXXABI_1.3.15`，系统 libstdc++ 6.0.30 无该符号，前置使驱动隐式 -lstdc++ 解析到环境 6.0.34 版本；路径不存在时空操作）+ `src/MainWindow/CMakeLists.txt` 增 UiIcons.cpp。挂载点事实修正：计划 2.2"关键按钮"清单中确认/处置/完成属死代码——DetectionControlPanel/RightPanelWidget/DeviceStatusPanel/SituationView 4 文件不在任何 CMakeLists target 源列表（永不编译），其图标编辑已撤销；"重置"由地图工具栏复位钮（fa_rotate_right）覆盖、紧急停止已挂载。色彩偏差记录：计划 2.2 草案"导航选中态主色、危险操作红色、默认辅助色"实现改为对齐 navBtn 既有 QSS 文字色（导航未选 TextSecondary、选中+悬停 TextPrimary；紧急停止恒禁用渲染灰），避免引入未登记语义色；色盲友好由字形可区分保证（图标非唯一信息载体）。HTML 原型本轮不接入 Font Awesome（保持文本字形 ◎ 近似），仅 Qt CURRENT 侧登记。验证：build 100% + ctest 16/16；像素门禁（对批次8 接受态）：弥散 p03 三视口 1280/1920/4K=0.77%/0.34%/0.09% + p01@1920 0.47%，聚类 p03 2 簇（导航+紧急停止）、p01 9 簇（导航+工具栏 6+HUD 时码+紧急停止），全部对应图标挂载点或时变文本；神秘簇定性：p01 (800,992)-(864,1024) 经代码取证为 `VideoOverlayWidget.cpp:114` HUD 时码（HH:mm:ss，m_hudTimer 每秒重绘），时变文本设计使然非本批回归。视觉复核（look_at 多模态超时×2 不可用，改程序化替代）：blob 清点 + 字形 ASCII-art 渲染复核 13/13 全对（导航 6+工具栏 7 字形特征匹配、无 tofu，证字体加载渲染正确）。文档登记：design-system.md §3.2 新增 `--size-icon-nav`/`--size-icon-action` token 行 + 文末新增 §8 图标体系（14 枚映射表、尺寸/状态色约定、与计划草案偏差说明）；application-shell.md §3.1 导航按钮形态/§3.2 CURRENT 映射补 `applyNavIcon`/§6.3 紧急停止图标注 三处同步。发现（预存在，未改）：application-shell.md §6.3"CURRENT 行为：点击弹出 QMessageBox::warning"与代码事实不符（实际 setEnabled(false) 禁用占位，属已提交基线，git diff 证实本批 StatusBarWidget.cpp 未提交改动仅图标行）；docs/ui/README.md §5 同源表述待核，留待用户裁决。证据：`/tmp/opencode/uiupgrade-batch7/batch9-after/`（compare9.py 弥散/聚类门禁）、`/tmp/opencode/batch9-review/`（blob 清点、cluster_analysis.py、icon_analysis2.py 字形 ASCII-art 复核）。
- **入库记录（2026-08-19，用户裁决三提交拆分）**：批次7+8 = 0083ad1、批次9 = 30b4c5e、紧急停止文档修正与计划更新随第三提交入库；死代码 4 文件按裁决保持现状（处置另立任务）；预存在紧急停止文档过期已按裁决修正（application-shell.md §6.1/§6.3/§7/§9、docs/ui/README.md §5、docs/ui/pages/situation.md §6.4，均为文档侧修正，代码与 HTML 原型无改动）。
- **待办（三裁决后更新）**：(1) 阶段2 整体效果确认与是否进入阶段3 待用户确认（批次7+8+9 已入库：0083ad1、30b4c5e 及第三提交）；(2) 死代码 4 文件（DetectionControlPanel/RightPanelWidget/DeviceStatusPanel/SituationView，不在任何 CMake target）处置另立任务（用户已裁决本轮保持现状）；(3) ~~预存在紧急停止文档过期修正~~ 已完成（见入库记录）；(4) GlobalStyle.h 9 个未入表 token（批次6 A5 完备性缺口，曾留阶段2）补登记 design-system.md；(5) xcb/Windows 真机三视口复核（可选，随用户侧构建）。
