#pragma once

#include <QRect>

namespace WindowFrame {

// Moves and resizes the window to an exact native rectangle without touching
// its state or z-order.
void setNativeRect(void *hwnd, const QRect &nativeRect);

// Work area of the window's monitor, in native pixels. Qt reports
// device-independent geometry; these are what the window manager actually uses.
QRect workArea(void *hwnd);
QRect windowRect(void *hwnd);

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