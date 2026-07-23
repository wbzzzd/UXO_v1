# 指挥席客户端开发与验证指南

版本：V0.2 草稿
状态：待评审

## 1. 文档作用

本文档只回答如何构建、运行、测试和判断任务完成。

- 产品需求与功能状态：`PRODUCT.md`
- 当前与目标架构：`ARCHITECTURE.md`
- 页面、交互和 UI 完成度：`UI.md`
- Agent 强制规则与安全边界：`AGENTS.md`

## 2. 当前工程环境

| 项 | 当前要求 |
|----|----------|
| 平台 | Linux；WSL2 + WSLg 已验证 |
| 构建系统 | CMake 3.16+ |
| 语言 | C++17 |
| Qt | Qt 5.15.15（`uxo-dev` micromamba 环境） |
| Qt 组件 | Core、Widgets、Gui、3DCore、3DRender、3DInput、3DExtras、Network、Sql、Test |
| 可选探测 | ZeroMQ、PostgreSQL；当前无目标链接或使用 |
| 中文字体 | Noto CJK 或等价字体 |

激活环境：

```bash
eval "$(/home/lin/.local/bin/micromamba shell hook --shell bash)"
micromamba activate uxo-dev
```

也可使用：

```bash
/home/lin/.local/bin/micromamba run -n uxo-dev <command>
```

## 3. 标准构建与运行

从仓库根目录执行：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target UXOMissionControl -j2
```

产物：

```text
build/src/App/UXOMissionControl
```

运行：

```bash
./build/src/App/UXOMissionControl
```

WSLg 演示可使用：

```bash
./scripts/run_demo.sh xcb
```

`scripts/build.sh` 会直接调用 `make`，末尾包含交互式安装询问，不作为 agent 自动验证入口。

## 4. 测试

执行全部测试：

```bash
ctest --test-dir build --output-on-failure
```

当前测试：

| CTest | 证明的能力 | 不证明的能力 |
|-------|------------|--------------|
| `startup_visible` | 主窗口可见、标题正确、模拟模式标识可见 | 配置、日志、数据库、通信初始化有效 |
| `demo_scenario_provider` | 固定场景含 1 目标、1 任务、至少 2 设备，引用关系正确 | 多场景、文件加载、数据变化 |
| `simulation_workflow` | 目标选择、四状态顺序、拒绝路径、日志顺序和重置 | 任务、设备、指派、持久化 |
| `simulation_workflow_ui` | 真实点击目标和三步按钮后，目标表、流程面板、决策状态标签与日志同步 | 3D 渲染结果、导航、视频、任务分配、设备联动、告警交互、状态栏紧急停止 |

测试通过只能证明表中列出的行为，不能用于声明整个客户端或某个未覆盖面板已完成。离屏 UI 测试中的 Qt3D 上下文警告不影响当前断言，因此 4/4 通过也不证明三维场景成功渲染。

## 5. 当前构建目标

```text
UXOMissionControl executable
  -> MainWindow static library
     -> Core static library
     -> Common static library
```

- `src/App`：应用入口和生命周期。
- `src/MainWindow`：Qt UI 组合与交互。
- `src/Core`：数据类型、模拟工作流和 3D 场景数据。
- `src/Common`：全局样式与当前未接入的 MockDataGenerator。

真实模块关系以 `ARCHITECTURE.md` 和各级 `CMakeLists.txt` 为准。

## 6. 开发规则

- 功能开发必须关联 `PRODUCT.md` 中已批准的需求 ID。
- 新功能必须先有状态为 `Approved` 的 `docs/features/<feature>.md`。
- Core 不依赖 MainWindow；UI 不自行成为业务状态权威。
- 代码注释使用中文。
- 模拟实现使用 `Mock`、`Simulation` 或 `Demo` 命名，并在 UI 中标明模拟语义。
- 不为没有调用方的未来模块创建空接口或目录。
- 不修改与当前任务无关的代码和用户改动。
- 真实设备控制和真实安全执行链不属于本仓库交付范围，普通任务授权不能解除该边界。未来如需客户端接入外部通信或存储，只能先批准最小适配边界；客户端不得绕过外部安全系统直接执行真实动作。

## 7. 质量门禁

当前可执行门禁：

- CMake 配置成功。
- `UXOMissionControl` 目标构建成功。
- 受影响的 CTest 全部通过。
- `git diff --check` 无空白错误。
- 暂存内容不包含 `.env`、明文密钥或凭据。
- UI 修改完成对应分辨率的实际运行和视觉检查。

当前尚未具备：

- CI 流水线。
- 覆盖率门禁。
- clang-tidy/cppcheck 门禁。
- CPack 或其他版本化发布包。
- 自动生成的需求追踪报告。

Agent 不得声称尚未具备的门禁已经存在。

## 8. 完成定义

任务完成必须同时满足：

1. 实现结果符合批准需求和功能设计，不以现有代码反向修改需求。
2. 相关构建和测试通过；新增行为有成功与失败场景测试。
3. 实际运行入口已验证，不以类存在、信号存在或测试编译通过代替功能验证。
4. 未违反模拟与真实接入安全边界。
5. 更新受影响的核心文档；工作记录只进入 `.omo/plans/` 或会话记录。
6. Git 差异只包含当前任务内容，且通过空白与密钥检查。
7. 最终报告明确说明已验证内容、未验证内容和遗留风险。

## 9. 文档读取路由

Agent 默认只加载 `AGENTS.md`，按任务读取相关文档：

| 任务 | 必读文档 |
|------|----------|
| 产品、需求、路线 | `PRODUCT.md` |
| 模块、状态、业务逻辑 | 相关需求 + `ARCHITECTURE.md` + 本文档 |
| 页面与交互 | 相关需求 + `UI.md` + 本文档 |
| 构建、测试、发布 | 本文档 |
| 已批准功能 | 对应 `features/<feature>.md` 及其明确引用章节 |

`docs/archive/`、`docs/research/`、旧 SRS/SDD/UI 和 `docs/dev/` 只在追溯来源或核验依据时按需读取。

## 10. 发布状态

当前仓库只有本地构建产物，没有版本化发布物：

- 无 CPack 或安装包。
- 无 CI 构建产物归档。
- 可执行文件没有自动注入 Git 版本。
- `scripts/build.sh` 的交互式 `sudo make install` 不等同于发布流程。

在发布要求得到批准前，不把本地 `build/` 产物描述为正式发布版本。
