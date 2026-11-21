#include "windowhittest.h"

#include "utils.h"

#include <QRect>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

namespace {

#ifdef Q_OS_WIN
// Frame metrics only change with the DPI factor. Caching them keeps the
// per-mouse-move cursor update (and every WM_NCHITTEST) free of system calls.
int nativeResizeBorder(UINT dpi)
{
    static UINT cachedDpi = 0;
    static int cachedBorder = 0;

    if (cachedDpi == dpi) {
        return cachedBorder;
    }

    const int frame = GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi);
    const int padded = GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
    cachedDpi = dpi;
    cachedBorder = frame + padded;
    return cachedBorder;
}
#endif

} // namespace

namespace WindowHitTest {

int resizeBorderThickness(void *hwnd)
{
#ifdef Q_OS_WIN
    const HWND win = static_cast<HWND>(hwnd);
    const UINT dpi = win != nullptr ? GetDpiForWindow(win) : 96;
    if (dpi == 0) {
        return 4;
    }

    // Windows scales the frame metrics with the DPI, so converting back to
    // logical pixels keeps the grab area the same physical size on every
    // monitor instead of shrinking to 4px on a 150% display.
    const int native = nativeResizeBorder(dpi);
    const int logical = (native * 96 + static_cast<int>(dpi) / 2) / static_cast<int>(dpi);
    return qBound(4, logical, 12);
#else
    Q_UNUSED(hwnd)
    return 4;
#endif
}

Qt::Edges edgesAtLocalPos(const QPoint &localPos,
                          const QSize &logicalSize,
                          int borderThickness,
                          bool maximized,
                          bool fullScreen)
{
    if (maximized || fullScreen || borderThickness <= 0) {
        return Qt::Edges();
    }

    const int w = logicalSize.width();
    const int h = logicalSize.height();
    if (w <= 0 || h <= 0) {
        return Qt::Edges();
    }

    const int x = localPos.x();
    const int y = localPos.y();
    if (x < 0 || y < 0 || x >= w || y >= h) {
        return Qt::Edges();
    }

    const int corner = borderThickness + 2;

    if (x < corner && y < corner) {
        return Qt::TopEdge | Qt::LeftEdge;
    }
    if (x >= w - corner && y < corner) {
        return Qt::TopEdge | Qt::RightEdge;
    }
    if (x < corner && y >= h - corner) {
        return Qt::BottomEdge | Qt::LeftEdge;
    }
    if (x >= w - corner && y >= h - corner) {
        return Qt::BottomEdge | Qt::RightEdge;
    }

    Qt::Edges edges;
    if (x < borderThickness) {
        edges |= Qt::LeftEdge;
    }
    if (x >= w - borderThickness) {
        edges |= Qt::RightEdge;
    }
    if (y < borderThickness) {
        edges |= Qt::TopEdge;
    }
    if (y >= h - borderThickness) {
        edges |= Qt::BottomEdge;
    }

    return edges;
}

int nonClientHitTest(const Context &context, const QPoint &globalPos)
{
#ifdef Q_OS_WIN
    const HWND hwnd = static_cast<HWND>(context.hwnd);
    if (hwnd == nullptr || context.fullScreen) {
        return HTCLIENT;
    }

    RECT windowRect{};
    if (!GetWindowRect(hwnd, &windowRect)) {
        return HTCLIENT;
    }

    const int left = windowRect.left;
    const int top = windowRect.top;
    const int right = windowRect.right - 1;
    const int bottom = windowRect.bottom - 1;
    const int x = globalPos.x();
    const int y = globalPos.y();

    if (x < left || x > right || y < top || y > bottom) {
        return HTCLIENT;
    }

    const int logicalWidth = qMax(1, context.logicalWidth);
    const int logicalHeight = qMax(1, context.logicalHeight);
    const QRect nativeWindowRect(left, top, right - left + 1, bottom - top + 1);
    const QPoint localPos = Utils::toLocalPos(globalPos, nativeWindowRect, logicalWidth, logicalHeight);

    const TitleRegion titleRegion = context.titleRegionResolver
        ? context.titleRegionResolver(localPos)
        : TitleRegion::None;

    const bool onTitleBarCaptionArea = titleRegion == TitleRegion::Caption;

    // The resize border wins over everything that lives inside it, caption
    // buttons included: the caption buttons reach the window edge now, and
    // without this the top/right border below them would stop resizing (and
    // therefore stop being draggable at all).
    if (!context.maximized) {
        const Qt::Edges edges = edgesAtLocalPos(localPos,
                                                QSize(logicalWidth, logicalHeight),
                                                resizeBorderThickness(context.hwnd),
                                                false,
                                                false);
        if (edges != Qt::Edges()) {
            return Utils::hitFromEdges(edges);
        }
    }

    if (titleRegion == TitleRegion::MaximizeButton) {
        if (GetKeyState(VK_LBUTTON) < 0) {
            return HTCLIENT;
        }
        if (context.snapLayoutEnabled) {
            return HTMAXBUTTON;
        }
        return HTCLIENT;
    }

    if (titleRegion == TitleRegion::MinimizeButton
        || titleRegion == TitleRegion::CloseButton
        || titleRegion == TitleRegion::OtherInteractive) {
        return HTCLIENT;
    }

    if (context.maximized) {
        return onTitleBarCaptionArea ? HTCAPTION : HTCLIENT;
    }

    if (onTitleBarCaptionArea) {
        return HTCAPTION;
    }

    return HTCLIENT;
#else
    Q_UNUSED(context)
    Q_UNUSED(globalPos)
    return 0;
#endif
}

} // namespace WindowHitTest
