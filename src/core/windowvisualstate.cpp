#include "windowvisualstate.h"

#include "win32/utils.h"

namespace WindowVisualState {

bool shouldUseDarkMode(ThemeManager::ThemeMode themeMode)
{
    return themeMode == ThemeManager::ThemeMode::Dark;
}

bool shouldUseTranslucentBackground(bool systemBackdropEnabled,
                                    bool minimized,
                                    WindowEffect::SystemBackdropPreference systemBackdropPreference)
{
#ifdef Q_OS_WIN
    if (!systemBackdropEnabled || minimized) {
        return false;
    }

    const Utils::WindowsCapabilities caps = Utils::detectWindowsCapabilities();
    const bool autoChainAvailable = caps.supportsSystemSystemBackdrop
                                    || caps.supportsLegacyMica
                                    || caps.supportsAcrylic;

    switch (systemBackdropPreference) {
    case WindowEffect::SystemBackdropPreference::Auto:
        return autoChainAvailable;
    case WindowEffect::SystemBackdropPreference::None:
        return false;
    case WindowEffect::SystemBackdropPreference::Mica:
        return caps.supportsSystemSystemBackdrop ? true : autoChainAvailable;
    case WindowEffect::SystemBackdropPreference::MicaLegacy:
        return caps.supportsLegacyMica ? true : autoChainAvailable;
    case WindowEffect::SystemBackdropPreference::Acrylic:
        return caps.supportsAcrylic ? true : autoChainAvailable;
    }

    return false;
#else
    Q_UNUSED(systemBackdropEnabled)
    Q_UNUSED(minimized)
    Q_UNUSED(systemBackdropPreference)
    return false;
#endif
}

WindowEffect::VisualEffectOptions buildVisualEffectOptions(bool shadowEnabled,
                                                              bool systemBackdropEnabled,
                                                              WindowEffect::SystemBackdropPreference systemBackdropPreference,
                                                              bool roundedCornersEnabled,
                                                              bool systemDarkModeEnabled,
                                                              ThemeManager::ThemeMode themeMode,
                                                              bool maximized,
                                                              bool minimized,
                                                              const QColor &borderColor)
{
    WindowEffect::VisualEffectOptions options;
    options.shadowEnabled = shadowEnabled;
    options.systemBackdropEnabled = systemBackdropEnabled;
    options.systemBackdropPreference = systemBackdropPreference;
    options.roundedCornersEnabled = roundedCornersEnabled;
    options.systemDarkModeEnabled = systemDarkModeEnabled;
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
                              bool systemBackdropEnabled,
                              bool roundedCornersEnabled,
                              bool systemDarkModeEnabled,
                              WindowEffect::SystemBackdropPreference systemBackdropPreference,
                              ThemeManager::ThemeMode themeMode,
                              bool translucentBackground)
{
    quint64 token = 0;
    token |= static_cast<quint64>(visible) << 0;
    token |= static_cast<quint64>(maximized) << 1;
    token |= static_cast<quint64>(minimized) << 2;
    token |= static_cast<quint64>(active) << 3;
    token |= static_cast<quint64>(shadowEnabled) << 4;
    token |= static_cast<quint64>(systemBackdropEnabled) << 5;
    token |= static_cast<quint64>(roundedCornersEnabled) << 6;
    token |= static_cast<quint64>(systemDarkModeEnabled) << 7;
    token |= static_cast<quint64>(shouldUseDarkMode(themeMode)) << 8;
    token |= static_cast<quint64>(translucentBackground) << 9;
    token |= static_cast<quint64>(static_cast<int>(systemBackdropPreference) & 0xF) << 10;
    token |= static_cast<quint64>(static_cast<int>(themeMode) & 0x3) << 14;
    return token;
}

} // namespace WindowVisualState
