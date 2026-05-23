# Theme & Visual Optimization Design

Date: 2026-05-23

## Scope

Visual overhaul of the Fluent dark/light theme palettes and title bar button styling, plus two functional additions (system theme following and preference persistence). Clean up stale archive docs.

## Visual Changes

### Dark Theme (Fluent Dark)

| Token | Old | New |
|-------|-----|-----|
| Window background | #202020 | #191919 |
| Title bar background | #2b2b2b solid | #2d2d2d, subtle gradient #242424 → #1e1e1e |
| Title bar border | #424242 | #2a2a2a |
| Text primary | #f1f1f1 | #fafafa |
| Text content | #d2d2d2 | #e0e0e0 |
| Button hover (min/max) | #444444 opaque | rgba(255,255,255,0.08) semi-transparent |
| Button pressed (min/max) | #545454 opaque | rgba(255,255,255,0.12) semi-transparent |
| Close hover | #e81123 | #c42b1c |
| Close pressed | #b50f1a | rgba(196,43,28,0.7) |
| Button disabled text | text.lighter(160) | rgba(255,255,255,0.35) |

### Light Theme (Fluent Light)

| Token | Old | New |
|-------|-----|-----|
| Window background | #f3f4f6 | #f5f5f5 |
| Title bar background | #ffffff | #fafafa → #f3f3f3 gradient |
| Title bar border | #c7ced8 | #e0e0e0 |
| Text primary | #222222 | #1a1a1a |
| Text content | #444444 | #3a3a3a |
| Button hover (min/max) | #e8e8e8 opaque | rgba(0,0,0,0.04) semi-transparent |
| Button pressed (min/max) | #d8d8d8 opaque | rgba(0,0,0,0.08) semi-transparent |
| Close hover | #e81123 | #c42b1c |
| Close pressed | #b50f1a | rgba(196,43,28,0.7) |
| Button disabled text | text.lighter(160) | rgba(0,0,0,0.25) |

### Title Bar Layout

| Token | Old | New |
|-------|-----|-----|
| Height | 36px | 44px |
| Button size | 40×25px | 46×30px |
| Button border-radius | 3px | 5px |
| Button spacing | 0px | 4px |
| Left padding | 10px | 14px |
| Right padding | 6px | 12px |
| Button glyph font size | 9px | 10px |

### Button State Transitions

- Hover/pressed backgrounds use rgba semi-transparent overlays instead of opaque solid colors
- Close button uses #c42b1c (Win11 red) instead of #e81123
- 5px border-radius matches Win11 native title bar button corners
- Button icons remain Segoe MDL2 Assets (Minimize: , Maximize: , Restore: , Close: )

## Functional Additions

### Follow System Theme

- New method: `FramelessWindow::setFollowSystemTheme(bool enabled)`
- New getter: `FramelessWindow::followsSystemTheme() const`
- Implementation: listen to `WM_SETTINGCHANGE` with `"ImmersiveColorSet"` registry change detection in `NativeMessageRouter`, auto-call `setThemeMode()` when system preference changes
- When enabled, manual `setThemeMode()` calls still work and temporarily override the system value for the session

### Theme Preference Persistence

- On `setThemeMode()` call, persist to `QSettings("better-frameless-window", "settings")` under key `"theme/mode"` (value: "light" or "dark")
- On `FramelessWindow` construction, if `followSystemTheme` is false, restore from QSettings
- When `followSystemTheme` is true, system preference takes precedence over stored value

## Cleanup

- Delete `docs/archives/` directory entirely
- The valid information in those files is already captured in this design and the existing README

## Files to Modify

| File | Change |
|------|--------|
| `src/thememanager.cpp` | Update color palette values (dark & light) |
| `src/thememanager.h` | No API change needed |
| `src/titlebar.h` | Update height constant |
| `src/titlebar.cpp` | Update layout dimensions, button sizes, spacing |
| `src/framelesswindow.h` | Add `setFollowSystemTheme` / `followsSystemTheme` |
| `src/framelesswindow.cpp` | Add follow-system-theme logic, QSettings persistence |
| `src/platform/win32/nativemessagerouter.h` | Add WM_SETTINGCHANGE routing |
| `src/platform/win32/nativemessagerouter.cpp` | Handle ImmersiveColorSet notification |

## Out of Scope

- Title bar button visibility configuration (postponed)
- Bug fixes for known issues in note.md (deferred to discovery)
