# Todo

Open work, newest decisions first. Completed items are not kept here — they
live in the commit history and in [`architecture.md`](architecture.md).

## Rendering / materials

- [ ] **Add `BackdropMode::MicaAlt` (`DWMSBT_TABBEDWINDOW`).**
      `Auto` stays on plain Mica. Motivation: on a light wallpaper plain Mica
      renders within ~2 RGB levels of the solid background, while the tabbed
      variant measured 77 vs 21 (dark theme) — i.e. the only built-in material
      that is visibly different from `None` without changing the theme wash.
      Where: `WindowEffect::BackdropKind`, `applyBackdrop()`,
      `WindowVisualState::resolveSurfacePolicy()`, tests, README.

- [ ] **Map `BackdropMode::Acrylic` to `DWMSBT_TRANSIENTWINDOW` on Windows 11.**
      Today Acrylic uses the legacy accent policy
      (`ACCENT_ENABLE_ACRYLICBLURBEHIND`), which no longer blurs on Windows 11
      and renders as a flat tint. `DWMSBT_TRANSIENTWINDOW` does blur what is
      behind the window. Keep the accent-policy path for Windows 10.

- [ ] **Shadow for translucent surfaces.**
      DWM never paints its frame shadow for a layered window, and a translucent
      surface is `WS_EX_LAYERED` — so a Mica/Acrylic window has no shadow, and
      the layered flag survives `BackdropMode::None` (Qt keeps it once the
      window was created translucent). Either draw the shadow ourselves (a
      second layered window behind the frame, blurred and DPI-aware) or move
      the render backend to one that composites without `WS_EX_LAYERED`
      (DirectComposition / Qt Quick). Until then `setShadowEnabled()` is
      documented as opaque-surface only and the example does not expose it.

- [ ] **Re-tune the translucent theme wash** (light `rgba(255,255,255,0.15)`,
      dark `rgba(0,0,0,0.35)`).
      The wash is what keeps text readable over a material, but it also
      compresses the material's remaining contrast to 65% (dark) and makes light
      Mica land on exactly the solid colour value. Trade-off to decide: wash
      strength vs. text contrast on colourful wallpapers.

- [ ] **Mica visibility is undocumented for users.** The README explains it in
      *Known Issues*; consider whether an app-facing capability query
      (`detectWindowsCapabilities().supportsMica`) plus docs is enough, or
      whether the library should warn when Mica is requested on a build that
      cannot show it.

## Platform coverage

- [ ] Native integration on Linux (tracked as #7): shadow, rounded corners,
      no dark-mode hint, no Snap Layout.
- [ ] Native integration on macOS (tracked as #8): same list.

## Theme

- [ ] `setAccentColor()` is not animated; theme changes animate over 300ms and
      the accent jumps (tracked as #12).

## API polish

- [ ] Decide whether `backdropMode()` should report the resolved kind
      (`Mica` when `Auto` resolved to Mica) in addition to the requested mode.
- [ ] `followsSystemTheme()` only applies on demand: nothing in the library
      listens for `QStyleHints::colorSchemeChanged`, so apps have to call
      `syncThemeWithSystemIfFollowing()` themselves. Either wire it internally
      or document the requirement more prominently.
- [ ] Per-application persistence: window geometry and theme are stored under
      the library's own settings key, so two apps using the library share one
      record.

## Manual verification backlog

- [ ] Windows 10 22H2: Acrylic path (accent policy still blurs there).
- [ ] Windows 11 21H2 (build 22000–22620): backdrop now falls back to Acrylic
      since the legacy Mica path was removed — confirm the fallback looks
      acceptable.
- [ ] Multi-monitor with mixed DPI: hit test and restore geometry.
