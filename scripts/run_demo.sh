#!/bin/bash
# 排弹抢修指挥系统 - 前端演示启动脚本
# 作用：在 WSL2 + WSLg 环境下，把编译好的 UXOMissionControl 桌面程序显示到 Windows 桌面
# 用法：
#   bash scripts/run_demo.sh            # 默认：WSLg 直接显示窗口（推荐）
#   bash scripts/run_demo.sh webgl      # 浏览器访问（远程演示）
#   bash scripts/run_demo.sh vnc        # VNC 客户端访问
#   bash scripts/run_demo.sh offscreen  # 无显示启动烟测

set -e

# 定位项目根目录（脚本位于 <root>/scripts/ 下）
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

# mamba/conda 环境路径（Qt5 库与平台插件均在此环境内）
MAMBA_PREFIX="$HOME/.local/share/mamba/envs/uxo-dev"

if [ ! -d "$MAMBA_PREFIX" ]; then
    echo "[错误] 找不到 mamba 环境: $MAMBA_PREFIX"
    echo "请确认 uxo-dev 环境已创建，或修改本脚本里的 MAMBA_PREFIX"
    exit 1
fi

# 让运行时能找到 Qt 库与平台插件（xcb/wayland/webgl/vnc/offscreen 等）
export LD_LIBRARY_PATH="$MAMBA_PREFIX/lib:$LD_LIBRARY_PATH"
export QT_PLUGIN_PATH="$MAMBA_PREFIX/plugins"

# WSLg 会自动注入 DISPLAY=:0；若缺失则兜底设置
export DISPLAY="${DISPLAY:-:0}"

# 待运行的二进制（注意：实际位于 build/src/App/，非 build/ 根目录）
BIN="$PROJECT_ROOT/build/src/App/UXOMissionControl"

if [ ! -x "$BIN" ]; then
    echo "[错误] 找不到可执行文件: $BIN"
    echo "请先构建："
    echo "  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release"
    echo "  cmake --build build --target UXOMissionControl -j2"
    exit 1
fi

PLATFORM="${1:-xcb}"

echo "=============================================="
echo "  排弹抢修指挥系统 - 前端演示"
echo "=============================================="
echo "项目目录 : $PROJECT_ROOT"
echo "可执行   : $BIN"
echo "Qt 环境  : $MAMBA_PREFIX"
echo "DISPLAY  : $DISPLAY"
echo "平台模式 : $PLATFORM"
echo "----------------------------------------------"

case "$PLATFORM" in
    xcb)
        # WSLg 原生显示：窗口直接出现在 Windows 桌面（推荐）
        echo "提示：窗口将直接显示在 Windows 桌面上（WSLg）。"
        echo "      若无窗口弹出，请确认 Windows 11 + WSL2 已启用 WSLg。"
        exec "$BIN"
        ;;
    webgl)
        # 通过浏览器访问 Qt 程序：适合远程演示或纯命令行环境
        # Qt WebGL 插件默认监听一个端口，终端会打印访问地址
        echo "提示：启动后终端会打印 http://localhost:<port> 访问地址。"
        echo "      浏览器打开该地址即可看到界面（3D/复杂控件支持有限）。"
        exec "$BIN" -platform webgl
        ;;
    vnc)
        echo "提示：Qt VNC 平台默认监听 5900 端口。"
        echo "      用 VNC 客户端连接 localhost:5900 查看。"
        exec "$BIN" -platform vnc
        ;;
    offscreen)
        echo "提示：离屏模式，不显示窗口，仅验证启动是否成功。"
        exec env QT_QPA_PLATFORM=offscreen "$BIN"
        ;;
    *)
        echo "[错误] 未知平台: $PLATFORM"
        echo "可选: xcb | webgl | vnc | offscreen"
        exit 1
        ;;
esac
