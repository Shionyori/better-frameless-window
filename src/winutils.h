#pragma once

#include <QPoint>
#include <QRect>
#include <Qt>

class TitleBar;
class QWidget;

namespace WinUtils {

void setMaximizeButtonNativeHover(TitleBar *titleBar, bool hovered);
QPoint toLocalPos(const QPoint &globalPos, const QRect &nativeWindowRect, int logicalWidth, int logicalHeight);
int hitFromEdges(Qt::Edges edges);
void syncNativeWindowStyles(void *hwnd, bool includeExStyle);
bool isOnMaximizeButton(const TitleBar *titleBar, const QWidget *hostWidget, const QPoint &localPos);
bool isOnTitleBarCaptionArea(const TitleBar *titleBar, const QWidget *hostWidget, const QPoint &localPos);

} // namespace WinUtils
