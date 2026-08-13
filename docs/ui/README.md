# UI 设计文档入口

状态：`TARGET / Draft / 设计评审原型 / 本地模拟`
关联基线：[docs/UI.md](../UI.md) · [docs/PRODUCT.md](../PRODUCT.md) · [docs/ARCHITECTURE.md](../ARCHITECTURE.md) · [docs/DEVELOPMENT.md](../DEVELOPMENT.md) · [AGENTS.md](../../AGENTS.md)

> 本目录是排弹抢修指挥系统六页 UI 的设计文档与 HTML 原型集合。读者先看本文了解整体结构与阅读顺序，再按需进入子文档查阅设计系统、应用壳或某一页的详细规格。所有内容均为设计评审原型，对应行为用本地模拟数据演示，不连接真实设备、不执行真实排爆动作。

## 1. 范围

本目录覆盖系统的六个页面：

| 页面（中文） | English | 导航 ID 前缀 | 设计状态 | 原型状态 | 截图状态 |
|--------------|---------|--------------|----------|----------|----------|
| 态势 | Situation | `SIT-` | 详细清单已交付 | 已交付 | 已交付 |
| 探测 | Detection | `DET-` | 详细清单已交付 | 已交付 | 已交付 |
| 决策 | Decision | `DEC-` | 详细清单已交付 | 已交付 | 已交付 |
| 设备 | Devices | `DEV-` | 详细清单已交付 | 已交付 | 已交付 |
| 统计 | Statistics | `STA-` | 详细清单已交付 | 已交付 | 已交付 |
| 配置 | Configuration | `CFG-` | 详细清单已交付 | 已交付 | 已交付 |

中英文对照表是六页的统一命名约定：目录名、原型路径、截图路径均使用英文（如 `prototypes/situation/index.html`），文档标题与正文使用中文（如"态势页"），两套命名一一对应。

- **态势页**有完整的逐控件契约（`pages/situation.md`），是各页详细清单的样板。
- **其余五页**均已交付详细控件清单（见 `pages/index.md` 链接）。

## 2. 证据边界（CURRENT 与 TARGET）

本目录严格区分两类证据，写文档时不得混写：

- **CURRENT**：当前 Qt 源码、CMake 和测试已经形成的行为。权威来源是 `src/MainWindow/`、`src/Common/GlobalStyle.*`、`include/Core/Data/Types.h`。CURRENT 只描述现状，不反向修改需求。
- **TARGET**：本目录文档定义的设计意图。来自 `PRODUCT.md` 已确认的产品边界和 `UI.md` 第 6 节 NEXT UI 需求。TARGET 不等于已实现，描述 TARGET 时不暗示 Qt 中已存在。

早期整体视觉与区域层级的参考图（历史 `image1.png`，已在文档整理中删除）曾用于设计参照，但不是"所有控件已实现或安全"的证据。早期截图中出现的"打开设备控制台""创建排爆任务""紧急停止"等控件属于视觉参考，本目录按 CURRENT 源码校正其真实行为，并对占位或危险控件做禁用或省略处理。

控件清单的每一行都带 `CURRENT 映射` 字段，指向具体源码位置；当 TARGET 行为与 CURRENT 不一致时，差异在 `原型行为` 与 `CURRENT 映射` 两列分别说明。

## 3. 文件索引

| 文件 | 作用 |
|------|------|
| `README.md`（本文） | 总览入口、范围、证据边界、安全边界、ID 与状态约定、视口规则 |
| `design-system.md` | 六页共用的颜色、字体、间距、动画 token 与组件原语、状态规则、三视口规则 |
| `application-shell.md` | 六页共用的主窗口壳、区域比例、导航、菜单、工具栏、状态栏的规格与 CURRENT 映射 |
| `pages/index.md` | 六页一览表，每页一段：定位、关键区域、控件 ID 前缀、截图路径、原型路径 |
| `pages/situation.md` | 态势页面完整设计契约、逐控件 `SIT-*` 清单、状态、交互、安全与 CURRENT 对照（详细样板） |
| `prototypes/<page>/index.html` | 各页 HTML 原型（内联 CSS+JS，单文件，本地模拟 fixture）；六页目录平级，每目录仅含 `index.html` |
| `images/<page>/overview-1920x1080.png` | 各页 1920×1080 权威整体设计图 |
| `prototypes/screenshot.js` | Playwright 截图脚本（参数化，`node screenshot.js [page]`，无参数跑全部六页） |
| `prototypes/package.json` | 原型工具链 npm 配置（依赖 playwright，本地安装不提交） |
| `mos-ui-design-brief.md` | MOS 页 UI 设计简报，从 `docs/dev/` 迁入的 UI 设计输入 |
| `product-design-mos.md` | MOS 产品设计文档，从 `docs/dev/` 迁入的 UI 设计输入 |

