# UXO 巡检员（Auto-Inspect）

自动 UI 巡检工具：在 `UXOMissionControl` 客户端上持续随机点击控件，检测自洽性问题（崩溃、卡死、状态不一致），输出 JSON 报告、截图和 HTML 时间线仪表盘。

**重要**：本工具只做 UI 自洽性巡检，**不修改任何业务代码**，全部新增文件位于 `tests/auto_inspect/` 下。所有交互均为本地模拟数据，不触碰真实设备、外部通信或数据库。

---

## 1. 工具组成

| 文件 | 作用 |
|---|---|
| `inspector_worker.cpp` | 核心 worker（C++/Qt）。启动客户端、随机执行动作、运行自洽性检查、写单轮 JSON 报告 |
| `run_loop.sh` | 循环调度器。持续启动 worker，每轮独立进程；崩溃/卡死时用相同 seed 自动复现；每轮自动刷新 STATUS.md、HTML 仪表盘和汇总 JSON |
| `aggregate_reports.py` | 汇总所有轮次报告，输出机器可读 `_summary.json` |
| `generate_html_report.py` | 生成深色主题 HTML 时间线仪表盘 `_timeline.html`（纯静态，无需后端） |
| `CMakeLists.txt` | worker 构建配置，需 `-DBUILD_AUTO_INSPECT=ON` 开启 |

---

## 2. 巡检员做什么

### 2.1 随机动作集（23 种）

worker 在客户端窗口上加权随机选择并执行动作：

| 动作 | 权重（启用时） | 说明 |
|---|---|---|
| `target_row` | 5 | 点击目标表行，触发选中 |
| `confirm` | 10 | 点击"模拟确认"按钮（Detected->Confirmed）|
| `start` | 10 | 点击"模拟处置"按钮（Confirmed->Disposing）|
| `complete` | 10 | 点击"完成"按钮（Disposing->Disposed）|
| `camera_top` | 2 | 点击"俯视"相机按钮 |
| `camera_side` | 2 | 点击"侧视"相机按钮 |
| `camera_3d` | 2 | 点击"3D视角"相机按钮 |
| `camera_reset` | 2 | 点击"复位"相机按钮 |
| `tab_switch` | 3 | 切换标签页（目标/任务/设备）|
| `refresh` | 2 | 点击"刷新"按钮 |
| `menu_action` | 3 | 触发菜单栏 action（文件/视图/工具/设备/帮助菜单）|
| `view_toggle` | 2 | 视图菜单 checkable action 切换后恢复（显示/隐藏面板）|
| `search_input` | 2 | 在搜索框输入随机文本后清空 |
| `nav_button` | 3 | 点击 6 个导航按钮（态势/探测/决策/设备/统计/配置）|
| `mission_row` | 3 | 点击任务表行（missionSelected 信号无消费者，验证不崩溃/不破坏选中）|
| `device_row` | 3 | 点击设备表行（deviceSelected 信号无消费者，验证不崩溃/不破坏选中）|
| `status_tab` | 2 | 点击状态子标签页按钮（待处置/处置中/已完成任务计数）|
| `key_tab` | 2 | 键盘 Tab（焦点切换，Qt 默认行为）|
| `key_enter` | 2 | 键盘 Enter（可能触发聚焦按钮或行激活）|
| `key_escape` | 2 | 键盘 Esc（关闭模态或清空选中）|
| `key_arrow` | 2 | 键盘方向键（上下左右随机选一个）|
| `menu_hover_timing` | 2 | 弹出下拉菜单后用 Esc 关闭，测量关闭延迟（R20 性能守护）|
| `search_robust` | 1 | 搜索框注入对抗性输入（超长文本/CRLF/正则字符/emoji/RTL/null 字节等），验证不崩溃不卡顿（R26 性能守护）|

- 禁用按钮权重降为 1（偶尔验证它们确实无反应）
- 不可用动作权重为 0（如无搜索框则 `search_input` 不参与选择）
- **菜单"退出"跳过**：避免意外关闭主窗口
- **模态对话框自动关闭**：文件/帮助菜单可能弹 QMessageBox，worker 预设 `QTimer::singleShot(300ms)` 自动关闭，避免卡住
- **覆盖率感知**：开启 `--coverage` 后，未探索的 (状态,动作) 对权重 ×3，过度探索（>3 次）的权重 ×½，引导跨状态路径探索

