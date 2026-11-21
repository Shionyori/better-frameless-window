# Architecture

How `better-frameless-window` is put together today. It describes the code as
it is, not as it was planned; see [`todo.md`](todo.md) for what is not done yet.

Target platform is Windows 10 1809+ / Windows 11. On Linux and macOS the same
tree builds and its tests run, but every native path is compiled out and the
window behaves like a plain frameless `QWidget` (see [Scope](../README.md#scope)).

---

## 1. Layer map

```
          ┌──────────────────────────────────────────────┐
   public │ FramelessWindow        TitleBar              │  ThemeManager
    API   │ (window state,         (chrome UI,           │  (colours → QSS)
          │  hit test, geometry)   control buttons)      │  Diagnostics
          └───────────────┬──────────────────┬───────────┘
                          │                  │
          ┌───────────────▼──────────────────▼───────────┐
 resolution│ WindowVisualState                            │  pure, testable
   (pure)  │ tokens · surface policy · DWM option struct  │  no Windows calls
          └───────────────┬──────────────────────────────┘
                          │
          ┌───────────────▼──────────────────────────────┐
  native  │ WindowEffect   NativeMessageRouter           │  Windows only
          │ (DWM attrs)    (WM_* → Qt)                   │  (#ifdef Q_OS_WIN)
          │ WindowHitTest  WindowCommand  SystemMenu     │
          │ WindowFrame    Utils (capabilities)          │
          └──────────────────────────────────────────────┘
```

Rules that keep the layers honest:

- `core/` holds no platform calls: its logic (resolution, tokens, geometry) is
  pure and therefore unit tested on any platform, which is why CI can run
  headless. The one reach into `platform/` is a convenience overload of
  `resolveSurfacePolicy()` that queries `Utils::detectWindowsCapabilities()`;
  callers that need determinism pass the capabilities in explicitly.
- `platform/win32/` is the only place that may include `<qt_windows.h>` or call
  DWM; everything it exposes is guarded by `#ifdef Q_OS_WIN`.
- `FramelessWindow` never talks to DWM directly. It asks `core/` what the
  window state *means*, then hands the result to `platform/`.

---

## 2. Rendering: one resolved surface

The hardest invariant in the library is that the Qt side and the DWM side must
describe the same surface. They are joined by `WindowVisualState::
resolveSurfacePolicy()`, the single source of truth:

```cpp
enum class SurfaceKind  { Opaque, Translucent };
struct SurfacePolicy {
    SurfaceKind kind;                    // Qt surface
    WindowEffect::BackdropKind backdrop; // what DWM draws behind it
};
```

Resolution order (highest precedence first):

| Condition | Surface | Backdrop |
|---|---|---|
| minimized | translucent kept stable | none (DWM owns the transition) |
| background image set | opaque | none — the image owns the pixels |
| `BackdropMode::None` | opaque | none |
| requested backdrop supported | translucent | Mica / Acrylic |
| requested backdrop unsupported | opaque | none |

Consequences, all covered by `tests/test_visualstate.cpp`:

- `Translucent ⇒ backdrop != None`. A see-through wash with nothing behind it
  can never be produced.
- A background image and a backdrop cannot compete: `setBackgroundImage()`
  drops the backdrop, `clearBackgroundImage()` lets it come back.
- The dark variant of the material is **not** a switch. DWM reads it from
  `DWMWA_USE_IMMERSIVE_DARK_MODE`, which is always written from `themeMode()`.
  Exposing it separately made the dark theme paint its wash over light Mica
  and turn the window flat grey.

Derived artefacts (all from the same call):

| Consumer | Uses |
|---|---|
| `applyTheme()` | `WA_TranslucentBackground`, and the wash in the theme style sheet |
| `applyVisualEffects()` | `VisualEffectOptions` (shadow/rounded/dark hint/backdrop/border) |
| `currentVisualStateToken()` | one bitfield that decides whether a refresh pass runs |
| `forceBackdropRebind()` | re-applies the material after restore / theme change |

`VisualRefreshCoordinator` coalesces refresh requests and skips the pass when
the token is unchanged, which is what keeps window moves and focus changes from
spamming DWM.

### Theme wash

`ThemeManager::buildStyleSheet()` paints `#FramelessWindow` with:

| Surface | Background |
|---|---|
| opaque | solid `#F5F5F5` / `#191919` |
| translucent | `rgba(255,255,255,0.15)` / `rgba(0,0,0,0.35)` |

The wash exists for text contrast over the material. It is also the reason
Mica can be invisible: Mica's own base colour (`#f3f3f3` / `#202020`) is within
a few levels of the solid colour, so the wash lands on nearly the same value.
See [`todo.md`](todo.md) for the open items here.

---

## 3. Window state machine

```
            ┌───────────┐  WM_ENTERSIZEMOVE   ┌──────────┐
  normal ──▶│  resizing │◀──────────────────▶│ restored │
            └───────────┘  WM_EXITSIZEMOVE   └──────────┘
                  │                              ▲   │
                  │ maximize                     │   │ minimize
                  ▼                              │   ▼
            ┌───────────┐   restore        ┌───────────┐
            │ maximized │─────────────────▶│ minimized │
            └───────────┘                  └───────────┘
```

- `NativeMessageRouter` translates `WM_NCHITTEST`, `WM_NCCALCSIZE`,
  `WM_ENTERSIZEMOVE`/`WM_EXITSIZEMOVE`, `WM_GETMINMAXINFO`, `WM_SYSCOMMAND`
  and forwards the ones that change state to `FramelessWindow`.
- Maximize and restore are animated in the library: the geometry moves to the
  target rectangle over 170ms (`InOutCubic`) and the window state flips only
  when it arrives (`WindowCommand::setMaximized()` → `ShowWindow`). Windows
  will not animate this window itself: the native transition belongs to the
  frame, and the frame is ours. `QWidget::showMaximized()` cannot be used for
  the flip either — once the geometry equals the maximized rectangle, Qt
  thinks the window is already maximized and never tells Windows.
- While resizing, the visual refresh pass is suppressed: calling
  `SetWindowPos(SWP_FRAMECHANGED)` or `DwmSetWindowAttribute` mid-drag fights
  the system loop and produces jitter.
- Restore transitions arm `beginBackdropTransitionGuard()`: the material is
  re-applied once the geometry settles, because DWM drops it during the
  transition.
- Fullscreen is handled through the same paths: titlebar hidden, no resize
  edges, `HTCLIENT` everywhere, geometry not persisted.

---

## 4. Input: one hit-test truth

`WindowHitTest` owns the geometry maths for both input paths:

- `edgesAtLocalPos()` decides cursor shape and `startSystemResize()` edges in Qt.
- `nonClientHitTest()` produces the `WM_NCHITTEST` answer for Windows.

Both call the same function, so the cursor and the native hit test cannot
disagree. HiDPI is handled by converting to logical coordinates once, using
`resizeBorderThickness()` (which is DPI-scaled).

Titlebar semantics come from `TitleBar::hitRegionAt()`, fed into the hit test
through a callback, so the native side knows exactly where
`HTCAPTION`/`HTMINBUTTON`/`HTMAXBUTTON`/`HTCLOSE` are. Snap Layout depends on
the `HTMAXBUTTON` answer; the click itself is intercepted so it maximizes once.

Two rules keep the caption usable:

- The **resize border wins over the caption buttons**. The buttons are pinned
  to the top-right corner, so without this the outer border over them would
  stop resizing (and stop being draggable at all).
- The buttons keep their **native size** (46x32) instead of stretching over a
  taller titlebar; a titlebar taller than that keeps a draggable strip below
  them.

Double-click on the caption is intercepted (`WM_NCLBUTTONDBLCLK` with
`HTCAPTION`) and routed through the same animated transition as the button.
DefWindowProc would otherwise maximize instantly and skip the restore
rectangle capture.

---

## 5. Theme pipeline

```
themeMode ──▶ ThemeManager (colors)
   │                │
   │                ├─▶ buildStyleSheet(translucent) ──▶ window + children
   │                └─▶ shouldUseDarkMode() ──▶ DWM dark hint ──▶ Mica/Acrylic variant
   │
   └─▶ animated 300ms via Q_PROPERTY themeTransitionProgress,
       then forceBackdropRebind() so the material re-evaluates

follow-system ──▶ setFollowSystemTheme() reads the OS preference
              └─▶ syncThemeWithSystemIfFollowing() re-reads it on demand
```

Persistence lives under the library's own `QSettings` key
(`better-frameless-window/settings`): theme mode and window geometry. It is per
library, not per application — that is a known limitation, not an accident.

---

## 6. Geometry persistence

`WindowGeometry` is pure: given the saved rectangle and the available screen
rectangles it answers "is this restorable" and "where should it go instead".
`FramelessWindow` only converts that answer into a `setGeometry()` call.
Saved geometry is validated on restore: off-screen and multi-monitor cases are
clamped, and the restore size is kept even when closing maximized. That
restore rectangle is tracked by the window itself (`rememberRestoreGeometry()`,
fed from move/resize while normal); it is preferred over
`GetWindowPlacement()`, whose "normal" rectangle the maximize animation would
otherwise overwrite with the work area.

---

## 7. Names

Names describe **what the caller controls or sees**, not where the
implementation comes from. The `system` prefix was removed for exactly this
reason — it described DWM rather than the effect:

| Name | Means |
|---|---|
| `setBackdropMode(BackdropMode)` | what the caller wants; `None` = off |
| `BackdropKind` | what DWM ends up drawing (resolved) |
| `setShadowEnabled`, `setRoundedCornersEnabled` | single, concrete effects |
| `applyDarkModeHint`, `applyBackdrop` | private DWM calls |
| `surfacePolicy()` / `usesTranslucentSurface()` | the resolved surface |
| `systemPrefersDarkTheme()` | reads the Windows preference |

`WindowEffect` only exposes `applyVisualEffects()` and `applyBackdrop()`; the
per-attribute helpers are private, so the ordering rules (shadow → corners →
dark hint → backdrop → border) stay inside the class.

---

## 8. Failure policy

- `setShadowEnabled()` only has a visible effect on an opaque surface: DWM does
  not paint the frame shadow for a layered window, and a translucent surface is
  layered. The library logs that once per state change under diagnostics.
- Every DWM attribute has a per-window validity flag: after the first failure
  it is not retried for that window (`m_darkMode20Valid`, …). This keeps
  unsupported attributes from spamming the log every refresh.
- The material degrades in a fixed order: Mica → Acrylic → none. On failure of
  the requested kind the next supported one is used, and the applied kind is
  cached so redundant calls are skipped.
- `Diagnostics::logWarning()` is silent unless `setDiagnosticsEnabled(true)`;
  it is process-wide, not per window, and never changes behaviour.

---

## 9. Testing

| Suite | Covers |
|---|---|
| `test_visualstate` | surface resolution (incl. `translucent ⇒ backdrop`), token, redundancy rules |
| `test_windowhittest` | edge detection, DPI scaling, `WM_NCHITTEST` mapping |
| `test_windowgeometry` | off-screen clamping, multi-monitor restore |
| `test_titlebar` | button visibility, hit regions |
| `test_thememanager` | palette, animated transition, style sheet |
| `test_utils` | build detection, capability flags |
| `test_version` | generated version header |

`core/` is platform-free on purpose, so the resolution rules above are tested
with synthetic capabilities on any OS. Native behaviour (materials actually
appearing, DWM accepting attributes) is verified manually — see
[`release-checklist.md`](release-checklist.md).
