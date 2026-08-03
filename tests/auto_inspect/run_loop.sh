#!/bin/bash
# 巡检员循环脚本：持续随机点击 + 自洽性检查
# 用法：./tests/auto_inspect/run_loop.sh [间隔秒数]
# 停止：Ctrl+C 或 kill 进程
#
# 查看状态：cat reports/STATUS.md
# 查看仪表盘：浏览器打开 reports/_timeline.html（每轮自动刷新）
#
# 可用环境变量：
#   REPORTS_DIR=自定义报告目录（默认 tests/auto_inspect/reports）

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
WORKER="$REPO_ROOT/build/tests/auto_inspect/inspector_worker"
REPORTS_DIR="${REPORTS_DIR:-$SCRIPT_DIR/reports}"
COVERAGE_FILE="$REPORTS_DIR/coverage.json"
STATUS_FILE="$REPORTS_DIR/STATUS.md"
INTERVAL="${1:-5}"
TIMEOUT_SECS=60
REPLAY_TIMEOUT=120
START_TIME=$(date '+%Y-%m-%d %H:%M:%S')
START_SEC=$(date +%s)

mkdir -p "$REPORTS_DIR"

if [ ! -f "$WORKER" ]; then
    echo "错误：找不到 worker，请先编译："
    echo "  cmake -S . -B build -DBUILD_AUTO_INSPECT=ON && cmake --build build --target inspector_worker"
    exit 1
fi

# 累计统计
ROUND=0
TOTAL_OK=0
TOTAL_CRASH=0
TOTAL_HANG=0
TOTAL_ISSUES=0
RECENT_ROUNDS=""
ALERTS=""
RUN_STATUS="运行中"

# 写 STATUS.md，用户随时 cat 即可看到当前状态和告警
update_status() {
    local now elapsed_sec elapsed_min elapsed_sec_rem elapsed_str cov_pairs
    now=$(date '+%Y-%m-%d %H:%M:%S')
    elapsed_sec=$(( $(date +%s) - START_SEC ))
    elapsed_min=$(( elapsed_sec / 60 ))
    elapsed_sec_rem=$(( elapsed_sec % 60 ))
    elapsed_str="${elapsed_min}分${elapsed_sec_rem}秒"
    cov_pairs="?"
    if [ -f "$COVERAGE_FILE" ]; then
        cov_pairs=$(python3 -c "import json; print(len(json.load(open('$COVERAGE_FILE'))))" 2>/dev/null || echo "?")
    fi

    {
        echo "# UXO 巡检员运行状态"
        echo ""
        echo "**状态**: $RUN_STATUS"
        echo "**起始时间**: $START_TIME"
        echo "**当前时间**: $now"
        echo "**已运行**: $elapsed_str"
        echo "**当前轮数**: $ROUND"
        echo ""
        echo "---"
        echo ""
        echo "## 汇总"
        echo ""
        echo "| 指标 | 数值 |"
        echo "|---|---|"
        echo "| 正常完成 | $TOTAL_OK |"
        echo "| 崩溃 | $TOTAL_CRASH |"
        echo "| 卡死 | $TOTAL_HANG |"
        echo "| 问题总数 | $TOTAL_ISSUES |"
        echo "| 覆盖率 | ${cov_pairs}/70 |"
        echo ""
        echo "---"
        echo ""
        echo "## 告警"
        echo ""
        if [ -z "$ALERTS" ]; then
            echo "（暂无告警）"
        else
            echo "$ALERTS"
        fi
        echo ""
        echo "---"
        echo ""
        echo "## 最近 10 轮"
        echo ""
        echo "| 轮次 | 时间 | 结果 | seed | 问题数 |"
        echo "|---|---|---|---|---|"
        echo "$RECENT_ROUNDS"
        echo ""
        echo "---"
        echo ""
        echo "## 查看详细报告"
        echo ""
        echo "- 浏览器打开：\`$REPORTS_DIR/_timeline.html\`（每轮自动刷新）"
        echo "- 单轮报告：\`$REPORTS_DIR/<时间戳>/report.json\`"
        echo "- 覆盖率：\`$COVERAGE_FILE\`"
    } > "$STATUS_FILE"
}

# 每轮结束后重新生成汇总 JSON 和 HTML 仪表盘
refresh_reports() {
    python3 "$SCRIPT_DIR/aggregate_reports.py" "$REPORTS_DIR" > /dev/null 2>&1 || true
    python3 "$SCRIPT_DIR/generate_html_report.py" "$REPORTS_DIR" > /dev/null 2>&1 || true
}

# 记录最近一轮（只保留最近 10 轮）
record_round() {
    local round=$1 time=$2 result=$3 seed=$4 issues=$5
    RECENT_ROUNDS="| $round | $time | $result | $seed | $issues |
$RECENT_ROUNDS"
    RECENT_ROUNDS=$(printf '%s\n' "$RECENT_ROUNDS" | head -10)
}

# 追加告警
add_alert() {
    ALERTS="${ALERTS}$1
"
}

# 退出时写最终状态
cleanup() {
    RUN_STATUS="已停止"
    update_status
    exit 0
}
trap cleanup INT TERM

