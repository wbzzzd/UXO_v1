# 原型基线同步计划（方案 A：HTML 原型刷新至 Qt 当前视觉基线）

状态：Approved（用户 2026-08-24 裁决"那就选A"）
计划所属 worktree：`/home/lin/UXO_v1-ui-visual-upgrade`
计划所属分支：`feature/prototype-baseline-sync`（基于 `origin/main` 240f5bf）
创建日期：2026-08-24
关联需求：REQ-010（`docs/requirements/REQ-010.md`，Implemented；本批为原型侧基线同步收尾，不新增产品功能）
关联功能设计：`docs/features/ui-visual-upgrade.md`（Approved）

## 1. 背景与裁决

用户命令检查"HTML 代码可能没有实时随着界面更新"。经比对 Qt 当前实现与六页 HTML 原型，确认四处视觉基线偏差。用户裁决选方案 A：以 Qt 当前实现为基准，把原型刷新到一致基线（不是把 Qt 回退到旧原型）。

偏差清单（均已核对源码）：

1. 原型导航/工具栏/急停用文本字形 `◎` 占位，Qt 自 REQ-010 批次起已接入 QtAwesome FA 图标（`MainWindow.cpp` createMapToolbar/createNav、`StatusBarWidget.cpp`）。
2. `design-system.md` §9 已登记 `--elevation-overlay`/`--elevation-modal` token，但态势页 PiP/目标浮层与决策页模态仍硬编码 rgba 投影。
3. 决策页 DEC-GEN-MODAL 原型为 radius 6px 卡片 + 标题色带；Qt `MosGeneratorDialog` 已是无边框窗 568×424 + 内卡 520×364（margin 24/24/24/36、radius 3px、Modal 投影、close 28×28）。
4. 探测页 Qt `DetectionView` 底部操作条含"移除记录"按钮（detectionRemoveButton：选中行启用，仅删当前行、删后选原位置后续行、表空再禁用、不受校验状态限制），原型 confirm-bar 缺失该按钮。

## 2. 范围

### 2.1 FA 图标接入（11/14 枚挂载，vendored 字体）

- 字体：`third_party/QtAwesome/QtAwesome/fonts/Font Awesome 7 Free-Solid-900.otf` → `docs/ui/prototypes/assets/fa-solid-900.otf`；六页 `@font-face` 引用（family "Font Awesome 7 Free"，weight 900，相对路径 `../assets/`）。
- 码点（`QtAwesomeEnumGenerated.h`）：态势 f5a0 / 探测 f7c0 / 决策 f24e / 设备 f2db / 统计 e0e3 / 配置 f013 / 视角复位 f065 / 图层 f5fd / 测量 f545 / 坐标拾取 f601 / 急停 f256。
- 挂载点：六页导航 6 枚 + 态势地图工具栏 4 枚（SIT-TB-RESET/LAYER/MEASURE/PICK）+ 五页急停 1 枚（决策页无急停按钮）。
- 未挂载 3 枚（重置 f2f9 / 开始 f04b / 结束 f04d）：仅存在于 CURRENT Qt 地图工具栏；`pages/situation.md` 已批准的工具栏规格为 4 可见按钮 + 3 省略占位（SIT-TB-SYNC/BOOKMARK/CONSOLE），无此三枚挂载点。按"已批准规格不因 CURRENT 扩张而回写"原则，本批不新增按钮，仅在文档说明差异。
- §8.2 约束：导航 `--size-icon-nav`(16px)、工具栏/急停 `--size-icon-action`(12px)；图标色跟随控件文本色 token；保留文字标签（图标不作唯一信息载体）。

### 2.2 token 注入与投影 var() 化

- 注入矩阵（全部为 §9 已登记 token，无需新登记）：六页 +`--size-icon-nav`；五页（除 decision）+`--size-icon-action`；situation +`--elevation-overlay`；decision +`--elevation-modal`。
- situation `.video-pip` / `.target-detail-overlay` 投影 → `var(--elevation-overlay)`。

### 2.3 决策页模态改写（DEC-GEN-MODAL / 新增 DEC-GEN-CARD）

