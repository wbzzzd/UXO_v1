# Frozen Detection Evidence and Sidebar Redesign

Status: Approved by user on 2026-08-07
Scope: REQ-009 local simulation only

## Decisions

- Live video keeps only UAV identity, telemetry, crosshair, and REC. It never renders target rectangles or labels.
- A detection copies the current decoded frame, annotates that copy, and retains it in `MainWindow` memory by target ID.
- Sidebar or map selection shows evidence only in `TargetDetailOverlay`; reset clears evidence and End preserves it.
- The target list has no checkbox or double-click selection path. Row click is the sole selection action and uses one green selection treatment.
- No image files, persistence, QVideoProbe, real detector, or generic store/event bus are added.

## TODOs

- [x] Extract `DESIGN.md` from current `GlobalStyle` and document target-list/evidence-detail primitives - expect tokenized implementation contract
- [x] Add immutable frame-snapshot access and make `VideoOverlayWidget` HUD-only - expect no live detection-box APIs or hit testing
- [x] Store annotated in-memory evidence and render it in `TargetDetailOverlay` on target selection - expect frozen image with provenance and timing metadata
- [x] Simplify `LeftPanelWidget` to one row-selection model without checkbox or duplicate selected states - expect one clear selection treatment
- [x] Replace obsolete tests and align REQ-009/CURRENT documentation - expect red-to-green coverage for evidence lifecycle and sidebar selection
- [x] Build, run CTest, capture XCB visual evidence, and obtain independent review - expect live HUD-only plus selected frozen evidence detail at required resolutions

## Verification

- Build `UXOMissionControl` and all affected test targets.
- Assert snapshot deep-copy behavior, no live target-box calls, selection-driven evidence, End preservation, and Reset clearing.
- Run XCB visual captures at 1280x720, 1920x1080, and 3840x2160 for live HUD, unselected detection, sidebar selection, map selection, End, and Reset.
- Run `git diff --check`; preserve unrelated dirty worktree changes.
