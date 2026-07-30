#!/usr/bin/env python3
# 巡检员 HTML 时间线报告生成器：把每轮动作序列和问题标记可视化为单一 HTML。
#
# 用法：python3 generate_html_report.py [reports目录]
# 默认目录：脚本同级的 reports/
#
# 输出：reports/_timeline.html
# 打开：用浏览器直接打开 _timeline.html 即可查看（截图为相对路径，需在 reports 目录内打开）

import base64
import html
import json
import sys
from collections import Counter, defaultdict
from pathlib import Path

ACTION_KIND_STYLE = {
    "init": ("初始化", "#6b7280"),
    "target_row": ("点击目标行", "#3b82f6"),
    "confirm": ("模拟确认", "#10b981"),
    "start": ("模拟处置", "#f59e0b"),
    "complete": ("处置完成", "#8b5cf6"),
    "camera_top": ("俯视", "#06b6d4"),
    "camera_side": ("侧视", "#0ea5e9"),
    "camera_3d": ("3D视角", "#14b8a6"),
    "camera_reset": ("相机复位", "#22d3ee"),
    "tab_switch": ("切换标签页", "#ec4899"),
    "refresh": ("刷新", "#84cc16"),
    "unknown": ("未知", "#9ca3af"),
}


def load_round(round_dir: Path):
    """读取单轮目录的所有产物，返回轮次记录字典。"""
    record = {
        "dir": round_dir.name,
        "outcome": "unknown",
        "issues": [],
        "action_log": [],
        "actions": 0,
        "seed": None,
        "last_action": None,
        "replay_log": None,
        "exit_code": None,
    }

    crash_file = round_dir / "crash.json"
    hang_file = round_dir / "hang.json"
    report_file = round_dir / "report.json"

    if crash_file.exists():
        record["outcome"] = "crash"
        data = json.loads(crash_file.read_text(encoding="utf-8"))
        record["last_action"] = data.get("last_action")
        record["seed"] = data.get("seed")
        record["replay_log"] = data.get("replay_log")
        record["exit_code"] = data.get("exit_code")
    elif hang_file.exists():
        record["outcome"] = "hang"
        data = json.loads(hang_file.read_text(encoding="utf-8"))
        record["last_action"] = data.get("last_action")
        record["seed"] = data.get("seed")
        record["replay_log"] = data.get("replay_log")
    elif report_file.exists():
        record["outcome"] = "completed"
        data = json.loads(report_file.read_text(encoding="utf-8"))
        record["actions"] = data.get("actions_executed", 0)
        record["seed"] = data.get("seed")
        record["issues"] = data.get("issues", [])
        record["action_log"] = data.get("action_log", [])

    # 尝试加载复现日志尾部（崩溃复现的最后停止点）
    replay_file = round_dir / "replay.log"
    if replay_file.exists():
        record["replay_tail"] = replay_file.read_text(encoding="utf-8", errors="replace").strip().splitlines()[-3:]

    return record


def screenshot_to_base64(round_dir: Path, screenshot_name: str) -> str:
    """把截图转 base64 内嵌，使 HTML 单文件可移植。"""
    if not screenshot_name:
        return ""
    img_path = round_dir / screenshot_name
    if not img_path.exists():
        return ""
    data = img_path.read_bytes()
    return base64.b64encode(data).decode("ascii")


def render_action_timeline(record):
    """渲染单轮动作时间线 HTML。"""
    parts = []
    parts.append('<div class="timeline">')

    for entry in record["action_log"]:
        step = entry.get("step", 0)
        action = entry.get("action", "?")
        kind = entry.get("kind", "unknown")
        executed = entry.get("executed", False)
        label, color = ACTION_KIND_STYLE.get(kind, ACTION_KIND_STYLE["unknown"])

        executed_class = "" if executed else " skipped"
        parts.append(
            f'<div class="step{executed_class}">'
            f'<span class="step-num">{step}</span>'
            f'<span class="step-dot" style="background:{color}" title="{html.escape(label)}"></span>'
            f'<span class="step-action">{html.escape(action)}</span>'
            f'<span class="step-kind">{html.escape(label)}</span>'
            f"</div>"
        )

    # 崩溃/卡死：在时间线末尾标注异常点
    if record["outcome"] == "crash":
        parts.append(
            '<div class="step crash-marker">'
            f'<span class="step-num">!</span>'
            '<span class="step-dot" style="background:#dc2626"></span>'
            f'<span class="step-action">💥 崩溃（退出码 {html.escape(str(record.get("exit_code", "?"))) }）</span>'
            '<span class="step-kind">CRASH</span>'
            "</div>"
        )
    elif record["outcome"] == "hang":
        parts.append(
            '<div class="step hang-marker">'
            '<span class="step-num">!</span>'
            '<span class="step-dot" style="background:#ea580c"></span>'
            '<span class="step-action">⚠️ 卡死（超时）</span>'
            '<span class="step-kind">HANG</span>'
            "</div>"
        )

    parts.append("</div>")
    return "".join(parts)


