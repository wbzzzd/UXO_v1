# UXOMissionControl 设计系统

状态：范围设计契约（CURRENT token 提取 + 已批准证据/侧栏原语扩展）
权威来源：`include/Common/GlobalStyle.h`、`src/Common/GlobalStyle.cpp`、`src/MainWindow/LeftPanelWidget.cpp`、`src/MainWindow/TargetDetailOverlay.cpp`、`src/MainWindow/MainWindow.cpp`、`src/MainWindow/VideoOverlayWidget.cpp`
关联文档：`docs/features/drone-detection-demo.md`（Approved，关联 REQ-009）、`docs/UI.md`、`docs/ui/design-system.md`（六页 TARGET 设计评审原型，Draft）。

> **文档对齐状态**：用户已在实现评审中批准以下行为——绿色选中替换旧蓝色选中、移除目标表勾选列、冻结证据由详情浮层 `QImage` 承载（实时视频 HUD-only）。`docs/features/drone-detection-demo.md`、`docs/UI.md`、`docs/ui/pages/situation.md` 已对齐冻结证据/HUD-only/移除勾选列描述。`docs/ui/design-system.md` 尚未登记绿色选中/冻结证据视口 token，列为待对齐。本文件描述已批准实现行为。

> 本文件是已批准目标列表与冻结证据详情原语的范围设计契约，不是全应用设计系统。现有 token 从 `GlobalStyle` 提取作参考，新增 token 仅服务于本范围原语。本范围原语不引入表外颜色、不引入第二字体族；如需新增 token，须先在本文件登记并标注 CURRENT 出处与用途。Qt 无 CSS 变量，token 以 `GlobalStyle::Colors/Fonts/Sizes/Animation` 命名空间常量形式表达，下文表格给出"角色 → GlobalStyle 常量 → 值 → 用途"映射。标记【新增】的 token 是本范围原语所需、当前源码尚未登记的 token；标记【内联】的值是当前 QSS 中以字面量出现但未进入 `GlobalStyle` 的颜色，列为已接受债务。

## 1. 氛围与识别

密集的暗色军事指挥台。信息密度高时压抑背景让数据浮起，密度低时留出深色呼吸空间。整体只读、克制、不娱乐化，任何亮色都必须承担状态或交互含义，不得作为装饰。

签名特征是"军绿强调在深灰层级之上"。表面靠明度阶差分层（主窗口最暗 → 面板 → 工具栏最亮），辅以 1px 极暗边框收边，几乎不用阴影。唯一允许的高饱和绿是 `PrimaryGreen #4A7A4C`，专用于主操作、聚焦边框与单一选中态；威胁色（红/橙/黄）只在威胁与优先级语义里出现，且永远伴随文字标签，不单独承担信息。

模拟数据必须可见地标注"模拟"或"演示"，这是氛围的一部分：用户随时能区分本地演练与真实接入，不被操作界面误导成真实设备状态。

## 2. 颜色

单一暗色主题，无 Light 模式。所有值来自 `GlobalStyle::Colors`（`include/Common/GlobalStyle.h:26`）。

### 2.1 表面与文本

| 角色 | GlobalStyle 常量 | 值 | 用途 |
|------|------------------|------|------|
| 表面/主窗口 | `Colors::Background` | `#1E1E1E` | 主窗口、地图容器、输入框、表格、列表背景 |
| 表面/面板 | `Colors::PanelBackground` | `#252526` | 左右侧面板、菜单选中态、下拉弹窗、浮动详情面板底色 |
| 表面/工具栏 | `Colors::ToolbarBackground` | `#2D2D2D` | 工具栏、菜单栏、表头、状态分段底色 |
| 表面/菜单 | `Colors::MenuBackground` | `#2D2D2D` | 菜单背景（与工具栏同值） |
| 文本/主 | `Colors::TextPrimary` | `#FFFFFF` | 正文、标题、选中态文本、按钮文本 |
| 文本/次 | `Colors::TextSecondary` | `#AAAAAA` | 辅助文本、未选中标签、关闭按钮默认态 |
| 文本/禁用 | `Colors::TextDisabled` | `#888888` | 禁用文本、placeholder、未选中标签页文字 |
| 边框/默认 | `Colors::Border` | `#3C3C3C` | 控件边框、分隔线、表格网格线、hover 背景次选 |
| 边框/聚焦 | `Colors::BorderFocus` | `#4A7A4C` | 输入框聚焦边框（与主色同值） |