### 2.2 状态机契约

巡检员基于以下状态机迁移规则检查合法性：

```
Detected --confirm--> Confirmed --start--> Disposing --complete--> Disposed
```

- 非法迁移（如 Disposed 状态点 confirm）必须被拒绝
- 按钮启用逻辑：confirm↔Detected、start↔Confirmed、complete↔Disposing

### 2.3 自洽性检查规则（28 条）

每个动作执行前后，worker 捕获快照（状态、目标 ID、日志行数、标签页索引）并运行 28 条检查：

| 规则 | 检查内容 |
|---|---|
| R1 | 面板状态字符串合法（Detected/Confirmed/Disposing/Disposed/None）|
| R2 | 启用按钮的状态契约（如 confirm 启用时状态应为 Detected）|
| R3 | 无选中行时禁用按钮不得启用 |
| R4 | 状态机迁移合法性 |
| R5 | 重复点击同一行的幂等性 |
| R6 | 日志内容与动作匹配 |
| R7 | 日志行数在动作后应增加（除非是无日志动作）|
| R8 | 日志格式合规 |
| R9 | 选中行后目标 ID 存在 |
| R10 | 点击行后目标 ID 与该行一致 |
| R11 | 标签页切换后 currentIndex 与目标一致 |
| R12 | 刷新动作不应改变选中状态（目标 ID + 面板状态不变）|
| R13 | 相机操作不应破坏状态（面板状态 + 日志行数不变）|
| R14 | 任何动作后主窗口仍可见（未意外关闭，如菜单"退出"误触发）|
| R15 | 视图切换（面板隐藏/显示）恢复后状态保持（状态/目标/日志不变）|
| R16 | 搜索框输入不应改变选中状态（目标 ID + 面板状态不变）|
| R17 | 任务/设备表行点击不应改变目标选中状态（目标 ID + 面板状态不变）|
| R18 | 状态子标签页点击不应改变目标选中状态（目标 ID + 面板状态不变）|
| R19 | 键盘导航（Tab/Enter/Esc/方向键）不应丢失选中（panelTargetId 不应从有值变为空）|
| R20 | 菜单关闭延迟应 <=300ms（Esc 关闭下拉菜单的耗时，性能守护规则）|
| R21 | 状态机按钮点击响应应 <=200ms（confirm/start/complete，性能守护规则）|
| R22 | 标签页切换响应应 <=200ms（tab_switch，性能守护规则）|
| R23 | 搜索框输入过滤响应应 <=200ms（search_input 触发 textChanged 同步过滤，性能守护规则）|
| R24 | 模态对话框弹出时标题应非空（menu_action 触发，捕获 tr() 缺失或编码异常）|
| R25 | 对话框关闭后不应残留模态状态（activeModalWidget 应为 null，捕获关闭失败或挂起）|
| R26 | 超长/对抗性输入过滤响应应 <=500ms（search_robust 注入超长文本/CRLF/正则字符/emoji/RTL/null 字节，主线程不应被同步过滤长时间阻塞）|
| R27 | 标签页切换后当前页 currentWidget 应非空且可见（tab_switch 后页面损坏/空页/隐藏页 bug）|
| R28 | 非状态机动作不应改变 panelStatus（相机/刷新/标签/搜索/导航/键盘 Tab/Esc/箭头等若改变状态，说明业务侧有副作用 bug）|

---

## 3. 怎么用

### 3.1 编译

```bash
# 在仓库根目录
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_AUTO_INSPECT=ON
cmake --build build --target inspector_worker -j2
```

> 需要工具链路径在 PATH 中（`/home/lin/.local/share/mamba/envs/uxo-dev/bin`）

### 3.2 跑单轮（手动）

```bash
export QT_QPA_PLATFORM=offscreen
build/tests/auto_inspect/inspector_worker \
    --report /tmp/r1/report.json \
    --state /tmp/r1/state.txt \
    --screenshots /tmp/r1 \
    --coverage /tmp/r1/cov.json \
    --seed 42 \
    --max-actions 20
```

