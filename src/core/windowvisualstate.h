#pragma once

#include "thememanager.h"
#include "win32/windoweffect.h"

#include <QColor>

#include <QtGlobal>

namespace WindowVisualState {

bool shouldUseDarkMode(ThemeManager::ThemeMode themeMode);

// Whether the currently applied backdrop has to be reset before the requested
// one is applied. DWM stores Mica as a window attribute, so it can only be
// replaced after an explicit reset; the accent-policy Acrylic path is
// idempotent, and clearing it makes the blur visibly drop for a frame.
bool shouldClearBackdropBeforeApply(WindowEffect::SystemBackdropMode appliedMode,
                                    WindowEffect::SystemBackdropMode requestedMode);

// Whether applying the requested backdrop would repeat what DWM already has.
// The Acrylic tint is baked into the accent gradient, so a theme switch has to
// re-apply it even when the backdrop mode itself is unchanged.
bool isBackdropApplyRedundant(WindowEffect::SystemBackdropMode requestedMode,
                              bool requestedUseDarkMode,
                              WindowEffect::SystemBackdropMode appliedMode,
                              bool appliedUseDarkMode,
                              bool appliedSuccessfully);

bool shouldUseTranslucentBackground(bool systemBackdropEnabled,
                                    bool minimized,
                                    WindowEffect::SystemBackdropPreference systemBackdropPreference);

WindowEffect::VisualEffectOptions buildVisualEffectOptions(bool shadowEnabled,
                                                              bool systemBackdropEnabled,
                                                              WindowEffect::SystemBackdropPreference systemBackdropPreference,
                                                              bool roundedCornersEnabled,
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
                              WindowEffect::SystemBackdropPreference systemBackdropPreference,
                              ThemeManager::ThemeMode themeMode,
                              bool translucentBackground);

} // namespace WindowVisualState
