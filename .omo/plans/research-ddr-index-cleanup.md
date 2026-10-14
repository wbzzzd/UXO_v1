# Research And DDR Index Cleanup

状态：Approved for execution

## 目标

在不修改研究报告正文、DDR 正文和核心基线内容的前提下，补齐 research 导航、归位一份调研报告，并校正 DDR 索引对历史材料权威性的表述。

## 范围

允许变更：

- 新增 `docs/research/README.md`。
- `git mv docs/未爆弹排除技术详细调研报告_v2.0.md docs/research/未爆弹排除技术详细调研报告_v2.0.md`，保持字节不变。
- 更新 `docs/README.md` 中 research 入口和被移动报告链接。
- 只更新 `docs/ddr/README.md` 的索引列名、八项定位和定位说明。

禁止变更：

- `docs/ddr/DDR-*.md` 正文。
- 所有研究报告正文，包括无人机 v1/v2。
- `docs/OMO_ADAPTATION_PLAN.md`、`docs/排弹抢修系统_硬件功能设计说明书_v2.2.md`（文件名 v2.2、正文 V2.3 的版本不一致文件）。
- UI、PRODUCT、ARCHITECTURE、DEVELOPMENT、docs/dev、.opencode、源码、CMake、测试。
- 不提交、不推送、不删除内容。

## Todos

- [x] 1. 记录当前 Git 状态和受保护路径基线
  - 确认前一阶段五个 staged `R100` 不变。
  - 记录待移动报告的 SHA-256 与 Git blob。
  - 确认新 README 和目标路径尚不存在。
  - QA：运行 `git status --short --untracked-files=all`、`git diff --cached --raw -M100%`、`sha256sum docs/未爆弹排除技术详细调研报告_v2.0.md`、`git hash-object docs/未爆弹排除技术详细调研报告_v2.0.md`。生成受保护文件清单：将固定文件 `SRS排弹抢修指挥系统_v1.0.md`、`SDD排弹抢修指挥系统_v1.0.md`、`docs/UI.md`、`docs/PRODUCT.md`、`docs/ARCHITECTURE.md`、`docs/DEVELOPMENT.md`、`CMakeLists.txt`、`docs/OMO_ADAPTATION_PLAN.md`、`docs/前端UI详细设计文档.md`、`docs/排弹抢修系统_产品形态设计说明书_v1.0.md`、`docs/排弹抢修系统_硬件功能设计说明书_v2.2.md`、`docs/硬件设备技术规格说明书_v1.0.md` 与 `git -c core.quotePath=false ls-files --cached --others --exclude-standard -- docs/ui docs/dev .opencode src include tests docs/research docs/ddr` 输出合并、排序去重，并用标准 `grep -F -v` 排除本阶段允许变化的 `docs/research/README.md`、`docs/research/未爆弹排除技术详细调研报告_v2.0.md`、`docs/ddr/README.md`；以 `while IFS= read -r file; do sha256sum "$file"; done` 对每个路径哈希，保存排序结果到仓库外 `/tmp/uxo-phase2-protected.before.sha256`。预期：中文路径不被 Git 转义，清单覆盖 tracked 与 untracked 文件，包括六份既有 research 报告、全部 DDR 正文、`docs/dev/README.md`、`prd-mos.md`、`prd-uxr.md` 以及全部固定来源资料；五个既有重命名仍为 `R100`；源报告存在；新 research README 与目标报告不存在；记录报告 SHA/blob 供任务 4 对比。

- [x] 2. 移动调研报告并新增 research 索引
  - 使用 `git mv` 完成单一移动，不编辑报告内容。
  - `docs/research/README.md` 明确 research 仅为证据与设计输入，核心基线冲突时以核心基线为准。
  - 索引七份资料；只在索引中标明无人机 v1 已由 v2 修订。
  - 验收：目标存在、旧路径消失、移动为 `R100`、报告 SHA/blob 不变、七个链接均存在。
  - QA：运行 `test -e docs/research/README.md`、`test -e docs/research/未爆弹排除技术详细调研报告_v2.0.md`、`test ! -e docs/未爆弹排除技术详细调研报告_v2.0.md`、`git diff HEAD -M100% --summary -- docs/未爆弹排除技术详细调研报告_v2.0.md docs/research/未爆弹排除技术详细调研报告_v2.0.md`，并对 README 表内七个相对链接逐个执行 `test -e`。预期：三个路径断言退出 0、报告显示 `rename ... (100%)`、七个目标全部存在、SHA/blob 与任务 1 相同。

