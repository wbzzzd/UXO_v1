# 配置页面设计

状态：`TARGET / Draft / 设计评审原型 / 本地模拟`
上级：[docs/ui/README.md](../README.md)
设计系统：[docs/ui/design-system.md](design-system.md)
应用壳：[docs/ui/application-shell.md](application-shell.md)
一览：[docs/ui/pages/index.md](index.md)
原型：[docs/ui/prototypes/configuration/index.html](../prototypes/configuration/index.html)
截图：[docs/ui/images/configuration/overview-1920x1080.png](../images/configuration/overview-1920x1080.png)

> 本文是配置页面（configuration page）的完整设计契约。每个交互控件拥有稳定 `CFG-*` ID 与全字段规格。HTML 原型实现必须以本文为唯一依据，控件 ID、区域结构和状态规则不得在实现阶段自行变更。所有视觉值取自 `design-system.md`。

CURRENT 来源：
- **本页在 CURRENT Qt 客户端中未实现为独立页面。** 以下 CURRENT 映射指向应用壳共享组件或标注"未实现"。
- 导航栏/菜单栏/工具栏/状态栏：见 [`application-shell.md`](../application-shell.md)
- 无 CURRENT 配置对话框或设置组件。配置页为 HTML 原型独有，TARGET 设计。

## 1. 页面概述

配置页面是系统的第六页（导航 `CFG-NAV-06` 默认选中）。它一屏呈现：左侧配置分类导航与系统版本信息；中心区按只读事实、临时展示（仅本会话）、禁用三类展示系统配置；右侧安全边界清单与变更记录（模拟）。所有配置不持久化，会话结束即清空，不连接真实设备、不写入数据库、不执行排爆动作、不进行外部通信。

页面整体布局由应用壳定义（见 `application-shell.md` 第 2 节）。本文档化页面内部的三个内容区域与复用的应用壳控件：

| 区域 | 位置 | 内部组件 | CURRENT 映射 |
|------|------|----------|--------------|
| A | 左面板 | 配置分类导航、系统版本信息（只读） | 未实现 |
| B | 中心区 | 中心头、配置内容区（6 个配置分类区块）、操作按钮 | 未实现 |
| C | 右面板 | 安全边界清单（5 项）、变更记录（模拟） | 未实现 |

应用壳控件（导航栏、菜单栏、工具栏、状态栏）的详细规格见 `application-shell.md`，本文第 5 节给出本页专属的 `CFG-*` ID 与简要规格。

配置分类有三类语义标记：

| 标记 | 含义 | 标签样式 |
|------|------|----------|
| 只读 | 当前构建事实，不可修改 | `.cfg-tag.readonly`：`--color-border` 背景、`--color-text-secondary` 文本 |
| 临时 | 仅当前会话生效，不持久化 | `.cfg-tag.pending`：`--color-selection` 背景、`--color-primary` 文本 |
| 禁用 | V1.0 不实现，已禁用 | `.cfg-tag.disabled`：`--color-bg` 背景、`--color-text-disabled` 文本、1px `--color-border` 边框 |

## 2. 区域 A：左面板

宽 260px 固定（`--size-left-panel-width`），背景 `--color-panel`，外边距 8px，间距 8px。从上到下：配置分类标题 -> 配置分类导航 -> 系统信息卡片。

### 2.1 配置分类标题（只读）

`QLabel` 风格文本"配置分类"，`--font-size-body`，`--color-text-primary`，加粗，内边距 `0 4px`。无 ID（只读标签）。

### 2.2 配置分类导航 `CFG-LP-NAV`

背景透明，垂直布局，间距 4px。包含 6 个分类项，每项高 44px，`flex` 布局，`align-items: center`，间距 8px，内边距 `0 12px`，圆角 `--radius-control`，`cursor: pointer`，`--color-text-secondary` 文本，`--font-size-body`，左边框 3px 透明，过渡动画 `--anim-short` `--anim-easing`。

默认态：透明背景、`--color-text-secondary` 文本、左边框 3px 透明。hover：背景 `--color-row-hover`、文本 `--color-text-primary`。选中态：背景 `--color-selection`、左边框 `--color-primary`、文本 `--color-text-primary`、加粗。

| 字段 | 值 |
|------|----|
| ID | `CFG-LP-NAV` |
| 类型 | div 容器（含 6 个分类项） |
| 位置 | 左面板，标题下方 |
| 用途 | 配置分类导航，点击切换选中态 |
| 默认选中 | 第 1 项"系统信息"（`selected` 类） |
| 分类项列表 | 系统信息(`data-cfg="system"`)、界面显示(`data-cfg="display"`)、数据存储(`data-cfg="data"`)、网络通信(`data-cfg="network"`)、安全控制(`data-cfg="safety"`)、关于(`data-cfg="about"`) |
| 点击结果 | 移除其他分类项的 `selected` 类，当前项添加 `selected`。**不切换中心区内容**：6 个配置区块始终全部可见，分类导航仅切换视觉高亮 |
| 键盘 | div 元素无 `tabindex`，不可键盘聚焦（无障碍缺口） |
| 原型行为 | 点击仅切换 `selected` 类，中心区内容不变，中心头标题"系统信息"也不随选择更新 |
| CURRENT 映射 | 未实现（CURRENT 无配置页面） |
| 安全 | 无实际配置变更，仅视觉切换 |

> 注：分类项图标统一为 `◎`（内联文本）。分类项无独立 `data-testid`，仅容器 `CFG-LP-NAV` 有 ID。如后续需对分类项单独测试，需在 HTML 中补齐 `data-testid` 并在本文登记。

### 2.3 系统信息 `CFG-LP-SYS`

背景 `--color-toolbar`，圆角 `--radius-control`，内边距 12px，垂直布局，间距 6px。包含 3 行键值对，每行 `flex` 布局，`justify-content: space-between`，字号 11px。

