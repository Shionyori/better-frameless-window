#pragma once

#include <QColor>

// Applies the native window effects. Callers hand in a resolved
// BackdropKind: deciding which backdrop is available belongs to
// WindowVisualState::resolveSurfacePolicy(), not to this class.
class WindowEffect
{
public:
    // What the caller asks for.
    enum class BackdropMode {
        Auto,
        None,
        Mica,
        Acrylic
    };

    // What DWM ends up drawing.
    enum class BackdropKind {
        None,
        Mica,
        Acrylic
    };

    struct VisualEffectOptions {
        bool shadowEnabled = true;
        bool roundedCornersEnabled = true;
        bool darkMode = false; // drives the DWM dark-mode hint and the tint
        bool maximized = false;
        bool minimized = false;
        BackdropKind backdrop = BackdropKind::None;
        QColor borderColor;
    };

    WindowEffect() = default;

    void applyVisualEffects(void *hwnd, const VisualEffectOptions &options) const;

    // Mica is stored by DWM as a window attribute, so reapplying the same kind
    // is a no-op. forceRebind resets it first, which is needed after a
    // maximize/restore round trip or a theme transition.
    void applyBackdrop(void *hwnd,
                       BackdropKind kind,
                       bool darkMode,
                       bool forceRebind = false) const;

private:
    void applyShadow(void *hwnd, bool enabled, bool maximized, bool minimized) const;
    void applyRoundedCorners(void *hwnd, bool enabled, bool maximized, bool minimized) const;
    void applyDarkModeHint(void *hwnd, bool darkMode) const;
    void applyBorderColor(void *hwnd, const QColor &borderColor) const;

    // Per-instance tracking to avoid redundant DWM calls during normal
    // visual refresh (window move, focus change). Force-rebind during
    // restore transitions bypasses this cache.
    mutable BackdropKind m_lastAppliedKind = BackdropKind::None;
    mutable bool m_lastAppliedSuccessfully = false;
    // The Acrylic tint is part of the applied accent gradient, so the cached
    // state has to remember which theme it was applied for.
    mutable bool m_lastAppliedDarkMode = false;

    // Per-window failure flags. Once a DWM attribute fails for a specific
    // HWND, subsequent calls skip that attribute for that window only.
    mutable bool m_ncRenderingPolicyValid = true;
    mutable bool m_extendFrameValid = true;
    mutable bool m_roundedCornersAttributeValid = true;
    mutable bool m_darkMode20Valid = true;
    mutable bool m_darkMode19Valid = true;
    mutable bool m_systemBackdropAttributeSupported = true;
    mutable bool m_borderColorAttributeValid = true;
};