- [x] 3. 更新 docs 导航和 DDR 索引定位
  - `docs/README.md` 的 research 行链接到 `research/README.md`，调研报告链接改到 `research/`。
  - `docs/ddr/README.md` 将“有效性”改为“定位”，八项分别标为历史评审、历史分析、讨论草案、未执行计划、部分废止、超范围全系统分析、非核心基线全系统原则、非 NEXT 技术可行性来源。
  - 不改 DDR 链接、标题、日期和正文。
  - QA：运行 `grep -E -n 'research/README.md|research/未爆弹排除技术详细调研报告_v2.0.md' docs/README.md`，预期各命中一次；运行 `if grep -E -n '✅ 有效|⚠️ 部分修订|有效性' docs/ddr/README.md; then exit 1; fi`，预期退出 0；对下方八条定位文案逐条运行 `grep -F -c`，预期每条计数为 1；运行 `git diff HEAD --quiet -- 'docs/ddr/DDR-*.md'`，预期退出 0。

- [x] 4. 独立验证路径、链接、历史和范围
  - 校验报告 SHA/blob 与 `R100`。
  - 校验 research 七个链接和 docs 导航。
  - 校验 DDR 八项定位各出现一次，旧“✅ 有效/⚠️ 部分修订”消失。
  - 校验 `git diff HEAD --check`、`git diff --cached --check`。
  - 校验所有禁止路径无新增差异，前一阶段改动与 `.omo` 状态未被覆盖或暂存。
  - QA 路径：`test -e docs/research/README.md && test -e docs/research/未爆弹排除技术详细调研报告_v2.0.md && test ! -e docs/未爆弹排除技术详细调研报告_v2.0.md`，预期退出 0。
  - QA 历史：运行 `sha256sum`、`git hash-object`、`git diff HEAD -M100% --summary`，预期与任务 1 SHA/blob 一致且新移动为 `100%`；原有五个 `R100` 仍存在。
  - QA 链接：解析 `docs/research/README.md` 和 `docs/README.md` 中本轮新增/修改的相对链接，并逐个 `test -e`，预期全部退出 0。
  - QA 文案：对八条定位逐条 `grep -F -c`，预期均为 1；旧标签使用 `if grep -E '✅ 有效|⚠️ 部分修订|有效性' docs/ddr/README.md; then exit 1; fi`，预期退出 0。
  - QA 范围：按任务 1 完全相同的规则重新生成 `/tmp/uxo-phase2-protected.after.sha256`，运行 `cmp /tmp/uxo-phase2-protected.before.sha256 /tmp/uxo-phase2-protected.after.sha256`，预期退出 0，证明 tracked、untracked 受保护文件和六份既有 research 正文均无变化；运行 `git diff HEAD --check`、`git diff --cached --check`，预期退出 0；运行 `if git diff --cached --name-only | grep -E '^\.omo/'; then exit 1; fi`，预期退出 0；删除两个 `/tmp/uxo-phase2-protected.*.sha256` 并确认不存在。

## DDR 定位文案

- DDR-001：`历史评审（部分技术债仍有参考价值）`
- DDR-002：`历史硬件依赖分析（当前 MVP 范围已由核心基线取代）`
- DDR-003：`讨论草案（未执行）`
- DDR-004：`历史计划（未执行）`
- DDR-005：`部分废止（QVariantMap extensions 已由 DDR-007 废除）`
- DDR-006：`全系统架构分析（超出当前客户端范围）`
- DDR-007：`全系统架构原则（非核心基线权威）`
- DDR-008：`技术可行性来源（不属于当前 NEXT）`

## 完成条件

- 仅允许范围内文件发生新变化。
- 报告移动保持 100% Git rename 和字节一致。
- 所有本地链接存在。
- DDR 索引不再把历史、未执行或超范围材料描述为当前有效权威。
- 不提交、不推送。
