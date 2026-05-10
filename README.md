# better-frameless-window

A Qt6 C++17 frameless window library for Windows, with DWM visual effects integration.

## Features

- **Frameless window** with custom titlebar (drag, double-click maximize/restore, right-click system menu)
- **8-direction resize** with HiDPI-aware hit testing
- **Windows 10/11 visual effects**: shadow, rounded corners, system dark mode, Mica / Acrylic backdrop
- **Snap Layout support** via native DWM hit-test forwarding
- **Theme manager** with Light / Dark modes and accent color customization
- **Titlebar extension slot** for custom widgets

## Build

```bash
cmake --preset windows-msvc-local-release
cmake --build build/windows-msvc-local-release
```

The build produces `bfw-example` from `example/main.cpp`.

**Requirements:** Qt 6.x (Core, Gui, Widgets), CMake 3.16+, MSVC 2022 (Windows).

## Quick Start

```cpp
#include <QApplication>
#include <QLabel>
#include "framelesswindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    FramelessWindow window;
    window.setWindowSizeLimits(QSize(480, 320), QSize());

    auto *label = new QLabel("Hello!");
    label->setAlignment(Qt::AlignCenter);
    window.setCentralWidget(label);

    window.show();
    return app.exec();
}
```

Link against the `better-frameless-window` static library and include `src/` and `src/platform/` directories.

## API Reference

### FramelessWindow

#### Window configuration

| Method | Description |
|---|---|
| `setWindowSizeLimits(min, max)` | Set minimum / maximum window size |
| `minimumWindowSize()` / `maximumWindowSize()` | Get current size limits |
| `setCentralWidget(widget)` | Set the main content widget |
| `centralWidget()` / `takeCentralWidget()` | Get / remove the main content widget |
| `addTitleBarWidget(widget)` | Add a widget to the titlebar center area |
| `clearTitleBarWidgets()` | Remove all titlebar center widgets |

#### Visual effects

| Method | Description |
|---|---|
| `setThemeMode(mode)` | Set Light / Dark theme |
| `themeMode()` | Get current theme mode |
| `setAccentColor(color)` | Set the accent color |
| `accentColor()` | Get current accent color |
| `setSystemShadowEnabled(bool)` | Enable / disable system window shadow |
| `setSystemBackdropEnabled(bool)` | Enable / disable system backdrop effects |
| `setSystemBackdropPreference(pref)` | Set backdrop mode: Auto / None / Mica / MicaLegacy / Acrylic |
| `setRoundedCornersEnabled(bool)` | Enable / disable rounded window corners |
| `setSystemDarkModeEnabled(bool)` | Enable / disable system dark mode titlebar |

#### Diagnostics

| Method | Description |
|---|---|
| `setDiagnosticsEnabled(bool)` | Enable debug logging for visual effect issues |

### ThemeManager::ThemeMode

- `Light` — Light application theme
- `Dark` — Dark application theme

### WindowEffect::SystemBackdropPreference

- `Auto` — Best available (Mica > MicaLegacy > Acrylic)
- `None` — Opaque window background
- `Mica` — Windows 11 Mica material
- `MicaLegacy` — Windows 10 Mica (fallback)
- `Acrylic` — Acrylic blur material

## Known Issues

| Issue | Details |
|---|---|
| Snaps Layout missing | Snap Layout overlay may not appear on maximize button hover |
| Resize jitter | Slight visual jitter during window resize |
| Backdrop after restore | Mica/Acrylic may briefly flash after maximize→restore transition |
| Dark mode titlebar | Dark mode may show incorrect background in normal (non-maximized) state |