| 行 | 标签 | 值 | 值文字色 |
|----|------|----|----------|
| 1 | 版本 | `V1.0.0 (原型)` | `--color-text-primary` |
| 2 | 构建 | `2026-07-23` | `--color-text-primary` |
| 3 | 模式 | `[模拟]` | `--color-status-busy` |

| 字段 | 值 |
|------|----|
| ID | `CFG-LP-SYS` |
| 类型 | div 容器（只读） |
| 位置 | 左面板，分类导航下方 |
| 用途 | 展示系统版本、构建日期与运行模式（只读） |
| 默认值 | 固定显示上述 3 行 |
| 点击结果 | 无（只读） |
| 键盘 | 不可聚焦 |
| 原型行为 | 固定显示，不随操作变化 |
| CURRENT 映射 | 未实现 |
| 安全 | 只读展示，模拟标注 |

## 3. 区域 B：中心区

`flex: 1`，背景 `--color-panel`，垂直布局，`overflow: hidden`。从上到下：中心头 -> 配置内容区 -> 操作按钮行。

### 3.1 中心头（只读）

高 40px，背景 `--color-toolbar`，底部 1px `--color-border` 边框，内边距 `0 16px`，间距 12px。包含标题与副标题，均无 ID（只读）。

| 元素 | 文本 | 样式 |
|------|------|------|
| 标题 | `系统信息` | `--font-size-title`，`--color-text-primary`，加粗 |
| 副标题 | `[只读] 当前事实 · [临时] 本会话展示` | `--color-text-secondary`，`--font-size-caption` |

> 注：中心头标题固定为"系统信息"，不随配置分类导航选择而更新。

### 3.2 配置内容区 `CFG-CE-CONTENT`

`flex: 1`，`overflow: auto`，内边距 10px，垂直布局，间距 8px。包含 6 个配置区块，每个区块为 `.cfg-section`：背景 `--color-panel`，1px `--color-border` 边框，圆角 `--radius-control`，内边距 `8px 14px`，垂直布局，间距 4px。

每个区块头为 `flex` 行，`justify-content: space-between`，包含区块标题（`.cfg-section-title`：`--font-size-body`，`--color-text-primary`，加粗，底部 1px `--color-border` 边框，`padding-bottom: 8px`）与区块注释（`.cfg-section-note`：`--color-status-busy`，11px，加粗）。

配置行（`.cfg-row`）默认 `grid-template-columns: 200px 1fr 220px`，间距 16px，`align-items: center`，`padding: 3px 0`。部分行覆盖列宽。

#### 3.2.1 系统事实（只读）

区块注释：`[只读] 以下为当前构建事实，不可修改`。6 行配置，每行末尾标签为 `.cfg-tag.readonly`。

| 行 | 标签 | 值 | 末尾标签 |
|----|------|----|----------|
| 1 | 产品名称 | `UXOMissionControl 排弹抢修指挥系统` | 只读 |
| 2 | 版本号 | `V1.0.0`（`.mono` 等宽字体） | 只读 |
| 3 | 构建日期 | `2026-07-23`（`.mono`） | 只读 |
| 4 | 运行模式 | `[模拟模式] 本地固定场景，无真实设备接入`（`[模拟模式]` 为 `.sim-tag`） | 只读 |
| 5 | 已实现模块 | `态势 / 探测 / 决策 / 设备 / 统计 / 配置（UI 原型）` | 只读 |
| 6 | 未实现模块 | `真实设备控制 / 排爆执行 / 外部通信 / 数据库写入`（`--color-text-disabled`） | 禁用（`.cfg-tag.disabled`） |

| 字段 | 值 |
|------|----|
| ID | 所属容器 `CFG-CE-CONTENT`（区块本身无独立 ID） |
| 类型 | div 区块（只读） |
| 位置 | 配置内容区第 1 个区块 |
| 用途 | 展示当前构建事实，不可修改 |
| 点击结果 | 无（只读） |
| 键盘 | 不可聚焦 |
| 原型行为 | 固定展示，不随操作变化 |
| CURRENT 映射 | 未实现 |
| 安全 | 只读事实，无配置入口 |

#### 3.2.2 界面显示（临时，仅本会话）

区块注释：`[临时] 修改仅当前会话生效，不持久化`。6 行配置。

| 行 | 标签 | 值 | 末尾标签 / 控件 |
|----|------|----|----------------|
| 1 | 主题 | `深色（唯一可选）` | 占位（`.cfg-tag.disabled`） |
| 2 | 语言 | `简体中文（唯一可选）` | 占位（`.cfg-tag.disabled`） |
| 3 | 地图底图 | `离线栅格图（模拟）` | 临时可切换（`.cfg-tag.pending`） |
| 4 | 坐标显示 | `显示模拟坐标` | `CFG-CE-TOGGLE-COORD`（开关，默认开） |
| 5 | 告警声音 | `关闭（模拟）` | `CFG-CE-TOGGLE-ALARM`（开关，默认关） |
| 6 | 刷新间隔 | `3000 ms`（`.input-fake` 占位）+ 占位按钮（禁用） | 无标签 |

##### 3.2.2.1 坐标显示开关 `CFG-CE-TOGGLE-COORD`

