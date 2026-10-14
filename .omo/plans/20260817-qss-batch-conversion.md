# QSS 属性化迁移：剩余 19 文件批次转换

状态：Draft（待用户确认，未获批不得实施）
范围：仅视觉层样式迁移，不改变任何业务行为

## 背景和当前事实

- 全库 `setStyleSheet` 内联样式约 215 处（含注释行），目标降到 50 以下。
- 试点已完成并全门禁通过（2026-08-17）：
  - 4 个试点文件 24 -> 3 处：NavigationWidget 0、RightPanelWidget 1（splitter）、LeftPanelWidget 2。
  - `GlobalStyle.cpp` 已建属性化 QSS 词汇表（L343-453）与颜色令牌参数链 %1-%26。
  - 修复 `MainWindow.cpp:158` 无选择器样式表级联地雷（改 `containerBg` 属性）。
  - 验证：增量构建通过、ctest 16/16、像素级比对全部精确匹配（#2A3F54 / #4A7A4C）。
- 已知同款地雷：`MainWindow.cpp:201` `m_mapContainer` 无选择器样式表，留待批次 1 一并处理。
- 剩余真实计数约 193 处（19 个待转换文件约 190 处 + 试点残留 3 处）。

## 目标

- 19 个文件内联样式转换为 `GlobalStyle.cpp` 属性化 QSS 规则，全库 `setStyleSheet` 真实调用 < 50。
- 每批次后构建 + ctest 全通过；关键页面像素级验证与转换前一致。
- 转换遵循试点方法论：视觉保真 ground truth = 原内联样式文本（已存 `/tmp/opencode/p12/calls-context.txt`）。

## 非目标

- 不改布局、字号、图标、交互逻辑和业务状态。
- 不处理 RightPanelWidget 死代码问题（单独决策，见待确认问题 3）。
- 不做地图底图加载、UAV 遥测裁切、紧急停止对比度等预存在问题修复。

## 安全边界

- 仅本地 UI 渲染层改动；不涉及真实设备控制、外部通信、数据库写入。
- 不把模拟状态写成真实设备状态；不动 `opencode.json`、密钥、凭据。
- 不提交 git；每批次保留可独立回滚的工作区改动。

## 分阶段任务（每批 2-4 文件，批后验证再进下一批）

| 批次 | 文件（计数） | 特殊事项 |
|---|---|---|
| 1 | MainWindow.cpp (11) | 修 L201 无选择器样式表地雷（同 L158 手法） |
| 2 | DecisionView (28)、DecisionViewLayout (2)、DecisionViewTier (1)、DecisionSuggestionPanel (14) | 计数最多批，arg 链如需 %27+ 按序扩展 |
| 3 | TargetDetailPanel (20)、TargetDetailOverlay (19)、TargetCardWidget (9) | 证据图/选中态样式 |
| 4 | SituationView (10)、DeviceStatusPanel (10)、DeviceResourceBar (7) | 地图叠加油漆不走 QSS，仅边框/背景类 |
| 5 | VideoStreamPanel (9)、AlertPanel (9)、DetectionControlPanel (8) | 告警闪烁类动态样式需 repolish 验证 |
| 6 | StatusBarWidget (9)、PlanCardWidget (9)、MosParamsPanel (7)、MosGeneratorDialog (4)、BatchOperationBar (4) | 对话框样式 |
| 7 | 试点残留：LeftPanelWidget L207/L224、RightPanelWidget splitter L39 | 表头/分隔条样式收尾 |

每批次流程：转换 -> 增量构建 -> ctest -> VNC 截图像素抽验 -> 记录证据。

## 验收标准

- 全库 `setStyleSheet` 真实调用 < 50（注释不计）。
- ctest 16/16 通过。
- 像素级抽验：每批次至少 1 张关键页截图，色值与转换前精确一致（注意 vncdotool R/B 互换，用 PIL 还原）。
- **原始基线已采集**：6 个导航页（态势/探测/决策/设备/统计/配置）截图存于 `/tmp/opencode/p12/baselines/p1..p6_*_true.png`（R/B 已还原并抽验通过）；终验以 p1-p6 与迁移后逐页比对。
- 无业务行为变化；`git diff` 仅含样式相关行。

## 验证命令

```bash
export PATH=$HOME/.local/share/mamba/envs/uxo-dev/bin:$PATH
cmake --build build --target UXOMissionControl -j2
ctest --test-dir build --output-on-failure
grep -rc "setStyleSheet" src/ --include="*.cpp" | grep -v ":0$"
```

## 是否需要用户确认

**是。** 本计划为 Draft，需用户明确确认后才可开始批次实现。同时待用户决策：
1. 试点效果是否接受；
2. 是否按本计划批次推进；
3. RightPanelWidget 死代码（全库零引用）保留还是删除。
