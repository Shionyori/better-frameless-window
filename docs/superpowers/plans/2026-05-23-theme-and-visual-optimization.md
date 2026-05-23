# Theme & Visual Optimization Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Overhaul dark/light theme color palettes to Fluent design, resize title bar to Win11 proportions, add system theme auto-follow and QSettings persistence.

**Architecture:** ThemeManager owns color values; TitleBar owns layout constants; FramelessWindow orchestrates theme changes and persistence; NativeMessageRouter detects WM_SETTINGCHANGE for system theme following. No new files — all changes are surgical edits to existing modules.

**Tech Stack:** Qt6 Widgets, C++17, Win32 API (DWM, registry), QSettings

---

### Task 1: Update ThemeManager dark theme color palette

**Files:**
- Modify: `src/thememanager.cpp` (entire buildStyleSheet method)

- [ ] **Step 1: Replace dark theme color values**

In `src/thememanager.cpp`, update the `buildStyleSheet` method. Replace the dark mode color block (lines 56-65):

```cpp
QString ThemeManager::buildStyleSheet(bool transparentWindowBackground) const
{
    const bool dark = isDarkMode();

    const QColor windowBg = dark ? QColor(25, 25, 25) : QColor(245, 245, 245);
    const QColor titleBg = dark ? QColor(45, 45, 45) : QColor(250, 250, 250);
    const QColor titleBorder = dark ? QColor(42, 42, 42) : QColor(224, 224, 224);
    const QColor textColor = dark ? QColor(250, 250, 250) : QColor(26, 26, 26);
    const QColor contentColor = dark ? QColor(224, 224, 224) : QColor(58, 58, 58);
    const QColor buttonHover = dark ? QColor(255, 255, 255, 20) : QColor(0, 0, 0, 10);
    const QColor buttonPressed = dark ? QColor(255, 255, 255, 31) : QColor(0, 0, 0, 20);
    const QColor disabledColor = dark ? QColor(255, 255, 255, 89) : QColor(0, 0, 0, 64);
    const QColor closeHover = QColor(196, 43, 28);
    // ... rest of method unchanged
```

Note: `buttonHover` and `buttonPressed` are now `QColor` with alpha, requiring CSS format change. Also update the CSS template to use `rgba()` for these colors.

- [ ] **Step 2: Update CSS template for rgba button colors**

In the same file, modify the stylesheet template. The button hover/pressed rules need `rgba()` format since buttonHover/buttonPressed now carry alpha:

Replace lines 105-112 (the button hover/pressed CSS rules):

```cpp
    return QStringLiteral(R"(
        #FramelessWindow {
            %1
            border: none;
        }
        #FramelessContentPanel {
            background: transparent;
            border: none;
        }
        TitleBar {
            background-color: %2;
            border-bottom: 1px solid %3;
        }
        #TitleBarLabel {
            color: %4;
            font-family: "Segoe UI";
            font-size: 13px;
            font-weight: 600;
            letter-spacing: 0.2px;
        }
        #ContentLabel {
            color: %5;
            font-size: 18px;
        }
        #TitleBarMinimizeButton,
        #TitleBarMaximizeButton,
        #TitleBarCloseButton {
            border: none;
            background: transparent;
            color: %4;
            font-family: "Segoe MDL2 Assets", "Segoe UI";
            font-size: 10px;
            padding: 0px;
            border-radius: 5px;
        }
        #TitleBarMinimizeButton[btnState="hover"],
        #TitleBarMaximizeButton[btnState="hover"] {
            background: %6;
        }
        #TitleBarMinimizeButton[btnState="pressed"],
        #TitleBarMaximizeButton[btnState="pressed"] {
            background: %7;
        }
        #TitleBarMinimizeButton[btnState="disabled"],
        #TitleBarMaximizeButton[btnState="disabled"],
        #TitleBarCloseButton[btnState="disabled"] {
            color: %8;
        }
        #TitleBarCloseButton[btnState="hover"] {
            background: %9;
            color: white;
        }
        #TitleBarCloseButton[btnState="pressed"] {
            background: %10;
            color: white;
        }
    )")
           .arg(windowBackgroundRule,
               colorToCss(titleBg),
               colorToCss(titleBorder),
               colorToCss(textColor),
               colorToCss(contentColor),
               colorToCss(buttonHover),
               colorToCss(buttonPressed),
               colorToCss(disabledColor),
               colorToCss(closeHover),
               colorToCss(closeHover.darker(115)));
```

Changes from original:
- `border-radius: 5px` added to button base style
- `font-size: 10px` (was 10px already in current? checking... current says "font-size: 10px" — yes, keep as is for now, but button glyph size will be controlled by TitleBar code)

