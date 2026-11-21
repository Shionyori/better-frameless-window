#include "windowcommand.h"

#include <QtGlobal>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

namespace WindowCommand {

bool setMaximized(void *hwnd, bool maximized)
{
#ifdef Q_OS_WIN
    const HWND win = static_cast<HWND>(hwnd);
    if (win == nullptr) {
        return false;
    }

    ShowWindow(win, maximized ? SW_MAXIMIZE : SW_RESTORE);

    // ShowWindow only reports the *previous* visibility; the window state is
    // what matters here, so ask for it.
    return IsZoomed(win) == (maximized ? TRUE : FALSE);
#else
    Q_UNUSED(hwnd)
    Q_UNUSED(maximized)
    return false;
#endif
}

} // namespace WindowCommand
