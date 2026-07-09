# 构建与运行

最后更新：2026-07-07

## 环境要求

- CMake 3.16 或更高版本。
- 支持 C++17 的 C++ 编译器。
- Qt 5 开发环境。

`CMakeLists.txt` 当前要求的 Qt 组件包括 `Core`、`Widgets`、`Gui`、`3DCore`、`3DRender`、`3DInput`、`3DExtras`、`Network`、`WebSockets`、`Sql` 和 `Charts`。

ZeroMQ 和 PostgreSQL 是可选依赖，只有找到时才链接。Redis、MQTT 等内容目前只出现在目标设计资料中，不是当前 CMake 的直接依赖。

## 推荐构建命令

从仓库根目录执行：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target UXOMissionControl -j2
```

成功后可执行文件位于：

```text
build/UXOMissionControl
```

## 构建脚本

也可以运行：

```bash
bash scripts/build.sh
```

该脚本末尾会询问是否执行 `sudo make install`。自动化验证优先使用推荐构建命令，避免交互式安装确认。

## 运行命令

从仓库根目录运行：

```bash
./build/UXOMissionControl
```

当前入口会在初始化成功后调用 `Application::run()`，创建并显示 `MainWindow`。在无图形环境中可使用 `QT_QPA_PLATFORM=offscreen` 做启动烟测。

## 测试

当前已有 CTest 验证：

```bash
ctest --test-dir build --output-on-failure
```

- `startup_visible`：验证 `Application::run()` 会显示标题为“排弹抢修指挥系统 V1.0”的 `MainWindow`。
- `demo_scenario_provider`：验证最小模拟演示场景包含模拟标识、1 个目标、1 个任务和至少 2 个模拟设备。

## 配置路径

`Application` 当前使用相对路径 `./config`。后续接入真实配置读取前，建议从仓库根目录运行程序。
