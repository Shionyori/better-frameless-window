#pragma once

#include <QPoint>
#include <QRect>
#include <Qt>

#include <cstdint>

class TitleBar;
class QWidget;

namespace WinUtils {

enum class TitleBarButtonType {
	None,
	Minimize,
	Maximize,
	Close,
	Other
};

struct WindowsCapabilities {
	bool isWindows = false;
	uint32_t buildNumber = 0;
	bool supportsImmersiveDarkMode = false;
	bool supportsRoundedCorners = false;
	bool supportsSystemBackdrop = false;
	bool supportsLegacyMica = false;
	bool supportsAcrylic = false;
};

QPoint toLocalPos(const QPoint &globalPos, const QRect &nativeWindowRect, int logicalWidth, int logicalHeight);
int hitFromEdges(Qt::Edges edges);
void syncNativeWindowStyles(void *hwnd, bool includeExStyle);
uint32_t windowsBuildNumber();
WindowsCapabilities detectWindowsCapabilities();
TitleBarButtonType titleBarButtonTypeAt(const TitleBar *titleBar, const QWidget *hostWidget, const QPoint &localPos);
bool isOnMaximizeButton(const TitleBar *titleBar, const QWidget *hostWidget, const QPoint &localPos);
bool isOnTitleBarCaptionArea(const TitleBar *titleBar, const QWidget *hostWidget, const QPoint &localPos);

} // namespace WinUtils
