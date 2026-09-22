# 工作会话记录 2026-09-22

## 会话目标

用户要求清理长期功能开发积累的分支与工作树。本会话先做只读调查评估，逐项与用户确认后执行清理；未确认项一律保留，未做任何未授权删除。

## 调查结论（执行前基线）

- 16 个工作树（11 个真实目录 + 4 个 /tmp 失效注册 + 主工作树）、18 个本地分支、14 个远端分支、3 个 stash。
- 4 个 /tmp/opencode 工作树注册指向已不存在的目录（prunable，零数据风险）。
- 12 个本地分支已合入 origin/main（PR #13/#18/#19/#20 等）且对应工作树干净。
- `UXO_v1-continuation`（feature/command-workflow）工作树含 27 个修改 + 6 个新文件 ≈ +1456 行未提交的 Approved 功能（指挥业务闭环，`CommandWorkflow.h/.cpp`、测试、`docs/features/command-workflow.md`），已验证这些文件从未提交到任何分支。
- 主工作树 6 份 `.omo/plans/` 计划文档为本地孤本（不在任何分支/远端）。
- 4 个未合并分支：`backup/mos-docs-pre-rebase`(9ccefef)、`recovery/accidental-docs-mos-integration`(b63614f)（07-30 事故快照）、`docs/product-md-layering-split`(f175297)（已验证被 main 经 dev-dissolve 等效取代且 main 更全）、`feature/prototype-responsive-viewport`(8549702)（1 个独有提交，Qt 版已合入）。

## 用户决策与执行

用户逐项确认后执行：

1. `git worktree prune` 清除 4 个 /tmp 失效注册。
2. 删除 7 个干净工作树：auto-inspect / database / detection-onnx / dev-dissolve / docs-integration / mos-docs / ui-visual-upgrade（全部常规 remove，无需 --force）。
3. 主工作树从 feature/media-assets 切回 main 并 fast-forward 到 936b87e（origin/main，含 PR #19/#20 共 9 个提交）。
4. 补录 6 份孤本计划文档入库（提交 8f51d72，979 行）。
5. `git branch -d` 删除 13 个已合并分支（12 个既定 + feature/media-assets，全部通过 -d 安全校验）。

## 保留项（用户明确要求不动）

- `UXO_v1-continuation` / feature/command-workflow：Approved 指挥业务闭环 WIP，继续开发；未提交内容原样保留。
- `UXO_v1-detection` / feature/detection-stage-demo：`visual-evidence/` 截图证据（800K，从未入库）保留。
- `UXO_v1-product-split` / docs/product-md-layering-split：未合并分支及其干净工作树保留。
- 3 个未合并分支 backup/mos-docs-pre-rebase、recovery/accidental-docs-mos-integration、feature/prototype-responsive-viewport 保留（tip 哈希见上，reflog 90 天可恢复）。
- 远端 origin 全部分支（含 minimax_temp）、3 个 stash、/tmp/opencode 散文件（0828.bak、日志、脚本）不动。

## 验证结果

- `git worktree list`：仅剩 4 个（主工作树[main]、continuation、detection、product-split）。
- `git branch -vv`：仅剩 7 个本地分支（main、feature/command-workflow、feature/detection-stage-demo、docs/product-md-layering-split、backup/mos-docs-pre-rebase、recovery/accidental-docs-mos-integration、feature/prototype-responsive-viewport）。
- 保留工作树脏状态与清理前一致：continuation 33 条、detection 2 条、product-split 0 条。
- stash 数量不变（3）。
- 主工作树仅剩 `.omo/ulw-research/`（运行态，按规则不入库）未跟踪。

## 附带发现

- `/home/lin/UXO_v1-integration-recovery-20260730T081101Z/`（10M）：07-30 整合事故的手工恢复快照（repository.bundle、omo.tar、补丁、校验和），非 git 工作树、不受本次清理影响，是否删除待用户决定。

## 遗留事项

- 本地 main 领先 origin/main 2 个提交（计划补录 + 本记录）；按项目规则 main 走 PR，推送方式待用户决定。
- continuation 工作树 1456 行 WIP 未提交，后续按任务粒度提交。
- 远端 13 个已合并特性分支 + minimax_temp 可另行清理（需明确授权）。
- stash@{0} 自述"已被 main 正式版取代"，可连同其余 2 个旧 stash 另行清理。
- `.omo/ulw-research/` 未被 .gitignore 覆盖，可考虑后续补充忽略规则。
