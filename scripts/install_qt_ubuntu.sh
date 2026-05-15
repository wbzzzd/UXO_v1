#!/bin/bash
# Qt 5.15 LTS 安装脚本 - 适配国产操作系统

set -e

echo "=========================================="
echo "  Qt 5.15 LTS 安装脚本"
echo "  适配银河麒麟 / 统信UOS / Ubuntu"
echo "=========================================="

# 检查是否为root用户
if [ "$EUID" -ne 0 ]; then
    echo "请使用 root 用户运行此脚本"
    echo "或者运行: sudo bash install_qt.sh"
    exit 1
fi

echo "[1/6] 更新软件包列表..."
apt update -qq

echo "[2/6] 安装基础开发工具..."
apt install -y build-essential cmake g++ gcc

echo "[3/6] 安装Qt依赖库..."
apt install -y \
    libgl1-mesa-dev \
    libxcb-xinerama0-dev \
    libxcb-icccm4-dev \
    libxcb-image0-dev \
    libxcb-keysyms1-dev \
    libxcb-randr0-dev \
    libxcb-render-util0-dev \
    libxcb-xkb-dev \
    libxkbcommon-dev \
    libxkbcommon-x11-dev \
    libfontconfig1-dev \
    libfreetype6-dev \
    libx11-dev \
    libxext-dev \
    libxfixes-dev \
    libxi-dev \
    libxrender-dev \
    libxcb-shape0-dev \
    libxcb-sync-dev \
    libxcb-xfixes0-dev \
    libxcb1-dev \
    libxcb-glx0-dev \
    libegl1-mesa-dev \
    libinput-dev \
    libxcb-xkb-dev \
    libxcb-util0-dev \
    libxcb-cursor-dev

echo "[4/6] 安装Qt Creator和Qt 5.15..."
apt install -y qt5-default qtcreator qtbase5-dev qtchooser qtbase5-dev-tools qt5-qmake

echo "[5/6] 安装Qt 3D模块（用于3D可视化）..."
apt install -y qt3d5-dev libqt53drender5 libqt53dinput5 libqt53dcore5

echo "[6/6] 安装Qt Quick Controls（更漂亮的UI）..."
apt install -y qml-module-qtquick-controls qml-module-qtquick-controls2 qml-module-qtquick-layouts qml-module-qtquick-window2 qml-module-qtquick-shapes qml-module-qtgraphicaleffects

echo ""
echo "=========================================="
echo "  安装完成！"
echo "=========================================="
echo ""
echo "验证安装："
qmake --version
echo ""
echo "启动Qt Creator："
echo "  qtcreator"
echo ""
echo "Qt项目目录："
echo "  ~/QtProjects/"
echo ""
echo "建议：重启系统以确保环境变量生效"