echo "UXO 巡检员已启动 (PID $$)"
echo "状态文件：$STATUS_FILE"
echo "HTML 仪表盘：$REPORTS_DIR/_timeline.html（每轮自动刷新）"
echo "停止：Ctrl+C 或 kill $$"
echo "----------------------------------------"

while true; do
    ROUND=$((ROUND + 1))
    TIMESTAMP=$(date +%Y%m%d_%H%M%S)
    TIME_SHORT=$(date '+%H:%M:%S')
    ROUND_DIR="$REPORTS_DIR/$TIMESTAMP"
    mkdir -p "$ROUND_DIR"

    REPORT="$ROUND_DIR/report.json"
    STATE="$ROUND_DIR/last_action.txt"
    SEED=$RANDOM

    # 跑一轮，带超时。offscreen 模式，软件渲染避免 GPU 依赖。
    set +e
    timeout "$TIMEOUT_SECS" env \
        QT_QPA_PLATFORM=offscreen \
        QT_OPENGL=software \
        LIBGL_ALWAYS_SOFTWARE=1 \
        "$WORKER" \
        --report "$REPORT" \
        --state "$STATE" \
        --screenshots "$ROUND_DIR" \
        --coverage "$COVERAGE_FILE" \
        --seed "$SEED" \
        2>"$ROUND_DIR/stderr.log"
    EXIT_CODE=$?
    set -e

    if [ "$EXIT_CODE" -eq 124 ]; then
        TOTAL_HANG=$((TOTAL_HANG + 1))
        LAST_ACTION=$(cat "$STATE" 2>/dev/null || echo "未知")
        set +e
        timeout "$REPLAY_TIMEOUT" env \
            QT_QPA_PLATFORM=offscreen \
            QT_OPENGL=software \
            LIBGL_ALWAYS_SOFTWARE=1 \
            "$WORKER" \
            --state "$ROUND_DIR/replay_state.txt" \
            --seed "$SEED" \
            --verbose \
            2>"$ROUND_DIR/replay.log"
        set -e
        REPLAY_LAST=$(tail -1 "$ROUND_DIR/replay.log" 2>/dev/null || echo "无输出")
        echo "{\"type\":\"hang\",\"last_action\":\"$LAST_ACTION\",\"timeout_seconds\":$TIMEOUT_SECS,\"seed\":$SEED,\"replay_log\":\"replay.log\"}" > "$ROUND_DIR/hang.json"
        add_alert "### ⚠️ 第 $ROUND 轮卡死 ($TIME_SHORT, seed=$SEED)
- 最后动作：$LAST_ACTION
- 复现停止点：$REPLAY_LAST
- 证据目录：\`$TIMESTAMP/\`"
        record_round "$ROUND" "$TIME_SHORT" "⚠️ 卡死" "$SEED" "-"
    elif [ "$EXIT_CODE" -ne 0 ]; then
        TOTAL_CRASH=$((TOTAL_CRASH + 1))
        LAST_ACTION=$(cat "$STATE" 2>/dev/null || echo "未知")
        set +e
        timeout "$REPLAY_TIMEOUT" env \
            QT_QPA_PLATFORM=offscreen \
            QT_OPENGL=software \
            LIBGL_ALWAYS_SOFTWARE=1 \
            "$WORKER" \
            --state "$ROUND_DIR/replay_state.txt" \
            --seed "$SEED" \
            --verbose \
            2>"$ROUND_DIR/replay.log"
        set -e
        REPLAY_LAST=$(tail -1 "$ROUND_DIR/replay.log" 2>/dev/null || echo "无输出")
        echo "{\"type\":\"crash\",\"exit_code\":$EXIT_CODE,\"last_action\":\"$LAST_ACTION\",\"seed\":$SEED,\"replay_log\":\"replay.log\"}" > "$ROUND_DIR/crash.json"
        add_alert "### 💥 第 $ROUND 轮崩溃 ($TIME_SHORT, seed=$SEED, 退出码 $EXIT_CODE)
- 最后动作：$LAST_ACTION
- 复现停止点：$REPLAY_LAST
- 证据目录：\`$TIMESTAMP/\`"
        record_round "$ROUND" "$TIME_SHORT" "💥 崩溃" "$SEED" "-"
    else
        TOTAL_OK=$((TOTAL_OK + 1))
        ISSUES=$(python3 -c "import json; print(json.load(open('$REPORT')).get('issues_found',0))" 2>/dev/null || echo "0")
        TOTAL_ISSUES=$((TOTAL_ISSUES + ISSUES))
        if [ "$ISSUES" -gt 0 ] 2>/dev/null; then
            add_alert "### 🔴 第 $ROUND 轮发现 $ISSUES 个问题 ($TIME_SHORT, seed=$SEED)
- 报告：\`$TIMESTAMP/report.json\`
- 截图：\`$TIMESTAMP/\`"
        fi
        record_round "$ROUND" "$TIME_SHORT" "✓ 正常" "$SEED" "$ISSUES"
    fi

    # 每轮结束后：刷新报告 + 更新状态文件
    refresh_reports
    update_status

    sleep "$INTERVAL"
done
