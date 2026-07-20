# 构建与运行

最后更新：2026-07-14

## 环境要求

- CMake 3.16 或更高版本。
- 支持 C++17 的 C++ 编译器。
- Qt 5 开发环境。

`CMakeLists.txt` 当前要求的 Qt 组件包括 `Core`、`Widgets`、`Gui`、`3DCore`、`3DRender`、`3DInput`、`3DExtras`、`Network`、`Sql` 和 `Test`。

ZeroMQ 和 PostgreSQL 是可选依赖，只有找到时才链接。Redis、MQTT 等内容目前只出现在目标设计资料中，不是当前 CMake 的直接依赖。

## 系统级构建命令

从仓库根目录执行：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target UXOMissionControl -j2
```

成功后可执行文件位于：

```text
build/src/App/UXOMissionControl
```

## Micromamba 开发环境

验证可用的 rootless Micromamba 环境名 `uxo-dev`，二进制位于 `/home/lin/.local/bin/micromamba`，根前缀在 `/home/lin/.local/share/mamba`，提供 Qt 5.15.15。

先激活环境再构建：

```bash
eval "$(/home/lin/.local/bin/micromamba shell hook --shell bash)"
micromamba activate uxo-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target UXOMissionControl -j2
```

也可不激活，直接用 `micromamba run` 单条命令执行：

```bash
micromamba run -n uxo-dev cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
micromamba run -n uxo-dev cmake --build build --target UXOMissionControl -j2
```

## 构建脚本

也可以运行：

```bash
bash scripts/build.sh
```

该脚本末尾会询问是否执行 `sudo make install`。自动化验证优先使用上面的 CMake 命令，避免交互式安装确认。

## 运行命令

激活 `uxo-dev` 后从仓库根目录运行：

```bash
./build/src/App/UXOMissionControl
```

当前入口会在初始化成功后调用 `Application::run()`，创建并显示 `MainWindow`。

在无图形环境（如 WSL 无 X server）下可使用 Qt offscreen 平台做启动烟测：

```bash
QT_QPA_PLATFORM=offscreen ./build/src/App/UXOMissionControl
```

若遇软件 OpenGL 渲染问题，可强制软件渲染并禁用 GPU 合成：

```bash
QT_QPA_PLATFORM=offscreen LIBGL_ALWAYS_SOFTWARE=1 QT_XCB_FORCE_SOFTWARE_OPENGL=1 ./build/src/App/UXOMissionControl
```

中文 UI 渲染需系统安装 `font-ttf-noto-cjk` 或等价中文字体，否则界面会出现方框或缺字。

## 测试

当前共有 4 个 CTest 用例：

```bash
ctest --test-dir build --output-on-failure
```

- `startup_visible`：验证 `Application::run()` 会显示标题为“排弹抢修指挥系统 V1.0”的 `MainWindow`。
- `demo_scenario_provider`：验证最小模拟演示场景包含模拟标识、1 个目标和至少 2 个模拟设备。
- `simulation_workflow`：验证 `SimulationWorkflow` 的目标选择与 `Detected -> Confirmed -> Disposing -> Disposed` 状态推进，以及有序操作日志的写入。
- `simulation_workflow_ui`：验证模拟工作流面板的交互行为。

最近一次在 `uxo-dev` 环境下的验证结果为 4/4 全部通过。

## 配置路径

`Application` 当前使用相对路径 `./config`。后续接入真实配置读取前，建议从仓库根目录运行程序。