- [ ] **Step 3: Update colorToCss helper to handle alpha**

The helper `colorToCss` is in the anonymous namespace. It should produce `rgba(r,g,b,a)` when alpha != 255, and `#rrggbb` otherwise:

```cpp
namespace {
QString colorToCss(const QColor &color)
{
    if (color.alpha() == 255) {
        return color.name(QColor::HexRgb);
    }
    return QStringLiteral("rgba(%1, %2, %3, %4)")
        .arg(color.red())
        .arg(color.green())
        .arg(color.blue())
        .arg(color.alphaF(), 0, 'f', 2);
}
// buildWindowBackgroundRule unchanged
}
```

- [ ] **Step 4: Commit**

```bash
git add src/thememanager.cpp
git commit -m "feat: update dark/light theme color palette to Fluent design"
```

---

### Task 2: Update TitleBar layout to Win11 proportions

**Files:**
- Modify: `src/titlebar.cpp` (constructor, initControlButton, updateControlButtonGlyphs)
- Modify: `src/titlebar.h` (no API change, only constants)

- [ ] **Step 1: Update title bar height and layout constants**

In `src/titlebar.cpp` constructor, change height, margins, button sizes:

```cpp
TitleBar::TitleBar(QWidget *parent)
    : QWidget(parent)
    , m_layout(new QHBoxLayout(this))
    , m_centerContainer(new QWidget(this))
    , m_centerLayout(new QHBoxLayout(m_centerContainer))
    , m_titleLabel(new QLabel("Better Frameless Window", this))
    , m_minimizeButton(new QPushButton(this))
    , m_maximizeButton(new QPushButton(this))
    , m_closeButton(new QPushButton(this))
    , m_leftPressed(false)
    , m_dragInitiated(false)
    , m_visualMaximized(false)
{
    setFixedHeight(44);
    setMouseTracking(true);

    m_layout->setContentsMargins(14, 0, 12, 0);
    m_layout->setSpacing(0);
    // ... rest unchanged
```

- [ ] **Step 2: Update button sizes**

In `initControlButton` method, change `setFixedSize(40, 25)` to `setFixedSize(46, 30)`:

```cpp
void TitleBar::initControlButton(QPushButton *button, const char *role)
{
    if (button == nullptr) {
        return;
    }

    button->setFixedSize(46, 30);
    // ... rest unchanged
```

- [ ] **Step 3: Update button glyph font size**

In `initControlButton` method, change `iconFont.setPixelSize(9)` to `iconFont.setPixelSize(10)`:

```cpp
    QFont iconFont(QStringLiteral("Segoe MDL2 Assets"));
    iconFont.setPixelSize(10);
    button->setFont(iconFont);
```

- [ ] **Step 4: Update button spacing in layout**

In the constructor, after `m_layout->addStretch()`, the spacing between buttons was implicit (0px via layout). Add explicit spacing:

The current code has:
```cpp
m_layout->addStretch();
m_layout->addSpacing(2);
m_layout->addWidget(m_minimizeButton);
m_layout->addWidget(m_maximizeButton);
m_layout->addWidget(m_closeButton);
```

Change to:
```cpp
m_layout->addStretch();
m_layout->addSpacing(4);
m_layout->addWidget(m_minimizeButton);
m_layout->addSpacing(4);
m_layout->addWidget(m_maximizeButton);
m_layout->addSpacing(4);
m_layout->addWidget(m_closeButton);
```

- [ ] **Step 5: Remove left title text padding icon hack**

The current code has `"▹ "` prefix on the title text (implicit in the setText call). This was a visual hack. Replace the title text:

In constructor, current:
```cpp
m_titleLabel->setObjectName("TitleBarLabel");
```

The title text `"Better Frameless Window"` is set in the initializer list. This is fine — keep it.

But actually, looking at the current title label: the display shows "▹ Better Frameless Window". That "▹" might be from stylesheet or default text. Let me check... No, looking at the constructor initializer list: `m_titleLabel(new QLabel("Better Frameless Window", this))`. The "▹" was in the visual mockup, not in the actual code. So no change needed for the title text.

- [ ] **Step 6: Update title font**

In constructor, the title font uses `QFont::DemiBold`. Keep the font but update pixel size from 13 to 14 for better proportion on 44px bar:

```cpp
QFont titleFont(QStringLiteral("Segoe UI"));
titleFont.setPixelSize(14);
titleFont.setWeight(QFont::DemiBold);
m_titleLabel->setFont(titleFont);
```

- [ ] **Step 7: Commit**

```bash
git add src/titlebar.cpp
git commit -m "feat: update title bar layout to Win11 proportions (44px, 46x30 buttons)"
```

---

