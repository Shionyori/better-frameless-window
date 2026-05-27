# Title Bar Button Visibility Configuration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add per-button visibility control (minimize/maximize/close) to TitleBar and FramelessWindow.

**Architecture:** Three new `set*ButtonVisible(bool)`/`is*ButtonVisible()` pairs on TitleBar, mirrored on FramelessWindow. When hidden, both the button and its leading 4px spacer collapse. Existing hit-test and visual-state code already checks `isVisible()` — no changes needed there.

**Tech Stack:** Qt 6 C++, Qt Test

---

### Task 1: Write the failing test

**Files:**
- Create: `tests/test_titlebar.cpp`
- Modify: `CMakeLists.txt:210`

- [ ] **Step 1: Create test file**

```cpp
#include <QtTest>
#include <titlebar.h>

class TestTitleBar : public QObject
{
    Q_OBJECT

private slots:
    void defaultButtonsVisible()
    {
        TitleBar tb;
        QVERIFY(tb.isMinimizeButtonVisible());
        QVERIFY(tb.isMaximizeButtonVisible());
        QVERIFY(tb.isCloseButtonVisible());
    }

    void hideMinimizeButton()
    {
        TitleBar tb;
        tb.setMinimizeButtonVisible(false);
        QVERIFY(!tb.isMinimizeButtonVisible());
        QVERIFY(tb.isMaximizeButtonVisible());
        QVERIFY(tb.isCloseButtonVisible());
    }

    void hideMaximizeButton()
    {
        TitleBar tb;
        tb.setMaximizeButtonVisible(false);
        QVERIFY(tb.isMinimizeButtonVisible());
        QVERIFY(!tb.isMaximizeButtonVisible());
        QVERIFY(tb.isCloseButtonVisible());
    }

    void hideCloseButton()
    {
        TitleBar tb;
        tb.setCloseButtonVisible(false);
        QVERIFY(tb.isMinimizeButtonVisible());
        QVERIFY(tb.isMaximizeButtonVisible());
        QVERIFY(!tb.isCloseButtonVisible());
    }

    void hideAllButtons()
    {
        TitleBar tb;
        tb.setMinimizeButtonVisible(false);
        tb.setMaximizeButtonVisible(false);
        tb.setCloseButtonVisible(false);
        QVERIFY(!tb.isMinimizeButtonVisible());
        QVERIFY(!tb.isMaximizeButtonVisible());
        QVERIFY(!tb.isCloseButtonVisible());
    }

    void toggleButtonVisibility()
    {
        TitleBar tb;
        tb.setMinimizeButtonVisible(false);
        QVERIFY(!tb.isMinimizeButtonVisible());
        tb.setMinimizeButtonVisible(true);
        QVERIFY(tb.isMinimizeButtonVisible());
    }
};

QTEST_MAIN(TestTitleBar)
#include "test_titlebar.moc"
```

- [ ] **Step 2: Register test in CMakeLists.txt**

After line 210 (`add_bfw_test(version)`), add:

```cmake
add_bfw_test(titlebar)
```

- [ ] **Step 3: Build and run test — expect COMPILE FAILURE**

Run: `cmake --build build/windows-msvc-local-debug --target test-titlebar 2>&1`
Expected: COMPILE FAIL — `isMinimizeButtonVisible` etc. not defined

---

### Task 2: Add member variables and method declarations to TitleBar

**Files:**
- Modify: `src/titlebar.h`

- [ ] **Step 1: Add forward declaration and spacer members**

At line 7 (after `class QHBoxLayout;`), add:
```cpp
class QSpacerItem;
```

After line 80 (`QPushButton *m_closeButton;`), add:
```cpp
QSpacerItem *m_minimizeSpacer = nullptr;
QSpacerItem *m_maximizeSpacer = nullptr;
QSpacerItem *m_closeSpacer = nullptr;
```

- [ ] **Step 2: Add method declarations**

After line 38 (`void clearCenterWidgets();`), add:
```cpp
void setMinimizeButtonVisible(bool visible);
bool isMinimizeButtonVisible() const;
void setMaximizeButtonVisible(bool visible);
bool isMaximizeButtonVisible() const;
void setCloseButtonVisible(bool visible);
bool isCloseButtonVisible() const;
```

- [ ] **Step 3: Build TitleBar only — expect compile failure (no impl yet)**

Run: `cmake --build build/windows-msvc-local-debug --target better-frameless-window 2>&1`
Expected: LINK FAIL (undefined symbols for the 6 methods)

---

### Task 3: Implement visibility methods in TitleBar

**Files:**
- Modify: `src/titlebar.cpp`

- [ ] **Step 1: Replace `addSpacing(4)` calls with explicit QSpacerItem**

Replace lines 72-77 in the constructor:
```cpp
    m_layout->addStretch();
    m_layout->addSpacing(4);
    m_layout->addWidget(m_minimizeButton);
    m_layout->addSpacing(4);
    m_layout->addWidget(m_maximizeButton);
    m_layout->addSpacing(4);
    m_layout->addWidget(m_closeButton);
```

