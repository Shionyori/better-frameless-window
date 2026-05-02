#pragma once

#include <QPoint>

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
    std::function<TitleRegion(const QPoint &)> titleRegionResolver;
};

int resizeBorderThickness(void *hwnd);
int nonClientHitTest(const Context &context, const QPoint &globalPos);

} // namespace WindowHitTest