### Task 3: Add system theme detection utility

**Files:**
- Modify: `src/platform/win32/utils.h` (add function declaration)
- Modify: `src/platform/win32/utils.cpp` (add implementation)

- [ ] **Step 1: Declare function in utils.h**

Add after the existing `detectWindowsCapabilities()` declaration:

```cpp
bool isSystemDarkModeEnabled();
```

- [ ] **Step 2: Implement in utils.cpp**

Add at the end of `src/platform/win32/utils.cpp`:

```cpp
bool isSystemDarkModeEnabled()
{
#ifdef Q_OS_WIN
    QSettings settings(
        QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"),
        QSettings::NativeFormat);
    return settings.value(QStringLiteral("AppsUseLightTheme"), 1).toInt() == 0;
#else
    return false;
#endif
}
```

This also requires adding `#include <QSettings>` at the top of utils.cpp.

- [ ] **Step 3: Commit**

```bash
git add src/platform/win32/utils.h src/platform/win32/utils.cpp
git commit -m "feat: add system dark mode detection utility"
```

---

### Task 4: Add followSystemTheme to FramelessWindow

**Files:**
- Modify: `src/framelesswindow.h` (add member, getter, setter)
- Modify: `src/framelesswindow.cpp` (add implementation, constructor init, applyTheme integration)

- [ ] **Step 1: Add declarations to framelesswindow.h**

Add public methods alongside existing theme API:

```cpp
void setFollowSystemTheme(bool enabled);
bool followsSystemTheme() const;
```

Add private member alongside existing m_themeManager:

```cpp
bool m_followSystemTheme;
```

- [ ] **Step 2: Initialize in constructor**

In the member initializer list of `FramelessWindow::FramelessWindow`, add:

```cpp
, m_followSystemTheme(false)
```

- [ ] **Step 3: Implement setter and getter**

Add to `src/framelesswindow.cpp`:

```cpp
void FramelessWindow::setFollowSystemTheme(bool enabled)
{
    if (m_followSystemTheme == enabled) {
        return;
    }

    m_followSystemTheme = enabled;

#ifdef Q_OS_WIN
    if (enabled) {
        const bool systemDark = Utils::isSystemDarkModeEnabled();
        setThemeMode(systemDark ? ThemeManager::ThemeMode::Dark
                                : ThemeManager::ThemeMode::Light);
    }
#endif
}

bool FramelessWindow::followsSystemTheme() const
{
    return m_followSystemTheme;
}
```

- [ ] **Step 4: Add method to sync from system**

Add a public method declaration in `framelesswindow.h`:

```cpp
void syncThemeWithSystemIfFollowing();
```

And implementation in `framelesswindow.cpp`:

```cpp
void FramelessWindow::syncThemeWithSystemIfFollowing()
{
#ifdef Q_OS_WIN
    if (!m_followSystemTheme) {
        return;
    }

    const bool systemDark = Utils::isSystemDarkModeEnabled();
    const ThemeManager::ThemeMode systemMode = systemDark
        ? ThemeManager::ThemeMode::Dark
        : ThemeManager::ThemeMode::Light;

    if (m_themeManager.themeMode() != systemMode) {
        setThemeMode(systemMode);
    }
#endif
}
```

- [ ] **Step 5: Commit**

```bash
git add src/framelesswindow.h src/framelesswindow.cpp
git commit -m "feat: add followSystemTheme API for auto light/dark switching"
```

---

### Task 5: Add QSettings persistence for theme mode

**Files:**
- Modify: `src/framelesswindow.cpp` (setThemeMode override, constructor init)

- [ ] **Step 1: Add QSettings include**

At the top of `src/framelesswindow.cpp`, add:

```cpp
#include <QSettings>
```

- [ ] **Step 2: Save on theme mode change**

Modify `FramelessWindow::setThemeMode` to persist after setting:

```cpp
void FramelessWindow::setThemeMode(ThemeManager::ThemeMode mode)
{
    if (m_themeManager.themeMode() == mode) {
        return;
    }

    m_themeManager.setThemeMode(mode);

    QSettings settings(QStringLiteral("better-frameless-window"), QStringLiteral("settings"));
    settings.setValue(QStringLiteral("theme/mode"),
                      mode == ThemeManager::ThemeMode::Dark
                          ? QStringLiteral("dark")
                          : QStringLiteral("light"));

    requestVisualRefresh();
}
```

- [ ] **Step 3: Restore on construction (when not following system)**

In `FramelessWindow::initWindow()`, after `setObjectName("FramelessWindow")` and before `applyTheme()`, add:

