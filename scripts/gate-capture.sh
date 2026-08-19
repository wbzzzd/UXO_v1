#!/usr/bin/env bash
# UI 门禁采集脚本（REQ-010 阶段2 批次7 起，用户裁决：字体环境固化到门禁脚本）
# 目的：离屏 A/B 采集时钉死字体解析环境，消除批次6 取证发现的"字体激活离群"不确定性，
#       保证 BEFORE/AFTER 两次采集（乃至跨重启）的字体渲染确定性。
# 固化项：
#   QT_QPA_PLATFORM=offscreen        离屏渲染平台（与历批门禁一致）
#   FONTCONFIG_FILE=<conda>/etc/fonts/fonts.conf  钉死 fontconfig 到构建所用 conda 环境
#                                   （系统 CJK 字体稀疏，仅 1 款；钉死配置避免宿主字体变动影响离屏渲染）
# 用法：scripts/gate-capture.sh <tag>
#   tag：输出目录标签，如 before / after；输出 /tmp/opencode/uiupgrade-batch7/<tag>/
# 采集矩阵：决策页(03)三视口 1280x720/1920x1080/3840x2160 + 态势页(01) 1920x1080（覆盖 QSS 密集页与 3D 画布页）
set -euo pipefail

cd "$(git rev-parse --show-toplevel)"

TAG="${1:?用法: scripts/gate-capture.sh <tag>}"
ENVROOT="/home/lin/.local/share/mamba/envs/uxo-dev"
OUT="/tmp/opencode/uiupgrade-batch7/${TAG}"
CAP="build-conda/MainWindowCapture"

# --- 字体环境固化（核心）---
export QT_QPA_PLATFORM=offscreen
export FONTCONFIG_FILE="${ENVROOT}/etc/fonts/fonts.conf"
[ -f "$FONTCONFIG_FILE" ] || { echo "错误: 字体配置不存在 $FONTCONFIG_FILE"; exit 1; }
[ -x "$CAP" ] || { echo "错误: 采集工具未构建 $CAP（先 cmake --build build-conda --target MainWindowCapture）"; exit 1; }

mkdir -p "$OUT"
# 环境证据落盘：复核字体固化是否生效
env | grep -E "QT_QPA|FONTCONFIG" > "${OUT}/env.txt"

# --- 采集矩阵 ---
$CAP "${OUT}/p03_1280x720.png"  1280x720  03
$CAP "${OUT}/p03_1920x1080.png" 1920x1080 03
$CAP "${OUT}/p03_3840x2160.png" 3840x2160 03
$CAP "${OUT}/p01_1920x1080.png" 1920x1080 01

echo "采集完成: ${OUT}"
ls -l "$OUT"
