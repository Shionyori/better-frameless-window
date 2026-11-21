#pragma once

#include <QRect>

namespace WindowFrame {

// Whether the window is maximized, straight from the window manager. Qt learns
// the state from WM_SIZE asynchronously, so this is the only synchronous answer
// available while a native maximize is being processed.
bool isMaximized(void *hwnd);

void syncWindowFrame(void *hwnd);
void forceDwmRefresh(void *hwnd);

// The geometry the window returns to when restored, read from the window
// manager (WINDOWPLACEMENT::rcNormalPosition). Reported in native pixels;
// returns an invalid rect when the window is minimized or has no placement.
QRect restoreGeometry(void *hwnd);

} // namespace WindowFrame