```cpp
void FramelessWindow::initWindow()
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint);
    setMinimumSize(480, 320);
    resize(960, 640);
    setAttribute(Qt::WA_StyledBackground, true);

    setObjectName("FramelessWindow");

    // Restore persisted theme preference (only when not following system)
    if (!m_followSystemTheme) {
        QSettings settings(QStringLiteral("better-frameless-window"), QStringLiteral("settings"));
        const QString savedMode = settings.value(QStringLiteral("theme/mode")).toString();
        if (savedMode == QStringLiteral("dark")) {
            m_themeManager.setThemeMode(ThemeManager::ThemeMode::Dark);
        } else if (savedMode == QStringLiteral("light")) {
            m_themeManager.setThemeMode(ThemeManager::ThemeMode::Light);
        }
        // If no saved value, keep default (Light)
    }

    applyTheme();
}
```

- [ ] **Step 4: Commit**

```bash
git add src/framelesswindow.cpp
git commit -m "feat: persist theme mode via QSettings, restore on startup"
```

---

### Task 6: Handle WM_SETTINGCHANGE for system theme detection

**Files:**
- Modify: `src/platform/win32/nativemessagerouter.cpp` (add WM_SETTINGCHANGE case)

- [ ] **Step 1: Add WM_SETTINGCHANGE handling**

In `NativeMessageRouter::handle()`, add a case in the switch statement for `WM_SETTINGCHANGE`. Add it before the `default:` case:

```cpp
    case WM_SETTINGCHANGE:
        if (msg->lParam != 0) {
            const auto *changed = reinterpret_cast<LPCWSTR>(msg->lParam);
            if (changed != nullptr && wcscmp(changed, L"ImmersiveColorSet") == 0) {
                window.syncThemeWithSystemIfFollowing();
            }
        }
        return false;
```

Also need to add `#include <cstring>` at the top of nativemessagerouter.cpp for `wcscmp` (it's already available via `<qt_windows.h>` which includes `<windows.h>` which has `lstrcmpW` — actually we should use `::lstrcmpW` or just compare since `<windows.h>` should already be included via `<qt_windows.h>`).

Wait, `<qt_windows.h>` includes `<windows.h>`, so `lstrcmpW` is available. But let's use `wcscmp` which is from `<cstring>`/`<string.h>`. Actually both work on Windows. Let's use `wcscmp` and add `<cstring>`.

Actually, looking at the existing includes, `<qt_windows.h>` already pulls in `<windows.h>`. `lstrcmpW` is available without extra includes. Let's just use it directly:

```cpp
    case WM_SETTINGCHANGE:
        if (msg->lParam != 0) {
            const auto *changed = reinterpret_cast<LPCWSTR>(msg->lParam);
            if (changed != nullptr && lstrcmpW(changed, L"ImmersiveColorSet") == 0) {
                window.syncThemeWithSystemIfFollowing();
            }
        }
        return false;
```

No extra include needed — `lstrcmpW` is declared in `<windows.h>` which comes via `<qt_windows.h>`.

- [ ] **Step 2: Commit**

```bash
git add src/platform/win32/nativemessagerouter.cpp
git commit -m "feat: handle WM_SETTINGCHANGE for system theme auto-follow"
```

---

### Task 7: Clean up old archive docs

**Files:**
- Delete: `docs/archives/` (entire directory)

- [ ] **Step 1: Remove archives directory**

```bash
git rm -r docs/archives/
```

- [ ] **Step 2: Commit**

```bash
git commit -m "chore: remove stale archive documentation"
```

---

### Task 8: Build and manual verification

**Files:** None (verification only)

- [ ] **Step 1: Build debug**

```bash
cmake --build build/windows-msvc-local-debug --config Debug
```

Expected: BUILD SUCCESS, no errors.

- [ ] **Step 2: Build release**

```bash
cmake --build build/windows-msvc-local-release --config Release
```

Expected: BUILD SUCCESS, no errors.

- [ ] **Step 3: Manual regression checklist**

Run `build/windows-msvc-local-debug/Debug/bfw-example.exe` and verify:

1. Window appears with new 44px title bar
2. Dark theme toggle produces Fluent dark colors (deep #191919 background, semi-transparent button hovers)
3. Light theme uses new Fluent light palette
4. Close button hover shows #c42b1c (muted red) instead of bright red
5. Button hover states use semi-transparent overlay (Mica/Acrylic shows through)
6. Title bar drag, double-click maximize/restore, right-click system menu all work
7. 8-direction resize works
8. Buttons are 46x30px with 5px border-radius
9. Restarting the app preserves the last theme selection
10. No crashes or visual regressions

- [ ] **Step 4: Commit any final fixes**

If any issues found during verification, fix and commit them separately.

```bash
git add <fixed-files>
git commit -m "fix: address visual verification issues"
```
