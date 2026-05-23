# Default Button Style Preset Implementation Plan

**Goal:** Add theme-aware default QPushButton style to ThemeManager stylesheet.

**Architecture:** Single-file change to `src/thememanager.cpp` — add `buttonBg` color variable, add `QPushButton` CSS rule, extend `.arg()` chain from 10 to 11 slots.

---

### Task 1: Add default QPushButton style

**Files:** Modify: `src/thememanager.cpp`

- [ ] **Step 1: Add buttonBg color variable**

In `buildStyleSheet()`, after the existing color declarations (after `closeHover`), add:

```cpp
const QColor buttonBg = dark ? QColor(255, 255, 255, 12) : QColor(0, 0, 0, 6);
```

- [ ] **Step 2: Add QPushButton CSS rule**

In the stylesheet template, after `#FramelessContentPanel` block, add:

```cpp
        QPushButton {
            color: %4;
            background: %11;
            border: none;
            border-radius: 5px;
            padding: 5px 14px;
            font-family: "Segoe UI";
            font-size: 12px;
        }
        QPushButton:hover {
            background: %6;
        }
        QPushButton:pressed {
            background: %7;
        }
        QPushButton:disabled {
            color: %8;
        }
```

- [ ] **Step 3: Extend .arg() chain to 11 slots**

Add `colorToCss(buttonBg)` as the 11th argument:

```cpp
           .arg(windowBackgroundRule,
               colorToCss(titleBg),
               colorToCss(titleBorder),
               colorToCss(textColor),
               colorToCss(contentColor),
               colorToCss(buttonHover),
               colorToCss(buttonPressed),
               colorToCss(disabledColor),
               colorToCss(closeHover),
               colorToCss(closeHover.darker(115)),
               colorToCss(buttonBg));
```

- [ ] **Step 4: Build and commit**

```bash
cmake --build build/windows-msvc-local-debug --config Debug
git add src/thememanager.cpp
git commit -m "feat: add theme-aware default QPushButton style"
```
