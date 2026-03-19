#pragma once

#include "thememanager.h"
#include "windoweffectwin.h"

#include <QColor>

#include <QtGlobal>

namespace WindowVisualState {

bool shouldUseDarkMode(ThemeManager::ThemeMode themeMode);

bool shouldUseTranslucentBackground(bool backdropEnabled,
                                    bool minimized,
                                    WindowEffectWin::BackdropPreference backdropPreference);

WindowEffectWin::VisualEffectOptions buildVisualEffectOptions(bool shadowEnabled,
                                                              bool backdropEnabled,
                                                              WindowEffectWin::BackdropPreference backdropPreference,
                                                              bool roundedCornersEnabled,
                                                              bool immersiveDarkModeEnabled,
                                                              ThemeManager::ThemeMode themeMode,
                                                              bool maximized,
                                                              bool minimized,
                                                              const QColor &borderColor);

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
                              bool translucentBackground);

} // namespace WindowVisualState