| 字段 | 值 |
|------|----|
| ID | `CFG-CE-TOGGLE-COORD` |
| 标签 | 无（行标签"坐标显示"在左侧列） |
| 类型 | div（自定义开关） |
| 位置 | 界面显示区块第 4 行右侧 |
| 用途 | 切换坐标显示（占位，仅本会话） |
| 尺寸 | 36x18px，圆形滑块 14x14px |
| 默认态 | 开（`.on` 类）：背景 `--color-primary`，不透明度 1，滑块在右侧（`left:20px`），白色 |
| 关闭态 | 无 `.on` 类：背景 `--color-border`，不透明度 0.5，滑块在左侧（`left:2px`），`--color-text-secondary` |
| cursor | `not-allowed`（CSS 对所有 `.toggle` 元素设置） |
| tooltip | `占位：本会话临时切换` |
| 点击结果 | 切换 `.on` 类，仅视觉变化，不持久化 |
| 键盘 | div 无 `tabindex`，不可键盘聚焦（无障碍缺口） |
| 原型行为 | 点击切换开关视觉状态；无实际配置效果，不写入任何存储 |
| CURRENT 映射 | 未实现 |
| 安全 | 占位控件，仅本会话视觉切换，不持久化 |

> 注：开关 CSS 设置 `cursor: not-allowed`，但 JS 仍绑定 click 事件使其可点击。视觉与交互存在不一致，TARGET 实现时应统一为明确的禁用或启用样式。

##### 3.2.2.2 告警声音开关 `CFG-CE-TOGGLE-ALARM`

| 字段 | 值 |
|------|----|
| ID | `CFG-CE-TOGGLE-ALARM` |
| 标签 | 无（行标签"告警声音"在左侧列） |
| 类型 | div（自定义开关） |
| 位置 | 界面显示区块第 5 行右侧 |
| 用途 | 切换告警声音（占位，仅本会话） |
| 尺寸 | 36x18px，圆形滑块 14x14px |
| 默认态 | 关（无 `.on` 类）：背景 `--color-border`，不透明度 0.5，滑块在左侧，`--color-text-secondary` |
| 开启态 | 有 `.on` 类：背景 `--color-primary`，不透明度 1，滑块在右侧，白色 |
| cursor | `not-allowed` |
| tooltip | `占位：本会话临时切换` |
| 点击结果 | 切换 `.on` 类，仅视觉变化，不持久化 |
| 键盘 | div 无 `tabindex`，不可键盘聚焦（无障碍缺口） |
| 原型行为 | 点击切换开关视觉状态；无实际配置效果，不播放声音 |
| CURRENT 映射 | 未实现 |
| 安全 | 占位控件，仅本会话视觉切换，不持久化 |

##### 3.2.2.3 刷新间隔占位行

第 6 行使用 `grid-template-columns: 200px 1fr`（2 列）。右侧为 `flex` 容器，间距 8px：`.input-fake` 占位文本 `3000 ms`（`--color-bg` 背景、`--color-text-disabled` 文本、1px `--color-border` 边框、圆角 `--radius-control`、等宽字体）+ 禁用按钮（`disabled` 属性、`.btn` 样式、`--color-text-disabled` 文本）。按钮无 `data-testid`，不纳入 ID 索引。

#### 3.2.3 数据存储（禁用）

区块注释：`[禁用] V1.0 不实现持久化`。1 行配置，`grid-template-columns: 200px 1fr 80px`。

| 行 | 标签 | 值 | 末尾标签 |
|----|------|----|----------|
| 1 | 禁用项 | `本地数据库 · 日志文件 · 导出报告 · 配置持久化`（`--color-text-disabled`） | 禁用 |

| 字段 | 值 |
|------|----|
| ID | 所属容器 `CFG-CE-CONTENT` |
| 类型 | div 区块（禁用展示） |
| 位置 | 配置内容区第 3 个区块 |
| 用途 | 标注 V1.0 不实现的数据存储功能 |
| 点击结果 | 无（只读） |
| 键盘 | 不可聚焦 |
| 原型行为 | 固定展示禁用项清单 |
| CURRENT 映射 | 未实现 |
| 安全 | 禁用项，不实现数据库写入、日志文件、导出报告、配置持久化 |

#### 3.2.4 网络通信（禁用）

区块注释：`[禁用] V1.0 不实现外部通信`。1 行配置，`grid-template-columns: 200px 1fr 80px`。

| 行 | 标签 | 值 | 末尾标签 |
|----|------|----|----------|
| 1 | 禁用项 | `设备接入协议 · 远程指挥链路 · 数据上报`（`--color-text-disabled`） | 禁用 |

| 字段 | 值 |
|------|----|
| ID | 所属容器 `CFG-CE-CONTENT` |
| 类型 | div 区块（禁用展示） |
| 位置 | 配置内容区第 4 个区块 |
| 用途 | 标注 V1.0 不实现的网络通信功能 |
| 点击结果 | 无（只读） |
| 键盘 | 不可聚焦 |
| 原型行为 | 固定展示禁用项清单 |
| CURRENT 映射 | 未实现 |
| 安全 | 禁用项，不实现设备接入协议、远程指挥链路、数据上报 |

#### 3.2.5 安全控制（锁定）

区块注释：`[锁定] 危险动作默认禁用，需授权`。1 行配置，`grid-template-columns: 200px 1fr 80px`。

| 行 | 标签 | 值 | 末尾标签 |
|----|------|----|----------|
| 1 | 锁定项 | `紧急停止 · 排爆执行 · 设备控制指令 · 数据库写入 · 外部通信`（`--color-danger`） | 锁定（`.cfg-tag.disabled`） |

| 字段 | 值 |
|------|----|
| ID | 所属容器 `CFG-CE-CONTENT` |
| 类型 | div 区块（锁定展示） |
| 位置 | 配置内容区第 5 个区块 |
| 用途 | 标注已锁定的危险动作 |
| 点击结果 | 无（只读） |
| 键盘 | 不可聚焦 |
| 原型行为 | 固定展示锁定项清单 |
| CURRENT 映射 | 未实现 |
| 安全 | 锁定项，紧急停止、排爆执行、设备控制指令、数据库写入、外部通信全部禁用 |

#### 3.2.6 关于

无区块注释。2 行配置，`grid-template-columns: 120px 1fr 200px`。