**命令行参数：**

| 参数 | 默认 | 说明 |
|---|---|---|
| `--report <path>` | 无 | 单轮 JSON 报告输出路径 |
| `--state <path>` | 无 | 每动作前写入当前状态，崩溃前留痕 |
| `--screenshots <dir>` | 无 | 截图输出目录 |
| `--coverage <path>` | 无 | 覆盖率 JSON 路径，跨轮持久化 |
| `--seed <int>` | 时间戳 | 随机种子，相同 seed 动作序列完全一致（可复现）|
| `--max-actions <int>` | 20 | 本轮动作数 |
| `--verbose` | 关 | 每步前向 stderr 输出状态，用于崩溃精确复现 |
| `--self-test` | 关 | 自检模式：注入已知不一致，验证检查规则能检出 |

### 3.3 跑持续循环（无人值守）

```bash
# 每 3 秒一轮，Ctrl+C 停止
bash tests/auto_inspect/run_loop.sh 3

# 也可自定义报告目录
REPORTS_DIR=/your/path bash tests/auto_inspect/run_loop.sh 3
```

`run_loop.sh` 行为：
1. 每轮生成随机 seed，启动 worker（60s 超时，offscreen 模式）
2. **正常完成**：记录问题数
3. **崩溃（非零退出码）**：用相同 seed + `--verbose` 自动复现，输出 `crash.json` + `replay.log`
4. **卡死（超时 124）**：用相同 seed + `--verbose` 自动复现，输出 `hang.json` + `replay.log`
5. 跨轮共享 `coverage.json`，引导探索未覆盖路径
6. **每轮结束后自动刷新** `_summary.json`、`_timeline.html` 和 `STATUS.md`

> 崩溃/卡死复现时若触发 `set -e` 导致脚本提前退出，可检查对应轮次目录下的 `crash.json`/`hang.json` 和 `replay.log`。

### 3.4 看状态和报告（无需手动跑命令）

循环跑起来后，**不用执行任何命令**，直接看这两个文件即可：

**看运行状态和告警**：
```bash
cat tests/auto_inspect/reports/STATUS.md
```
每轮自动更新，包含：运行状态、汇总统计（正常/崩溃/卡死/问题数）、覆盖率、告警区（崩溃/卡死/问题自动追加详情）、最近 10 轮记录。

**看可视化仪表盘**：
```bash
xdg-open tests/auto_inspect/reports/_timeline.html
```
纯静态 HTML，不需要后端服务。每轮自动重新生成，浏览器刷新即最新。包含统计卡片、动作覆盖率柱状图、每轮动作时间线、问题截图。

> 也可以单独手动生成汇总和 HTML（不在循环中时）：
> ```bash
> python3 tests/auto_inspect/aggregate_reports.py tests/auto_inspect/reports/
> python3 tests/auto_inspect/generate_html_report.py tests/auto_inspect/reports/
> ```

---

## 4. 产出物说明

所有产出在 `tests/auto_inspect/reports/` 下（已 `.gitignore`，不提交）：

```
reports/
├── STATUS.md               # 运行状态+告警（每轮自动更新，cat 即可看）
├── _timeline.html          # HTML 时间线仪表盘（浏览器打开，每轮自动刷新）
├── _summary.json           # 机器可读汇总
├── coverage.json           # 覆盖率：50 个 (状态,动作) 对的执行计数
├── 20260730_162201/        # 每轮一个目录
│   ├── report.json         # 单轮报告：动作日志 + 问题列表 + seed
│   ├── last_action.txt     # 最后执行的动作（崩溃前留痕）
│   ├── stderr.log          # worker 的 stderr 输出
│   ├── issue_0.png         # 问题截图（如有）
│   └── (crash.json / hang.json + replay.log  # 仅崩溃/卡死时)
├── 20260730_162205/
└── ...
```

### 4.1 单轮 report.json 结构

