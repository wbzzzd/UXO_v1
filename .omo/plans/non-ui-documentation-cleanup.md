# non-ui-documentation-cleanup - Work Plan

## TL;DR (For humans)
<!-- Fill this LAST, after the detailed plan below is written, so it summarizes the REAL plan. -->
<!-- Plain English for a non-engineer: NO file paths, NO todo numbers, NO wave/agent/tool names. -->

**What you'll get:** A clear documentation map, a status index for retained development drafts, and five superseded engineering guides preserved in a clearly labeled archive.

**Why this approach:** Only documents demonstrably replaced by the core baselines move. Everything ambiguous stays in place and receives an explicit status instead of being treated as obsolete.

**What it will NOT do:** It will not touch UI, DDR, research, requirements, source code, or remaining future-design drafts. Nothing is deleted or committed.

**Effort:** Short
**Risk:** Low - file moves can break links, so every active reference and byte identity is checked.
**Decisions to sanity-check:** Only five replaced engineering guides move; all other development material remains where it is.

Your next move: approve execution. Full execution detail follows below.

---

> TL;DR (machine): Short, low-risk Markdown reorganization with five 100% Git renames, three indexes, and targeted reference repairs.

## Scope
### Must have
- Move `docs/dev/{current-state,build-and-run,mvp-scope,architecture-boundaries,simulation-policy}.md` byte-identically to `docs/archive/development/` with `git mv`.
- Add `docs/README.md` as the canonical folder-purpose and reading-route map.
- Add `docs/dev/README.md` classifying every retained file as a decision record or Draft/source/design input.
- Update `docs/archive/README.md` with the five original paths, destinations, replacement authorities, and provenance rule.
- Repair only active links broken or made misleading by the moves.
### Must NOT have (guardrails, anti-slop, scope boundaries)
- Do not modify `docs/UI.md`, `docs/ui/**`, `docs/前端UI详细设计文档.md`, `docs/dev/mos-ui-design-brief.md`, or `docs/dev/product-design-mos.md`.
- Do not modify DDRs, research, requirements, architecture content, source code, build files, or tests.
- Do not delete content, rewrite moved files, promote Draft material, commit, or push.

## Verification strategy
> Zero human intervention - all verification is agent-executed.
- Test decision: tests-after using shell path assertions, `cmp`, `rg`, optional offline `lychee`, and Git diff checks; no product tests are required for documentation-only moves.
- Evidence: <attemptDir>/task-<N>-non-ui-documentation-cleanup.<ext> (attemptDir = currentAttemptDir from 'omo ulw-loop status --json', .omo/evidence/ulw/<session>/<goalId>/a<attempt>; outside ulw-loop use .omo/evidence/)

## Execution strategy
### Parallel execution waves
> Target 5-8 todos per wave. Fewer than 3 (except the final) means you under-split.

- Wave 1: preflight, failure assertions, five Git moves.
- Wave 2: create the three navigation/index documents.
- Wave 3: repair active references in non-overlapping files.
- Wave 4: run independent path, provenance, link, exclusion, and scope verification.

### Dependency matrix
| Todo | Depends on | Blocks | Can parallelize with |
| --- | --- | --- | --- |
| 1 | None | 2, 3, 4, 5 | None |
| 2 | 1 | 5 | 3, 4 |
| 3 | 1 | 5 | 2, 4 |
| 4 | 1 | 5 | 2, 3 |
| 5 | 2, 3, 4 | Final verification | None |

## Todos
> Implementation + Test = ONE todo. Never separate.
<!-- APPEND TASK BATCHES BELOW THIS LINE WITH edit/apply_patch - never rewrite the headers above. -->
- [x] 1. Move exactly five superseded development baselines without content changes
  What to do / Must NOT do: Preflight `git status`; create `docs/archive/development/`; use `git mv` for `current-state.md`, `build-and-run.md`, `mvp-scope.md`, `architecture-boundaries.md`, and `simulation-policy.md`. Do not edit moved bytes or stage unrelated `.omo/**` files.
  Parallelization: Wave 1 | Blocked by: None | Blocks: 2, 3, 4, 5
  References: `docs/archive/README.md:3-14`; `docs/ARCHITECTURE.md:18-135`; `docs/DEVELOPMENT.md:15-102`; `docs/PRODUCT.md:1-220`.
  Acceptance criteria: All five destinations exist, all five old paths do not exist, `git diff HEAD -M100% --summary` reports five 100% renames, and each destination matches `git show HEAD:<old-path>` via `cmp`.
  QA scenarios: Happy: run path loop, five `cmp` commands, and rename summary. Failure: any pre-existing destination, changed bytes, or non-100% rename stops work. Evidence `.omo/evidence/task-1-non-ui-documentation-cleanup.txt`.
  Commit: N | User did not request a commit.

