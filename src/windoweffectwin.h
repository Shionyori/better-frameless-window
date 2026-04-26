#pragma once

#include <QColor>

class WindowEffectWin
{
public:
    enum class BackdropMode {
        None,
        Mica,
        MicaLegacy,
        Acrylic
    };

    struct VisualEffectOptions {
        bool shadowEnabled = true;
        bool roundedCornersEnabled = true;
        bool SystemDarkModeEnabled = true;
        BackdropMode SystemBackdropMode = BackdropMode::None;
        bool useDarkMode = false;
        bool maximized = false;
        bool minimized = false;
        QColor borderColor;
    };

    WindowEffectWin() = default;

    void applyVisualEffects(void *hwnd, const VisualEffectOptions &options) const;
    void applyShadow(void *hwnd, bool enabled, bool maximized, bool minimized) const;
    void applyRoundedCorners(void *hwnd, bool enabled, bool maximized, bool minimized) const;
    void applySystemDarkMode(void *hwnd, bool enabled, bool useDarkMode) const;
    void applySystemBackdrop(void *hwnd,
                                    bool useDarkMode,
                                    bool maximized,
                                    bool minimized,
                                    BackdropMode mode) const;
    void applyBorderColor(void *hwnd, const QColor &borderColor) const;
};
