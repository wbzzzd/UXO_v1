# 工程开发基线

最后更新：2026-07-20
状态：初始草稿，待逐节评审。

本文档是 UXO_v1 的唯一工程手册，覆盖环境、构建、测试、架构纪律、安全边界、质量门禁与发布物缺口。`AGENTS.md` 仍是 agent 必读的短规则入口，本文档提供可执行的细节。命令与状态均以源码、CMake 与实际构建为准。

## 1. 目的与定位

- 短规则入口：`AGENTS.md`（agent 常驻上下文）。
- 工程细节手册：本文档。
- 当前事实与目标设计的差异：见 `docs/dev/current-state.md` 等 `docs/dev/` 文件。
- 目标设计资料（SRS、SDD、旧 UI 设计及功能草案）只作为需求输入，不作为已实现事实。

## 2. 开发环境

| 项 | 要求 | 说明 |
|---|---|---|
| 操作系统 | Linux（WSL2 + WSLg 已验证） | 本地与远程演示均覆盖 |
| 构建系统 | CMake ≥ 3.16 | `CMakeLists.txt` 顶部声明 |
| 编译器 | 支持 C++17 | `CMAKE_CXX_STANDARD 17 REQUIRED ON` |
| Qt 版本 | Qt 5.15.15（`uxo-dev` micromamba 环境） | 二进制 `/home/lin/.local/bin/micromamba`，根前缀 `/home/lin/.local/share/mamba` |
| Qt 组件 | Core, Widgets, Gui, 3DCore, 3DRender, 3DInput, 3DExtras, Network, Sql, Test | `find_package(Qt5 ...)` 强制依赖 |
| 可选探测 | ZeroMQ、PostgreSQL | CMake 会尝试查找，但当前没有目标链接或使用它们 |
| 中文字体 | `font-ttf-noto-cjk` 或等价 | 否则中文 UI 出现方框 |

激活环境：

```bash
eval "$(/home/lin/.local/bin/micromamba shell hook --shell bash)"
micromamba activate uxo-dev
```

未激活时可用 `micromamba run -n uxo-dev <command>` 单条执行。

未配置 clangd 或 LSP，仓库未提供 `compile_commands.json` 软链。这是环境事实，不作为门禁。

## 3. 标准构建、运行与测试命令

下列命令已在本仓库验证。除非本文档另有说明，agent 应直接使用这些命令。

