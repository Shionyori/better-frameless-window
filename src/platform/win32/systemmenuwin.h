#pragma once

#include <QPoint>

namespace SystemMenuWin {

struct WindowState {
    bool maximized = false;
    bool minimized = false;
};

void showForWindow(void *hwnd, const QPoint &globalPos, const WindowState &windowState);

} // namespace SystemMenuWin
