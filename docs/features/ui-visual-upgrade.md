# UI 视觉治理

状态：Approved
关联产品需求：REQ-010
关联版本：待确认

## 1. 问题与目标

指挥席客户端 `UXOMissionControl` 被用户反馈"界面像二十年前"。经调查，根因不是军事指控视觉风格选错（暗色/高密度/克制/威胁色阶/等宽字体是合理领域选择），而是执行未到位：`GlobalStyle` token 体系名存实亡（`src/` 215 处内联 `setStyleSheet` 散落 22 文件绕开 token），未应用 Fusion 风格导致平台原生控件外露，可读性指标偏低（辅助文本对比度 4.8:1 临界值），几乎无图标辅助态势识别，`GlobalStyle.cpp` 存在 scrollbar hover 缺陷与缺失资源引用。

目标：让 token 体系重新生效、跨平台外观一致、可读性达标、图标体系建立、修复已知缺陷。不改风格方向、不改业务逻辑、不改模拟边界。

## 2. 范围与非目标

**范围**：
- 视觉层：QSS、字体、Fusion 风格、`GlobalStyle`、`design-system.md` token、图标。
- 收敛 `src/` 内联 `setStyleSheet` 至 50 以下（基线 215）。
- 字号/对比度达 WCAG AA（辅助文本 >= 4.5:1）。
- 导航栏/工具栏/关键按钮配矢量图标。

**非目标**：
- 不清理占位控件（用户明确排除：工具栏 QLabel 占位、未实现导航页、菜单占位项保持现状）。
- 不改设计风格方向（暗色/克制/高密度保留）。
- 不重写为 QML/Qt Quick。
- 不引入干扰决策的花哨动效。
- 不动业务逻辑、状态所有权、数据流、模拟边界。
- 不引入网络、持久化、设备控制、真实排爆能力。

## 3. 用户流程

本功能是横切视觉治理，不新增用户流程。现有各页面交互流程不变，仅视觉表现层改善。

## 4. 需求增量

关联 REQ-010（UI 视觉治理，`Approved`）。无其他需求增量。

## 5. 架构增量

无架构增量。不改模块边界、状态所有权、依赖关系、运行流程。仅改视觉层（QSS/字体/图标/Fusion 风格）。

## 6. UI 增量

分三阶段（执行计划见 `.omo/plans/20260813-ui-visual-upgrade.md`）：

- **阶段 1（P0）**：全局 Fusion 风格 + 强制字体（`setFamilies`+`setPixelSize`，修正 `Fonts::Family` 逗号分隔串与 px/pt 混淆）；收敛 215 处内联样式至 < 50；修复 scrollbar hover（纵向颜色 + 横向补规则）与 `QDockWidget` 缺失资源。
- **阶段 2（P1）**：字号/对比度 token 提升（body 14->15px，辅助文本对比度达 WCAG AA）；引入矢量图标体系（选型经用户确认）；圆角/间距微调。
- **阶段 3（P2 可选）**：elevation/阴影（`QGraphicsDropShadowEffect`，QSS 不支持 box-shadow）；克制过渡动画（`QPropertyAnimation`，QSS 不支持 transition）；QSS 框架评估。

token 变更按 `docs/ui/README.md` §9 同步更新 `design-system.md` token 表、原型与截图（`node prototypes/screenshot.js <page>`）。

## 7. 验收标准

| 编号 | 标准 |
|------|------|
| A1 | Release 构建退出 0 |
| A2 | `ctest` 现有测试全绿 |
| A3 | `rg "setStyleSheet" src/` < 50（基线 215），无业务代码硬编码颜色字面量 |
| A4 | 三视口（1280/1920/4K）视觉检查 `overflow_rows=0`、无平台原生残留，截图环境与 DPR 已记录 |
| A5 | `design-system.md` token 与 `GlobalStyle` 源码完全一致 |
| A6 | 图标三视口清晰，色盲友好 |
| A7 | 对比度达 WCAG AA（辅助文本 >= 4.5:1） |
| A8 | 模拟标注保留且可读 |
| A9 | 改动范围限定视觉层（`GlobalStyle.*`、`main.cpp`、`.qrc`、`design-system.md`、`src/MainWindow/*.cpp` 含内联样式的控件文件） |

## 8. 对核心文档的影响

- `docs/ui/design-system.md`：token 表更新（字号/对比度/圆角/间距/阴影），按 `docs/ui/README.md` §9 同步规则。
- `docs/ui/README.md`：若 token 变更涉及原型，按 §9.2 重新生成截图。
- `docs/UI.md`：若通用视觉基线章节需更新（§8），同步修订。
- `docs/ARCHITECTURE.md`：无影响（不改架构）。
- `docs/DEVELOPMENT.md`：无影响（不改构建/测试门禁）。
- `docs/PRODUCT.md`：无影响（不改产品范围）。

## 9. 待确认事项

- 阶段 2 图标库选型（QtAwesome / 自绘 SVG / PNG），需用户确认。
- 阶段 3 是否实施，以及 3.3 是否引入第三方 QSS 框架，需用户决定。
- 视觉 QA 方案（扩展 ScreenshotTool vs 手动截图契约），阶段 1 执行时确定。
- `setFamilies` 需 Qt >= 5.13，执行时确认项目 Qt 版本。
