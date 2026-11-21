#include "windowvisualstate.h"

#include "win32/utils.h"

namespace WindowVisualState {

namespace {

WindowEffect::BackdropKind bestSupportedBackdrop(const BackdropCapabilities &capabilities)
{
    if (capabilities.mica) {
        return WindowEffect::BackdropKind::Mica;
    }
    if (capabilities.acrylic) {
        return WindowEffect::BackdropKind::Acrylic;
    }
    return WindowEffect::BackdropKind::None;
}

} // namespace

SurfacePolicy resolveSurfacePolicy(WindowEffect::BackdropMode requested,
                                   bool minimized,
                                   bool hasBackgroundImage,
                                   const BackdropCapabilities &capabilities)
{
    SurfacePolicy policy;

    // States where the theme must own the surface: a minimized window keeps no
    // backdrop, and a background image is painted by the theme itself.
    if (minimized || hasBackgroundImage || requested == WindowEffect::BackdropMode::None) {
        return policy;
    }

    WindowEffect::BackdropKind backdrop = WindowEffect::BackdropKind::None;
    switch (requested) {
    case WindowEffect::BackdropMode::Auto:
    case WindowEffect::BackdropMode::Mica:
        backdrop = bestSupportedBackdrop(capabilities);
        break;
    case WindowEffect::BackdropMode::Acrylic:
        backdrop = capabilities.acrylic ? WindowEffect::BackdropKind::Acrylic
                                        : bestSupportedBackdrop(capabilities);
        break;
    case WindowEffect::BackdropMode::None:
        break;
    }

    if (backdrop == WindowEffect::BackdropKind::None) {
        return policy;
    }

    policy.kind = SurfaceKind::Translucent;
    policy.backdrop = backdrop;
    return policy;
}

SurfacePolicy resolveSurfacePolicy(WindowEffect::BackdropMode requested,
                                   bool minimized,
                                   bool hasBackgroundImage)
{
    const Utils::WindowsCapabilities caps = Utils::detectWindowsCapabilities();
    BackdropCapabilities capabilities;
    capabilities.mica = caps.supportsMica;
    capabilities.acrylic = caps.supportsAcrylic;
    return resolveSurfacePolicy(requested, minimized, hasBackgroundImage, capabilities);
}

bool usesTranslucentSurface(const SurfacePolicy &policy)
{
    return policy.kind == SurfaceKind::Translucent;
}

bool shouldUseDarkMode(ThemeManager::ThemeMode themeMode)
{
    return themeMode == ThemeManager::ThemeMode::Dark;
}

bool shouldClearBackdropBeforeApply(WindowEffect::BackdropKind appliedKind,
                                    WindowEffect::BackdropKind requestedKind)
{
    switch (appliedKind) {
    case WindowEffect::BackdropKind::Mica:
        return true;
    case WindowEffect::BackdropKind::Acrylic:
        return requestedKind != WindowEffect::BackdropKind::Acrylic;
    case WindowEffect::BackdropKind::None:
        return false;
    }

    return false;
}

bool isBackdropApplyRedundant(WindowEffect::BackdropKind requestedKind,
                              bool requestedDarkMode,
                              WindowEffect::BackdropKind appliedKind,
                              bool appliedDarkMode,
                              bool appliedSuccessfully)
{
    if (!appliedSuccessfully || requestedKind != appliedKind) {
        return false;
    }

    if (requestedKind == WindowEffect::BackdropKind::Acrylic) {
        return requestedDarkMode == appliedDarkMode;
    }

    return true;
}

WindowEffect::VisualEffectOptions buildVisualEffectOptions(bool shadowEnabled,
                                                           bool roundedCornersEnabled,
                                                           ThemeManager::ThemeMode themeMode,
                                                           bool maximized,
                                                           bool minimized,
                                                           const SurfacePolicy &surface,
                                                           const QColor &borderColor)
{
    WindowEffect::VisualEffectOptions options;
    options.shadowEnabled = shadowEnabled;
    options.roundedCornersEnabled = roundedCornersEnabled;
    options.darkMode = shouldUseDarkMode(themeMode);
    options.maximized = maximized;
    options.minimized = minimized;
    options.backdrop = surface.backdrop;
    options.borderColor = borderColor;
    return options;
}

quint64 buildVisualStateToken(bool visible,
                              bool maximized,
                              bool minimized,
                              bool active,
                              bool shadowEnabled,
                              bool roundedCornersEnabled,
                              WindowEffect::BackdropKind backdrop,
                              ThemeManager::ThemeMode themeMode,
                              bool translucentSurface)
{
    quint64 token = 0;
    token |= static_cast<quint64>(visible) << 0;
    token |= static_cast<quint64>(maximized) << 1;
    token |= static_cast<quint64>(minimized) << 2;
    token |= static_cast<quint64>(active) << 3;
    token |= static_cast<quint64>(shadowEnabled) << 4;
    token |= static_cast<quint64>(roundedCornersEnabled) << 5;
    token |= static_cast<quint64>(shouldUseDarkMode(themeMode)) << 6;
    token |= static_cast<quint64>(translucentSurface) << 7;
    token |= static_cast<quint64>(static_cast<int>(backdrop) & 0x3) << 8;
    token |= static_cast<quint64>(static_cast<int>(themeMode) & 0x3) << 10;
    return token;
}

} // namespace WindowVisualState