### 2.2 强调与状态

| 角色 | GlobalStyle 常量 | 值 | 用途 |
|------|------------------|------|------|
| 强调/主 | `Colors::PrimaryGreen` | `#4A7A4C` | 主按钮、进度条、聚焦边框、选中下划线、刷新按钮、单一选中态（见 2.4） |
| 强调/主 hover | `Colors::PrimaryGreenHover` | `#5A8A5C` | 主按钮 hover、刷新按钮 hover、详情主操作按钮 hover |
| 危险/默认 | `Colors::DangerRed` | `#D32F2F` | 危险按钮、紧急停止占位 |
| 危险/hover | `Colors::DangerRedHover` | `#B71C1C` | 危险按钮 hover |
| 状态/在线 | `Colors::StatusOnline` | `#4CAF50` | 设备在线、操作反馈成功文本 |
| 状态/离线 | `Colors::StatusOffline` | `#888888` | 设备离线 |
| 状态/忙碌 | `Colors::StatusBusy` | `#FFB74D` | 设备忙碌（与中威胁同值） |
| 状态/错误 | `Colors::StatusError` | `#FF5252` | 设备错误（与高威胁同值） |

### 2.3 威胁与优先级语义色

威胁与优先级共用同一组色阶：高/P0 红、中/P1 橙、低/P2 黄。颜色不得作为唯一信息。详情浮层威胁值配 `●` 字形与文字（`TargetDetailOverlay::threatText`）；目标列表仅以威胁色作类型列前景，类型名本身即文字标签。

| 角色 | GlobalStyle 常量 | 值 | 用途 |
|------|------------------|------|------|
| 威胁/高 · P0 | `Colors::ThreatHigh` / `Colors::PriorityP0` | `#FF5252` | 高威胁、严重威胁、P0 优先级、目标类型前景色 |
| 威胁/中 · P1 | `Colors::ThreatMedium` / `Colors::PriorityP1` | `#FFB74D` | 中威胁、P1 优先级 |
| 威胁/低 · P2 | `Colors::ThreatLow` / `Colors::PriorityP2` | `#FFF176` | 低威胁、P2 优先级 |

### 2.4 交互态扩展（含新增）

| 角色 | GlobalStyle 常量 | 值 | 出处 / 状态 | 用途 |
|------|------------------|------|------|------|
| 行/hover | `Colors::RowHover` | `#2A2A2A` | 【内联】`GlobalStyle.cpp` 列表 hover 字面量 | 列表/表格行 hover 背景 |
| 行/选中（旧） | — | `#2A3F54` | 【内联·待替换】`GlobalStyle.cpp` `getMainWindowStyle`/`getTableWidgetStyle` 选中字面量 | 旧蓝色选中背景；用户已批准以绿色选中替换，`docs/UI.md`/`docs/ui/design-system.md` 待对齐 |
| 行/选中（新） | `Colors::RowSelected` | `#2E3D2F` | 【新增】替换上述蓝色字面量 | 单一绿色选中态背景；介于 `PanelBackground #252526` 与 `PrimaryGreen #4A7A4C` 之间的暗橄榄绿，选中即"军绿选中"，不与威胁色冲突 |
| 证据视口/背景 | `Colors::EvidenceViewport` | `#161616` | 【新增】冻结证据图像视口专用 | 比 `Background #1E1E1E` 更暗，使冻结标注截图作为详情面板焦点内容浮起 |

> 选中态规则：行选中只有"选中"一个态，不定义独立的 selected-hover；选中行上 hover 不改变背景（保持 `RowSelected`），以符合"单一绿色选中态"约束。选中行文本保持 `TextPrimary #FFFFFF`。

### 2.5 三维场景色（仅 Qt3D，不用于控件）

| 角色 | GlobalStyle 常量 | 值 | 用途 |
|------|------------------|------|------|
| 场景/草地 | `Colors::Ground` | `#2D4A2D` | 机场草地 |
| 场景/跑道 | `Colors::Runway` | `#3D3D3D` | 主跑道 |
| 场景/滑行道 | `Colors::Taxiway` | `#4A4A4A` | 滑行道 |

### 2.6 颜色规则

