#include "windowcommand.h"

#include <QtGlobal>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

namespace WindowCommand {

bool toggleMaximizeRestore(void *hwnd, bool currentlyMaximized)
{
#ifdef Q_OS_WIN
    const HWND win = static_cast<HWND>(hwnd);
    if (win == nullptr) {
        return false;
    }

    const WPARAM command = currentlyMaximized ? SC_RESTORE : SC_MAXIMIZE;
    SendMessage(win, WM_SYSCOMMAND, command, 0);
    return true;
#else
    Q_UNUSED(hwnd)
    Q_UNUSED(currentlyMaximized)
    return false;
#endif
}

} // namespace WindowCommand