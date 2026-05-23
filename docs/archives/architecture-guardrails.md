# Architecture Guardrails (Refactor Gate)

## Scope
- Project: better-frameless-window
- Goal: prevent expectation-mismatch bugs during refactor by enforcing stable behavior contracts.

## Invariants (Must Hold)
1. Single native message entry:
- `FramelessWindow::nativeEvent` only does event-type filtering and delegates to platform router.

2. Single visual refresh entry:
- state-triggered visual refresh must go through `VisualRefreshCoordinator::requestRefresh`.
- direct setter-triggered visual changes must enter through `FramelessWindow::requestVisualRefresh`.

3. Single source of hit-test truth:
- non-client hit-testing policy is implemented in `platform/win32/windowhittestwin.*`.

4. Platform command/frame isolation:
- system menu logic is implemented in `platform/win32/systemmenuwin.*`.
- native frame sync and DWM refresh are implemented in `platform/win32/windowframewin.*`.
- maximize/restore system command is implemented in `platform/win32/windowcommandwin.*`.

5. Layer direction:
- `ui/container` -> `core` -> `platform/win32`.
- Utility layer must not depend on `TitleBar` or other UI widgets.

6. Behavior baseline:
- maximize/restore, system menu, Snap hit target, resize edges, theme/effect switching remain functionally equivalent unless explicitly accepted.

## Phase Gate Checklist

### Gate A: Build
- Debug build passes.
- Release build passes.

### Gate B: Routing
- `FramelessWindow` contains no WM_* switch logic.
- router owns WM_* handling.

### Gate C: Visual Pipeline
- no direct flush state machine fields in `FramelessWindow`.
- coordinator controls pass count and reschedule.

### Gate D: Hit Test
- `FramelessWindow::hitTest` delegates to platform hit-test module.
- title bar region mapping remains explicit and testable.

### Gate E: Manual Regression
1. Drag title bar moves window.
2. Edge/corner resize works at normal state.
3. Double-click title bar toggles maximize/restore.
4. Right-click title bar opens system menu with correct enabled states.
5. Maximize button still supports Snap hit behavior.
6. Theme/effect transitions have no obvious flicker regression.

## Change Policy
- If any gate fails, stop further refactor and fix gate before next phase.
- Any intentional behavior change must record: trigger, expected delta, acceptance reason.
