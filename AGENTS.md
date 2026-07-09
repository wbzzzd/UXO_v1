# 排弹抢修指挥系统 Agent 规则

本文件是 UXO_v1 的项目级 agent 规则。opencode 原生 agent、oh-my-openagent agent 和后续子代理都必须优先遵守。

## 项目事实

- 项目是 Qt 5 / CMake / C++17 桌面客户端，目标程序名为 `UXOMissionControl`。
- 当前真实纳入构建的代码主要在 `src/App/`、`src/MainWindow/`、`include/App/`、`include/MainWindow/` 和 `include/Core/Data/Types.h`。
- `src/App/main.cpp` 当前创建 `App::Application` 并进入 `app.exec()`，但尚未实例化 `MainWindow`。
- SRS、SDD、`PROJECT_STRUCTURE.md` 和 UI 设计文档是目标/设计资料，不代表所有模块已经实现。
- 当前 MVP 应优先做可见、可演示、模拟数据闭环的指挥席流程。

## 安全边界

- 真实设备控制、排弹动作、外部通信命令、数据库写入和安全关键动作默认禁止实现或执行。
- 未经用户明确授权，只能使用本地模拟数据、只读分析或接口占位。
- 不要把模拟状态写成真实设备状态；文档和 UI 都必须明确区分模拟、占位和真实接入。
- `opencode.json` 只能使用环境变量引用 provider key。不得复制、打印、提交或复述任何明文密钥。
- `.env`、密钥、凭据文件不得提交；只允许提交 `.env.example` 这类变量名示例文件。

## 开发流程

- 先读 `docs/OMO_ADAPTATION_PLAN.md`、`docs/dev/` 和当前源码，再制定计划。
- 多文件或架构性修改必须先计划，得到用户确认后再实现。
- 修改前检查 `git status --short`，不要还原、覆盖或提交用户已有改动。
- 文档事实以源码和实际构建为准；目标设计文档只能作为需求输入。
- 代码变更必须写中文注释；按任务粒度及时提交 git，commit message 须说明变更内容与目的。
- 每次工作会话保存记录并附总结；任务完成后在对应计划文档更新完成状态。

## 构建验证

优先使用以下命令验证当前客户端构建：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target UXOMissionControl -j2
```

`scripts/build.sh` 也可用，但末尾有交互式安装确认，不适合作为自动化首选。

## 文档纪律

- `README.md` 只做入口导航。
- `docs/dev/` 记录当前工程事实、MVP 范围、架构边界和模拟策略。
- `.omo/rules/` 放项目级 OMO 规则；`.omo/plans/` 放可评审计划；`.omo/run-continuation/` 等运行态数据不得提交。
- 新增或修改入口、构建方式、配置、模块边界时，同步更新对应文档。

## 禁止事项

- 不要把 SRS/SDD 中尚未实现的模块写成当前代码事实。
- 不要为了让检查通过而删除失败测试或弱化验证。
- 不要使用破坏性 git 命令，除非用户明确要求。
- 不要主动提交或推送；提交必须由用户明确要求。