| 行 | 标签 | 值 | 末尾标签 |
|----|------|----|----------|
| 1 | 项目 | `UXO_v1 排弹抢修指挥系统 · 文档: PRODUCT/ARCHITECTURE/UI/DEVELOPMENT.md` | `[TARGET / Draft]`（`.sim-tag`） |
| 2 | 安全声明 | `本原型不控制真实设备、不执行排爆动作、不进行外部通信、不写入数据库。`（`--color-text-secondary`） | 只读 |

| 字段 | 值 |
|------|----|
| ID | 所属容器 `CFG-CE-CONTENT` |
| 类型 | div 区块（只读） |
| 位置 | 配置内容区第 6 个区块 |
| 用途 | 展示项目信息与安全声明 |
| 点击结果 | 无（只读） |
| 键盘 | 不可聚焦 |
| 原型行为 | 固定展示 |
| CURRENT 映射 | 未实现 |
| 安全 | 只读声明，重申安全边界 |

### 3.3 操作按钮

位于配置内容区底部，`flex` 布局，`justify-content: flex-end`，间距 8px，`padding-top: 8px`。

#### 3.3.1 放弃修改 `CFG-CE-BTN-CANCEL`

| 字段 | 值 |
|------|----|
| ID | `CFG-CE-BTN-CANCEL` |
| 标签 | 放弃修改 |
| 类型 | button |
| 位置 | 操作按钮行左 |
| 用途 | 放弃本次会话的临时修改（占位） |
| 样式 | `.btn`：`--color-bg` 背景、`--color-text-secondary` 文本、1px `--color-border` 边框、圆角 `--radius-control`、`--font-size-caption`、内边距 `4px 12px` |
| 默认态 | **始终禁用**（`disabled` 属性 + `data-disabled="true"`）：`--color-text-disabled` 文本，`cursor: not-allowed` |
| tooltip | `占位控件，未实现` |
| 点击结果 | 无（disabled，不响应点击） |
| 键盘 | 不可聚焦（disabled） |
| 原型行为 | 禁用占位，不执行任何操作 |
| CURRENT 映射 | 未实现 |
| 安全 | 占位控件，无实际效果 |

#### 3.3.2 应用 `CFG-CE-BTN-APPLY`

| 字段 | 值 |
|------|----|
| ID | `CFG-CE-BTN-APPLY` |
| 标签 | 应用（仅本会话） |
| 类型 | button |
| 位置 | 操作按钮行右 |
| 用途 | 应用本次会话的临时修改（占位） |
| 样式 | `.btn.btn-primary`：`--color-primary` 背景、白色文本、`--color-primary` 边框、圆角 `--radius-control`、`--font-size-caption`、内边距 `4px 12px` |
| 默认态 | **始终禁用**（`disabled` 属性 + `data-disabled="true"`）：`--color-border` 背景、`--color-text-disabled` 文本、`--color-border` 边框，`cursor: not-allowed` |
| tooltip | `占位控件，本会话临时展示，不持久化` |
| 点击结果 | 无（disabled，不响应点击） |
| 键盘 | 不可聚焦（disabled） |
| 原型行为 | 禁用占位，不执行任何操作 |
| CURRENT 映射 | 未实现 |
| 安全 | 占位控件，无实际效果，不持久化 |

## 4. 区域 C：右面板

宽 320px 固定（`--size-right-panel-width`），背景 `--color-panel`，垂直布局，`overflow: hidden`。两个等高区块（`flex: 1` 各占一半）：安全边界与变更记录。

### 4.1 安全边界

区块头 32px：背景 `--color-toolbar`，底部 1px `--color-border` 边框，内边距 `0 8px`。标题"安全边界"（`.rp-section-title`：`flex: 1`，`--font-size-body`，`--color-text-primary`，加粗）。

内容区 `flex: 1`，`overflow: auto`，内边距 12px，间距 10px。包含 5 张安全项卡片，每张背景 `--color-toolbar`，圆角 `--radius-control`，内边距 10px，垂直布局，间距 6px。

每张卡片结构：标题（`.safety-title`：`--color-text-primary`，`--font-size-caption`，加粗）+ 描述（`.safety-desc`：`--color-text-secondary`，11px，行高 1.5）+ 状态行（`.safety-status`：`flex`，间距 6px，11px，含 8x8px 圆点 + 标签）。

| 序号 | 标题 | 描述 | 圆点状态 | 状态标签 |
|------|------|------|----------|----------|
| 1 | 真实设备控制 | V1.0 不实现真实设备控制；原型所有设备状态为模拟。 | `.dot.disabled`（`--color-text-disabled`） | 已禁用（`--color-text-secondary`） |
| 2 | 排爆动作执行 | V1.0 不实现任何排爆、引爆、拆除动作。 | `.dot.disabled` | 已禁用 |
| 3 | 外部通信 | V1.0 不与外部系统、网络或远程指挥通信。 | `.dot.disabled` | 已禁用 |
| 4 | 数据库写入 | V1.0 不持久化任何数据，会话结束即清空。 | `.dot.disabled` | 已禁用 |
| 5 | 紧急停止 | 原型中为占位按钮，无设备停止效果，已锁定禁用。 | `.dot.disabled` | 已锁定 |

| 字段 | 值 |
|------|----|
| ID | 无（区块及卡片均无 `data-testid`） |
| 类型 | div 容器（只读） |
| 位置 | 右面板上半 |
| 用途 | 展示安全边界清单，5 项全部禁用或锁定 |
| 默认值 | 固定显示 5 张卡片 |
| 点击结果 | 无（只读） |
| 键盘 | 不可聚焦 |
| 五态 | 正常：卡片列表；加载：不适用（静态）；空：不适用（固定 5 项）；错误：不适用；禁用：不适用（始终只读） |
| 原型行为 | 固定展示，不随操作变化 |
| CURRENT 映射 | 未实现 |
| 安全 | 只读展示，重申安全边界，全部禁用/锁定 |

