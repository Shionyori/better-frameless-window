#pragma once

#include "thememanager.h"
#include "windoweffectwin.h"

#include <QColor>

#include <QtGlobal>

namespace WindowVisualState {

bool shouldUseDarkMode(ThemeManager::ThemeMode themeMode);

bool shouldUseTranslucentBackground(bool systemBackdropEnabled,
                                    bool minimized,
                                    WindowEffectWin::SystemBackdropPreference systemBackdropPreference);

WindowEffectWin::VisualEffectOptions buildVisualEffectOptions(bool shadowEnabled,
                                                              bool systemBackdropEnabled,
                                                              WindowEffectWin::SystemBackdropPreference systemBackdropPreference,
                                                              bool roundedCornersEnabled,
                                                              bool systemDarkModeEnabled,
                                                              ThemeManager::ThemeMode themeMode,
                                                              bool maximized,
                                                              bool minimized,
                                                              const QColor &borderColor);

quint64 buildVisualStateToken(bool visible,
                              bool maximized,
                              bool minimized,
                              bool active,
                              bool shadowEnabled,
                              bool systemBackdropEnabled,
                              bool roundedCornersEnabled,
                              bool systemDarkModeEnabled,
                              WindowEffectWin::SystemBackdropPreference systemBackdropPreference,
                              ThemeManager::ThemeMode themeMode,
                              bool translucentBackground);

} // namespace WindowVisualState