def render_issues(record, round_dir):
    """渲染单轮问题列表（含截图缩略图）。"""
    if not record["issues"]:
        return ""

    parts = ['<div class="issues"><h4>问题</h4>']
    for i, iss in enumerate(record["issues"], 1):
        rule = html.escape(iss.get("rule", ""))
        details = html.escape(iss.get("details", ""))
        action = html.escape(iss.get("action", ""))
        screenshot = iss.get("screenshot", "")
        b64 = screenshot_to_base64(round_dir, screenshot)
        img_tag = (
            f'<img class="issue-screenshot" alt="截图" src="data:image/png;base64,{b64}">'
            if b64
            else ""
        )
        parts.append(
            f'<div class="issue">'
            f'<div class="issue-rule">#{i} {rule}</div>'
            f'<div class="issue-action">触发动作：{action}</div>'
            f'<div class="issue-details">{details}</div>'
            f"{img_tag}"
            "</div>"
        )
    parts.append("</div>")
    return "".join(parts)


def render_round_card(record, round_dir):
    """渲染单轮卡片。"""
    outcome = record["outcome"]
    outcome_class = f" outcome-{outcome}"
    outcome_label = {"completed": "✓ 完成", "crash": "💥 崩溃", "hang": "⚠️ 卡死"}.get(outcome, "?")

    seed_text = f"seed={record['seed']}" if record["seed"] is not None else "seed=?"
    actions_text = f"{record['actions']} 步" if record["actions"] else "?"
    issues_count = len(record["issues"])

    parts = [
        f'<div class="round-card{outcome_class}">',
        '<div class="round-header">',
        f'<span class="round-time">{html.escape(record["dir"])}</span>',
        f'<span class="round-outcome">{outcome_label}</span>',
        f'<span class="round-meta">{seed_text} | {actions_text} | {issues_count} 问题</span>',
        "</div>",
    ]

    # 崩溃/卡死额外信息
    if record["last_action"]:
        parts.append(
            f'<div class="last-action">最后动作：{html.escape(record["last_action"])}</div>'
        )
    if record.get("replay_tail"):
        tail = "\n".join(record["replay_tail"])
        parts.append(
            f'<details class="replay"><summary>复现停止点</summary><pre>{html.escape(tail)}</pre></details>'
        )

    # 时间线
    if record["action_log"] or outcome in ("crash", "hang"):
        parts.append(render_action_timeline(record))

    # 问题
    parts.append(render_issues(record, round_dir))

    parts.append("</div>")
    return "".join(parts)


