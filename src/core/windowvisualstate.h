#pragma once

#include "thememanager.h"
#include "windoweffectwin.h"

#include <QColor>

#include <QtGlobal>

namespace WindowVisualState {

bool shouldUseDarkMode(ThemeManager::ThemeMode themeMode);

bool shouldUseTranslucentBackground(bool minimized,
                                    WindowEffectWin::BackdropMode systemBackdropMode);

WindowEffectWin::VisualEffectOptions buildVisualEffectOptions(bool shadowEnabled,
                                                              WindowEffectWin::BackdropMode systemBackdropMode,
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
                              bool roundedCornersEnabled,
                              bool systemDarkModeEnabled,
                              WindowEffectWin::BackdropMode systemBackdropMode,
                              ThemeManager::ThemeMode themeMode,
                              bool translucentBackground);

} // namespace WindowVisualState