- 表面靠明度阶差分层（`Background` < `PanelBackground` < `ToolbarBackground`），不靠阴影。
- `PrimaryGreen` 只用于交互元素与单一选中态，绝不作装饰。
- 威胁色不作唯一信息载体（`docs/UI.md` 第 8 节）：详情浮层威胁值配 `●` 字形与文字（见 2.3）；目标列表类型列以类型名作文字标签，无字形要求。
- 本范围原语不引入本表以外的颜色。`#2A3F54`、`#2A2A2A`、`#555555` 等【内联】字面量列为已接受债务（见第 8 节）；本范围新增控件改用 `RowSelected`/`RowHover`/`PanelBackground` 等已登记 token，存量字面量在独立 QSS 清理任务中统一替换。

## 3. 字体

字体来自 `GlobalStyle::Fonts`（`include/Common/GlobalStyle.h:74`）。单一字体族，无衬线，CJK 优先。

### 3.1 字阶

| 级别 | GlobalStyle 常量 | 字号 | 字重 | 用途 |
|------|-------------------|------|------|------|
| 标题 | `Fonts::TitleSize` / `Fonts::TitleWeight` | 16px | bold | 窗口标题、模块标题、目标详情 ID 标签 |
| 正文 | `Fonts::BodySize` / `Fonts::BodyWeight` | 14px | normal | 正文、按钮、输入框、表格、菜单、工具提示 |
| 辅助 | `Fonts::CaptionSize` | 12px | normal | 时间戳、状态说明、详情行标签与值、状态分段、折叠态纵向文字、关闭按钮、刷新按钮 |

### 3.2 字体族

`Fonts::Family` = `"Microsoft YaHei, Source Han Sans SC, SimHei, sans-serif"`

优先微软雅黑，回退思源黑体、黑体，最后无衬线。本范围原语沿用此族，不引入第二字体族。

### 3.3 字体规则

- 本范围原语沿用 `Fonts::Family` 单一字体族，不引入等宽或衬线字体。
- 正文最小 14px；辅助最小 12px；不出现 12px 以下文字。
- 威胁等级值用 `CaptionSize + bold` 强调（见 `TargetDetailOverlay::refreshDetail`），属语义强调，不新增字阶。

## 4. 间距与布局

4px 为基准单位。本范围原语间距应是 4 的倍数；非 4 倍数的值列为债务（见第 8 节）。

### 4.1 间距 token（CURRENT 观察值）

| 倍数 | 值 | 用途出处 |
|------|----|------|
| ×1 | 4px | 表格 item padding（`LeftPanelWidget` 内联 QSS）、状态分段 padding、详情行间距、`QMenu::separator` margin |
| ×2 | 8px | 面板内容 margins、面板内 spacing、按钮水平 padding 基数、标题栏 spacing、详情头部 spacing、表格 item padding（`GlobalStyle` 全局） |
| ×3 | 12px | 详情面板 margins（`TargetDetailOverlay` 12,12,12,12）、地图浮层边距 `kPipMargin`（`MainWindow.cpp:39`）、状态分段容器 margins |
| ×4 | 16px | 按钮水平 padding（`6px 16px` 的 16）、标签页 padding、表头 padding |
| ×6 | 24px | 详情头部右侧为关闭按钮预留的 24px |
| ×8 | 32px | — |
| ×10 | 40px | 折叠态窄条宽度 `kCollapsedWidth`（`LeftPanelWidget.cpp:30`） |
| ×14 | 56px | `Sizes::TargetItemHeight`（CURRENT 实际行高 40px，见债务） |
| ×16 | 64px | 详情行标签固定宽 `label->setFixedWidth(64)` |
| ×20 | 80px | 导航栏宽 `Sizes::NavigationBarWidth` |
| ×80 | 320px | 左面板宽 `Sizes::LeftPanelWidth` |
| ×90 | 360px | 右面板宽 `Sizes::RightPanelWidth` |

### 4.2 区域与控件尺寸（`GlobalStyle::Sizes`）

