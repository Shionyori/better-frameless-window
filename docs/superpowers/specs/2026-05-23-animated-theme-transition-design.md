# Animated Theme Transition Design

Date: 2026-05-23

## Scope

Add a 300ms InOutCubic color interpolation transition when switching themes (Light ↔ Dark), driven by QPropertyAnimation on FramelessWindow. Only Qt-layer colors are interpolated; DWM backdrop is untouched during the transition.

## Design

### ThemeManager Changes

Add transition state:

```cpp
// thememanager.h
class ThemeManager {
public:
    void setTransitionProgress(qreal progress);
    qreal transitionProgress() const;
    bool isTransitioning() const;
    ThemeMode previousMode() const;

private:
    ThemeMode m_previousMode = ThemeMode::Light;
    qreal m_transitionProgress = 1.0;  // 1.0 = settled, no transition
    QColor lerpColor(const QColor &a, const QColor &b, qreal t) const;
    QColor themeColor(const QColor &lightValue, const QColor &darkValue) const;
};
```

`themeColor()` resolves each color token based on transition state:
- Not transitioning: return dark or light value directly (as before)
- Transitioning: lerp between source and target themed values

Set `m_previousMode = currentMode`, `m_transitionProgress = 0.0` when a transition starts.

### FramelessWindow Changes

Add dynamic property for animation target:

```cpp
// framelesswindow.h
Q_PROPERTY(qreal themeTransitionProgress READ themeTransitionProgress WRITE setThemeTransitionProgress)
```

```cpp
void FramelessWindow::setThemeMode(ThemeManager::ThemeMode mode, bool persist, bool animated)
{
    if (m_themeManager.themeMode() == mode) return;

    if (animated && isVisible()) {
        // Remember previous mode, reset progress, change target mode
        m_themeManager.startTransition(mode);
        // Animate progress 0→1
        auto *anim = new QPropertyAnimation(this, "themeTransitionProgress", this);
        anim->setDuration(300);
        anim->setEasingCurve(QEasingCurve::InOutCubic);
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    } else {
        // Instant switch
        m_themeManager.setThemeMode(mode);
        m_themeManager.setTransitionProgress(1.0);
    }
    // ... persist, requestVisualRefresh as before
}
```

`setThemeTransitionProgress()` feeds into ThemeManager, then calls `requestVisualRefresh()` — each animation frame rebuilds the full stylesheet with interpolated colors.

### Color Interpolation Logic

For each color token, `themeColor(lightValue, darkValue)`:
- Computes `fromColor` = value for `m_previousMode`
- Computes `toColor` = value for current `m_themeMode`
- Returns `lerpColor(fromColor, toColor, m_transitionProgress)`

`lerpColor` performs per-channel (R, G, B, A) linear interpolation.

### Animated vs Non-Animated

| Caller | Animated |
|--------|----------|
| User `setThemeMode()` | `true` (default) |
| `setFollowSystemTheme()` initial sync | `true` |
| `syncThemeWithSystemIfFollowing()` | `false` (system triggered, subtle) |
| `initWindow()` restore | `false` (startup, not visible) |

### Files to Modify

| File | Change |
|------|--------|
| `src/thememanager.h` | Add transition state members, `themeColor()`, `lerpColor()`, `setTransitionProgress()`, `isTransitioning()` |
| `src/thememanager.cpp` | Implement lerp, `themeColor()`, update `buildStyleSheet()` to use `themeColor()` for all values |
| `src/framelesswindow.h` | Add `Q_PROPERTY`, animation helper |
| `src/framelesswindow.cpp` | Add property getter/setter, update `setThemeMode()` signature, animate |

### Out of Scope

- Animating DWM backdrop (stays at final mode from start)
- Animating light theme ↔ transparent background mode toggles
- Animating accent color changes
