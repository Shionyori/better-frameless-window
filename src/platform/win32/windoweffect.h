#pragma once

#include <QColor>

class WindowEffect
{
public:
    enum class SystemBackdropPreference {
        Auto,
        None,
        Mica,
        MicaLegacy,
        Acrylic
    };

    enum class SystemBackdropMode {
        None,
        Mica,
        MicaLegacy,
        Acrylic
    };

    struct VisualEffectOptions {
        bool shadowEnabled = true;
        bool systemBackdropEnabled = true;
        bool roundedCornersEnabled = true;
        bool systemDarkModeEnabled = true;
        SystemBackdropPreference systemBackdropPreference = SystemBackdropPreference::Auto;
        bool useDarkMode = false;
        bool maximized = false;
        bool minimized = false;
        QColor borderColor;
    };

    WindowEffect() = default;

    void applyVisualEffects(void *hwnd, const VisualEffectOptions &options) const;
    void applyShadow(void *hwnd, bool enabled, bool maximized, bool minimized) const;
    void applyRoundedCorners(void *hwnd, bool enabled, bool maximized, bool minimized) const;
    void applySystemDarkMode(void *hwnd, bool enabled, bool useDarkMode) const;
    void applySystemBackdropEffects(void *hwnd,
                              bool enabled,
                              bool useDarkMode,
                              bool maximized,
                              bool minimized,
                              SystemBackdropPreference systemBackdropPreference) const;
    void applyBorderColor(void *hwnd, const QColor &borderColor) const;

private:
    SystemBackdropMode selectSystemBackdropMode(bool enabled,
                                    bool minimized,
                                    SystemBackdropPreference systemBackdropPreference) const;

    // Per-instance tracking to avoid redundant DWM calls during normal
    // visual refresh (window move, focus change). Force-rebind during
    // restore transitions bypasses this cache via enabled=false→true cycle.
    mutable SystemBackdropMode m_lastAppliedMode = SystemBackdropMode::None;
    mutable bool m_lastAppliedSuccessfully = false;

    // Per-window failure flags. Once a DWM attribute fails for a specific
    // HWND, subsequent calls skip that attribute for that window only.
    mutable bool m_ncRenderingPolicyValid = true;
    mutable bool m_extendFrameValid = true;
    mutable bool m_roundedCornersAttributeValid = true;
    mutable bool m_darkMode20Valid = true;
    mutable bool m_darkMode19Valid = true;
    mutable bool m_systemBackdropAttributeSupported = true;
    mutable bool m_legacyMicaAttributeSupported = true;
    mutable bool m_borderColorAttributeValid = true;
};