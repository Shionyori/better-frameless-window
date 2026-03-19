#include "winutils.h"

#include "diagnostics.h"

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

namespace WinUtils {

QPoint toLocalPos(const QPoint &globalPos, const QRect &nativeWindowRect, int logicalWidth, int logicalHeight)
{
    if (nativeWindowRect.isEmpty()) {
        return QPoint(0, 0);
    }

    const int nativeWidth = qMax(1, nativeWindowRect.width());
    const int nativeHeight = qMax(1, nativeWindowRect.height());

    return QPoint(
        qBound(0, ((globalPos.x() - nativeWindowRect.left()) * logicalWidth) / nativeWidth, logicalWidth - 1),
        qBound(0, ((globalPos.y() - nativeWindowRect.top()) * logicalHeight) / nativeHeight, logicalHeight - 1));
}

int hitFromEdges(Qt::Edges edges)
{
#ifdef Q_OS_WIN
    if (edges == (Qt::TopEdge | Qt::LeftEdge))
        return HTTOPLEFT;
    if (edges == (Qt::TopEdge | Qt::RightEdge))
        return HTTOPRIGHT;
    if (edges == (Qt::BottomEdge | Qt::LeftEdge))
        return HTBOTTOMLEFT;
    if (edges == (Qt::BottomEdge | Qt::RightEdge))
        return HTBOTTOMRIGHT;
    if (edges == Qt::TopEdge)
        return HTTOP;
    if (edges == Qt::BottomEdge)
        return HTBOTTOM;
    if (edges == Qt::LeftEdge)
        return HTLEFT;
    if (edges == Qt::RightEdge)
        return HTRIGHT;

    return HTCLIENT;
#else
    Q_UNUSED(edges)
    return 0;
#endif
}

void syncNativeWindowStyles(void *hwnd, bool includeExStyle)
{
#ifdef Q_OS_WIN
    const HWND win = static_cast<HWND>(hwnd);
    if (win == nullptr) {
        return;
    }

    SetLastError(0);
    LONG_PTR style = GetWindowLongPtr(win, GWL_STYLE);
    if (style == 0 && GetLastError() != 0) {
        Diagnostics::logWarning(QStringLiteral("syncNativeWindowStyles: GetWindowLongPtr(GWL_STYLE) failed"));
        return;
    }

    if (includeExStyle) {
        style &= ~static_cast<LONG_PTR>(WS_POPUP);
    }
    style |= WS_OVERLAPPED | WS_THICKFRAME | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU;

    SetLastError(0);
    if (SetWindowLongPtr(win, GWL_STYLE, style) == 0 && GetLastError() != 0) {
        Diagnostics::logWarning(QStringLiteral("syncNativeWindowStyles: SetWindowLongPtr(GWL_STYLE) failed"));
        return;
    }

    if (includeExStyle) {
        SetLastError(0);
        LONG_PTR exStyle = GetWindowLongPtr(win, GWL_EXSTYLE);
        if (exStyle == 0 && GetLastError() != 0) {
            Diagnostics::logWarning(QStringLiteral("syncNativeWindowStyles: GetWindowLongPtr(GWL_EXSTYLE) failed"));
            return;
        }

        exStyle &= ~static_cast<LONG_PTR>(WS_EX_TOOLWINDOW);
        exStyle |= WS_EX_APPWINDOW;

        SetLastError(0);
        if (SetWindowLongPtr(win, GWL_EXSTYLE, exStyle) == 0 && GetLastError() != 0) {
            Diagnostics::logWarning(QStringLiteral("syncNativeWindowStyles: SetWindowLongPtr(GWL_EXSTYLE) failed"));
            return;
        }
    }

    const BOOL updated = SetWindowPos(win, nullptr, 0, 0, 0, 0,
                                      SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    if (updated == FALSE) {
        Diagnostics::logWarning(QStringLiteral("syncNativeWindowStyles: SetWindowPos(SWP_FRAMECHANGED) failed"));
        return;
    }
#else
    Q_UNUSED(hwnd)
    Q_UNUSED(includeExStyle)
#endif
}

uint32_t windowsBuildNumber()
{
#ifdef Q_OS_WIN
    static uint32_t cachedBuild = 0;
    static bool initialized = false;
    if (initialized) {
        return cachedBuild;
    }

    initialized = true;

    using RtlGetVersionPtr = LONG(WINAPI *)(PRTL_OSVERSIONINFOW);
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (ntdll == nullptr) {
        return cachedBuild;
    }

    const auto rtlGetVersion = reinterpret_cast<RtlGetVersionPtr>(GetProcAddress(ntdll, "RtlGetVersion"));
    if (rtlGetVersion == nullptr) {
        return cachedBuild;
    }

    RTL_OSVERSIONINFOW ver{};
    ver.dwOSVersionInfoSize = sizeof(ver);
    if (rtlGetVersion(&ver) == 0) {
        cachedBuild = static_cast<uint32_t>(ver.dwBuildNumber);
    }

    return cachedBuild;
#else
    return 0;
#endif
}

WindowsCapabilities detectWindowsCapabilities()
{
    WindowsCapabilities caps;

#ifdef Q_OS_WIN
    caps.isWindows = true;
    caps.buildNumber = windowsBuildNumber();
    caps.supportsImmersiveDarkMode = caps.buildNumber >= 17763;
    caps.supportsRoundedCorners = caps.buildNumber >= 22000;
    caps.supportsSystemBackdrop = caps.buildNumber >= 22621;
    caps.supportsLegacyMica = caps.buildNumber >= 22000;
    caps.supportsAcrylic = caps.buildNumber >= 17763;
#endif

    return caps;
}

} // namespace WinUtils