| 常量 | 值 | 用途 |
|------|----|------|
| `WindowWidth` × `WindowHeight` | 1920 × 1080 | 默认窗口尺寸（最小 1280×720，见 `docs/UI.md` 第 3 节） |
| `NavigationBarWidth` | 80px | 左侧六项导航 |
| `LeftPanelWidth` | 320px | 展开态目标列表面板 |
| `RightPanelWidth` | 360px | 右侧态势/设备/决策面板 |
| `StatusBarHeight` | 28px | 底部状态栏 |
| `ToolbarHeight` | 32px | 工具栏 |
| `MenuBarHeight` | 30px | 菜单栏 |
| `TitleBarHeight` | 32px | 标题栏 |
| `ButtonMinWidth` | 80px | 按钮最小宽 |
| `ButtonHeight` | 32px | 按钮高 |
| `IconButtonSize` | 24px | 图标按钮尺寸（折叠/展开按钮 `24×24`） |

### 4.3 新增尺寸 token

| 常量 | 值 | 用途 |
|------|----|------|
| `Sizes::EvidenceViewportHeight` | 180px | 【新增】冻结证据图像视口最小高度。详情面板内容宽 = 340 − 2×12 = 316px，16:9 对应约 178px，取整 180px 作为最小高度，保证标注截图可读 |

### 4.4 网格与视口

- 非流式网格，固定面板宽 + 弹性中央区。左 320 / 中弹性（地图+视频PiP+浮层）/ 右 360。
- 三视口门禁见 `docs/UI.md` 第 7 节：1280×720 最小可用（CURRENT 已知决策面板约 5px 底部溢出）、1920×1080 默认与权威截图、3840×2160 4K 等比。
- 目标表列宽固定（`LeftPanelWidget.cpp:25`）：类型列 92、置信度列 56、位置列 88、状态列 stretch（4 列，勾选列已移除）。置信度列 36->56 以充分容纳 "88%" 与表头"置信度"；位置列 100->88 补偿置信度增宽，状态列 stretch 保持不变。

### 4.5 间距规则

- 4px 基准，本范围原语间距映射到 4 的倍数。
- 非对称间距必须有理由：详情头部 `contentsMargins(0,0,24,0)` 右侧 24px 是为浮动关闭按钮留位（`TargetDetailOverlay.cpp:69`）。
- 6px（按钮垂直 padding、详情按钮 padding）与 22px（关闭按钮 `22×22`）不是 4 的倍数，列为债务。

## 5. 组件

记录已批准目标列表与冻结证据详情原语，以及它们依赖的现有共享原语。现有原语来自 `GlobalStyle.cpp` 的 QSS 工厂函数与 `LeftPanelWidget`/`TargetDetailOverlay` 实现。

### 5.1 目标列表行（已批准原语 · `LeftPanelWidget` 目标表）

- **结构**：单行。CURRENT 列序：类型 / 置信度 / 位置 / 模拟状态（4 列，勾选列已移除）。行高 40px（CURRENT `setRowHeight(row, 40)`）。
- **变体**：默认 / hover / 选中。仅一个选中态，不定义 selected-hover。
- **间距**：item padding 4px（`LeftPanelWidget` 内联 QSS），列宽见 4.4。
- **状态**：
  - 默认：`PanelBackground #252526` 背景，`TextPrimary` 文本，类型列前景为威胁色。
  - hover：`PanelBackground` 背景（本地 `LeftPanelWidget` QSS 用 `::item:hover` 覆盖全局 `getMainWindowStyle` 的 `RowHover`，使未选中 hover 不引入第二种未选中底色）。
  - 选中：`RowSelected #2E3D2F`（新增绿色）背景，`TextPrimary` 文本，单一绿色选中态。选中行 hover 仍保持 `RowSelected`（本地 `::item:selected:hover` 规则以 QSS specificity 守护，颜色等同选中态，非独立视觉变体）。
  - 交替行：`alternatingRowColors` 关闭，所有未选中行统一 `PanelBackground` 单一底色，避免奇偶行产生两种未选中底。
- **交互**：单击发出 `targetSelected`，驱动模拟工作流与右侧目标状态；不绑定双击事件（详情由地图浮层承担）。
- **无障碍**：类型列仅以威胁色作前景（类型名本身即文字标签，无 `●` 字形；`●` 仅出现在详情浮层威胁值）；模拟状态文本以 `[模拟]` 前缀（`simulationTargetStatusText`）；行可达 Tab，选中态不仅靠颜色。
- **无勾选框**：已移除 `kTargetCheckColumn` 勾选列与 `Qt::ItemIsUserCheckable`，单选改为 `SingleSelection` 行选（`setColumnCount(4)`）。

### 5.2 目标详情浮层（已批准原语 · `TargetDetailOverlay`）