### 构建

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target UXOMissionControl -j2
```

可执行文件位于 `build/src/App/UXOMissionControl`。

### 测试

```bash
ctest --test-dir build --output-on-failure
```

### 运行

激活 `uxo-dev` 后从仓库根目录执行：

```bash
./build/src/App/UXOMissionControl
```

无图形环境（如 WSL 无 X server）下的启动烟测：

```bash
QT_QPA_PLATFORM=offscreen ./build/src/App/UXOMissionControl
```

软渲染兜底（出现 OpenGL 异常时）：

```bash
QT_QPA_PLATFORM=offscreen LIBGL_ALWAYS_SOFTWARE=1 QT_XCB_FORCE_SOFTWARE_OPENGL=1 ./build/src/App/UXOMissionControl
```

### 脚本

- `scripts/build.sh`：调用 CMake 与 `make -j$(nproc)`，末尾会交互式询问 `sudo make install`，不适合自动化。自动化验证优先使用上面的 CMake 命令。
- `scripts/run_demo.sh`：在 WSL2 + WSLg 下启动可执行文件，支持 `xcb`（默认）/`webgl`/`vnc`/`offscreen` 四种平台模式。依赖 `uxo-dev` 环境路径。
- `scripts/install_qt_ubuntu.sh`：Ubuntu 系统 Qt 安装辅助脚本，与 micromamba 路径无关。

## 4. 仓库结构

CMake 当前纳入构建的目录：

```
src/App/         可执行目标 UXOMissionControl（main.cpp, Application.cpp）
src/MainWindow/ 静态库 MainWindow（15 个 UI 源文件，含 MainWindow/SituationView/LeftPanelWidget 等）
src/Core/        静态库 Core（AirportData, 3D/AirportSceneFactory, Simulation/DemoScenarioProvider, Simulation/SimulationWorkflow）
src/Common/      静态库 Common（MockDataGenerator, GlobalStyle）
include/App/     App 头文件
include/MainWindow/ MainWindow 头文件
include/Core/Data/Types.h 共享数据类型入口
tests/           4 个 CTest 用例
config/          system.json, devices.json（本地配置输入）
scripts/         build.sh, run_demo.sh, install_qt_ubuntu.sh
docs/dev/        当前工程事实与边界文档
```

`MainWindow` 依赖 `Common` 与 `Core`；`App` 依赖 `MainWindow`；`Core` 依赖 Qt 3D 与 Gui；`Common` 仅依赖 Core 与 Gui。测试目标直接链接 `MainWindow`/`Core`/`Common` 静态库。

未在 CMake 中纳入的目录（如 `Modules/`、`Drivers/`、`Storage/`、`docs/` 下的目标设计）属于规划，不视为当前事实。

## 5. 架构与编码规则

### 分层边界

- `App`：应用生命周期、配置路径、初始化流程。`Application::initialize()` 当前调用的配置、日志、数据库、通信、模块初始化均为占位返回成功，不接真实服务。
- `MainWindow`：UI 组合、用户交互、展示状态。不直接控制真实设备。流程状态由 `SimulationWorkflow` 承载。
- `Core`：稳定数据模型、服务接口、纯逻辑。`Core::Simulation::SimulationWorkflow` 在内存中维护目标列表、当前选中目标与有序操作日志，不持久化。
- `Common`：共享模拟数据生成与全局样式。
- `config/`：本地配置与后续模拟数据输入位置。

### 编码纪律

- 代码注释用中文。
- 模拟服务类名含 `Mock`/`Simulation`/`Demo` 前缀；真实接入接口用 `Interface`/`Adapter` 并注释标注尚未接入。
- UI 文案包含「模拟」「演示」等明确措辞，区分模拟、占位与真实接入。
- 新增字段或结构体不破坏现有 `TargetInfo` 等数据模型，全部带默认值。
- 不为尚未实现的模块创建空目录树。新增边界前必须有调用方或演示流程需要。
- 修改遵循最小 diff 原则，不重构无关代码，不调整他人已有改动。

## 6. 模拟与安全约束

详见 `docs/dev/simulation-policy.md`。要点：

- 仅允许本地 JSON 或内存对象模拟任务、目标、设备状态、处置方案。
- 禁止发送真实设备控制命令、执行真实排弹动作、写入真实数据库或外部系统、未经授权接入外部通信。
- 模拟状态流转 `Detected → Confirmed → Disposing → Disposed` 与操作日志仅存在于进程内存，重启重置。
- UI 触发的模拟操作文案必须包含「模拟」字样。决策建议面板与设备状态面板当前只读展示，不发起真实控制。
- 真实设备驱动、数据库写入、外部通信必须在用户明确授权后才能引入，且与模拟实现隔离。

## 7. 测试分层

| 层级 | 范围 | 现状 |
|---|---|---|
| L1 单元 | `QTest` + `ctest` | 4 个用例（见下） |
| L2 UI 契约 | 离屏平台 `QTest` | `simulation_workflow_ui` 已纳入 |
| L3 端到端 | 手动视觉 QA | 通过 `scripts/run_demo.sh xcb` 在 WSLg 下检查 |

当前 4 个 CTest 用例：

1. `startup_visible`：验证 `Application::run()` 显示标题为「排弹抢修指挥系统 V1.0」的 `MainWindow`（离屏平台）。
2. `demo_scenario_provider`：验证最小模拟演示场景含模拟标识、1 个目标、至少 2 个模拟设备。
3. `simulation_workflow`：验证目标选择与 `Detected → Confirmed → Disposing → Disposed` 状态推进，以及有序操作日志写入。
4. `simulation_workflow_ui`：验证模拟工作流面板的交互行为（离屏 + 软渲染）。

最近一次 `uxo-dev` 环境下验证结果为 4/4 全部通过。

## 8. 质量门禁

### 当前已落地门禁

- CMake 配置成功。
- `cmake --build build --target UXOMissionControl -j2` 成功产出可执行文件。
- `ctest --test-dir build --output-on-failure` 全部通过（当前 4/4）。
- 提交前 `git diff --check` 无空白错误。
- 提交前密钥检查：暂存与未暂存 diff 不得包含 provider key 形态或明文 API key 字符串。`.env` 不得提交，仅允许 `.env.example`。
- 提交前不得纳入无关的 `CMakeLists.txt` 用户改动。

### 规划中、尚未实现的门禁

- CI 流水线（GitHub Actions 或本地 hook）：未配置。
- 测试覆盖率门禁：未配置，无 `gcov`/`lcov` 集成。
- 静态分析门禁（clang-tidy、cppcheck 等）：未配置。
- 打包与发布物门禁：见第 11 节。

agent 不得声称上述门禁已存在。新增代码时以「当前已落地门禁」为最低要求。

## 9. 完成定义（Definition of Done）

一个任务视为完成需满足：

- 本地用第 3 节标准命令构建成功。
- 受影响的测试通过；若新增功能，应补 `ctest` 用例并保持 4/4 以上全绿。
- 新增代码有中文注释，模拟/占位实现有 `Mock`/`Interface` 前缀与「模拟」「尚未接入」标注。
- 未引入真实设备控制、数据库写入、外部通信代码（除非用户明确授权）。
- 未触碰无关用户改动，`git status --short` 仅显示本任务相关文件。
- 产品范围、架构、UI 或工程规则变化时，已更新对应核心文档；文档重整过渡期内仍需同步受影响的 `docs/dev/` 事实来源。
- 暂存 diff 通过密钥检查。
- 若任务含 UI 改动，已在 WSLg 或离屏平台下完成视觉烟测，并记录结果路径。
- 最终回复明确报告跳过或未验证的门禁。

## 10. 文档维护规则

- 产品目标、范围、需求或路线变化：更新 `docs/PRODUCT.md`。
- 模块边界、状态所有权或运行流程变化：更新 `docs/ARCHITECTURE.md`。
- 页面、交互、设计系统或状态规范变化：更新 `docs/UI.md`。
- 构建、测试、质量门禁、发布或完成定义变化：更新本文档。
- 文档重整过渡期内，`docs/dev/` 保留为来源材料；初稿评审完成后再逐项决定合并或归档。
- `AGENTS.md` 保持简短，长内容写入本文档或 `docs/dev/`。
- `README.md` 仅作导航，不承载长篇规则。
- 不在多处重复维护同一说明，每条规则有唯一权威文件。

## 11. 发布物缺口

当前不存在版本化发布物。事实：

- 无 `CPack` 配置，无 `make package` 目标。
- 无 `make install` 之外的安装器或分发包。
- `scripts/build.sh` 末尾的 `sudo make install` 是交互式提示，不适合自动化发布。
- 可执行产物路径固定为 `build/src/App/UXOMissionControl`，无版本号注入。
- 无 CI 触发的构建产物归档。

agent 不得声称已具备打包、分发或版本化发布能力。如需发布物，应先在本文档登记新门禁，再实现。

## 12. 已知问题

| 编号 | 问题 | 严重度 | 现状 |
|---|---|---|---|
| K1 | 1280×720 窗口最小尺寸下 `DecisionSuggestionPanel` 底部约 5px 溢出 | P2 不阻塞 | 默认 1920×1080 演示不触发，留待后续修复 |
| K2 | `scripts/build.sh` 末尾交互式 `sudo make install` 提示 | P2 不阻塞 | 自动化验证改用 CMake 命令 |
| K3 | `Application::initialize()` 配置、日志、数据库、通信、模块初始化均为占位 | 已知缺口 | MVP 阶段保留，真实接入需用户授权 |
| K4 | 无 CI、无打包、无覆盖率、无静态分析 | 已知缺口 | 见第 8、11 节 |
| K5 | 无 `compile_commands.json` 软链，无 clangd 配置 | 环境事实 | 不作为门禁，agent 可按需手动生成 |

## 13. 待评审问题

下列问题需用户或架构评审确认，agent 不得自行决定：

- Q1：是否将 `ctest` 全绿设为合并前的硬门禁，并在 `scripts/build.sh` 中默认执行？
- Q2：是否新增最小 CI 工作流（本地 hook 或 GitHub Actions）跑构建 + ctest？
- Q3：是否拆分 `scripts/build.sh`，将交互式安装提示移到独立 `scripts/install.sh`，使主构建脚本可自动化？
- Q4：是否补 `CPack` 配置与版本号注入，作为 P1 发布物门禁？
- Q5：是否生成并提交 `compile_commands.json` 软链以支持 clangd？
- Q6：是否新增覆盖率与静态分析门禁？若新增，阈值与工具选型待确认。

---

本文档基于源码、CMake 与 `docs/dev/` 现状编写。命令或边界变更时，先更新本文档与对应 `docs/dev/` 文件，再实施代码改动。
