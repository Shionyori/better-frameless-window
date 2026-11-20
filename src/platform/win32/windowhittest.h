#pragma once

#include <QPoint>
#include <QSize>

#include <Qt>

#include <functional>

namespace WindowHitTest {

enum class TitleRegion {
    None,
    Caption,
    MinimizeButton,
    MaximizeButton,
    CloseButton,
    OtherInteractive
};

struct Context {
    void *hwnd = nullptr;
    int logicalWidth = 0;
    int logicalHeight = 0;
    bool maximized = false;
    bool fullScreen = false;
    bool snapLayoutEnabled = true;
    std::function<TitleRegion(const QPoint &)> titleRegionResolver;
};

// Resize border thickness in logical pixels for the window's current DPI.
int resizeBorderThickness(void *hwnd);

// Single source of truth for "which window edge is this point on", shared by
// the Qt cursor/startSystemResize path and the native WM_NCHITTEST path so the
// two can never disagree.
Qt::Edges edgesAtLocalPos(const QPoint &localPos,
                          const QSize &logicalSize,
                          int borderThickness,
                          bool maximized,
                          bool fullScreen);

int nonClientHitTest(const Context &context, const QPoint &globalPos);

} // namespace WindowHitTest