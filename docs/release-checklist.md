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
- [ ] Double-click the title bar toggles maximize/restore
- [ ] Click the maximize button: instant toggle, no blank/flash frame
- [ ] Drag a **maximized** window by its title bar: it restores and keeps moving
- [ ] Drag to the top edge: snaps/maximizes; window stays draggable afterwards
- [ ] Resize from all four edges and four corners; cursor matches what is draggable
- [ ] Resize at 150% / 200% display scaling: grab area still comfortable
- [ ] Minimize, restore from the taskbar, re-activate: title bar buttons show no stuck hover state
- [ ] `showFullScreen()`: title bar hidden, edges not resizable
- [ ] System menu (right-click title bar): opens, items enabled/disabled correctly, closes when clicking elsewhere

## Snap Layout

- [ ] Hover the maximize button: Snap Layout flyout appears
- [ ] Pick a zone: window snaps and the title bar still works

## Persistence

- [ ] Move/resize, close, relaunch: same size and position
- [ ] Maximize, close, relaunch: maximized; restore returns the pre-maximize size
- [ ] Move the window to a second monitor, close, relaunch with that monitor disconnected: window comes back inside the primary work area
- [ ] Taskbar is never covered by a maximized window

## Theme and backdrop

- [ ] Light ↔ Dark: 300ms transition, title bar, background and backdrop tint all follow
- [ ] Mica: applied at startup, survives maximize → restore
- [ ] Acrylic: applied at startup, survives maximize → restore, tint follows the theme
- [ ] `setSystemBackdropPreference(None)` clears the backdrop
- [ ] `setBorderColor()` shows the border on Windows 11

## Content

- [ ] Background image in all five modes (Cover / Fit / Stretch / Tile / Center)
- [ ] `setBackgroundOpacity()` blends toward the theme color
- [ ] Title bar buttons can be hidden individually and the layout does not collapse
- [ ] `addTitleBarWidget()` widget stays clickable and does not start window drags
- [ ] Embedded QOpenGLWidget / QQuickWidget content still renders after maximize, restore and theme switch