- 纯 CSS 几何改写：新增 `.modal-window` 568×424 包裹层（透明区为投影空间）；`.modal` 内卡 520×364（margin 24px 24px 36px、radius 3px、`var(--elevation-modal)`、overflow hidden、去标题色带对齐 Qt 平卡）；close 24×24 → 28×28；body/footer 弹性收尾。
- HTML 表单内容、既有 testid 与交互逻辑不变；卡体新增 `data-testid="DEC-GEN-CARD"`。

### 2.4 探测页移除记录按钮（新增 DET-CE-REMOVE）

- 落点：中心区 confirm-bar（与 模拟确认/拒绝确认 同组，对齐 Qt 底部操作条三键结构），初始禁用。
- 行为（对齐 Qt DetectionView）：行点击启用；点击仅删当前行（列表管理，不动证据源数据）；删后选原位置后续行（末行回退）；表空再禁用；不受确认/校验状态限制。

### 2.5 文档同步（`docs/ui/README.md` §9 规则）

- `design-system.md`：§8 原型未接入 FA 的陈述更新（11/14 挂载 + 3 枚差异说明）；§9 原型消费点陈述修正。
- `application-shell.md`：导航叙述与导航表图标列（◎ → 字形名）、急停行补图标说明。
- `pages/situation.md`：工具栏 4 行 + 急停行补图标标注。
- `pages/detection.md`：DET-CE-REMOVE 控件行 + 原型行为。
- `pages/decision.md`：§6 模态基准更新（无边框窗几何 + DEC-GEN-CARD）+ ID 索引。
- `docs/ui/README.md`：文件索引补 `prototypes/assets/fa-solid-900.otf` 行。

## 3. 执行顺序与验证门禁

1. HTML 六页（token → 字体 → 图标 → 模态 → 移除按钮）。
2. 文档同步（§9.2 流程：原型先行）。
3. `node screenshot.js` 重生成六页 1920×1080 截图；决策页补模态打开态专项截图验证几何。
4. look_at 逐页目检：图标渲染非 tofu、模态几何 568×424/520×364、投影一致、移除按钮存在与交互。
5. data-testid 与文档 ID 索引一一对应检查。
6. 本批无 C++ 变更，无构建门禁；提交与 PR 待用户确认。

## 4. 完成记录

- **2026-08-24 批次完成**（分支 `feature/prototype-baseline-sync`，基线 origin/main 240f5bf；详细验证证据见 `.omo/evidence/prototype-baseline-sync/VERIFY.md`）：
  1. HTML 六页：FA 实心字体（vendored `assets/fa-solid-900.otf`）+ 11 枚码点挂载（主导航 6、态势工具栏 4、急停 1）；token 注入与投影 `var()` 化；决策页无边框模态（`.modal-window` 568×424 + `DEC-GEN-CARD` 520×364）；探测页 `DET-CE-REMOVE`（次级样式、随选中启用）。
  2. 文档同步（§9.2 原型先行）：`design-system.md`、`application-shell.md`、`docs/ui/README.md` 与六份 `pages/*.md` 共 9 份；git diff 15 文件 +200/−106。
  3. 截图：六页 1920×1080 整体图重生成；决策页模态打开态专项截图（`.omo/evidence/prototype-baseline-sync/decision-modal-open.png`）。
  4. 验证（程序化）：六页字体加载成功；11 枚码点全部在字体 cmap 中（非 tofu 决定性证据）；模态几何 568×424 / 520×364 精确匹配、投影与 `--elevation-modal` token 一致；DET-CE-REMOVE 初始随 `target-001` 启用、删除后续选原位置后续行、表空禁用；残留扫描（`1012×700` 0 处、旧 `7.[4-7] 节` 引用 0 处、`fa_hand` 五页齐全）。
  5. 门禁（§3）：第 1/2/3/5 项通过；第 4 项目检因执行模型不支持图像输入且 look_at 代理不可用，以上述程序化证据替代；第 6 项提交与 PR 待用户确认。
  6. 环境备注：chromium 需 `LD_LIBRARY_PATH=/home/lin/.local/lib/chromium-deps` 启动（系统缺 libnspr4 / libnss3）。
  7. 遗留（超出本批范围，仅报告不修改）：统计/配置次级导航仍为 `◎` 占位；`design-system.md` CURRENT 记述（QtAwesome/FA6）与 HTML `@font-face`（Font Awesome 7 Free）的版本表述差异。