### 4.2 变更记录（模拟）

区块头 32px：标题"变更记录（模拟）"。内容区 `flex: 1`，`overflow: auto`，内边距 12px，间距 10px。包含 4 条变更记录卡片，每张背景 `--color-toolbar`，圆角 `--radius-control`，内边距 10px，垂直布局，间距 4px。

每张卡片结构：时间（`.history-time`：`--color-text-secondary`，10px）+ 动作（`.history-action`：`--color-text-primary`，`--font-size-caption`）+ 用户（`.history-user`：`--color-text-disabled`，10px）。

| 序号 | 时间 | 动作 | 用户 | 备注 |
|------|------|------|------|------|
| 1 | `14:30:00` | `[模拟] 会话启动` | `admin (演示)` | |
| 2 | `14:30:05` | `[模拟] 加载默认配置` | `admin (演示)` | |
| 3 | `14:31:12` | `[模拟] 切换至配置页` | `admin (演示)` | |
| 4 | `--:--:--` | `[禁用] 历史记录未持久化` | `--` | `opacity: 0.5`，动作文本 `--color-text-disabled` |

| 字段 | 值 |
|------|----|
| ID | 无（区块及卡片均无 `data-testid`） |
| 类型 | div 容器（只读） |
| 位置 | 右面板下半 |
| 用途 | 展示模拟变更记录 |
| 默认值 | 固定显示 4 条记录（前 3 条为模拟会话事件，第 4 条为禁用占位） |
| 点击结果 | 无（只读） |
| 键盘 | 不可聚焦 |
| 五态 | 正常：记录列表；加载：不适用（静态）；空：不适用（固定 4 条）；错误：不适用；禁用：第 4 条以 `opacity: 0.5` 标注"历史记录未持久化" |
| 原型行为 | 固定展示，不随操作追加记录；第 4 条标注历史记录未持久化 |
| CURRENT 映射 | 未实现 |
| 安全 | 模拟数据，不持久化，会话结束即清空 |

## 5. 应用壳控件

本节文档化配置页复用的应用壳控件（导航栏、菜单栏、工具栏、状态栏），为其分配 `CFG-*` ID。详细规格（尺寸、间距、跨页一致性规则）见 `application-shell.md`。

### 5.1 导航栏

宽 80px 固定（`--size-nav-width`），背景 `--color-bg`，右边框 1px `--color-border`。垂直布局：Logo + 间距 + 6 个导航项 + 弹性留白。

| ID | 标签 | 类型 | 位置 | 用途 | 默认态 | hover | 选中态 | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|--------|-------|--------|---------|------|---------|---------------|------|
| `CFG-NAV-LOGO` | UXO | div | 导航栏顶，40px 高 | 品牌标识 | `--color-primary` 色，18px，加粗，字间距 2px，居中 | 不适用 | 不适用 | 无 | 不可聚焦 | 同 CURRENT | 见 `application-shell.md` 第 3 节 | 无 |
| `CFG-NAV-01` | 态势 | div | 导航项 1，56px 高 | 切换到态势页 | `--color-text-secondary`，左边框 3px 透明 | 背景 `--color-row-hover`、文本 `--color-text-primary` | 背景 `--color-selection`、左边框 `--color-primary`、文本 `--color-text-primary`、加粗 | 移除其他项 selected，当前加 selected（无实际页面跳转） | div 无 tabindex，不可键盘聚焦 | 仅切换高亮，不跳转页面 | 见 `application-shell.md` 第 3 节 | 无 |
| `CFG-NAV-02` | 探测 | div | 导航项 2，56px 高 | 切换到探测页 | 未选中 | 同上 | 同上 | 同上 | 同上 | 同上 | 见 `application-shell.md` 第 3 节 | 无 |
| `CFG-NAV-03` | 决策 | div | 导航项 3，56px 高 | 切换到决策页 | 未选中 | 同上 | 同上 | 同上 | 同上 | 同上 | 见 `application-shell.md` 第 3 节 | 无 |
| `CFG-NAV-04` | 设备 | div | 导航项 4，56px 高 | 切换到设备页 | 未选中 | 同上 | 同上 | 同上 | 同上 | 同上 | 见 `application-shell.md` 第 3 节 | 无 |
| `CFG-NAV-05` | 统计 | div | 导航项 5，56px 高 | 切换到统计页 | 未选中 | 同上 | 同上 | 同上 | 同上 | 同上 | 见 `application-shell.md` 第 3 节 | 无 |
| `CFG-NAV-06` | 配置 | div | 导航项 6，56px 高 | 切换到配置页（当前页） | 选中 | 同上 | 同上 | 同上 | 同上 | 默认选中；点击仅保持选中 | 见 `application-shell.md` 第 3 节 | 无 |

导航项图标统一为 `◎`（18px）。导航项内边距由 flex 居中控制，字号 `--font-size-caption`，间距 4px。

> 注：原型中导航点击仅切换 selected 类，不执行实际页面跳转（单页原型）。CURRENT Qt 客户端中导航切换通过 `QStackedWidget` 实现，见 `application-shell.md`。

### 5.2 菜单栏

高 30px（`--size-menu-bar-height`），背景 `--color-menu`，底部 1px `--color-border` 边框，内边距 `0 4px`。4 个菜单项为 `<button>` 元素，内边距 `6px 12px`，`--font-size-body`。

默认态：透明背景、`--color-text-primary` 文本。hover：背景 `--color-border`。禁用态（`data-disabled="true"`）：`--color-text-disabled` 文本，hover 不变背景，`cursor: not-allowed`。

