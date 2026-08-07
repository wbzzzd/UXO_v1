#!/usr/bin/env bash
# verify_decision_contract.sh - 扫描 docs/ui/pages/decision.md 冻结 P0 Qt 合同 token
# 用法: bash verify_decision_contract.sh --scenario=happy       (exit 0)
#       bash verify_decision_contract.sh --scenario=fixed-delay  (exit 1)
set -eu

SCENARIO="${1:-}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DOC="${SCRIPT_DIR}/../../docs/ui/pages/decision.md"

# 4 个必需 token（档位选择/弱显/去延时/单向导出）
REQUIRED_TOKENS=(
  "P0 支持在已计算档位之间选择"
  "弱显全部档位"
  "Qt 实现去除固定"
  "QSaveFile"
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
    echo "OK: decision.md 合同 token 全部存在（4/4）"
    exit 0
    ;;
  --scenario=fixed-delay)
    # 创建临时文件，移除 "Qt 实现去除固定" 去延时合同 token 模拟违规
    tmpfile="$(mktemp)"
    trap 'rm -f "$tmpfile"' EXIT
    sed 's/Qt 实现去除固定/Qt 实现保留固定/g' "$DOC" > "$tmpfile"
    if scan_tokens "$tmpfile"; then
      echo "FAIL: fixed-delay 场景应检测到去延时合同违规但未检测到" >&2
      exit 1
    else
      echo "违反合同：Qt 实现须去除固定 350ms/500ms 人工延时" >&2
      exit 1
    fi
    ;;
  *)
    echo "用法: $0 --scenario=happy | --scenario=fixed-delay" >&2
    exit 2
    ;;
esac
