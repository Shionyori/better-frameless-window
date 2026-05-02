#include "systemmenu.h"

#include "diagnostics.h"

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

namespace SystemMenu {

void showForWindow(void *hwnd, const QPoint &globalPos, const WindowState &windowState)
{
#ifdef Q_OS_WIN
    const HWND win = static_cast<HWND>(hwnd);
    if (win == nullptr) {
        Diagnostics::logWarning(QStringLiteral("showSystemMenu: null HWND"));
        return;
    }

    HMENU menu = GetSystemMenu(win, FALSE);
    if (menu == nullptr) {
        Diagnostics::logWarning(QStringLiteral("showSystemMenu: GetSystemMenu returned null"));
        return;
    }

    auto setItemEnabled = [menu](UINT item, bool enabled) {
        EnableMenuItem(menu, item, MF_BYCOMMAND | (enabled ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
    };

    setItemEnabled(SC_RESTORE, windowState.maximized || windowState.minimized);
    setItemEnabled(SC_MOVE, !windowState.maximized && !windowState.minimized);
    setItemEnabled(SC_SIZE, !windowState.maximized && !windowState.minimized);
    setItemEnabled(SC_MINIMIZE, !windowState.minimized);
    setItemEnabled(SC_MAXIMIZE, !windowState.maximized);

    DrawMenuBar(win);

    const POINT cursorPos{globalPos.x(), globalPos.y()};
    const UINT flags = TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON;
    const int command = TrackPopupMenu(menu, flags, cursorPos.x, cursorPos.y, 0, win, nullptr);
    if (command == 0) {
        return;
    }

    const UINT sysCommand = static_cast<UINT>(command) & 0xFFF0;
    if (sysCommand == SC_MOVE || sysCommand == SC_SIZE) {
        ReleaseCapture();
    }

    SendMessage(win, WM_SYSCOMMAND, static_cast<WPARAM>(sysCommand), 0);
#else
    Q_UNUSED(hwnd)
    Q_UNUSED(globalPos)
    Q_UNUSED(windowState)
#endif
}

} // namespace SystemMenu