#include "windowvisualstate.h"

#include "winutils.h"

namespace WindowVisualState {

bool shouldUseDarkMode(ThemeManager::ThemeMode themeMode)
{
    return themeMode == ThemeManager::ThemeMode::Dark;
}

bool shouldUseTranslucentBackground(bool nativeEffectsEnabled,
                                    bool minimized,
                                    WindowEffectWin::BackdropPreference nativeBackdropPreference)
{
#ifdef Q_OS_WIN
    if (!nativeEffectsEnabled || minimized) {
        return false;
    }

    const WinUtils::WindowsCapabilities caps = WinUtils::detectWindowsCapabilities();
    const bool autoChainAvailable = caps.supportsSystemBackdrop
                                    || caps.supportsLegacyMica
                                    || caps.supportsAcrylic;

    switch (nativeBackdropPreference) {
    case WindowEffectWin::BackdropPreference::Auto:
        return autoChainAvailable;
    case WindowEffectWin::BackdropPreference::None:
        return false;
    case WindowEffectWin::BackdropPreference::MicaSystem:
        return caps.supportsSystemBackdrop ? true : autoChainAvailable;
    case WindowEffectWin::BackdropPreference::MicaLegacy:
        return caps.supportsLegacyMica ? true : autoChainAvailable;
    case WindowEffectWin::BackdropPreference::Acrylic:
        return caps.supportsAcrylic ? true : autoChainAvailable;
    }

    return false;
#else
    Q_UNUSED(nativeEffectsEnabled)
    Q_UNUSED(minimized)
    Q_UNUSED(nativeBackdropPreference)
    return false;
#endif
}

WindowEffectWin::VisualEffectOptions buildVisualEffectOptions(bool shadowEnabled,
                                                              bool nativeEffectsEnabled,
                                                              WindowEffectWin::BackdropPreference nativeBackdropPreference,
                                                              bool roundedCornersEnabled,
                                                              bool immersiveDarkModeEnabled,
                                                              ThemeManager::ThemeMode themeMode,
                                                              bool maximized,
                                                              bool minimized,
                                                              const QColor &borderColor)
{
    WindowEffectWin::VisualEffectOptions options;
    options.shadowEnabled = shadowEnabled;
    options.nativeEffectsEnabled = nativeEffectsEnabled;
    options.nativeBackdropPreference = nativeBackdropPreference;
    options.roundedCornersEnabled = roundedCornersEnabled;
    options.immersiveDarkModeEnabled = immersiveDarkModeEnabled;
    options.useDarkMode = shouldUseDarkMode(themeMode);
    options.maximized = maximized;
    options.minimized = minimized;
    options.borderColor = borderColor;
    return options;
}

quint64 buildVisualStateToken(bool visible,
                              bool maximized,
                              bool minimized,
                              bool active,
                              bool shadowEnabled,
                              bool backdropEnabled,
                              bool roundedCornersEnabled,
                              bool immersiveDarkModeEnabled,
                              WindowEffectWin::BackdropPreference backdropPreference,
                              ThemeManager::ThemeMode themeMode,
                              bool translucentBackground)
{
    quint64 token = 0;
    token |= static_cast<quint64>(visible) << 0;
    token |= static_cast<quint64>(maximized) << 1;
    token |= static_cast<quint64>(minimized) << 2;
    token |= static_cast<quint64>(active) << 3;
    token |= static_cast<quint64>(shadowEnabled) << 4;
    token |= static_cast<quint64>(backdropEnabled) << 5;
    token |= static_cast<quint64>(roundedCornersEnabled) << 6;
    token |= static_cast<quint64>(immersiveDarkModeEnabled) << 7;
    token |= static_cast<quint64>(shouldUseDarkMode(themeMode)) << 8;
    token |= static_cast<quint64>(translucentBackground) << 9;
    token |= static_cast<quint64>(static_cast<int>(backdropPreference) & 0xF) << 10;
    token |= static_cast<quint64>(static_cast<int>(themeMode) & 0x3) << 14;
    return token;
}

} // namespace WindowVisualState