```json
{
  "timestamp": "2026-07-30T16:22:01",
  "seed": 3191,
  "status": "completed",
  "actions_executed": 20,
  "issues_found": 0,
  "action_log": [
    {"step": 0, "action": "初始状态", "kind": "init", "executed": true},
    {"step": 1, "action": "点击目标表第0行", "kind": "target_row", "executed": true},
    {"step": 2, "action": "点击 俯视 按钮", "kind": "camera_top", "executed": true}
  ],
  "issues": []
}
```

### 4.2 问题（issue）结构

```json
{
  "rule": "R4",
  "severity": "error",
  "action": "点击 模拟确认 按钮",
  "message": "状态从 Disposed 非法迁移到 Confirmed",
  "screenshot": "issue_0.png"
}
```

### 4.3 coverage.json 结构

键为 `状态:动作`，值为该组合已执行次数：

```json
{
  "Detected:confirm": 8,
  "Confirmed:start": 8,
  "Disposing:complete": 7,
  "None:target_row": 8
}
```

状态空间：5 个状态 × 23 个动作 = 115 个对。

### 4.4 HTML 仪表盘内容

- 顶部统计卡片：总轮数、崩溃数、卡死数、问题数
- 动作覆盖率柱状图：21 种动作各自的执行次数
- 唯一问题表：去重后的所有问题
- 每轮卡片：动作时间线（彩色圆点按动作类型着色）+ 问题截图 + 崩溃/卡死标记

---

## 5. 最近一次巡检结果

**时间**：2026-08-03（E 系列业务流自洽性扩展验证，3 轮 × 100 动作）

| 指标 | 数值 |
|---|---|
| 总轮数 | 3 |
| 正常完成 | 3 |
| 崩溃 | 0 |
| 卡死 | 0 |
| 总问题数 | 0 |
| 动作执行总数 | 300 |
| 覆盖率 | 持续累积中 |

**新增业务流自洽性检查**：E 系列新增 R28 规则，挂载到所有动作上（无需新增动作，denominator 保持 115）。R4 仅检查状态迁移合法性（允许前进），但无法捕获"非状态机动作意外触发状态改变"的副作用 bug。R28 补齐这一缺口：当已选目标（状态非 None）时，只有 Confirm/Start/Complete/TargetRow/KeyEnter 可合法改变 panelStatus，其他动作（相机/刷新/标签/搜索/导航/键盘 Tab/Esc/箭头/菜单/视图/任务表/设备表/状态子标签/菜单悬停/对抗性搜索）若改变了状态即报警。

**验证结果**：3 轮 × 100 动作全部通过（0 issues），说明当前业务侧非状态机动作无状态副作用 bug。R28 作为业务流守护规则，未来若刷新触发状态重置、搜索触发误迁移、标签切换误清空状态等会自动报警。

**历史扩展**：A 系列（R20-R23 性能时序）、D 系列（R24/R25 对话框守护）、F 系列（R26 输入鲁棒性）、C 系列（R27 标签页视觉）均已在之前批次验证通过，本次 E 系列验证未触发回归。

**关于用户原问题（菜单移开后过一阵才消失）**：offscreen 模式下"鼠标移开关闭菜单"无法复现（offscreen 不处理鼠标移出事件），R20 改用 Esc 关闭测量。当前测出 0ms 延迟，说明菜单响应关闭指令无延迟。R20 作为性能守护规则，未来若菜单关闭变慢（动画/定时器/主线程卡顿）会自动报警。

---

## 6. 设计原则与约束

- **不改业务代码**：全部新增文件在 `tests/auto_inspect/`，业务源码零改动
- **可复现**：每轮记录 seed，崩溃后用相同 seed + `--verbose` 精确复现最小动作序列
- **跨轮持久**：覆盖率文件跨轮累积，引导探索未覆盖的状态路径
- **offscreen 运行**：`QT_QPA_PLATFORM=offscreen` 软件渲染，无 GPU/显示器依赖，适合服务器后台
- **安全边界**：仅本地模拟数据交互，不触碰真实设备、外部通信或数据库
- **构建开关**：`-DBUILD_AUTO_INSPECT=ON`（默认 OFF），不影响常规构建

---

## 7. 已知局限