def render_dashboard(rounds):
    """渲染顶部统计仪表盘。"""
    total = len(rounds)
    completed = sum(1 for r in rounds if r["outcome"] == "completed")
    crashes = sum(1 for r in rounds if r["outcome"] == "crash")
    hangs = sum(1 for r in rounds if r["outcome"] == "hang")
    total_issues = sum(len(r["issues"]) for r in rounds)

    # 动作覆盖率
    kind_counter = Counter()
    for r in rounds:
        for entry in r["action_log"]:
            kind = entry.get("kind", "unknown")
            if kind != "init":
                kind_counter[kind] += 1

    # 唯一问题去重
    issue_map = defaultdict(lambda: {"count": 0, "rule": "", "details": "", "actions": set()})
    for r in rounds:
        for iss in r["issues"]:
            key = (iss.get("rule", ""), iss.get("details", ""))
            entry = issue_map[key]
            entry["count"] += 1
            entry["rule"] = iss.get("rule", "")
            entry["details"] = iss.get("details", "")
            entry["actions"].add(iss.get("action", ""))
    unique_issues = sorted(issue_map.values(), key=lambda e: -e["count"])

    parts = ['<div class="dashboard">']

    # 数字卡片
    parts.append('<div class="stat-cards">')
    parts.append(f'<div class="stat-card"><div class="stat-num">{total}</div><div class="stat-label">总轮数</div></div>')
    parts.append(f'<div class="stat-card stat-ok"><div class="stat-num">{completed}</div><div class="stat-label">正常完成</div></div>')
    parts.append(f'<div class="stat-card stat-crash"><div class="stat-num">{crashes}</div><div class="stat-label">崩溃</div></div>')
    parts.append(f'<div class="stat-card stat-hang"><div class="stat-num">{hangs}</div><div class="stat-label">卡死</div></div>')
    parts.append(f'<div class="stat-card stat-warn"><div class="stat-num">{total_issues}</div><div class="stat-label">总问题</div></div>')
    parts.append(f'<div class="stat-card stat-warn"><div class="stat-num">{len(unique_issues)}</div><div class="stat-label">唯一问题</div></div>')
    parts.append("</div>")

    # 动作覆盖率
    if kind_counter:
        parts.append('<div class="coverage"><h3>动作覆盖率</h3>')
        total_actions = sum(kind_counter.values())
        for kind, count in sorted(kind_counter.items(), key=lambda x: -x[1]):
            label, color = ACTION_KIND_STYLE.get(kind, ACTION_KIND_STYLE["unknown"])
            pct = (count / total_actions * 100) if total_actions else 0
            parts.append(
                f'<div class="coverage-row">'
                f'<span class="coverage-label">{html.escape(label)}</span>'
                f'<div class="coverage-bar"><div class="coverage-fill" style="width:{pct:.1f}%;background:{color}"></div></div>'
                f'<span class="coverage-count">{count}</span>'
                f"</div>"
            )
        parts.append("</div>")

    # 唯一问题表
    if unique_issues:
        parts.append('<div class="unique-issues"><h3>唯一问题（按出现次数降序）</h3>')
        for i, iss in enumerate(unique_issues, 1):
            actions = ", ".join(sorted(iss["actions"]))
            parts.append(
                f'<div class="unique-issue">'
                f'<div class="ui-head">#{i} {html.escape(iss["rule"])} <span class="ui-count">×{iss["count"]}</span></div>'
                f'<div class="ui-details">{html.escape(iss["details"])}</div>'
                f'<div class="ui-actions">触发动作：{html.escape(actions)}</div>'
                "</div>"
            )
        parts.append("</div>")

    parts.append("</div>")
    return "".join(parts)