## 4. 阅读顺序

### 4.1 首次了解整体设计

1. 先读 `design-system.md`，建立颜色、字体、间距与状态词汇。
2. 再读 `application-shell.md`，理解窗口壳与五大区域如何拼装。
3. 最后读 `pages/index.md`，按页查阅每页的定位、关键区域与截图路径。

### 4.2 查阅某一页的详细规格

- **六页**：直接读对应 `pages/<page>.md`，按区域查阅每个控件的完整规格。各页详细清单链接见 `pages/index.md` 总览表。

### 4.3 实现 HTML 时

每个文档化的 `<PREFIX>-*` ID 必须在 HTML 中作为 `data-testid` 出现，且与 Playwright 选择器一一对应；视觉值只能取自 `design-system.md` 的 token 表，不得硬编码新值。态势页已按此规则实现，作为各页 HTML 原型的参照样板。

### 4.4 在线查看原型演示

HTML 原型已通过 GitHub Pages 托管，无需 clone 仓库，点击下方链接即可在浏览器中交互演示：

| 页面 | 在线演示 |
|------|---------|
| 态势 | https://wbzzzd.github.io/UXO_v1/ui/prototypes/situation/index.html |
| 探测 | https://wbzzzd.github.io/UXO_v1/ui/prototypes/detection/index.html |
| 决策 | https://wbzzzd.github.io/UXO_v1/ui/prototypes/decision/index.html |
| 设备 | https://wbzzzd.github.io/UXO_v1/ui/prototypes/devices/index.html |
| 统计 | https://wbzzzd.github.io/UXO_v1/ui/prototypes/statistics/index.html |
| 配置 | https://wbzzzd.github.io/UXO_v1/ui/prototypes/configuration/index.html |

静态截图见 `images/<page>/overview-1920x1080.png`，逐控件规格见 `pages/<page>.md`。本地开发时也可直接用浏览器打开 `prototypes/<page>/index.html`。

## 5. 安全边界

本目录严格遵守 `AGENTS.md` 与 `PRODUCT.md` 第 10 节的强制安全边界：

- 不发送真实设备控制命令，不执行真实排爆或抢修动作。
- 不写入真实数据库或外部系统，不接入未授权通信链路。
- 不实现登录、角色切换、外部通信、持久化、UXR、MOS。
- UI 中的模拟操作和结果必须明确标注"模拟"或"演示"。
- 不把模拟状态描述为真实设备状态。

早期参考图中的"紧急停止"在 CURRENT 源码里是一个显示确认框并发出信号的占位按钮，但 `MainWindow` 没有消费者，实际不会停止任何设备，且文案"所有设备将立即停止"会误导用户。本目录将其作为危险占位控件处理：原型中禁用并标注"模拟占位，无实际效果"，详见 `pages/situation.md` 的状态栏章节与 `application-shell.md` 的遗漏与禁用清单。

早期参考图中的"打开设备控制台"在 CURRENT 中是连接空 lambda 的占位菜单项，本目录省略其可操作入口，仅保留只读展示。

## 6. 控件 ID 约定

- 每个文档化的交互控件拥有唯一且稳定的 `<PREFIX>-*` ID，区域前缀加序号，例如 `SIT-NAV-01`、`DET-LP-SEARCH`、`CFG-SB-EMERGENCY`。
- 各页 ID 前缀固定：态势 `SIT-`、探测 `DET-`、决策 `DEC-`、设备 `DEV-`、统计 `STA-`、配置 `CFG-`。
- ID 一经分配不复用；原型实现、HTML `data-testid` 与 Playwright 选择器必须使用同一 ID。
- 仅展示、不可交互的元素（纯标签、只读数值）不分配 ID，但在控件清单中以"只读"标注。
- 控件清单每条记录包含以下字段：标签、类型、位置、用途、样式与状态（默认/hover/focus/active/disabled）、点击结果、键盘行为、原型行为、CURRENT 映射、安全。

