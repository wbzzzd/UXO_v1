# QSS 主题框架评估报告（Qt-Material vs 自绘 QSS）

> 状态：研究评估资料（`docs/research/`），仅供决策参考，不构成需求、CURRENT 实现事实或已批准计划。关联：REQ-010 阶段3 任务 3.3（`.omo/plans/20260813-ui-visual-upgrade.md` §4）。
> 日期：2026-08-20。事实核查来源：qt-material 官方仓库与文档（GitHub / readthedocs，2026-08-20 检索）与本仓阶段1-2 批次1-9 执行记录。

## 1. 背景与决策请求

REQ-010 阶段3 任务 3.3 要求评估引入 QSS 主题框架（计划草案点名 `Qt-Material（MIT）`）能否一次性提供现代化效果，对比自绘 QSS 的成本与可控性；**不擅自引入第三方依赖**，评估报告交付后由用户裁决。若决定不引入，沿用阶段1-2 的自绘 QSS。

先更正一处计划草案事实错误：**Qt-Material 许可证为 BSD-2-Clause，非 MIT**（见 §2 事实清单）。

## 2. 候选库事实核查（2026-08-20 检索）

### 2.1 主候选：qt-material

| 维度 | 事实 | 来源 |
|------|------|------|
| 定位 | "Material inspired stylesheet"，面向 **PySide2/PySide6/PyQt5/PyQt6** 的 Python 包 | [UN-GCPDS/qt-material](https://github.com/UN-GCPDS/qt-material) README |
| 实现语言 | Python 98.8%（**不是 C++ 库**） | GitHub 语言统计 |
| 许可证 | **BSD-2-Clause**（计划草案误记为 MIT） | GitHub License 字段 |
| 社区规模 | 约 2.8k stars / 284 forks | GitHub（2026-08-20） |
| 维护状态 | 原仓库最后推送 2024-05-16；2025-04 宣布迁移至后继仓库 [dunderlab/qt-material](https://github.com/dunderlab/qt-material)（v2.17，2025-04-21 发布，最后推送 2025-06-17） | issue #120、后继仓库 |
| Qt 版本 | 原仓库支持 Qt5 绑定（PyQt5/PySide2）；后继仓库仅 PySide6/PyQt6（Qt6 绑定） | 两仓库 README |
| C++ 集成路径 | 唯一路径为离线 `export_theme()`：用 Python 生成静态 `.qss` + SVG 图标目录（或 `.rcc`），C++ 侧 `QFile` 读取后 `setStyleSheet`。**无 C++ 运行时库、无 API** | 官方文档 Export Guide、issue #25 |
| C++ 路径已知坑 | 导出 QSS 内图标引用 `url(icon:/...)` 需 `QDir::addSearchPath` 或手工改路径/编 qrc 才能显示；QMenu 在不同后端渲染不一致需逐项调参 | issue #25 实录、README Troubleshoots |
| Python 专属能力（导出路径**不可得**） | 运行时换肤（`QtStyleTools`）、density_scale 密度缩放、Ripple 动效、主题实时预览 | readthedocs 功能清单 |

结论性事实：**对 C++/Qt5/CMake 项目而言，qt-material 可消费的产物只有"一份静态 Material 风格 QSS + SVG 资产"**，其文档宣传的现代化能力大半绑定 Python 运行时，导出路径不可获得。

### 2.2 次候选简评

- **QDarkStyleSheet**（MIT）：同为 Python 包生成扁平暗色 QSS，无 Material 层级/动效诉求，视觉上限低于本项目现有自绘体系，无引入价值。
- **qtmodern** 等窗口装饰类库：仅提供窗口镜像/边框，不解决控件级主题，与本任务无关。

## 3. 与 CURRENT 自绘体系的冲突面

引入 qt-material 导出 QSS 意味着**全局样式表整体替换**，与阶段1-2 建成的基础设施正面冲突：

1. **属性词汇体系作废面大**：`GlobalStyle.cpp` 已建成属性化词汇（`containerBg`/`btnVariant`/`labelBg`/`textColor`/`stateBanner`/`chipStyle` 等），存量 89 处 `setStyleSheet` 依赖该词汇（基线 215 -> 89，达标 ≤90）。外部 QSS 与应用级规则并存必然触发 Qt 样式表合并顺序问题——批次3/4 已为此类级联冲突付过两次回归代价（父级裸样式表压过应用级规则、QSS 盒影响 QLabel sizeHint），引入第二全局源等于系统性扩大该风险面。
2. **token 对账制度失去单一事实源**：`design-system.md` 37 行颜色 token 与 `GlobalStyle.h` 逐值对账（含今日补登记的 9 个）。qt-material 主题色经 Python 端 XML + `extra` 字典注入，色值脱离本仓 token 管线；即便改造其 `material.css.template` 注入本仓 token，等于维护一份外来模板 fork——复杂度高于直接持有 `GlobalStyle.cpp`。
3. **像素门禁基线全部作废**：批次4-9 以 0d8fab8 基线建立 offscreen A/B 像素门禁（三视口 + map 矩阵），全局样式替换后六页基线需推倒重立、逐页重验证，回归成本与阶段2 全程相当。
4. **HTML 原型 token 桥断裂**：六页原型 `:root` token 与 Qt 侧同步是文档纪律（§9.1 同步更新清单）的组成部分，Material 调色板无法映射回现有 37 token 契约。
5. **军事指控风格不匹配 Material 默认审美**：Material 强调亮色大圆角、大留白、强调色块；本仓验收约束是"信息密度保持军事指控风格"（批次8 panel-gap 评估已引用）。qt-material 暗色主题（dark_teal 等 9 种）均为 Material 调色板，向 `#1E1E1E`/`#4A7A4C` 军绿体系定制需重制主题 XML，所得产物仍是外来静态 QSS。
6. **QSS 能力天花板并未突破**：qt-material 的 QSS 同样受 Qt 限制（无 `box-shadow`、无 `transition`，其"阴影"为 SVG 贴图近似）。阶段3 3.1 的浮层纵深已确定用 `QGraphicsDropShadowEffect` 原生实现，不依赖任何主题框架。

## 4. 成本/收益对比

| 维度 | 引入 qt-material（C++ 导出路径） | 延续自绘 QSS（现状 + 阶段3） |
|------|------|------|
| 一次性获得 | Material 风格静态 QSS + SVG 控件图标集 | 无（逐项手工） |
| 运行时能力 | 无（换肤/密度/涟漪均为 Python 专属） | 同样无，但按需用 `QGraphicsDropShadowEffect`/`QPropertyAnimation` 原生补 |
| token 治理 | 破坏单一事实源，需 fork 模板才可救 | 37 token 对账制度持续生效 |
| 词汇体系 | 89 处存量需重映射或双轨并存 | 持续收敛（目标 <50 不变） |
| 像素门禁 | 六页基线推倒重立 | 基线延续 |
| 原型同步链 | 断裂，需另建映射 | 延续 |
| 供应链 | 新增 Python 构建期工具链依赖（后继仓库转向 Qt6 绑定，Qt5 项目被迫钉住旧版 UN-GCPDS 快照） | 零新增（QtAwesome 先例为纯 CMake 静态库） |
| 估算工作量 | 主题定制 + 全量样式迁移 + 六页三视口重验证，量级≈重做阶段1-2 | 阶段3 剩余项（3.1 阴影）数小时级 |

## 5. 风险清单（若仍选择引入）

- 双全局样式源级联冲突（批次3/4 已证此类 bug 难排查）。
- 钉住的 UN-GCPDS 旧快照无维护（后继仓库 Qt6-only），安全与缺陷修复断供。
- SVG 图标资产路径适配（issue #25 实录的手工 hack）成为长期构建脆弱点。
- 军事密度约束与 Material 审美的持续张力，最终仍需大改其模板。

## 6. 建议

**建议不引入**，沿用阶段1-2 自绘 QSS 并按原计划完成阶段3 剩余项（3.1 阴影原生实现）。核心理由：对 C++/Qt5 项目，qt-material 唯一可得的静态 QSS 产物既不突破 QSS 能力天花板（阴影/过渡仍需原生手段），又以破坏 token 对账、词汇体系、像素基线、原型同步链四项已验收基础设施为代价；其维护线已转向 Qt6 绑定，Qt5 项目引入即钉死无维护快照。

**决策项（留用户裁决）**：
- A（建议）：不引入，3.3 按计划取消，沿用自绘 QSS；
- B：不引入框架，但指定借鉴项（如 Material 密度/间距参数表）并入 design-system.md 评审；
- C：仍引入，则需另立评审任务（主题定制 + 迁移 + 全量门禁重建的工作量与基线作废面先获确认）。