地图浮层，选中目标时在地图容器右上角浮现，关闭后地图恢复干净（`TargetDetailOverlay.h:11`）。固定宽 340px，不透明深色面板（`PanelBackground`）。

- **结构**（自上而下）：
  1. 证据图像视口（新增，见 5.3）——冻结标注截图，最焦点内容。
  2. 头部：独立容器 `ToolbarBackground` 不透明底（与 `PanelBackground` 面板形成明度阶差，避免地图从头部行透显），内含目标 ID（`TitleSize` bold）+ 类型标签（`CaptionSize`，`Border` 底色 chip）。
  3. 详情行容器：威胁等级 / 置信度 / 坐标（WGS84 经度/纬度）/ 检测设备 / 距跑道。每行 label 固定宽 64px（`TextSecondary`）+ value（`TextPrimary`，威胁值用威胁色 + bold）。坐标为 WGS84 经纬度（REQ-009 已批准经纬度坐标；`TargetInfo.position` 已存储 WGS84 经度/纬度，仅 CURRENT 标签与侧栏 x/z 呈现有误，列为待对齐债务，见第 8 节）。
  4. 操作按钮：创建处置任务（主按钮 `PrimaryGreen`）/ 指派设备（普通按钮）/ 查看历史检测（普通按钮）。
  5. 待检测提示：目标未检测到时显示，隐藏详情行与操作区。
  6. 操作反馈：`StatusOnline` 色 `CaptionSize` 文本，承载 `[模拟]` 反馈。
- **变体**：已检测态（显示详情+操作）/ 待检测态（仅提示）/ 隐藏态。
- **间距**：root margins 12,12,12,12，spacing 8；详情行 spacing 4；操作区 spacing 6；头部右侧 24px 留关闭按钮位。
- **底色**：`PanelBackground #252526`（不透明），1px `Border` 边框，3px 圆角。已由原 `rgba(37,37,38,240)` 半透明字面量替换为不透明 token，地图不再透显。
- **关闭按钮**：`22×22`，绝对定位右上角，6px margin（`resizeEvent` 重定位）。默认 `TextSecondary`，hover 转 `DangerRed` 底 + `TextPrimary`。
- **无障碍**：所有操作结果以 `[模拟]` 文本反馈；威胁值有文字+字形；浮层作为地图容器子控件，Tab 可达。
- **动作语义**：3 个操作为模拟反馈，不改状态机（`TargetDetailOverlay.h:13`）。冻结证据是只读快照，不可在浮层内编辑。

### 5.3 证据图像视口（已批准原语 · 新增）

冻结标注截图视口，是详情浮层的最焦点内容。承载目标被发现时刻的静态标注截图，与地图视频 PiP 的实时 HUD 区分：视频 PiP 是实时流，证据视口是冻结快照。

- **结构**：`QLabel` 或自定义 `QWidget`，`aspectRatioMode` 保持 16:9，最小高 `Sizes::EvidenceViewportHeight = 180px`，宽填满浮层内容区（316px）。背景 `Colors::EvidenceViewport #161616`，1px `Border` 边框，3px 圆角。
- **内容**：冻结标注截图（`QImage`），与实时视频 PiP 区分--视频 PiP 是实时流且 HUD-only（十字准星、REC、遥测 LAT/LON/ALT/HDG、时间码），证据视口是冻结快照。视口内只读，不可裁剪、不可缩放交互（最小实现）。
- **所有权与生命周期**：冻结 `QImage` 由 `MainWindow` 内存持有，按目标 ID 索引；检测发生时（`DetectionEngine::imageAnalyzed` 异常帧）捕获引擎热力图叠加图（`heatmapOverlay`，已含标注），仅在详情浮层显示（detail-only）。[结束] 保留已有证据（`onStopDetection` 保留目标/侧栏/选中），[重置] 清空（`onResetDetection` 清空目标/冻结证据/航迹）。不持久化到磁盘，进程结束即释放。
- **冻结标识**：左上角小 chip，`CaptionSize`，`TextSecondary` 文本 "证据快照（已冻结）"，`PanelBackground` 半透明底，配锁形 SVG 图标。不新增颜色，复用现有 token。冻结语义由文字+图标承担，不靠新颜色。
- **变体**：有证据（显示截图）/ 无证据（占位：`EvidenceViewport` 底 + `TextDisabled` "暂无证据快照[模拟]"）/ 待检测态（整个浮层为待检测提示，视口不显示）。
- **状态**：默认只读。无 hover/active 交互（非交互元素，符合"motion serves meaning"）。
- **无障碍**：占位态有文字；截图需提供 `accessibleDescription` 文本描述（如"目标 TGT-001 的冻结标注截图，置信度 92%"）。

