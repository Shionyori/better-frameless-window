#pragma once

namespace WindowCommand {

// Shows a top-level window maximized or restored. The caller animates the
// geometry first and flips the state last, so this only changes the window
// state, never the rectangle.
bool setMaximized(void *hwnd, bool maximized);

} // namespace WindowCommand
