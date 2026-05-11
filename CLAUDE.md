# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

```bash
cmake --preset windows-msvc-local-release
cmake --build build/windows-msvc-local-release
```

A debug preset (`windows-msvc-local-debug`) also exists. The build produces a static library (`better-frameless-window`) and an example executable (`bfw-example`). Requires Qt 6.x (Core, Gui, Widgets), CMake 3.16+, MSVC 2022.

## Architecture

The library provides a single top-level widget class, `FramelessWindow`, that users instantiate directly. It's a `QWidget` subclass with `Qt::FramelessWindowHint`, replacing the native title bar entirely.

### Core classes

- **`FramelessWindow`** (`src/framelesswindow.h`) — top-level frameless window widget. Owns a `TitleBar`, a content panel, `ThemeManager`, and `WindowEffect`. All visual property setters (`setSystemShadowEnabled`, `setSystemBackdropPreference`, `setRoundedCornersEnabled`, `setSystemDarkModeEnabled`, `setThemeMode`, `setAccentColor`) follow the same pattern: guard against no-op, store the value, then call `requestVisualRefresh()`.

- **`TitleBar`** (`src/titlebar.h`) — custom titlebar (fixed 36px height) with window title label, minimize/maximize/close buttons (Segoe MDL2 Assets glyphs), a center extension slot (`addCenterWidget`), and drag-to-move via `systemMoveRequested` signal. Button hover/press states use QGraphicsOpacityEffect animations driven by an event filter that syncs from cursor position.

- **`NativeMessageRouter`** (`src/platform/win32/nativemessagerouter.h`) — static class that intercepts Windows messages in `FramelessWindow::nativeEvent`. Handles WM_NCHITTEST (forwards to DWM first for snap layouts, then falls back to our hit-test), WM_NCCALCSIZE, WM_SIZE (detects maximize→restore transitions and triggers backdrop recovery), WM_ENTERSIZEMOVE/WM_EXITSIZEMOVE (tracks resize state to suppress DWM calls during system resize), and WM_NC*-button messages (routes to our custom buttons).

- **`VisualRefreshCoordinator`** (`src/core/visualrefreshcoordinator.h`) — coalesces visual refresh requests via zero-timer (QTimer::singleShot(0)). On flush, it computes a state token before and after running the refresh pass. If state changed during the pass, it runs up to 3 convergence passes. The token transition handler forces DWM refresh when tokens diverge.

- **`WindowVisualState`** (`src/core/windowvisualstate.h`) — namespace of pure functions that compute visual state from configuration. `buildVisualStateToken()` packs visibility, maximize/minimize/active state, all effect booleans, backdrop preference, and theme mode into a `quint64` bitmask. `buildVisualEffectOptions()` assembles the `VisualEffectOptions` struct for `WindowEffect::applyVisualEffects`. `shouldUseTranslucentBackground()` checks Windows capability detection against the requested backdrop preference.

- **`ThemeManager`** (`src/thememanager.h`) — holds Light/Dark mode and accent color. `buildStyleSheet()` generates the full Qt stylesheet string with theme-appropriate colors for the window background, titlebar, buttons, and content labels.

- **`WindowEffect`** (`src/platform/win32/windoweffect.h`) — wraps DWM API calls (`DwmSetWindowAttribute`, `DwmExtendFrameIntoClientArea`). Applies shadow, rounded corners, dark mode titlebar, and backdrop (Mica/MicaLegacy/Acrylic) effects. Has per-instance caching (`m_lastAppliedMode`) to avoid redundant DWM calls; force-rebinds during restore transitions bypass this cache by clearing to None first, then re-applying.

### Platform layer (`src/platform/win32/`)

Each namespace wraps a narrow Windows API concern:
- `WindowHitTest` — converts local mouse position to NC hit-test result (HTLEFT, HTRIGHT, HTTOP, etc.) for 8-direction resize, with HiDPI awareness and titlebar region delegation. Also provides `resizeBorderThickness()`.
- `WindowFrame` — `syncWindowFrame()` adjusts window style flags (WS_THICKFRAME, WS_CAPTION) for proper resize borders. `forceDwmRefresh()` calls `SetWindowPos(SWP_FRAMECHANGED)` to force DWM to re-evaluate the window.
- `WindowCommand` — `toggleMaximizeRestore()` via `ShowWindow(SW_MAXIMIZE)`/`ShowWindow(SW_RESTORE)` + `SendMessage(WM_SYSCOMMAND)`.
- `SystemMenu` — displays the native system menu via `GetSystemMenu` + `TrackPopupMenu`.
- `Utils` — Windows build number detection and capability flags (supports Mica, Acrylic, dark mode, rounded corners).

### Visual refresh pipeline

```
User changes property → requestVisualRefresh()
  → if not visible: performVisualRefreshPass() + update() immediately
  → if visible: VisualRefreshCoordinator::requestRefresh()
    → zero-timer coalescing
    → flush():
      1. compute state token (before)
      2. performVisualRefreshPass():
         a. applyTheme() — rebuilds stylesheet, sets WA_TranslucentBackground
         b. if not m_resizing: syncNativeWindowFrame() + applyVisualEffects()
         c. updateMaximizeButtonState()
      3. compute state token (after)
      4. if changed: forceNativeDwmRefresh() + update()
      5. if still dirty or token changed: repeat up to 3 passes
```

During system resize (`m_resizing == true`, set by WM_ENTERSIZEMOVE/WM_EXITSIZEMOVE), steps 2b (frame sync + DWM effects) are skipped to prevent visual jitter.

### Backdrop recovery after maximize→restore

The `FramelessWindow::shouldStartRestoreTransitionFromSizeState()` method tracks `m_lastNativeSizeMaximized` to detect maximize→restore transitions. When detected:
1. `beginSystemBackdropTransitionGuard()` sets a guard flag and schedules a 160ms deferred callback
2. The callback calls `forceSystemBackdropRebind()` which clears backdrop to None, then re-applies the target — this forces DWM to treat it as a genuine state transition, recovering the backdrop

### Conditional compilation

All Win32 platform code is guarded with `#ifdef Q_OS_WIN` / `#else` (empty stubs). Non-Windows platforms get a frameless window without native DWM effects.
