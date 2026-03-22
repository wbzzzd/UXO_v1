#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"

echo "====================================="
echo "排弹抢修指挥系统 - 构建脚本"
echo "====================================="

if [ ! -d "$BUILD_DIR" ]; then
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR"

echo "[1/4] 运行 CMake 配置..."
cmake .. -DCMAKE_BUILD_TYPE=Release

echo "[2/4] 检查 Qt 依赖..."
for pkg in Qt5Core Qt5Widgets Qt5Gui Qt53DCore Qt53DRender Qt53DInput Qt53DExtras Qt5Network Qt5WebSockets Qt5Sql Qt5Charts; do
    if ! pkg-config --exists "$pkg"; then
        echo "警告: 未找到 $pkg"
    fi
done

echo "[3/4] 编译项目..."
make -j$(nproc)

echo "[4/4] 安装 (可选)..."
read -p "是否安装到系统? (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    sudo make install
fi

echo ""
echo "====================================="
echo "构建完成!"
echo "可执行文件: $BUILD_DIR/UXOMissionControl"
echo "====================================="