## 7. 状态词汇与视口规则

每个可交互区域与控件必须定义以下五态，缺一不可：

| 状态 | 含义 |
|------|------|
| 正常 (normal) | 有数据且可交互的默认态 |
| 加载 (loading) | 数据尚未就绪时的占位态 |
| 空 (empty) | 无数据时的引导态 |
| 错误 (error) | 数据获取或操作失败时的提示态 |
| 禁用 (disabled) | 因前置条件未满足而不可交互的态 |

状态颜色不得作为唯一信息，必须同时给出文字。模拟操作与结果必须带"模拟"或"演示"字样。

三视口检查规则（细则见 `design-system.md` 第 7 节与 `application-shell.md` 第 8 节）：

| 视口 | 用途 | 关键约束 |
|------|------|----------|
| 1280×720 | 最小可用 | 区域不溢出、决策面板末两行不被截断、文字不裁切 |
| 1920×1080 | 默认与权威截图 | 默认设计尺寸，整体图按此视口生成 |
| 3840×2160 | 4K | 控件密度与间距按 token 等比放大，不出现大片留白或控件过小 |

CURRENT 在 1280×720 下决策面板存在约 5px 底部溢出（见 `UI.md` 第 3 节已知问题）。TARGET 将该问题纳入设计修正目标。

## 8. 后续任务

本目录当前交付到"六页 HTML 原型+截图+六页详细控件清单"为止。后续未完成任务（不属本轮交付）：

- 为六页 HTML 原型补齐 Playwright 契约测试。
- 完成 1280×720 与 3840×2160 两视口的截图与验证（当前只交付 1920×1080）。

## 9. 维护规则

> 设计变更与文档必须同步更新，禁止只改原型不改文档或只改文档不改原型。

### 9.1 同步更新清单

当发生以下任一变更时，必须同步更新对应文档：

| 变更内容 | 必须同步更新的文档 |
|---------|-------------------|
| 新增/删除/重命名 HTML 原型中的 `data-testid` | 对应 `pages/<page>.md` 的控件规格与 ID 索引、`pages/index.md` 一览表 |
| 修改 HTML 原型的交互行为（点击结果、状态切换、键盘行为） | 对应 `pages/<page>.md` 的控件规格"原型行为"字段 |
| 新增/删除/修改 HTML 原型的区域布局 | 对应 `pages/<page>.md` 的区域章节、`pages/index.md` 关键区域表 |
| 修改颜色/字体/间距/动画 token | `design-system.md` 的 token 表 |
| 修改导航栏/菜单栏/工具栏/状态栏 | `application-shell.md` 对应章节 |
| 新增页面 | `README.md` 范围表、`pages/index.md` 总览表 |
| 修改页面截图或重新截图 | 确认截图与原型一致，无需改文档 |

### 9.2 同步更新流程

1. 先改 HTML 原型（`prototypes/<page>/index.html`）。
2. 再改对应 `pages/<page>.md` 的控件规格、ID 索引、状态规则。
3. 如涉及共用元素（导航、菜单、工具栏、状态栏、token），再改 `application-shell.md` 或 `design-system.md`。
4. 如涉及页面增减或关键区域变化，再改 `pages/index.md` 和 `README.md`。
5. 运行 `node prototypes/screenshot.js <page>` 重新生成截图。
6. 检查所有 `data-testid` 与文档 ID 索引一一对应。

### 9.3 禁止事项

- 禁止只改原型不改文档：原型中新增了 `data-testid` 但文档 ID 索引未更新。
- 禁止只改文档不改原型：文档中描述了控件但原型中不存在对应 `data-testid`。
- 禁止文档与原型行为不一致：文档写"点击后弹出确认框"但原型实际行为不同。
- 禁止跳过截图更新：原型布局变化后截图未重新生成。

## 10. 完成条件

本轮文档整改完成的判定：读者只看 `README.md` 能了解目录整体结构与阅读顺序；查阅 `pages/index.md` 能知道六页各是什么、关键区域、控件 ID 前缀与截图路径；查阅 `design-system.md` 与 `application-shell.md` 能知道六页共用的视觉与壳契约；查阅 `pages/<page>.md` 能知道对应页每个控件的完整规格。没有 Approved/Implemented、真实状态或真实控制表述。