- 相机按钮和刷新按钮无 `objectName`，靠文本查找（`findButtonByText`）；若 UI 文案改动需同步更新
- 状态机覆盖依赖业务侧的状态字符串，若状态枚举扩展需同步更新规则
- 标签页"任务/设备"页内容多为占位，巡检员仅验证切换行为本身，不深入验证页面内容
- 菜单/视图/导航/搜索框控件均无 `objectName`，靠 `menuBar()->actions()`、`isCheckable()`、`placeholderText`、`navIndex` 属性等结构性特征发现；若 UI 重构改变这些特征需同步更新发现逻辑
- 任务表/设备表无 `objectName`，靠 QTabWidget 页序定位（index 1=任务表、index 2=设备表）；若标签页顺序调整需同步更新 `findMissionTable`/`findDeviceTable`
- 状态子标签页按钮无 `objectName`，靠文本"待处置任务"/"处置中任务"/"已完成任务"查找；若 UI 文案改动需同步更新 `findStatusTabButtons`
- 模态对话框自动关闭依赖 `QTimer::singleShot(300ms)`；若业务侧引入更慢的异步弹框（如网络请求后再弹），可能需要增大延迟
- 菜单"退出"按文本跳过；若未来新增其他会关闭窗口的 action（如"关闭"），需在 `collectMenuActions` 中补充跳过规则
- 键盘导航动作对当前焦点控件发送按键；若焦点不在预期控件（如模态对话框弹出期间），按键可能被对话框消费。源码当前无自定义键盘处理，走 Qt 默认行为；若业务侧新增快捷键或 keyPressEvent，需评估是否影响 R19
- R20 菜单关闭延迟用 Esc 测量（offscreen 下"鼠标移开关闭菜单"不可复现，因 offscreen 不处理鼠标移出事件）；当前测出 0ms，若未来菜单关闭变慢（动画/定时器/主线程卡顿）R20 会报警
- R21/R22 时序测量包含 `mouseClick + processEvents` 整体耗时（含 Qt 事件循环派发与槽函数执行），不含按钮查找与快照捕获；当前均在 200ms 内，若业务侧引入同步重计算或阻塞式槽函数会触发报警
- R23 搜索过滤时序测量包含 `setText + processEvents` 耗时（触发 `textChanged -> onSearchTextChanged` 同步遍历过滤），不含恢复阶段 `clear()`；当前目标表行数少故无延迟，若数据量增大或过滤逻辑变复杂（如正则/模糊匹配）会触发报警
- R24/R25 仅对 `menu_action` 触发的模态对话框生效；若业务侧新增经按钮（非菜单）触发的对话框，需在对应动作执行器中同样接入 `scheduleModalDialogAutoClose` 双定时器机制
- R24 标题抓取依赖 50ms 定时器（exec 弹出后标题已就绪）；若业务侧引入异步加载标题（如网络请求后 `setWindowTitle`），可能需要增大抓取延迟
- R26 对抗性输入样本固定 10 类（超长文本/CRLF/正则元字符/HTML/emoji/RTL/null/制表符/混合 CJK）；若业务侧新增其他危险输入类型（如 SQL 注入字符、Unicode BOM、零宽字符），需扩充 `robustSamples` 数组。阈值 500ms 比 R23 宽，因超长文本会放大 O(rows×cols) 遍历成本，但若数据量显著增大（如目标表行数 >1000）可能需要重新校准
- R27 仅检查 `currentWidget` 非空且 `isVisible()`，不验证页面内容完整性（如页面内子控件是否全部渲染、数据是否加载）；offscreen 模式下 `isVisible()` 返回值可能与有屏幕环境不一致，当前源码无 `setVisible(false)` 误调用故通过，若业务侧引入条件隐藏逻辑需评估是否影响 R27
- R28 白名单包含 KeyEnter（可能触发聚焦按钮导致状态迁移，由 R4 覆盖合法性）；若业务侧新增其他可合法改变状态的非状态机动作（如快捷键 Ctrl+Enter 触发确认），需扩充白名单。R28 仅在已选目标（状态非 None）时生效，未选目标时状态本就是 None 不适用
- 巡检员仅检测崩溃/卡死/自洽性违规，不验证业务功能正确性（如"打开预案"是否真的加载了数据）
