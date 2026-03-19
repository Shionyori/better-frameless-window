#pragma once

#include <QColor>

class WindowEffectWin
{
public:
    enum class BackdropPreference {
        Auto,
        None,
        MicaSystem,
        MicaLegacy,
        Acrylic
    };

    enum class BackdropMode {
        None,
        MicaSystem,
        MicaLegacy,
        Acrylic
    };

    struct VisualEffectOptions {
        bool shadowEnabled = true;
        bool backdropEnabled = true;
        bool roundedCornersEnabled = true;
        bool immersiveDarkModeEnabled = true;
        BackdropPreference backdropPreference = BackdropPreference::Auto;
        bool useDarkMode = false;
        bool maximized = false;
        bool minimized = false;
        QColor borderColor;
    };

    WindowEffectWin() = default;

    void applyVisualEffects(void *hwnd, const VisualEffectOptions &options) const;
    void applyShadow(void *hwnd, bool enabled, bool maximized, bool minimized) const;
    void applyRoundedCorners(void *hwnd, bool enabled, bool maximized, bool minimized) const;
    void applyImmersiveDarkMode(void *hwnd, bool enabled, bool useDarkMode) const;
    void applyBackdropEffects(void *hwnd,
                              bool enabled,
                              bool useDarkMode,
                              bool maximized,
                              bool minimized,
                              BackdropPreference backdropPreference) const;
    void applyBorderColor(void *hwnd, const QColor &borderColor) const;

private:
    BackdropMode selectBackdropMode(bool enabled,
                                    bool maximized,
                                    bool minimized,
                                    BackdropPreference backdropPreference) const;
};