- [x] 2. Add the canonical docs map and retained-development status index
  What to do / Must NOT do: Add `docs/README.md` explaining core baselines and every docs folder; add `docs/dev/README.md` classifying every retained file. Do not duplicate requirement statuses or detailed DDR/research/UI indexes.
  Parallelization: Wave 2 | Blocked by: 1 | Blocks: 5 | Can parallelize with: 3, 4
  References: `README.md:9-24`; `AGENTS.md:5-21`; `docs/ddr/README.md:1-19`; `docs/features/README.md`; `docs/ui/README.md:1-49`; retained files under `docs/dev/`.
  Acceptance criteria: `docs/README.md` links all core baselines and folders; `docs/dev/README.md` lists every retained top-level file and `decisions/`, marks all UXR/MOS material Draft/source, and states both Approved gates.
  QA scenarios: Happy: compare `rg --files docs/dev` against the README index and test every linked path exists. Failure: missing retained file or any statement that Draft content guides implementation fails. Evidence `.omo/evidence/task-2-non-ui-documentation-cleanup.txt`.
  Commit: N | User did not request a commit.

- [x] 3. Update the archive inventory and provenance contract
  What to do / Must NOT do: Update `docs/archive/README.md` to document `development/`, all five moves, original paths, replacement authorities, and retained-in-place categories. Remove only the obsolete blanket statement that all docs/dev files remain pending.
  Parallelization: Wave 2 | Blocked by: 1 | Blocks: 5 | Can parallelize with: 2, 4
  References: `docs/archive/README.md:1-14`; five moved files; four core baselines.
  Acceptance criteria: Archive README names all five moves, says bytes were unchanged, preserves the non-authoritative/history rule, and explicitly says UI, DDR, research, and remaining dev drafts stayed in place.
  QA scenarios: Happy: `rg` finds all five filenames and four core-baseline links. Failure: any deletion permission, current-authority claim, or omitted retained category fails. Evidence `.omo/evidence/task-3-non-ui-documentation-cleanup.txt`.
  Commit: N | User did not request a commit.

- [x] 4. Repair active navigation and references made stale by the moves
  What to do / Must NOT do: Replace root README's `docs/dev/current-state.md` link with `docs/README.md`; update `AGENTS.md` and `docs/DEVELOPMENT.md` routing; update active references in `docs/dev/incremental-prd-uxo-recognition-and-mos.md`, `docs/dev/architecture-uxo-recognition-and-mos.md`, `.opencode/commands/analyze-current.md`, and `.opencode/commands/plan-mvp.md`. Leave archived snapshots and historical decisions unchanged.
  Parallelization: Wave 3 | Blocked by: 1 | Blocks: 5 | Can parallelize with: 2, 3
  References: `README.md:9-24`; `AGENTS.md:5-21`; `docs/DEVELOPMENT.md:148-160`; old-path matches in the two retained Draft files and two active `.opencode/commands/` templates.
  Acceptance criteria: No active guidance references the five old paths; historical matches remain only in archived snapshots, `docs/OMO_ADAPTATION_PLAN.md`, and `docs/dev/decisions/0001-opencode-omo-workflow.md`.
  QA scenarios: Happy: scoped `rg` returns no obsolete active matches and all replacement targets pass `test -e`. Failure: excluded UI path changes or historical content rewrites fail. Evidence `.omo/evidence/task-4-non-ui-documentation-cleanup.txt`.
  Commit: N | User did not request a commit.