| ID | 标签 | 类型 | 位置 | 用途 | 默认态 | hover | disabled | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|--------|-------|----------|---------|------|---------|---------------|------|
| `CFG-MENU-FILE` | 文件(&F) | button | 菜单栏左 1 | 文件菜单（占位） | 透明背景、主文本色 | 背景 `--color-border` | 不适用 | 无（无 JS 事件绑定） | Tab 聚焦，Enter 触发 | 点击无效果，不展开下拉菜单 | 见 `application-shell.md` 第 4 节 | 无 |
| `CFG-MENU-VIEW` | 视图(&V) | button | 菜单栏左 2 | 视图菜单（占位） | 同上 | 同上 | 不适用 | 无 | 同上 | 同上 | 见 `application-shell.md` 第 4 节 | 无 |
| `CFG-MENU-TOOLS` | 工具(&T) | button | 菜单栏左 3 | 工具菜单（禁用占位） | `--color-text-disabled` 文本 | 不变背景 | `data-disabled="true"`，tooltip `占位控件，未实现` | 无（disabled） | 不可聚焦 | **禁用并标注"占位"**，不响应点击 | 见 `application-shell.md` 第 4 节 | 无 |
| `CFG-MENU-HELP` | 帮助(&H) | button | 菜单栏左 4 | 帮助菜单（占位） | 透明背景、主文本色 | 背景 `--color-border` | 不适用 | 无 | Tab 聚焦，Enter 触发 | 点击无效果 | 见 `application-shell.md` 第 4 节 | 无 |

### 5.3 工具栏

高 32px（`--size-toolbar-height`），背景 `--color-toolbar`，底部 1px `--color-border` 边框，内边距 `0 8px`，间距 8px。从左到右：保存 -> 恢复默认 -> 导入配置 -> 导出配置 -> 弹性留白 -> 当前用户标签。

工具栏前 4 项为 `<span class="tb-label">` 元素（非 button），默认 `--color-text-secondary` 文本、`--font-size-caption`、内边距 4px。禁用态（`data-disabled="true"`）：`--color-text-disabled` 文本、`cursor: not-allowed`、hover 不变。

| ID | 标签 | 类型 | 位置 | 用途 | 默认态 | hover | disabled | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|--------|-------|----------|---------|------|---------|---------------|------|
| `CFG-TB-SAVE` | 保存 | span | 工具栏左 1 | 保存配置（禁用占位） | `--color-text-disabled`，`--font-size-caption` | 不变（disabled） | `data-disabled="true"`，tooltip `占位控件，未实现` | 无（span 非 button，且 disabled） | 不可聚焦 | **禁用并标注"占位"**，不执行保存 | 见 `application-shell.md` 第 5 节 | 不持久化，不写入数据库 |
| `CFG-TB-RESET` | 恢复默认 | span | 工具栏左 2 | 恢复默认配置（禁用占位） | 同上 | 同上 | 同上 | 无 | 不可聚焦 | **禁用并标注"占位"** | 见 `application-shell.md` 第 5 节 | 同上 |
| `CFG-TB-IMPORT` | 导入配置 | span | 工具栏左 3 | 导入配置文件（禁用占位） | 同上 | 同上 | 同上 | 无 | 不可聚焦 | **禁用并标注"占位"** | 见 `application-shell.md` 第 5 节 | 不读取外部文件 |
| `CFG-TB-EXPORT` | 导出配置 | span | 工具栏左 4 | 导出配置文件（禁用占位） | 同上 | 同上 | 同上 | 无 | 不可聚焦 | **禁用并标注"占位"** | 见 `application-shell.md` 第 5 节 | 不写入外部文件 |
| `CFG-TB-USER` | 当前用户: admin (演示) | span | 工具栏右，弹性留白后 | 显示当前用户（只读） | `--color-text-secondary`，`--font-size-caption`，内边距 4px | 不适用 | 不适用 | 无（只读） | 不可聚焦 | 固定显示 `当前用户: admin (演示)` | 见 `application-shell.md` 第 5 节 | 演示用户，不实现登录 |

### 5.4 状态栏

高 28px，背景 `--color-bg`，顶部 1px `--color-border` 边框，内边距 `0 16px`，间距 16px。从左到右：版本标签 + 分隔线 + 模拟模式标签 + 分隔线 + 告警滚动区（弹性）+ 紧急停止按钮。

状态栏分隔线为 1px 宽、18px 高的 `--color-border` 竖线。

| ID | 标签 | 类型 | 位置 | 用途 | 默认态 | hover | disabled | 点击结果 | 键盘 | 原型行为 | CURRENT 映射 | 安全 |
|----|------|------|------|------|--------|-------|----------|---------|------|---------|---------------|------|
| `CFG-SB-VERSION` | 版本: V1.0.0 (原型) | span | 状态栏左 1 | 显示系统版本（只读） | `--color-text-primary`，`--font-size-caption` | 不适用 | 不适用 | 无（只读） | 不可聚焦 | 固定显示 `版本: V1.0.0 (原型)` | 见 `application-shell.md` 第 6 节 | 模拟标注 |
| `CFG-SB-SIM` | [模拟模式] | span | 状态栏左 2，分隔线后 | 标注当前为模拟模式 | `--color-status-busy`，`--font-size-caption`，加粗 | 不适用 | 不适用 | 无（只读） | 不可聚焦 | 固定显示 `[模拟模式]` | 见 `application-shell.md` 第 6 节 | 模拟标注 |
| `CFG-SB-ALARM` | - | div 容器 | 状态栏中，弹性宽 | 展示模拟提示条目 | `min-width:400px`，`overflow:hidden`；条目 `--color-status-busy` 色、`--font-size-caption`、`--color-toolbar` 背景、内边距 `2px 8px`、圆角 `--radius-control` | 不适用 | 不适用 | 无（只读） | 不可聚焦 | 固定显示 1 条提示 `模拟提示: 配置为只读事实 + 临时展示，不持久化` | 见 `application-shell.md` 第 6 节 | 模拟提示 |
| `CFG-SB-EMERGENCY` | 紧急停止 | button | 状态栏右，80x20 | 紧急停止所有设备（禁用占位） | **始终禁用**：`--color-border` 背景、`--color-text-disabled` 文本、11px、加粗、圆角 3px；tooltip `危险占位：无设备停止效果，本原型禁用` | 不适用 | `disabled` + `data-disabled="true"` | 无（disabled，不响应点击） | 不可聚焦 | **模拟占位，无实际效果**；原型中禁用并标注"危险占位" | 见 `application-shell.md` 第 6 节 | 模拟占位，无设备停止效果 |

