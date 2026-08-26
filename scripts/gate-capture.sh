#!/usr/bin/env bash
# UI 门禁采集脚本（REQ-010 阶段2 批次7 起，用户裁决：字体环境固化到门禁脚本）
# 目的：离屏 A/B 采集时钉死字体解析环境，消除批次6 取证发现的"字体激活离群"不确定性，
#       保证 BEFORE/AFTER 两次采集（乃至跨重启）的字体渲染确定性。
# 固化项：
#   QT_QPA_PLATFORM=offscreen        离屏渲染平台（与历批门禁一致）
#   FONTCONFIG_FILE=<conda>/etc/fonts/fonts.conf  钉死 fontconfig 到构建所用 conda 环境
#                                   （系统 CJK 字体稀疏，仅 1 款；钉死配置避免宿主字体变动影响离屏渲染）
# 用法：scripts/gate-capture.sh <tag>
#   tag：输出目录标签，如 before / after / req011-verify；输出 /tmp/opencode/ui-gate/<tag>/
# 采集矩阵（REQ-011 起 9 张）：态势页(01)/探测页(02)/决策页(03) 三页 × 1280x720/1920x1080/3840x2160 三视口（覆盖 3D 画布页、左表格页与 QSS 密集页）
# 确定性说明：p02 三视口逐字节确定；p01（画中画时钟/告警跑马灯）与 p03（跑道画布选中目标脉冲动画）存在帧级时变，A/B 差异仅落在上述已知区域时视同噪声，不算回归
set -euo pipefail

cd "$(git rev-parse --show-toplevel)"

TAG="${1:?用法: scripts/gate-capture.sh <tag>}"
ENVROOT="/home/lin/.local/share/mamba/envs/uxo-dev"
OUT="/tmp/opencode/ui-gate/${TAG}"
CAP="build-conda/MainWindowCapture"

# --- 字体环境固化（核心）---
export QT_QPA_PLATFORM=offscreen
export FONTCONFIG_FILE="${ENVROOT}/etc/fonts/fonts.conf"
[ -f "$FONTCONFIG_FILE" ] || { echo "错误: 字体配置不存在 $FONTCONFIG_FILE"; exit 1; }
[ -x "$CAP" ] || { echo "错误: 采集工具未构建 $CAP（先 cmake --build build-conda --target MainWindowCapture）"; exit 1; }

mkdir -p "$OUT"
# 环境证据落盘：复核字体固化是否生效
env | grep -E "QT_QPA|FONTCONFIG" > "${OUT}/env.txt"

# --- 采集矩阵（REQ-011 起扩展为 9 张：三页 × 三视口，与计划 §8 验证命令一致）---
for page in 01 02 03; do
  for vp in 1280x720 1920x1080 3840x2160; do
    $CAP "${OUT}/p${page}_${vp}.png" "$vp" "$page"
  done
done

echo "采集完成: ${OUT}"
ls -l "$OUT"