With:
```cpp
    m_minimizeSpacer = new QSpacerItem(4, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
    m_maximizeSpacer = new QSpacerItem(4, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
    m_closeSpacer = new QSpacerItem(4, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);

    m_layout->addStretch();
    m_layout->addSpacerItem(m_minimizeSpacer);
    m_layout->addWidget(m_minimizeButton);
    m_layout->addSpacerItem(m_maximizeSpacer);
    m_layout->addWidget(m_maximizeButton);
    m_layout->addSpacerItem(m_closeSpacer);
    m_layout->addWidget(m_closeButton);
```

Also add `#include <QSpacerItem>` after line 8 (`#include <QLabel>`).

- [ ] **Step 2: Add the 6 method implementations at the end of file (before closing)**

```cpp
void TitleBar::setMinimizeButtonVisible(bool visible)
{
    m_minimizeButton->setVisible(visible);
    m_minimizeSpacer->changeSize(
        visible ? 4 : 0, 0,
        QSizePolicy::Fixed,
        visible ? QSizePolicy::Minimum : QSizePolicy::Fixed);
    m_layout->invalidate();
}

bool TitleBar::isMinimizeButtonVisible() const
{
    return m_minimizeButton->isVisible();
}

void TitleBar::setMaximizeButtonVisible(bool visible)
{
    m_maximizeButton->setVisible(visible);
    m_maximizeSpacer->changeSize(
        visible ? 4 : 0, 0,
        QSizePolicy::Fixed,
        visible ? QSizePolicy::Minimum : QSizePolicy::Fixed);
    m_layout->invalidate();
}

bool TitleBar::isMaximizeButtonVisible() const
{
    return m_maximizeButton->isVisible();
}

void TitleBar::setCloseButtonVisible(bool visible)
{
    m_closeButton->setVisible(visible);
    m_closeSpacer->changeSize(
        visible ? 4 : 0, 0,
        QSizePolicy::Fixed,
        visible ? QSizePolicy::Minimum : QSizePolicy::Fixed);
    m_layout->invalidate();
}

bool TitleBar::isCloseButtonVisible() const
{
    return m_closeButton->isVisible();
}
```

- [ ] **Step 3: Filter hidden buttons in syncButtonVisualStatesFromCursor**

In `syncButtonVisualStatesFromCursor()` (line 334), change:
```cpp
    const QList<QPushButton *> buttons = {m_minimizeButton, m_maximizeButton, m_closeButton};
    for (QPushButton *button : buttons) {
        if (button == nullptr) {
            continue;
        }
```

To:
```cpp
    const QList<QPushButton *> buttons = {m_minimizeButton, m_maximizeButton, m_closeButton};
    for (QPushButton *button : buttons) {
        if (button == nullptr || !button->isVisible()) {
            continue;
        }
```

- [ ] **Step 4: Build and run TitleBar test**

Run:
```bash
cmake --build build/windows-msvc-local-debug --target test-titlebar && ctest --test-dir build/windows-msvc-local-debug -R titlebar -V
```
Expected: ALL 6 TESTS PASS

- [ ] **Step 5: Commit**

```bash
git add src/titlebar.h src/titlebar.cpp tests/test_titlebar.cpp CMakeLists.txt
git commit -m "feat: add title bar button visibility control to TitleBar"
```

---

### Task 4: Add mirror methods to FramelessWindow

**Files:**
- Modify: `src/framelesswindow.h`
- Modify: `src/framelesswindow.cpp`

- [ ] **Step 1: Add declarations to framelesswindow.h**

After line 77 (`int titleBarHeight() const;`), add:
```cpp
    void setMinimizeButtonVisible(bool visible);
    bool isMinimizeButtonVisible() const;
    void setMaximizeButtonVisible(bool visible);
    bool isMaximizeButtonVisible() const;
    void setCloseButtonVisible(bool visible);
    bool isCloseButtonVisible() const;
```

- [ ] **Step 2: Add implementations to framelesswindow.cpp**

After the `titleBarHeight()` implementation block (line 234), add:

```cpp
void FramelessWindow::setMinimizeButtonVisible(bool visible)
{
    if (m_titleBar != nullptr) {
        m_titleBar->setMinimizeButtonVisible(visible);
    }
}

bool FramelessWindow::isMinimizeButtonVisible() const
{
    return m_titleBar != nullptr && m_titleBar->isMinimizeButtonVisible();
}

void FramelessWindow::setMaximizeButtonVisible(bool visible)
{
    if (m_titleBar != nullptr) {
        m_titleBar->setMaximizeButtonVisible(visible);
    }
}

bool FramelessWindow::isMaximizeButtonVisible() const
{
    return m_titleBar != nullptr && m_titleBar->isMaximizeButtonVisible();
}

void FramelessWindow::setCloseButtonVisible(bool visible)
{
    if (m_titleBar != nullptr) {
        m_titleBar->setCloseButtonVisible(visible);
    }
}

bool FramelessWindow::isCloseButtonVisible() const
{
    return m_titleBar != nullptr && m_titleBar->isCloseButtonVisible();
}
```

- [ ] **Step 3: Build the library**

Run: `cmake --build build/windows-msvc-local-debug --target better-frameless-window`
Expected: BUILD SUCCESS

- [ ] **Step 4: Run full test suite**

Run: `ctest --test-dir build/windows-msvc-local-debug -V`
Expected: ALL TESTS PASS

- [ ] **Step 5: Commit**

```bash
git add src/framelesswindow.h src/framelesswindow.cpp
git commit -m "feat: expose title bar button visibility through FramelessWindow"
```