## 6. 状态规则汇总

每个区域必须定义五态。下表汇总各区域的五态实现：

| 区域 | 正常 | 加载 | 空 | 错误 | 禁用 |
|------|------|------|-----|------|------|
| 左面板配置导航 | 分类项可点击 | 不适用（静态） | 不适用 | 不适用 | 不适用 |
| 左面板系统信息 | 固定键值对展示 | 不适用 | 不适用 | 不适用 | 不适用 |
| 中心配置内容区 | 6 个区块展示 | 不适用（静态） | 不适用 | 不适用 | 部分行标注"禁用"或"锁定"（数据存储、网络通信、安全控制区块） |
| 中心开关 | 可点击切换 | 不适用 | 不适用 | 不适用 | cursor 为 not-allowed，但 JS 仍绑定 click（视觉与交互不一致） |
| 中心操作按钮 | 不适用 | 不适用 | 不适用 | 不适用 | 放弃修改与应用按钮始终禁用 |
| 右面板安全边界 | 5 项卡片展示 | 不适用（静态） | 不适用（固定 5 项） | 不适用 | 全部标注"已禁用"或"已锁定" |
| 右面板变更记录 | 4 条记录展示 | 不适用（静态） | 不适用（固定 4 条） | 不适用 | 第 4 条以 opacity 0.5 标注"历史记录未持久化" |
| 导航栏 | 导航项可点击 | 不适用 | 不适用 | 不适用 | 不适用 |
| 菜单栏 | 菜单项可点击 | 不适用 | 不适用 | 不适用 | 工具菜单禁用占位 |
| 工具栏 | 不适用 | 不适用 | 不适用 | 不适用 | 保存、恢复默认、导入、导出全部禁用占位 |
| 状态栏 | 只读展示 | 不适用 | 不适用 | 不适用 | 紧急停止禁用占位 |

原型中所有区域的五态均为"正常"态展示（静态内容），加载、空、错误态未实现。禁用态通过 `data-disabled="true"`、`disabled` 属性或 `opacity` 降低实现。状态颜色不作为唯一信息，必须同时给出文字。

## 7. 交互流程

### 7.1 配置分类切换流程

1. 用户在 `CFG-LP-NAV` 中点击某个配置分类项。
2. JS 移除其他分类项的 `selected` 类，当前项添加 `selected`。
3. **注意**：中心区内容不随分类切换而变化。6 个配置区块始终全部可见，中心头标题"系统信息"也不更新。分类导航仅起视觉高亮作用，TARGET 实现时应补齐内容联动或滚动定位。

### 7.2 开关切换流程

1. 用户点击 `CFG-CE-TOGGLE-COORD` 或 `CFG-CE-TOGGLE-ALARM`。
2. JS 切换该开关的 `.on` 类。
3. 视觉状态变化：开 -> 背景变 `--color-primary`、滑块移至右侧、不透明度恢复 1；关 -> 背景变 `--color-border`、滑块移至左侧、不透明度降为 0.5。
4. **注意**：切换仅影响视觉，不写入任何存储，不修改实际配置，会话结束即丢失。CSS `cursor: not-allowed` 与可点击行为存在不一致。

### 7.3 导航切换流程

1. 用户点击导航项 `CFG-NAV-01` 至 `CFG-NAV-06`。
2. JS 移除其他导航项的 `selected` 类，当前项添加 `selected`。
3. **注意**：原型为单页，不执行实际页面跳转。CURRENT Qt 客户端中导航切换通过 `QStackedWidget` 实现，见 `application-shell.md`。

## 8. 视口适配

| 视口 | 关键约束 |
|------|----------|
| 1280x720 | 导航栏 80px + 左面板 260px + 右面板 320px = 660px 固定，中心区 620px；配置行默认 3 列布局（200px + 220px 标签 + 间距 32px = 452px）需在 620px 内完整显示；配置区块内边距 `8px 14px` 两侧共 28px，实际内容宽约 592px；右面板安全边界卡片与变更记录卡片需在 320px 内不截断 |
| 1920x1080 | 默认尺寸；中心区 1260px；所有控件按 token 展示；配置行有充足水平空间 |
| 3840x2160 | 固定区域不变（导航 80px + 左面板 260px + 右面板 320px = 660px）；中心区 3180px 弹性放大；配置区块水平方向有大量留白；字号与控件尺寸保持固定 px |

原型 `.app` 容器固定为 `1920x1080`（`width:1920px;height:1080px`），不随视口缩放。TARGET 实现时应使布局响应三视口，固定面板宽度不变，中心区弹性。

## 9. 安全清单

本页面所有控件必须遵守以下安全约束：

