#pragma once

#include "thememanager.h"
#include "windoweffectwin.h"

#include <QColor>

#include <QtGlobal>

namespace WindowVisualState {

bool shouldUseDarkMode(ThemeManager::ThemeMode themeMode);

bool shouldUseTranslucentBackground(bool nativeEffectsEnabled,
                                    bool minimized,
                                    WindowEffectWin::BackdropPreference nativeBackdropPreference);

WindowEffectWin::VisualEffectOptions buildVisualEffectOptions(bool shadowEnabled,
                                                              bool nativeEffectsEnabled,
                                                              WindowEffectWin::BackdropPreference nativeBackdropPreference,
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
