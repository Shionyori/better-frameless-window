#include "windowframewin.h"

#include "winutils.h"

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

namespace WindowFrameWin {

void syncWindowFrame(void *hwnd)
{
#ifdef Q_OS_WIN
    WinUtils::syncNativeWindowStyles(hwnd, true);
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

} // namespace WindowFrameWin