| 控件 | 约束 |
|------|------|
| 工具栏保存按钮 `CFG-TB-SAVE` | 禁用占位，不执行保存，不写入数据库或文件 |
| 工具栏恢复默认按钮 `CFG-TB-RESET` | 禁用占位，不执行恢复 |
| 工具栏导入按钮 `CFG-TB-IMPORT` | 禁用占位，不读取外部文件 |
| 工具栏导出按钮 `CFG-TB-EXPORT` | 禁用占位，不写入外部文件 |
| 坐标显示开关 `CFG-CE-TOGGLE-COORD` | 占位，仅本会话视觉切换，不持久化 |
| 告警声音开关 `CFG-CE-TOGGLE-ALARM` | 占位，仅本会话视觉切换，不播放声音，不持久化 |
| 放弃修改按钮 `CFG-CE-BTN-CANCEL` | 禁用占位，不执行任何操作 |
| 应用按钮 `CFG-CE-BTN-APPLY` | 禁用占位，不执行任何操作，不持久化 |
| 紧急停止 `CFG-SB-EMERGENCY` | 模拟占位，无设备停止效果，原型中禁用 |
| 工具菜单 `CFG-MENU-TOOLS` | 禁用占位，不展开 |
| 导航项 `CFG-NAV-*` | 仅切换高亮，不执行实际页面跳转（单页原型） |
| 数据存储区块 | 禁用，不实现本地数据库、日志文件、导出报告、配置持久化 |
| 网络通信区块 | 禁用，不实现设备接入协议、远程指挥链路、数据上报 |
| 安全控制区块 | 锁定，紧急停止、排爆执行、设备控制指令、数据库写入、外部通信全部禁用 |
| 右面板安全边界 | 只读展示，5 项全部禁用或锁定 |
| 右面板变更记录 | 模拟数据，不持久化，会话结束即清空 |
| 系统信息 `CFG-LP-SYS` | 只读展示，模拟标注 |

所有模拟操作与结果必须带"模拟"或"演示"字样。涉及安全边界的内容若来自本地固定展示，必须在区块标题或控件旁标注"禁用""锁定"或"只读"。本页面不实现登录、角色切换、外部通信、持久化、UXR、MOS。不提供设备控制命令，不执行排爆动作，不写入数据库。

## 10. CURRENT 映射总结

| 页面元素 | CURRENT 源码位置 |
|---------|------------------|
| **页面级** | **本页在 CURRENT Qt 客户端中未实现为独立页面** |
| 配置分类导航 `CFG-LP-NAV` | 未实现 |
| 系统信息 `CFG-LP-SYS` | 未实现 |
| 配置内容区 `CFG-CE-CONTENT` | 未实现 |
| 坐标显示开关 `CFG-CE-TOGGLE-COORD` | 未实现 |
| 告警声音开关 `CFG-CE-TOGGLE-ALARM` | 未实现 |
| 放弃修改按钮 `CFG-CE-BTN-CANCEL` | 未实现 |
| 应用按钮 `CFG-CE-BTN-APPLY` | 未实现 |
| 右面板安全边界 | 未实现 |
| 右面板变更记录 | 未实现 |
| 导航栏 `CFG-NAV-*` | 见 `application-shell.md` 第 3 节 |
| 菜单栏 `CFG-MENU-*` | 见 `application-shell.md` 第 4 节 |
| 工具栏 `CFG-TB-*` | 见 `application-shell.md` 第 5 节 |
| 状态栏 `CFG-SB-*` | 见 `application-shell.md` 第 6 节 |

## 11. CFG-* ID 索引

下表列出本文档化的全部 `CFG-*` ID，供 HTML 原型与 Playwright 测试对齐：

| ID | 控件 | 区域 |
|----|------|------|
| `CFG-NAV-LOGO` | 导航栏 Logo | 应用壳 |
| `CFG-NAV-01` | 导航项：态势 | 应用壳 |
| `CFG-NAV-02` | 导航项：探测 | 应用壳 |
| `CFG-NAV-03` | 导航项：决策 | 应用壳 |
| `CFG-NAV-04` | 导航项：设备 | 应用壳 |
| `CFG-NAV-05` | 导航项：统计 | 应用壳 |
| `CFG-NAV-06` | 导航项：配置（选中） | 应用壳 |
| `CFG-MENU-FILE` | 菜单：文件 | 应用壳 |
| `CFG-MENU-VIEW` | 菜单：视图 | 应用壳 |
| `CFG-MENU-TOOLS` | 菜单：工具（禁用占位） | 应用壳 |
| `CFG-MENU-HELP` | 菜单：帮助 | 应用壳 |
| `CFG-TB-SAVE` | 工具栏：保存（禁用占位） | 应用壳 |
| `CFG-TB-RESET` | 工具栏：恢复默认（禁用占位） | 应用壳 |
| `CFG-TB-IMPORT` | 工具栏：导入配置（禁用占位） | 应用壳 |
| `CFG-TB-EXPORT` | 工具栏：导出配置（禁用占位） | 应用壳 |
| `CFG-TB-USER` | 工具栏：当前用户标签（只读） | 应用壳 |
| `CFG-LP-NAV` | 配置分类导航容器 | 左面板 |
| `CFG-LP-SYS` | 系统信息（只读） | 左面板 |
| `CFG-CE-CONTENT` | 配置内容区容器 | 中心区 |
| `CFG-CE-TOGGLE-COORD` | 坐标显示开关（占位） | 中心区 |
| `CFG-CE-TOGGLE-ALARM` | 告警声音开关（占位） | 中心区 |
| `CFG-CE-BTN-CANCEL` | 放弃修改按钮（禁用占位） | 中心区 |
| `CFG-CE-BTN-APPLY` | 应用按钮（禁用占位） | 中心区 |
| `CFG-SB-VERSION` | 版本标签 | 状态栏 |
| `CFG-SB-SIM` | 模拟模式标签 | 状态栏 |
| `CFG-SB-ALARM` | 告警滚动区 | 状态栏 |
| `CFG-SB-EMERGENCY` | 紧急停止按钮（禁用占位） | 状态栏 |

导航栏、菜单栏、工具栏、状态栏的 `CFG-*` ID 详细规格见 `application-shell.md` 第 3 节（导航栏）、第 4 节（菜单栏）、第 5 节（工具栏）、第 6 节（状态栏）。
