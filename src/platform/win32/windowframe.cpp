#include "windowframe.h"

#include "utils.h"

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

namespace WindowFrame {

void syncWindowFrame(void *hwnd)
{
#ifdef Q_OS_WIN
    Utils::syncNativeWindowStyles(hwnd, true);
#else
    Q_UNUSED(hwnd)
#endif
}

void forceDwmRefresh(void *hwnd)
{
#ifdef Q_OS_WIN
    const HWND win = static_cast<HWND>(hwnd);
    if (win == nullptr) {
        return;
    }

    SetWindowPos(win,
                 nullptr,
                 0,
                 0,
                 0,
                 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    RedrawWindow(win,
                 nullptr,
                 nullptr,
                 RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
#else
    Q_UNUSED(hwnd)
#endif
}


QRect workArea(void *hwnd)
{
#ifdef Q_OS_WIN
    const HWND win = static_cast<HWND>(hwnd);
    if (win == nullptr) {
        return QRect();
    }

    MONITORINFO info{};
    info.cbSize = sizeof(info);
    if (!GetMonitorInfoW(MonitorFromWindow(win, MONITOR_DEFAULTTONEAREST), &info)) {
        return QRect();
    }

    const RECT &work = info.rcWork;
    return QRect(work.left, work.top, work.right - work.left, work.bottom - work.top);
#else
    Q_UNUSED(hwnd)
    return QRect();
#endif
}

QRect windowRect(void *hwnd)
{
#ifdef Q_OS_WIN
    const HWND win = static_cast<HWND>(hwnd);
    if (win == nullptr) {
        return QRect();
    }

    RECT rect{};
    if (!GetWindowRect(win, &rect)) {
        return QRect();
    }

    return QRect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
#else
    Q_UNUSED(hwnd)
    return QRect();
#endif
}

void setNativeRect(void *hwnd, const QRect &nativeRect)
{
#ifdef Q_OS_WIN
    const HWND win = static_cast<HWND>(hwnd);
    if (win == nullptr || !nativeRect.isValid()) {
        return;
    }

    SetWindowPos(win,
                 nullptr,
                 nativeRect.x(),
                 nativeRect.y(),
                 nativeRect.width(),
                 nativeRect.height(),
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
#else
    Q_UNUSED(hwnd)
    Q_UNUSED(nativeRect)
#endif
}

bool isMaximized(void *hwnd)
{
#ifdef Q_OS_WIN
    const HWND win = static_cast<HWND>(hwnd);
    return win != nullptr && IsZoomed(win) != FALSE;
#else
    Q_UNUSED(hwnd)
    return false;
#endif
}

QRect restoreGeometry(void *hwnd)
{
#ifdef Q_OS_WIN
    const HWND win = static_cast<HWND>(hwnd);
    if (win == nullptr) {
        return QRect();
    }

    WINDOWPLACEMENT placement{};
    placement.length = sizeof(placement);
    if (!GetWindowPlacement(win, &placement)) {
        return QRect();
    }

    if (placement.showCmd == SW_SHOWMINIMIZED) {
        return QRect();
    }

    const RECT &rect = placement.rcNormalPosition;
    return QRect(rect.left,
                 rect.top,
                 rect.right - rect.left,
                 rect.bottom - rect.top);
#else
    Q_UNUSED(hwnd)
    return QRect();
#endif
}

} // namespace WindowFrame