### 5.4 地图浮层行为（map-floated detail behavior）

- **宿主**：`TargetDetailOverlay` 父控件为 `m_mapContainer`（`MainWindow.cpp:184`），与 `TacticalMapWidget`、`VideoStreamPanel` 同级。
- **定位**：地图容器右上角，`setGeometry(cw - overlayW - kPipMargin, kPipMargin, overlayW, overlayH)`（`MainWindow.cpp:670`），`kPipMargin = 12px`。每次地图容器 resize 重新定位并 `raise()`。
- **与视频 PiP 关系**：视频 PiP 在地图容器左下角（`kPipMargin, ch - pipH - kPipMargin`），浮层在右上角，互不遮挡。
- **生命周期**：选中目标 → `showTarget` → `show()` + `raise()`；关闭按钮或取消选中 → `hide()`。地图恢复干净。
- **不抢占焦点**：浮层显现不强制激活窗口，避免打断地图交互；点击浮层内部控件由 Qt 焦点链处理。

### 5.5 现有共享原语（依赖项）

- **主按钮 / 普通按钮 / 危险按钮**：`GlobalStyle::getButtonStyle` + `[primary="true"]`/`[danger="true"]` 属性。圆角 4px，padding `6px 16px`，`BodySize`。主按钮 `PrimaryGreen`/`PrimaryGreenHover`；普通按钮 `ToolbarBackground`/`Border` hover；危险按钮 `DangerRed`/`DangerRedHover`。详情浮层按钮用 3px 圆角、`CaptionSize`、padding `6px`，属局部变体。
- **输入框**：`getLineEditStyle`，1px `Border`，聚焦 1px `BorderFocus`，4px 圆角，padding `6px 8px`。搜索框（`LeftPanelWidget`）高 28px，placeholder 用 `TextDisabled`。
- **状态分段**：3 个 `QPushButton`（待检测/处置中/已完成），`[selected="true"]` 属性切换，选中态 2px `PrimaryGreen` 下边框 + `TextPrimary`，未选中 `TextSecondary`。
- **标签页**：`getTabWidgetStyle`，选中态 2px `PrimaryGreen` 下边框。
- **滚动条**：`getScrollBarStyle`，宽 10px（目标表内联 QSS 用 8px，债务），handle `TextSecondary`，5px 圆角。
- **折叠/展开窄条**：40px 宽，纵向文字 `目\n标\n列\n表`（`TextSecondary` `CaptionSize`），`▶/◀` 按钮 `24×24`，整条可点击展开。

## 6. 动效与交互

Qt QSS 无 CSS 过渡，动效通过 `QPropertyAnimation` 或 `QVariantAnimation` 实现。时序 token 来自 `GlobalStyle::Animation`（`include/Common/GlobalStyle.h:115`）。

### 6.1 时序

| 类型 | GlobalStyle 常量 | 时长 | 缓动 | 用途 |
|------|------------------|------|------|------|
| 微 | `Animation::DurationShort` | 150ms | `ease-in-out` | 按钮按压、toggle |
| 标准 | `Animation::DurationNormal` | 200ms | `ease-in-out` | 面板开合、标签切换、浮层显现候选 |
| 长 | `Animation::DurationLong` | 300ms | `ease-in-out` | 大范围过渡 |

`Animation::Easing` = `"ease-in-out"`，统一缓动曲线。

### 6.2 CURRENT 实际动效

- 地图目标红点脉冲：`TacticalMapWidget` 自绘脉冲动画（唯一持续运行的视觉动效）。
- 视频叠加层：CURRENT `VideoOverlayWidget` 为 HUD-only（十字准星、REC、遥测、时间码），不绘制检测框。冻结证据由详情浮层 `QImage` 承载（见 5.3）。
- 详情浮层显现：CURRENT 为瞬时 `show()/hide()`，无淡入。新增证据视口原语若加淡入，须用 `DurationNormal` 200ms + `ease-in-out`，只动画 `windowOpacity` 或 `pos`/`geometry`，不动画布局属性。
- 列表/表格选中：瞬时切换背景色，无过渡。

