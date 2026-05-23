# Default Button Style Preset Design

Date: 2026-05-23

## Scope

Add a theme-aware default `QPushButton` style to `ThemeManager::buildStyleSheet()`, so generic buttons in user content change appearance when the theme toggles between Light and Dark. Title bar buttons are unaffected (already have higher-specificity `#id` rules). Users can override the default with custom CSS.

## Design

### New CSS in buildStyleSheet()

Add a `QPushButton` rule between `#FramelessContentPanel` and `TitleBar`:

```css
QPushButton {
    color: %4;                    /* textColor */
    background: %6;               /* buttonHover — same as titlebar hover */
    border: none;
    border-radius: 5px;
    padding: 6px 14px;
    font-family: "Segoe UI";
    font-size: 12px;
}
QPushButton:hover {
    background: %7;               /* buttonPressed — slightly stronger */
}
QPushButton:pressed {
    background: %6;               /* buttonHover — lighter again */
}
QPushButton:disabled {
    color: %8;                    /* disabledColor */
}
```

Wait — the color slot mapping needs rethinking. Current slots:
- %4 = textColor
- %5 = contentColor
- %6 = buttonHover (alpha overlay)
- %7 = buttonPressed (alpha overlay)
- %8 = disabledColor

For QPushButton normal state background, using `buttonHover` (the lighter alpha overlay) would make buttons look like they're always in a hover state. We need a dedicated color.

### New Color Variables

Add `buttonBg` color — a subtle solid/semi-transparent background for buttons in normal state:

- Dark: `QColor(255, 255, 255, 12)` — very faint white overlay
- Light: `QColor(0, 0, 0, 6)` — very faint black overlay

### Revised CSS

Need to add an 11th `.arg()` slot for `buttonBg`:

```css
QPushButton {
    color: %4;
    background: %11;              /* buttonBg — very subtle */
    border: none;
    border-radius: 5px;
    padding: 5px 14px;
    font-family: "Segoe UI";
    font-size: 12px;
}
QPushButton:hover {
    background: %6;               /* buttonHover */
}
QPushButton:pressed {
    background: %7;               /* buttonPressed */
}
QPushButton:disabled {
    color: %8;                    /* disabledColor */
}
```

### Specificity Hierarchy

1. `QPushButton` — default, lowest priority (this task)
2. `#TitleBarMinimizeButton, ...` — titlebar buttons, higher priority (existing)
3. User-defined rules with more specific selectors — highest priority (existing capability)

### Color Values Summary

| Token | Dark | Light |
|-------|------|-------|
| buttonBg (new) | rgba(255,255,255,0.05) | rgba(0,0,0,0.02) |
| buttonHover (existing %6) | rgba(255,255,255,0.08) | rgba(0,0,0,0.04) |
| buttonPressed (existing %7) | rgba(255,255,255,0.12) | rgba(0,0,0,0.08) |
| disabledColor (existing %8) | rgba(255,255,255,0.35) | rgba(0,0,0,0.25) |
| textColor (existing %4) | #fafafa | #1a1a1a |

## Files to Modify

| File | Change |
|------|--------|
| `src/thememanager.cpp` | Add `buttonBg` variable, `QPushButton` CSS rule, update `.arg()` chain from 10 to 11 slots |

## Out of Scope

- Changing existing title bar button styles
- Adding custom button widget subclasses
- Button size variants (small/large)
