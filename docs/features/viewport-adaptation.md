# Qt 客户端三视口适配

状态：Approved（2026-08-26 用户批准）
关联产品需求：REQ-011
关联版本：待确认

## 1. 问题与目标

六页 HTML 原型的三视口适配已于 2026-08-25 完成验收并提交（分支 `feature/prototype-responsive-viewport`，待合并），验证了“固定区域 + 弹性中心区”的设计方向。Qt 客户端实现侧尚无对应的系统性三视口验收：

- REQ-010 A4 门禁矩阵（`scripts/gate-capture.sh`）仅覆盖决策页三视口与态势页 1920×1080，且采集输出至 /tmp 临时目录、未在 `.omo/evidence/` 留存；态势页在 1280×720、3840×2160 档位与探测页在全部三档视口下均无 fresh 像素证据。
- 决策页历史三视口几何证据（`.omo/evidence/mos-p0-qt-closure-final/REPORT.md`）已被 `UI.md` 第 3 节判定为单档位渲染修正之前的旧证据。
- `UI.md` 第 3 节、`design-system.md` §7.0 末段、`docs/ui/README.md` §7 三处登记的“1280×720 下约 5px 底部溢出”所指 `DecisionSuggestionPanel` 等组件未纳入 CMake 编译目标，属过时记录。

目标：MainWindow 壳层与态势、探测、决策三页在 1280×720、1920×1080、3840×2160 下满足 `design-system.md` §7 视口规则，以 9 张离屏截图门禁形成可复核证据，并刷新上述过时记录。适配沿用原型批次验证的“橡皮筋”方向：token 固定 px 不缩放，固定区域尺寸不变，中心区弹性。

## 2. 范围与非目标

**范围**：

- MainWindow 壳层与态势、探测、决策三页的布局呈现层最小侵入修复（Qt 布局代码：size hint、最小尺寸、弹性策略）。
- 9 张离屏截图门禁（3 页 × 3 视口）与本地证据留存（`.omo/evidence/qt-viewport-adaptation/`）。
- `scripts/gate-capture.sh` 采集矩阵从 5 张扩展至 9 张。
- `UI.md` 第 3 节、`design-system.md` §7.0、`docs/ui/README.md` §7 的过时视口记录刷新。

**非目标**：

- 不改 token 值、颜色、字体、间距（REQ-010 已交付）。
- 不做 4K 全局壳或字体放大（§7.2 密度保持；壳不放大为 §7.0 登记的 pre-existing 已知模式）。
- 不改六页 HTML 原型（原型批次已完成）。
- 不做右面板弹性宽度（壳层右面板未纳入编译目标；决策页内部候选方案右栏维持 CURRENT 固定宽度不变）。
- 不实现设备、统计、配置页（导航 04–06 为占位回退态势）。
- 不动业务逻辑、模拟边界与状态边界。
- xcb/Windows 真机多 DPR 复核留用户侧（离屏门禁 DPR=1）。

本批交付的壳层适配、三视口规则与门禁基础设施可被未实现页面复用：设备/统计/配置页实现后，在采集矩阵中各加一行即可进入同一门禁流程，页面内部布局按 `design-system.md` §7 随页面开发实现；无需为后续页面再立同类专项，仅需按本门禁做每页例行验收。

## 3. 用户流程

无新增用户流程。仅改变不同窗口尺寸下的布局呈现，交互、数据与模拟行为不变。

## 4. 需求增量

关联 REQ-011（Qt 客户端三视口适配，`Draft`，随本设计一并呈批）。无其他需求增量。

## 5. 架构增量

无。布局呈现层适配不新增模块、不改依赖与状态所有权；修复限于 Qt 布局代码，`MosRunwayWidget` clamp 缩放与 `DecisionViewLayout::applyViewportScale` 既有视口机制保持不变。

## 6. UI 增量

执行计划见 `.omo/plans/20260826-qt-viewport-adaptation.md`（已批准，2026-08-26）。阶段划分：构建基线 → before 实测与问题清单冻结（用户裁决）→ 最小侵入修复 → after 门禁与 A/B 比对 → 过时记录刷新。修复对象限 MainWindow 壳层与三页的布局约束（size hint / 最小尺寸 / 弹性策略），预期不动 token 与视觉样式。

## 7. 验收标准

| 编号 | 标准 |
|------|------|
| A1 | 构建 `UXOMissionControl`、`MainWindowCapture`、`WidgetDump` 全部退出 0 |
| A2 | ctest 不低于实施前基线（预期 15/17；2 项既有失败为 REQ-009 登记的 ONNX 集成问题，非本批回归） |
| A3 | 9 张离屏截图（3 页 × 3 视口）全部采集并留存于 `.omo/evidence/qt-viewport-adaptation/`（本地留存，该目录按仓库惯例不入库） |
| A4 | 1280×720：三页区域无溢出、无意外裁切、关键内容可用（用户在问题清单裁决中接受的项除外） |
| A5 | 1920×1080：与 before 基线零回归（画中画实时时钟、告警跑马灯等动画帧差异除外） |
| A6 | 3840×2160：无大片留白（决策页中心画布留白随视口增大属 1920 权威设计预期）；固定 token 区域与字体不缩放；`MosRunwayWidget` 缩放上限 2× |
| A7 | `UI.md` 第 3 节、`design-system.md` §7.0、`docs/ui/README.md` §7 的过时 5px 记录与旧证据表述刷新为 fresh 事实 |

## 8. 对核心文档的影响

- `docs/UI.md`：第 3 节过时 5px 记录与旧证据表述刷新为 fresh 三视口事实。
- `docs/ui/design-system.md`：§7.0 末段过时记录刷新，补充 Qt 三视口 fresh 证据引用。
- `docs/ui/README.md`：第 7 节 CURRENT 溢出表述刷新（非核心基线，一并同步）。
- `docs/PRODUCT.md`、`docs/ARCHITECTURE.md`、`docs/DEVELOPMENT.md`：无影响（不改产品范围、状态边界、构建测试门禁）。

## 9. 待确认事项

- 本设计与 REQ-011 的整体批准：已于 2026-08-26 获用户批准（双 `Approved` 前提已满足）。
- before 实测问题清单的裁决：哪些 1280×720 / 3840×2160 表现属可接受的设计内行为、哪些必须修复。
- 提交节奏：默认建议三条提交（三件套文档一条、代码与门禁脚本一条、文档刷新一条），批准时可另行指定。