### 6.3 动效规则

- 只动画 `transform`/`opacity` 等价物（Qt 中为 `pos`、`windowOpacity`、`geometry` 中不触发布局重算的部分），不动画布局属性。
- 本范围交互元素须有 hover + active + focus（见第 5 节组件状态）。
- 非交互元素不加装饰性动效（证据视口不加 hover 动效）。
- 唯一签名动效是地图目标脉冲；其余动效服务状态切换。
- 减弱动效：Qt 不自动响应 `prefers-reduced-motion`；CURRENT 无全局开关，列为债务（见第 8 节）。

## 7. 深度与表面

策略：**tonal-shift + 1px 边框**（混合，以明度阶差为主，边框收边）。不使用阴影。Qt 阴影需 `QGraphicsDropShadowEffect`，CURRENT 未使用，本契约不引入。

### 7.1 表面阶差（由暗到亮）

| 层级 | token | 值 | 用途 |
|------|-------|----|------|
| L0 最暗 | `Colors::EvidenceViewport` | `#161616` | 证据视口（新增，最焦点） |
| L1 | `Colors::Background` | `#1E1E1E` | 主窗口、地图容器、输入框、表格 |
| L2 | `Colors::PanelBackground` | `#252526` | 面板、浮层底色、菜单选中 |
| L3 最亮 | `Colors::ToolbarBackground` | `#2D2D2D` | 工具栏、菜单栏、表头、状态分段 |

> 注：暗色主题下"最亮"指明度最高，仍属深色范围。证据视口 `#161616` 反向低于 `Background`，使截图作为焦点内容在更暗的框中浮起。

### 7.2 边框

| 类型 | 值 | 用途 |
|------|----|------|
| 默认 | 1px solid `Colors::Border #3C3C3C` | 控件边框、分隔线、表格网格线、浮层边框 |
| 聚焦 | 1px solid `Colors::BorderFocus #4A7A4C` | 输入框聚焦（与主色同值） |
| 选中下划线 | 2px solid `Colors::PrimaryGreen #4A7A4C` | 标签页、状态分段选中态 |

### 7.3 圆角

| 用途 | 值 |
|------|----|
| 按钮、输入框、下拉框、复选框、搜索框 | 4px |
| 浮层、证据视口、详情按钮、关闭按钮、类型 chip | 3px |
| 滚动条 handle | 5px |
| 单选框 | 8px |

### 7.4 深度规则

- 表面靠 4 阶明度分层，不靠阴影。
- 边框只用于收边与分隔，1px 极暗。
- 浮层用半透明（240/255）表达"浮于地图之上"，不加重影。
- 证据视口用比 `Background` 更暗的 `EvidenceViewport` 表达焦点，不靠边框加粗。

## 8. 无障碍约束与已接受债务

### 8.1 约束

- **WCAG 目标**：2.1 AA。对比地板：正文 4.5:1，大字/14px+bold 3:1。
- **对比度核查**（深色底）：
  - `TextPrimary #FFFFFF` on `Background/PanelBackground`：约 15:1，通过。
  - `TextSecondary #AAAAAA` on `PanelBackground #252526`：约 6.6:1，通过。
  - `TextPrimary #FFFFFF` on `PrimaryGreen #4A7A4C`（主按钮）：约 4.6:1，通过（边界值）。
  - `TextPrimary #FFFFFF` on `RowSelected #2E3D2F`（新增）：约 11.6:1，通过。
  - `TextDisabled #888888` on `PanelBackground #252526`：约 4.35:1，**低于 AA 4.5**，仅用于禁用/placeholder，列为债务。
  - `ThreatHigh #FF5252` 作文本 on `PanelBackground`：约 4.06:1，**低于 AA 4.5**，由 `●` 字形+文字标签缓解，列为债务。
- **颜色非唯一信息**：威胁/优先级/状态色不作唯一信息载体（`docs/UI.md` 第 8 节）。详情浮层威胁值配 `●` 字形与文字；目标列表类型列以类型名作文字标签，无字形要求，通过。
- **可见聚焦**：输入框 1px `BorderFocus` 聚焦边框。自定义绘制控件（表格、浮层按钮）依赖 Qt 默认焦点框，权重不足，列为债务。
- **键盘可达**：所有交互控件通过 Qt Tab 链可达；浮层作为地图容器子控件纳入焦点链。
- **模拟标注**：所有模拟操作与结果以 `[模拟]`/`演示` 前缀（`docs/UI.md` 第 8 节、`AGENTS.md` 安全边界），通过。
- **不显示误导性危险控件**：紧急停止等无消费者占位控件按 `docs/UI.md` 第 4.4 节处理为禁用/省略。

