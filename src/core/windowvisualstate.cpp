#include "windowvisualstate.h"

#include "winutils.h"

namespace WindowVisualState {

bool shouldUseDarkMode(ThemeManager::ThemeMode themeMode)
{
    return themeMode == ThemeManager::ThemeMode::Dark;
}

bool shouldUseTranslucentBackground(bool minimized,
                                    WindowEffectWin::BackdropMode systemBackdropMode)
{
#ifdef Q_OS_WIN
    if (systemBackdropMode == WindowEffectWin::BackdropMode::None || minimized) {
        return false;
    }

    const WinUtils::WindowsCapabilities caps = WinUtils::detectWindowsCapabilities();
    const bool anyBackdropAvailable = caps.supportsSystemBackdrop
                                      || caps.supportsLegacyMica
                                      || caps.supportsAcrylic;

    switch (systemBackdropMode) {
    case WindowEffectWin::BackdropMode::None:
        return false;
    case WindowEffectWin::BackdropMode::Mica:
        return caps.supportsSystemBackdrop ? true : anyBackdropAvailable;
    case WindowEffectWin::BackdropMode::MicaLegacy:
        return caps.supportsLegacyMica ? true : anyBackdropAvailable;
    case WindowEffectWin::BackdropMode::Acrylic:
        return caps.supportsAcrylic ? true : anyBackdropAvailable;
    }

    return false;
#else
    Q_UNUSED(minimized)
    Q_UNUSED(systemBackdropMode)
    return false;
#endif
}

WindowEffectWin::VisualEffectOptions buildVisualEffectOptions(bool shadowEnabled,
                                                              WindowEffectWin::BackdropMode systemBackdropMode,
                                                              bool roundedCornersEnabled,
                                                              bool systemDarkModeEnabled,
                                                              ThemeManager::ThemeMode themeMode,
                                                              bool maximized,
                                                              bool minimized,
                                                              const QColor &borderColor)
{
    WindowEffectWin::VisualEffectOptions options;
    options.shadowEnabled = shadowEnabled;
    options.SystemBackdropMode = systemBackdropMode;
    options.roundedCornersEnabled = roundedCornersEnabled;
    options.SystemDarkModeEnabled = systemDarkModeEnabled;
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
                              bool roundedCornersEnabled,
                              bool systemDarkModeEnabled,
                              WindowEffectWin::BackdropMode systemBackdropMode,
                              ThemeManager::ThemeMode themeMode,
                              bool translucentBackground)
{
    quint64 token = 0;
    token |= static_cast<quint64>(visible) << 0;
    token |= static_cast<quint64>(maximized) << 1;
    token |= static_cast<quint64>(minimized) << 2;
    token |= static_cast<quint64>(active) << 3;
    token |= static_cast<quint64>(shadowEnabled) << 4;
    token |= static_cast<quint64>(systemBackdropMode != WindowEffectWin::BackdropMode::None) << 5;
    token |= static_cast<quint64>(roundedCornersEnabled) << 6;
    token |= static_cast<quint64>(systemDarkModeEnabled) << 7;
    token |= static_cast<quint64>(shouldUseDarkMode(themeMode)) << 8;
    token |= static_cast<quint64>(translucentBackground) << 9;
    token |= static_cast<quint64>(static_cast<int>(systemBackdropMode) & 0xF) << 10;
    token |= static_cast<quint64>(static_cast<int>(themeMode) & 0x3) << 14;
    return token;
}

} // namespace WindowVisualState
