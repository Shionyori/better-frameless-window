#pragma once

#include "thememanager.h"
#include "win32/windoweffect.h"

#include <QColor>

#include <QtGlobal>

namespace WindowVisualState {

// What the window actually paints. Exactly one of these is live at any time:
//   Opaque      - the theme paints a solid background, DWM draws nothing behind
//   Translucent - the window surface has alpha and DWM draws the backdrop
enum class SurfaceKind {
    Opaque,
    Translucent
};

// Only the backdrops that can actually end up on screen.
struct BackdropCapabilities {
    bool mica = false;
    bool acrylic = false;
};

// The resolved surface: the Qt side (translucency, theme wash) and the DWM
// side (backdrop kind) are derived from this one value, so they cannot
// disagree.
struct SurfacePolicy {
    SurfaceKind kind = SurfaceKind::Opaque;
    WindowEffect::BackdropKind backdrop = WindowEffect::BackdropKind::None;
};

// Single source of truth for the window surface.
//
// Three surface families, each owning its background completely:
//
//   BackdropMode::None  -> opaque, painted by the application: background
//                          colour, image and opacity are its business.
//   Mica                -> translucent, the system material (a solid-looking
//                          colour derived from the wallpaper) owns the surface.
//   Acrylic             -> translucent, the frosted material owns the surface.
//
// Precedence:
//   1. minimized -> the window manager owns the state, DWM drops the backdrop
//   2. requested None -> opaque, no backdrop
//   3. requested Auto/Mica/Acrylic -> best supported backdrop, otherwise opaque
//
// A translucent surface without a backdrop never happens: it would expose the
// desktop instead of blending with anything. Because the material owns the
// surface, the application-painted background is not used while it is active.
SurfacePolicy resolveSurfacePolicy(WindowEffect::BackdropMode requested,
                                   bool minimized,
                                   const BackdropCapabilities &capabilities);

// Same as above, using the capabilities of the running OS.
SurfacePolicy resolveSurfacePolicy(WindowEffect::BackdropMode requested, bool minimized);

bool usesTranslucentSurface(const SurfacePolicy &policy);

bool shouldUseDarkMode(ThemeManager::ThemeMode themeMode);

// Whether the currently applied backdrop has to be reset before the requested
// one is applied. DWM stores Mica as a window attribute, so it can only be
// replaced after an explicit reset; the accent-policy Acrylic path is
// idempotent, and clearing it makes the blur visibly drop for a frame.
bool shouldClearBackdropBeforeApply(WindowEffect::BackdropKind appliedKind,
                                    WindowEffect::BackdropKind requestedKind);

// Whether applying the requested backdrop would repeat what DWM already has.
// The Acrylic tint is baked into the accent gradient, so a theme switch has to
// re-apply it even when the backdrop kind itself is unchanged.
bool isBackdropApplyRedundant(WindowEffect::BackdropKind requestedKind,
                              bool requestedDarkMode,
                              WindowEffect::BackdropKind appliedKind,
                              bool appliedDarkMode,
                              bool appliedSuccessfully);

WindowEffect::VisualEffectOptions buildVisualEffectOptions(bool shadowEnabled,
                                                           bool roundedCornersEnabled,
                                                           ThemeManager::ThemeMode themeMode,
                                                           bool maximized,
                                                           bool minimized,
                                                           const SurfacePolicy &surface,
                                                           const QColor &borderColor);

quint64 buildVisualStateToken(bool visible,
                              bool maximized,
                              bool minimized,
                              bool active,
                              bool shadowEnabled,
                              bool roundedCornersEnabled,
                              WindowEffect::BackdropKind backdrop,
                              ThemeManager::ThemeMode themeMode,
                              bool translucentSurface);

} // namespace WindowVisualState
