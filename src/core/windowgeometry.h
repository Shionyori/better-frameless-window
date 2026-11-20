#pragma once

#include <QList>
#include <QRect>

// Pure helpers for deciding whether a persisted window rectangle can be
// restored as-is. Kept free of any window/OS dependency so it can be unit
// tested headlessly.
namespace WindowGeometry {

// Height of the strip at the top of the saved rectangle that has to stay
// reachable, approximating the window caption the user grabs to move it.
int dragStripHeight();

// Minimum width of that strip that has to be visible on a screen.
int minimumVisibleWidth();

// Whether enough of the saved rectangle is on a screen that the user can find
// and grab it again. A rectangle that only touches a screen edge by a few
// pixels is not restorable even though it "intersects" one.
bool isRestorable(const QRect &saved, const QList<QRect> &availableGeometries);

// Move a saved rectangle onto the nearest available screen, shrinking it when
// it does not fit, so a window persisted on a since-disconnected monitor does
// not come back off-screen.
QRect clampToNearestScreen(const QRect &saved, const QList<QRect> &availableGeometries);

} // namespace WindowGeometry