HTML_HEAD = """<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<title>UXO 巡检员时间线报告</title>
<style>
* { box-sizing: border-box; margin: 0; padding: 0; }
body { font-family: -apple-system, "Segoe UI", "PingFang SC", "Microsoft YaHei", sans-serif;
       background: #0f172a; color: #e2e8f0; padding: 20px; line-height: 1.5; }
h1 { color: #f8fafc; margin-bottom: 8px; }
.subtitle { color: #94a3b8; margin-bottom: 24px; font-size: 14px; }
.dashboard { background: #1e293b; border-radius: 12px; padding: 24px; margin-bottom: 24px; }
.stat-cards { display: flex; gap: 16px; flex-wrap: wrap; margin-bottom: 24px; }
.stat-card { background: #334155; border-radius: 10px; padding: 20px; min-width: 100px; text-align: center; }
.stat-num { font-size: 32px; font-weight: 700; color: #f8fafc; }
.stat-label { font-size: 12px; color: #94a3b8; margin-top: 4px; }
.stat-ok .stat-num { color: #4ade80; }
.stat-crash .stat-num { color: #f87171; }
.stat-hang .stat-num { color: #fb923c; }
.stat-warn .stat-num { color: #fbbf24; }
.coverage, .unique-issues { margin-top: 20px; }
h3 { color: #cbd5e1; margin-bottom: 12px; font-size: 16px; }
.coverage-row { display: flex; align-items: center; gap: 12px; margin-bottom: 8px; }
.coverage-label { width: 100px; font-size: 13px; color: #cbd5e1; }
.coverage-bar { flex: 1; height: 10px; background: #334155; border-radius: 5px; overflow: hidden; }
.coverage-fill { height: 100%; border-radius: 5px; }
.coverage-count { width: 50px; text-align: right; font-size: 13px; color: #94a3b8; }
.unique-issue { background: #334155; border-left: 3px solid #f87171; padding: 12px 16px; border-radius: 6px; margin-bottom: 8px; }
.ui-head { font-weight: 600; color: #f8fafc; }
.ui-count { color: #fbbf24; font-weight: 400; margin-left: 8px; }
.ui-details { color: #cbd5e1; font-size: 13px; margin-top: 4px; }
.ui-actions { color: #94a3b8; font-size: 12px; margin-top: 4px; }
.round-card { background: #1e293b; border-radius: 12px; padding: 20px; margin-bottom: 16px; border-left: 4px solid #475569; }
.round-card.outcome-completed { border-left-color: #4ade80; }
.round-card.outcome-crash { border-left-color: #f87171; }
.round-card.outcome-hang { border-left-color: #fb923c; }
.round-header { display: flex; align-items: center; gap: 16px; flex-wrap: wrap; margin-bottom: 12px; }
.round-time { font-weight: 600; color: #f8fafc; font-family: monospace; }
.round-outcome { font-size: 13px; font-weight: 600; }
.round-meta { color: #94a3b8; font-size: 13px; margin-left: auto; }
.last-action { background: #334155; padding: 8px 12px; border-radius: 6px; font-size: 13px; color: #fca5a5; margin-bottom: 12px; }
.replay { margin-bottom: 12px; }
.replay summary { cursor: pointer; color: #94a3b8; font-size: 13px; }
.replay pre { background: #0f172a; padding: 12px; border-radius: 6px; margin-top: 8px; font-size: 12px; overflow-x: auto; color: #fbbf24; }
.timeline { display: flex; flex-direction: column; gap: 2px; margin-bottom: 12px; }
.step { display: flex; align-items: center; gap: 10px; padding: 6px 8px; border-radius: 4px; font-size: 13px; }
.step:hover { background: #334155; }
.step.skipped { opacity: 0.5; }
.step-num { width: 28px; text-align: center; color: #64748b; font-family: monospace; font-size: 12px; }
.step-dot { width: 12px; height: 12px; border-radius: 50%; flex-shrink: 0; }
.step-action { flex: 1; color: #e2e8f0; }
.step-kind { color: #64748b; font-size: 11px; }
.crash-marker { background: rgba(248,113,113,0.15); }
.hang-marker { background: rgba(251,146,60,0.15); }
.issues { margin-top: 12px; }
.issues h4 { color: #f87171; margin-bottom: 8px; font-size: 14px; }
.issue { background: #334155; border-left: 3px solid #f87171; padding: 10px 14px; border-radius: 6px; margin-bottom: 8px; }
.issue-rule { font-weight: 600; color: #fca5a5; font-size: 13px; }
.issue-action { color: #94a3b8; font-size: 12px; margin-top: 4px; }
.issue-details { color: #e2e8f0; font-size: 13px; margin-top: 4px; white-space: pre-wrap; }
.issue-screenshot { max-width: 100%; max-height: 300px; border-radius: 6px; margin-top: 8px; border: 1px solid #475569; }
</style>
</head>
<body>
<h1>UXO 巡检员时间线报告</h1>
"""

HTML_TAIL = """
</body>
</html>
"""


def main():
    reports_dir = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).parent / "reports"
    if not reports_dir.exists():
        print(f"错误：报告目录不存在：{reports_dir}", file=sys.stderr)
        return 1

    rounds = []
    for child in sorted(reports_dir.iterdir()):
        if child.is_dir() and not child.name.startswith("_"):
            rounds.append(load_round(child))

    if not rounds:
        print(f"错误：{reports_dir} 下没有轮次目录", file=sys.stderr)
        return 1

    parts = [HTML_HEAD]
    parts.append(f'<div class="subtitle">报告目录：{html.escape(str(reports_dir))} | 共 {len(rounds)} 轮</div>')
    parts.append(render_dashboard(rounds))
    parts.append("<hr>")
    for r in rounds:
        parts.append(render_round_card(r, reports_dir / r["dir"]))
    parts.append(HTML_TAIL)

    out_file = reports_dir / "_timeline.html"
    out_file.write_text("".join(parts), encoding="utf-8")
    print(f"HTML 报告已生成：{out_file}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
