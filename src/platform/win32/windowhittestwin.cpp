#include "windowhittestwin.h"

#include "winutils.h"

#include <QRect>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

namespace WindowHitTestWin {

int nonClientHitTest(const Context &context, const QPoint &globalPos)
{
#ifdef Q_OS_WIN
    const HWND hwnd = static_cast<HWND>(context.hwnd);
    if (hwnd == nullptr) {
        return HTCLIENT;
    }

    RECT windowRect{};
    if (!GetWindowRect(hwnd, &windowRect)) {
        return HTCLIENT;
    }

    const UINT dpi = GetDpiForWindow(hwnd);
    const int frame = GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi);
    const int padded = GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
    const int border = qBound(4, frame + padded, 12);
    const int corner = border + 2;

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
    const QPoint localPos = WinUtils::toLocalPos(globalPos, nativeWindowRect, logicalWidth, logicalHeight);

    const TitleRegion titleRegion = context.titleRegionResolver
        ? context.titleRegionResolver(localPos)
        : TitleRegion::None;

    if (titleRegion == TitleRegion::MaximizeButton) {
        if (GetKeyState(VK_LBUTTON) < 0) {
            return HTCLIENT;
        }
        return HTMAXBUTTON;
    }

    if (titleRegion == TitleRegion::MinimizeButton
        || titleRegion == TitleRegion::CloseButton
        || titleRegion == TitleRegion::OtherInteractive) {
        return HTCLIENT;
    }

    const bool onTitleBarCaptionArea = titleRegion == TitleRegion::Caption;
    if (context.maximized) {
        return onTitleBarCaptionArea ? HTCAPTION : HTCLIENT;
    }

    Qt::Edges edges;

    const bool onLeftCorner = (x >= left - corner && x < left + corner);
    const bool onRightCorner = (x <= right + corner && x > right - corner);
    const bool onTopCorner = (y >= top - corner && y < top + corner);
    const bool onBottomCorner = (y <= bottom + corner && y > bottom - corner);

    if (onLeftCorner && onTopCorner)
        edges = Qt::TopEdge | Qt::LeftEdge;
    else if (onRightCorner && onTopCorner)
        edges = Qt::TopEdge | Qt::RightEdge;
    else if (onLeftCorner && onBottomCorner)
        edges = Qt::BottomEdge | Qt::LeftEdge;
    else if (onRightCorner && onBottomCorner)
        edges = Qt::BottomEdge | Qt::RightEdge;
    else {
        if (x >= left - border && x < left + border)
            edges |= Qt::LeftEdge;
        if (x <= right + border && x > right - border)
            edges |= Qt::RightEdge;
        if (y >= top - border && y < top + border)
            edges |= Qt::TopEdge;
        if (y <= bottom + border && y > bottom - border)
            edges |= Qt::BottomEdge;
    }

    if (edges != Qt::Edges()) {
        return WinUtils::hitFromEdges(edges);
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

} // namespace WindowHitTestWin
