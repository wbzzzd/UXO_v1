# 态势页面 UI 文档与 HTML 原型试点计划

状态：Approved for pilot execution
批准依据：用户确认先制作态势页面，并要求参考当前已实现页面。

## 目标

以 CURRENT Qt 主界面和 `docs/images/image1.png` 为参考，在 `docs/ui/` 下交付一个可评审的态势页面：读者能先看完整页面，再查询每个控件的样式、功能、状态、反馈和 CURRENT 映射，并能在浏览器中体验安全的本地模拟交互。

## 范围

### 必须包含

- `docs/ui/README.md`：试点入口、证据边界和打开方式。
- `docs/ui/design-system.md`：本试点使用的颜色、字体、间距、组件和状态规则。
- `docs/ui/application-shell.md`：主窗口壳、区域比例、导航和状态栏规则。
- `docs/ui/pages/situation.md`：整体设计、区域结构、逐控件清单、状态、交互、安全和 CURRENT 对照。
- `docs/ui/prototypes/situation/`：原生 HTML/CSS/JS、隔离 Node/Playwright、固定本地模拟数据和测试。
- `docs/ui/images/situation/overview-1920x1080.png`：一张权威整体设计图。
- 1280x720、1920x1080、3840x2160 三种视口的浏览器布局检查。

### 禁止包含

- 其余五个页面的详细设计或可操作原型。
- C++、Qt、CMake、业务状态和页面路由修改。
- 真实设备控制、真实处置、紧急停止、外部通信、数据库、持久化、认证、UXR、MOS 或网络请求。
- 把 HTML 原型测试结果描述为 Qt 已实现功能或 REQ 完成证据。
- 覆盖、回退或格式化用户现有脏文件。

## CURRENT 参考

- `src/MainWindow/MainWindow.cpp:163`：导航、左侧面板、视频/信息中心区和右侧面板装配。
- `src/MainWindow/NavigationWidget.cpp:14`：六项导航及当前仅高亮的行为。
- `src/MainWindow/RightPanelWidget.cpp:26`：三维态势、设备状态和决策建议区。
- `src/MainWindow/SituationView.cpp:77`：当前 Qt3D 态势视图入口。
- `docs/UI.md:27`：CURRENT 主界面结构和功能矩阵。
- `docs/images/image1.png`：整体视觉和区域层级参考；不作为所有控件已实现或安全的证据。

## 执行任务

- [ ] 1. 建立试点文档契约和 CURRENT/TARGET 证据矩阵
  - 创建 `docs/ui/README.md`、`design-system.md`、`application-shell.md`、`pages/situation.md`。
  - 每个交互控件必须有唯一 `SIT-*` ID，并记录名称、类型、位置、样式、默认/hover/focus/active/disabled 状态、点击结果、键盘行为、模拟边界、HTML-only 行为和 CURRENT Qt 映射。
  - 页面级显示 `TARGET / Draft / 设计评审原型`，动作级显示“模拟”或“演示”。
  - 验收：页面区域、控件 ID 和 CURRENT 引用完整；没有 Approved/Implemented、真实状态或真实控制表述。
  - QA：Markdown 链接、重复 ID、禁用词和控件覆盖由后续 Node 校验器机械检查；人工语义审查保存到 `.omo/evidence/ui-situation-pilot/task-1.md`。

- [ ] 2. 建立隔离工具链、设计 token 和应用壳
  - 创建 `docs/ui/prototypes/situation/package.json`、lockfile、Playwright 配置、`index.html`、`styles.css`、固定 fixture 和契约测试。
  - `.gitignore` 只为 `docs/ui/prototypes/**/*.html` 添加窄例外，并忽略该目录的依赖与测试报告。
  - 先写失败契约：必需区域/控件/标识、无网络/存储、键盘可达、危险控件缺失、三视口无溢出。
  - 验收：`npm ci --prefix docs/ui/prototypes/situation` 成功；预实现契约按预期失败；所有视觉值来自文档化 token。
  - QA：失败日志保存到 `.omo/evidence/ui-situation-pilot/task-2-red.txt`。

