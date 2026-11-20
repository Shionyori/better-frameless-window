#include "windowgeometry.h"

#include <QPoint>

namespace {

int distanceToRect(const QPoint &point, const QRect &rect)
{
    const int dx = qMax(0, qMax(rect.left() - point.x(), point.x() - rect.right()));
    const int dy = qMax(0, qMax(rect.top() - point.y(), point.y() - rect.bottom()));
    return dx + dy;
}

} // namespace

namespace WindowGeometry {

int dragStripHeight()
{
    return 40;
}

int minimumVisibleWidth()
{
    return 80;
}

bool isRestorable(const QRect &saved, const QList<QRect> &availableGeometries)
{
    if (!saved.isValid() || availableGeometries.isEmpty()) {
        return false;
    }

    const int stripHeight = qMin(dragStripHeight(), saved.height());
    const int stripWidth = saved.width();
    const QRect dragStrip(saved.left(), saved.top(), stripWidth, stripHeight);

    const int requiredWidth = qMin(minimumVisibleWidth(), stripWidth);

    for (const QRect &available : availableGeometries) {
        const QRect overlap = available.intersected(dragStrip);
        if (overlap.width() >= requiredWidth && overlap.height() >= stripHeight) {
            return true;
        }
    }

    return false;
}

QRect clampToNearestScreen(const QRect &saved, const QList<QRect> &availableGeometries)
{
    if (availableGeometries.isEmpty() || isRestorable(saved, availableGeometries)) {
        return saved;
    }

    const QPoint center = saved.isValid() ? saved.center() : QPoint(0, 0);

    QRect target = availableGeometries.first();
    int bestDistance = distanceToRect(center, target);
    for (const QRect &available : availableGeometries) {
        const int distance = distanceToRect(center, available);
        if (distance < bestDistance) {
            bestDistance = distance;
            target = available;
        }
    }

    const QSize size = saved.size().boundedTo(target.size());
    const int width = qMax(1, size.width());
    const int height = qMax(1, size.height());

    // Entirely on a monitor that is gone: bring it back centred, which is
    // predictable, instead of shoving it into a corner.
    if (!target.intersects(saved)) {
        QRect centred(QPoint(0, 0), QSize(width, height));
        centred.moveCenter(target.center());
        return centred;
    }

    // Partially visible: keep the window where the user left it as far as the
    // screen allows, so the sizing and relative placement survive.
    const int maxX = target.right() - width + 1;
    const int maxY = target.bottom() - height + 1;
    const int x = qBound(target.left(), saved.left(), qMax(target.left(), maxX));
    const int y = qBound(target.top(), saved.top(), qMax(target.top(), maxY));

    return QRect(x, y, width, height);
}

} // namespace WindowGeometry
