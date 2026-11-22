# Release Checklist

Manual pass to run before tagging a release. Everything here needs a real
Windows desktop session; the automated tests only cover the pure logic.

Build both configurations and run the full suite first:

```bash
cmake --preset windows-msvc-local-debug
cmake --build build/windows-msvc-local-debug --config Debug
ctest --test-dir build/windows-msvc-local-debug -C Debug

cmake --preset windows-msvc-local-release
cmake --build build/windows-msvc-local-release --config Release
ctest --test-dir build/windows-msvc-local-release -C Release
```

CI builds with Qt 6.9.x. Verify with a Qt version as close to that as you have;
`tests/test_utils.cpp` inspects the native window style and only runs against
the real `windows` platform plugin, so keep `QT_QPA_PLATFORM=windows` on Windows.

## Window interaction

- [ ] Drag by the title bar; window follows the cursor without lag
- [ ] Drag a **maximized** window by its title bar: it restores and keeps moving
- [ ] Resize from all four edges and four corners; cursor matches what is draggable
- [ ] Resize at 150% / 200% display scaling: grab area still comfortable
- [ ] Minimize, restore from the taskbar, re-activate: title bar buttons show no stuck hover state
- [ ] `showFullScreen()`: title bar hidden, edges not resizable
- [ ] System menu (right-click title bar): opens, items enabled/disabled correctly, closes when clicking elsewhere

## Caption buttons

- [ ] Buttons sit flush with the top-right corner (46x32, native size)
- [ ] Close button hover fill reaches the window corner (no rounded gap)
- [ ] The titlebar strip below the buttons still drags the window
- [ ] The outer resize border works on top of the buttons (top/right edge)
- [ ] Hovering a button then leaving the window (through the top edge, the right edge, or straight off the desktop) clears the hover
- [ ] Buttons can be hidden individually and the layout does not collapse
- [ ] `addTitleBarWidget()` widget stays clickable and does not start window drags

## Maximize and restore

Every path uses the same 200ms transition; the window state flips at the end.

- [ ] Caption button and double-click animate, with no blank frame and no flash at the end
- [ ] Win+Up / Win+Down animate
- [ ] Dragging the window to the top edge (Aero snap) animates to the work area
- [ ] Dragging a maximized window off the top restores natively, without fighting the drag
- [ ] Restoring a minimized window from the taskbar still works
- [ ] Snap to the top by dragging, then restore: the window returns to the position it had before the drag (not to the top edge)
- [ ] A second click during the animation completes it instead of stacking; dragging the caption or an edge takes over cleanly
- [ ] A maximized window fills the work area exactly on a scaled display (150%): no desktop strip at the edges
- [ ] A maximized window never covers the taskbar

## Snap Layout

- [ ] Hover the maximize button: Snap Layout flyout appears
- [ ] Pick a zone: window snaps and the title bar still works
- [ ] Maximize → restore, then hover the button again: the flyout still appears

## Theme and backdrop

- [ ] Light ↔ Dark: 300ms transition, title bar, background and backdrop tint all follow
- [ ] System: follows the Windows setting, including a live change while running
- [ ] Mica: applied at startup, survives maximize → restore
- [ ] Acrylic: applied at startup, survives maximize → restore, tint follows the theme
- [ ] Dark theme + Mica: the window is dark, never flat grey
- [ ] `setBackdropMode(None)` clears the backdrop and drops the translucent surface
- [ ] Switching Mica / Acrylic ↔ None: no flash, and the background becomes the solid theme colour / material
- [ ] Background controls (colour / image / opacity) are disabled while Mica or Acrylic is active and re-enable on `None`
- [ ] An image set on `None` disappears when switching to a material and returns when switching back
- [ ] `setBorderColor()` shows the border on Windows 11; `QColor()` restores the system default; the border is suppressed on Acrylic
- [ ] Shadow: visible with `BackdropMode::None`, absent with Mica/Acrylic (DWM drops it for layered windows)

## Content

- [ ] Background image in all five modes (Cover / Fit / Stretch / Tile / Center)
- [ ] `setBackgroundOpacity()` blends toward the theme colour
- [ ] Accent colour: titlebar button hover and the selected state of checkable buttons follow it
- [ ] Embedded QOpenGLWidget / QQuickWidget content still renders after maximize, restore and theme switch

## Persistence

- [ ] Move/resize, close, relaunch: same size and position
- [ ] Maximize, close, relaunch: maximized; restore returns the pre-maximize size
- [ ] Move the window to a second monitor, close, relaunch with that monitor disconnected: the window comes back inside the primary work area
