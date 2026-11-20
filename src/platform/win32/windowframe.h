#pragma once

#include <QRect>

namespace WindowFrame {

void syncWindowFrame(void *hwnd);
void forceDwmRefresh(void *hwnd);

// The geometry the window returns to when restored, read from the window
// manager (WINDOWPLACEMENT::rcNormalPosition). Reported in native pixels;
// returns an invalid rect when the window is minimized or has no placement.
QRect restoreGeometry(void *hwnd);

} // namespace WindowFrame