### 8.2 已接受债务

| 项 | 位置 | 接受原因 | 退出条件 |
|----|------|------|------|
| 内联 QSS 颜色字面量（`#2A3F54`、`#2A2A2A`、`#555555`、硬编码 `#2D2D2D`/`#FFFFFF` 表头） | `GlobalStyle.cpp`、`LeftPanelWidget.cpp:223/240` 等 | 现有实现历史遗留，与本契约新增原语无冲突；合并属独立重构 | 实现目标列表/证据原语时，新控件改用 `RowSelected`/`RowHover`/`PanelBackground` 等 token；存量字面量在独立 QSS 清理任务中统一替换 |
| ~~目标表勾选列 `kTargetCheckColumn`~~ | `LeftPanelWidget.cpp` | 已解决：`setColumnCount(4)` 移除勾选列，`SingleSelection` 行选 | ~~实现目标列表原语时移除该列与 `ItemIsUserCheckable`~~ 已完成 |
| 旧蓝色选中 `#2A3F54` | `GlobalStyle.cpp` 选中字面量 | 用户已批准替换为 `RowSelected #2E3D2F`，`docs/UI.md`/`docs/ui/design-system.md` 待对齐 | 实现目标列表原语时替换，并登记 `Colors::RowSelected` |
| 目标坐标标签与侧栏呈现为 x/z 而非 WGS84 经纬度 | `TargetDetailOverlay.cpp`/`LeftPanelWidget.cpp` 坐标行 | REQ-009 已批准经纬度坐标；`TargetInfo.position` 已存储 WGS84 经度/纬度，仅显示标签与侧栏 x/z 呈现有误 | 实现时修正标签为经度/纬度 |
| `TargetItemHeight=56` 与实际行高 40 不一致 | `GlobalStyle.h:101` vs `LeftPanelWidget.cpp:342` | 现有代码事实，行高 40 在 320px 面板内密度合理 | 独立尺寸对齐任务中统一为 40 或 56 |
| 非 4 倍数间距（6px 按钮 padding、22px 关闭按钮、10px 滚动条宽） | `GlobalStyle.cpp`、`TargetDetailOverlay.cpp:57`、`LeftPanelWidget.cpp:258` | Qt 控件习惯尺寸，改动影响面广 | 独立间距规整任务中评估对齐到 4 倍数 |
| 表格 item padding 不一致（全局 8px vs 目标表 4px） | `GlobalStyle.cpp` vs `LeftPanelWidget.cpp:249` | 目标表密度需求 | 独立 QSS 清理任务统一 |
| 聚焦边框 1px、自定义控件焦点框弱 | `GlobalStyle.cpp` 输入框聚焦、表格/浮层 | Qt QSS 焦点表达有限 | 评估改用 2px 聚焦或 `QGraphicsDropShadowEffect`，独立任务 |
| 无减弱动效全局开关 | 全局 | Qt 不自动响应 `prefers-reduced-motion` | 评估增加应用级设置项，独立任务 |
| `TextDisabled` 对比 4.35:1、`ThreatHigh` 文本对比 4.06:1 | `Colors::TextDisabled`、`Colors::ThreatHigh` | 前者仅禁用/placeholder，后者配字形+文字 | 若需严格 AA，调亮 `TextDisabled` 至 `#999999`、`ThreatHigh` 文本改用更亮的 `#FF6B6B`，独立任务评估 |
| 详情浮层瞬时显现无淡入 | `TargetDetailOverlay::showTarget` | CURRENT 行为 | 证据原语实现时可选加 200ms `windowOpacity` 淡入，不阻塞 |

### 8.3 债务规则

- 新增债务在接受时即登记于此，不得静默。
- 本契约只新增已批准目标列表与冻结证据详情原语所需 token（`RowSelected`、`EvidenceViewport`、`EvidenceViewportHeight`）；其余债务均为现有实现事实，不在本任务范围内修复。
- 任何新控件不得复用上述【内联】字面量，须使用已登记 token。