- [ ] 3. 实现态势页面整体 HTML 原型和安全交互
  - 实现完整应用壳、左侧目标/任务/设备上下文、中心视频与信息区、右侧态势/设备/决策区和底部状态栏。
  - 参考 `image1.png` 的层级和密度，但用 CURRENT 源码校正控件状态；占位或危险控制禁用/省略并在文档解释。
  - 仅实现本地可逆 UI 状态：搜索筛选、目标选择、Tab、视频布局/播放演示、态势图层/缩放/复位、详情展开、模拟确认和状态消息。
  - 验收：文档、HTML `data-testid` 和 Playwright 选择器一一对应；刷新恢复固定初始状态；无网络、存储和真实执行路径。
  - QA：`npm --prefix docs/ui/prototypes/situation test` 全部通过，结果保存到 `.omo/evidence/ui-situation-pilot/task-3-tests.txt`。

- [ ] 4. 执行浏览器交互与视觉 QA，生成整体设计图
  - 用 Playwright 实际打开页面，在 1280x720、1920x1080、3840x2160 检查文字裁切、重叠、滚动、焦点、禁用状态和交互反馈。
  - 生成且只提交 `docs/ui/images/situation/overview-1920x1080.png`；截图必须包含完整页面、TARGET/Draft 和本地模拟标识，无浏览器框、光标或临时提示。
  - 连续两次生成截图并比较 SHA-256；页面、服务器和浏览器进程全部清理。
  - 验收：三视口无非预期溢出/遮挡，关键鼠标与键盘路径通过，1920x1080 PNG 尺寸和哈希确定性通过。
  - QA：截图、测试报告与清理回执保存到 `.omo/evidence/ui-situation-pilot/task-4/`。

- [ ] 5. 独立审查试点的覆盖、安全与工作树完整性
  - 独立审查者读取全部新增文件、整体图、自动测试和初始/最终 `git status`。
  - 核对仅有态势页面、所有可交互元素有规格、CURRENT/TARGET 不混写、无危险能力、无用户 hunk 回退。
  - 执行 `npm --prefix docs/ui/prototypes/situation run validate` 和 `npm --prefix docs/ui/prototypes/situation test`，两者必须退出 0；validate 必须输出六项通过计数：页面=1、控件 ID 无重复、Markdown/HTML/test 三方覆盖一致、相对链接有效、危险活动控件=0、外部请求/持久化调用=0。
  - 执行 `git diff --name-only`，除 `.omo/` 计划状态外，只允许 `.gitignore`、`docs/ui/**`；执行 `git diff --check` 必须退出 0。
  - 执行 `rg -n '(fetch\(|XMLHttpRequest|WebSocket|localStorage|sessionStorage|紧急停止|真实处置|设备控制命令)' docs/ui`；代码调用必须零匹配，文档中若出现禁用说明必须由审查者逐条确认不构成可操作能力。
  - 由未参与实现的 reviewer 对上述原始输出、全部新增文件和截图逐项复核，按 DoneClaim/AdversarialVerify 格式返回；只有 `verdict: confirmed` 且 confidence >= 0.9 才通过。
  - 验收：全部命令满足预期且审查结果为 `confirmed`；任一缺失控件、错误映射、安全越界、测试失败、截图问题或脏文件覆盖均为阻塞。
  - 证据：`.omo/evidence/ui-situation-pilot/task-5-review.md`。

## 最终验证

- [ ] F1. `npm --prefix docs/ui/prototypes/situation test` 退出 0。
- [ ] F2. 三视口实际浏览器 QA 和 1920x1080 权威截图通过视觉审查。
- [ ] F3. `git diff --check` 退出 0，新增范围不含 C++/Qt/CMake 和其余五页。
- [ ] F4. 独立 reviewer 复跑 `npm --prefix docs/ui/prototypes/situation run validate`、`npm --prefix docs/ui/prototypes/situation test`、`git diff --check` 和任务 5 的路径/危险能力扫描；全部满足预期并返回 `AdversarialVerify.verdict: confirmed`、confidence >= 0.9，才能确认控件覆盖、安全边界、CURRENT/TARGET 证据和用户脏文件完整性。

## 完成条件

用户可从 `docs/ui/README.md` 打开态势页面规格、HTML 原型和整体设计图；只看整体图即可理解页面布局，查看控件表即可知道每个操作的样式、功能、状态和反馈。完成后暂停，不继续制作其余页面，等待用户评审。