- [x] 5. Validate links, scope, provenance, and clean handoff
  What to do / Must NOT do: Run path assertions, content identity checks, active-reference scans, optional offline `lychee`, UI exclusion guard, `git diff HEAD --check`, rename summary, and final status. Do not install tools if `lychee` is absent. The required target ledger is:
  `test -e docs/README.md`; `test -e docs/dev/README.md`; `test -e docs/archive/README.md`; `test -e docs/archive/development/current-state.md`; `test -e docs/archive/development/build-and-run.md`; `test -e docs/archive/development/mvp-scope.md`; `test -e docs/archive/development/architecture-boundaries.md`; `test -e docs/archive/development/simulation-policy.md`; `test -e docs/PRODUCT.md`; `test -e docs/ARCHITECTURE.md`; `test -e docs/UI.md`; `test -e docs/DEVELOPMENT.md`. Each must exit 0. The five corresponding `test ! -e docs/dev/<name>` checks must also exit 0.
  Parallelization: Wave 4 | Blocked by: 2, 3, 4 | Blocks: Final verification
  References: All files changed by todos 1-4; initial `git status` evidence.
  Acceptance criteria: All checks exit 0; five renames remain 100%; no UI/DDR/research/source files changed; pre-existing `.omo/**` files remain untracked and unstaged; unavailable fragment validation is reported explicitly.
  QA scenarios: Happy: execute the full command ledger and save outputs. Failure: broken path, stale active reference, whitespace issue, extra changed file, or staged `.omo/**` blocks completion. Evidence `.omo/evidence/task-5-non-ui-documentation-cleanup.txt`.
  Commit: N | User did not request a commit.

## Final verification wave
> Runs in parallel after ALL todos. ALL must APPROVE. Surface results and wait for the user's explicit okay before declaring complete.
- [x] F1. Plan compliance audit
  Tool and steps: Use `git diff HEAD --name-status -M100%`, the five destination/old-path assertions, `git diff HEAD -M100% --summary`, and `git status --short --untracked-files=all` for new README files. Compare every changed and untracked path against the plan's exact allowlist plus the pre-existing `.omo/**` allowlist.
  Approval result: APPROVE only when exactly five 100% renames and the named index/reference files changed; no missing planned file and no extra path.
- [x] F2. Documentation quality review
  Tool and steps: Read all changed Markdown; use `rg -n 'Approved|Implemented|当前事实|指导实现' docs/README.md docs/dev/README.md docs/archive/README.md` and inspect every match in context. Verify each retained dev item has one classification and no copied requirement status.
  Approval result: APPROVE only when authority, Draft, archive, and research boundaries are unambiguous and no index promotes source material to implementation authority.
- [x] F3. Navigation and link QA
  Tool and steps: Run every concrete `test -e` and `test ! -e` command listed in todo 5; run offline `lychee --include-fragments` on changed navigation/reference files when available, otherwise manually resolve every changed relative link against its parent directory and record the unavailable tool.
  Approval result: APPROVE only when every changed local link resolves, every intended entry point opens an existing file/folder, and no old active path remains.
- [x] F4. Scope fidelity and safety audit
  Tool and steps: Run `git diff HEAD --quiet -- docs/UI.md docs/ui docs/前端UI详细设计文档.md docs/dev/mos-ui-design-brief.md docs/dev/product-design-mos.md docs/ddr docs/research src include CMakeLists.txt`; run `git diff HEAD --check`; compare final `git status --short --untracked-files=all` with preflight status.
  Approval result: APPROVE only when all exclusions are unchanged, whitespace checks pass, no `.omo/**` artifact is staged, and no code/build/test/requirement file changed.

## Commit strategy

No commit is created without a separate explicit request. If later requested, use atomic documentation commits: five move-only commits grouped by responsibility, one index commit, one retained-Draft reference commit, and one active-routing commit. Never stage `.omo/**` runtime artifacts.

## Success criteria

- Exactly five byte-identical files move into `docs/archive/development/`.
- Every other `docs/dev` file stays in place and is classified by `docs/dev/README.md`.
- `docs/README.md` answers where each documentation category belongs and which files are authoritative.
- All active references resolve; historical references remain intact for provenance.
- UI, DDR, research, requirements, architecture content, source, build, tests, and user `.omo/**` artifacts remain unchanged.
- Verification passes with no commit or push.
