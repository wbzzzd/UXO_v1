#!/bin/bash
# 巡检员循环脚本：持续随机点击 + 自洽性检查
# 用法：./tests/auto_inspect/run_loop.sh [间隔秒数]
# 停止：Ctrl+C 或 kill 进程
#
# 工作方式：循环启动 inspector_worker，每轮独立进程。
# - 正常退出：读报告 JSON
# - 非零退出码：崩溃，读 state 文件确认最后动作
# - 超时(124)：卡死，读 state 文件确认最后动作

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
WORKER="$REPO_ROOT/build/tests/auto_inspect/inspector_worker"
REPORTS_DIR="$SCRIPT_DIR/reports"
INTERVAL="${1:-5}"
TIMEOUT_SECS=60

mkdir -p "$REPORTS_DIR"

# 检查 worker 是否已编译
if [ ! -f "$WORKER" ]; then
    echo "错误：找不到 worker 可执行文件"
    echo "  期望路径：$WORKER"
    echo "  请先编译：cmake -S . -B build -DBUILD_AUTO_INSPECT=ON && cmake --build build --target inspector_worker"
    exit 1
fi

echo "========================================"
echo "  UXO 巡检员已启动"
echo "========================================"
echo "报告目录：$REPORTS_DIR"
echo "间隔：${INTERVAL}秒 | 超时：${TIMEOUT_SECS}秒"
echo "停止：Ctrl+C 或 kill $$"
echo "----------------------------------------"

ROUND=0
while true; do
    ROUND=$((ROUND + 1))
    TIMESTAMP=$(date +%Y%m%d_%H%M%S)
    ROUND_DIR="$REPORTS_DIR/$TIMESTAMP"
    mkdir -p "$ROUND_DIR"

    REPORT="$ROUND_DIR/report.json"
    STATE="$ROUND_DIR/last_action.txt"

    echo "[$(date '+%H:%M:%S')] 第 $ROUND 轮开始..."

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
        2>"$ROUND_DIR/stderr.log"
    EXIT_CODE=$?
    set -e

    if [ "$EXIT_CODE" -eq 124 ]; then
        # timeout 命令返回 124 表示超时
        LAST_ACTION=$(cat "$STATE" 2>/dev/null || echo "未知")
        echo "[$(date '+%H:%M:%S')] ⚠️  第 $ROUND 轮：卡死（超时 ${TIMEOUT_SECS}s）"
        echo "  最后动作：$LAST_ACTION"
        echo "{\"type\":\"hang\",\"last_action\":\"$LAST_ACTION\",\"timeout_seconds\":$TIMEOUT_SECS}" > "$ROUND_DIR/hang.json"
        echo "  证据目录：$ROUND_DIR"
    elif [ "$EXIT_CODE" -ne 0 ]; then
        # 非零退出码表示崩溃
        LAST_ACTION=$(cat "$STATE" 2>/dev/null || echo "未知")
        echo "[$(date '+%H:%M:%S')] 💥 第 $ROUND 轮：崩溃（退出码 $EXIT_CODE）"
        echo "  最后动作：$LAST_ACTION"
        echo "  错误输出：$ROUND_DIR/stderr.log"
        echo "{\"type\":\"crash\",\"exit_code\":$EXIT_CODE,\"last_action\":\"$LAST_ACTION\"}" > "$ROUND_DIR/crash.json"
        echo "  证据目录：$ROUND_DIR"
    else
        # 正常完成，从 stderr 摘要行读问题数
        SUMMARY=$(grep "^SUMMARY:" "$ROUND_DIR/stderr.log" 2>/dev/null || echo "SUMMARY: issues=?")
        echo "[$(date '+%H:%M:%S')] ✓  第 $ROUND 轮：$SUMMARY"
        echo "  报告：$REPORT"
    fi

    sleep "$INTERVAL"
done
