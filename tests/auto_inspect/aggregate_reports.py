#!/usr/bin/env python3
# 巡检员报告聚合脚本：扫描 reports/ 目录，去重汇总问题、统计崩溃/卡死、生成摘要。
#
# 用法：python3 aggregate_reports.py [reports目录]
# 默认目录：脚本同级的 reports/
#
# 输出：
#   - stdout 人类可读摘要
#   - reports/_summary.json 机器可读摘要

import json
import sys
from collections import Counter, defaultdict
from pathlib import Path


def load_round(round_dir: Path):
    """读取单轮目录的所有产物，返回轮次记录字典。"""
    record = {"dir": round_dir.name, "outcome": "unknown", "issues": [],
              "actions": 0, "seed": None, "action_kinds": [], "last_action": None}

    crash_file = round_dir / "crash.json"
    hang_file = round_dir / "hang.json"
    report_file = round_dir / "report.json"

    if crash_file.exists():
        record["outcome"] = "crash"
        data = json.loads(crash_file.read_text(encoding="utf-8"))
        record["last_action"] = data.get("last_action")
    elif hang_file.exists():
        record["outcome"] = "hang"
        data = json.loads(hang_file.read_text(encoding="utf-8"))
        record["last_action"] = data.get("last_action")
    elif report_file.exists():
        record["outcome"] = "completed"
        data = json.loads(report_file.read_text(encoding="utf-8"))
        record["actions"] = data.get("actions_executed", 0)
        record["seed"] = data.get("seed")
        record["issues"] = data.get("issues", [])
        for entry in data.get("action_log", []):
            kind = entry.get("kind", "unknown")
            if kind != "init":
                record["action_kinds"].append(kind)

    return record


def aggregate(reports_dir: Path):
    """聚合所有轮次，返回摘要字典。"""
    rounds = []
    for child in sorted(reports_dir.iterdir()):
        if child.is_dir() and not child.name.startswith("_"):
            rounds.append(load_round(child))

    total = len(rounds)
    completed = sum(1 for r in rounds if r["outcome"] == "completed")
    crashes = sum(1 for r in rounds if r["outcome"] == "crash")
    hangs = sum(1 for r in rounds if r["outcome"] == "hang")

    # 问题去重：按 (rule, details) 聚合
    issue_map = defaultdict(lambda: {"count": 0, "first_seen": None, "last_seen": None,
                                     "rule": "", "details": "", "type": "", "actions": set()})
    for r in rounds:
        for iss in r["issues"]:
            key = (iss.get("rule", ""), iss.get("details", ""))
            entry = issue_map[key]
            entry["count"] += 1
            entry["rule"] = iss.get("rule", "")
            entry["details"] = iss.get("details", "")
            entry["type"] = iss.get("type", "")
            entry["actions"].add(iss.get("action", ""))
            ts = r["dir"]
            if entry["first_seen"] is None or ts < entry["first_seen"]:
                entry["first_seen"] = ts
            if entry["last_seen"] is None or ts > entry["last_seen"]:
                entry["last_seen"] = ts

    unique_issues = []
    for entry in issue_map.values():
        entry["actions"] = sorted(entry["actions"])
        unique_issues.append(entry)
    unique_issues.sort(key=lambda e: -e["count"])

    # 动作覆盖率
    kind_counter = Counter()
    for r in rounds:
        kind_counter.update(r["action_kinds"])

    total_issues = sum(len(r["issues"]) for r in rounds)

    return {
        "reports_dir": str(reports_dir),
        "total_rounds": total,
        "completed": completed,
        "crashes": crashes,
        "hangs": hangs,
        "total_issues": total_issues,
        "unique_issues": len(unique_issues),
        "unique_issue_details": unique_issues,
        "action_coverage": dict(kind_counter),
    }


def print_summary(summary):
    """打印人类可读摘要到 stdout。"""
    print("=" * 60)
    print("  UXO 巡检员报告汇总")
    print("=" * 60)
    print(f"报告目录：{summary['reports_dir']}")
    print(f"总轮数：{summary['total_rounds']}")
    print(f"  正常完成：{summary['completed']}")
    print(f"  崩溃：{summary['crashes']}")
    print(f"  卡死：{summary['hangs']}")
    print(f"总问题数：{summary['total_issues']}")
    print(f"去重后唯一问题：{summary['unique_issues']}")
    print(f"动作覆盖率：{summary['action_coverage']}")

    if summary["unique_issues"]:
        print("-" * 60)
        print("唯一问题列表（按出现次数降序）：")
        for i, iss in enumerate(summary["unique_issue_details"], 1):
            print(f"  [{i}] {iss['rule']}")
            print(f"      类型：{iss['type']} | 次数：{iss['count']}")
            print(f"      首现：{iss['first_seen']} | 末现：{iss['last_seen']}")
            print(f"      细节：{iss['details']}")
            print(f"      触发动作：{', '.join(iss['actions'])}")

    # 崩溃/卡死明细
    if summary["crashes"] > 0 or summary["hangs"] > 0:
        print("-" * 60)
        print("异常轮次明细：")
        for iss in summary["unique_issue_details"]:
            pass  # 异常已在轮次统计中
    print("=" * 60)


def main():
    reports_dir = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).parent / "reports"
    if not reports_dir.exists():
        print(f"错误：报告目录不存在：{reports_dir}", file=sys.stderr)
        return 1

    summary = aggregate(reports_dir)
    print_summary(summary)

    summary_file = reports_dir / "_summary.json"
    summary_file.write_text(json.dumps(summary, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"\n机器可读摘要已写入：{summary_file}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
