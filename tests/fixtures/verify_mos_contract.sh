#!/usr/bin/env bash
# verify_mos_contract.sh - 扫描 docs/features/mos-planning.md 冻结 P0 合同 token
# 用法: bash verify_mos_contract.sh --scenario=happy     (exit 0)
#       bash verify_mos_contract.sh --scenario=bad-y-bound (exit 1)
set -eu

SCENARIO="${1:-}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DOC="${SCRIPT_DIR}/../../docs/features/mos-planning.md"

# 10 个必需 token（坐标/公式/档位/碰撞/导出/生成器）
REQUIRED_TOKENS=(
  "[0,L] × [-W/2,W/2]"
  "x=[0,L], y=[-40,40]"
  "visibleRadius × expand"
  "K × cbrt(syntheticYield)"
  "floor(tierIndex * N / (T-1))"
  "序数合成标签"
  "std::nextafter"
  "相切视为碰撞"
  "QSaveFile"
  "mulberry32"
)

# 扫描文件中所有必需 token，缺失则打印并返回 1
scan_tokens() {
  local file="$1"
  local missing=()
  for token in "${REQUIRED_TOKENS[@]}"; do
    if ! grep -qF "$token" "$file"; then
      missing+=("$token")
    fi
  done
  if [ "${#missing[@]}" -gt 0 ]; then
    echo "违反合同：以下 token 缺失：" >&2
    for m in "${missing[@]}"; do
      echo "  - $m" >&2
    done
    return 1
  fi
  return 0
}

case "$SCENARIO" in
  --scenario=happy)
    scan_tokens "$DOC"
    echo "OK: mos-planning.md 合同 token 全部存在（10/10）"
    exit 0
    ;;
  --scenario=bad-y-bound)
    # 创建临时文件，将 y=[-40,40] 替换为 y=[-200,200] 模拟违规
    tmpfile="$(mktemp)"
    trap 'rm -f "$tmpfile"' EXIT
    sed 's/y=\[-40,40\]/y=[-200,200]/g' "$DOC" > "$tmpfile"
    if scan_tokens "$tmpfile"; then
      echo "FAIL: bad-y-bound 场景应检测到 y 边界违规但未检测到" >&2
      exit 1
    else
      echo "违反合同：y 边界应为 [-40,40]，发现 [-200,200]" >&2
      exit 1
    fi
    ;;
  *)
    echo "用法: $0 --scenario=happy | --scenario=bad-y-bound" >&2
    exit 2
    ;;
esac
