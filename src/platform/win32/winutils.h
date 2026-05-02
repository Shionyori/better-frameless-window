#pragma once

#include <QPoint>
#include <QRect>
#include <Qt>

#include <cstdint>

namespace WinUtils {

struct WindowsCapabilities {
	bool isWindows = false;
	uint32_t buildNumber = 0;
	bool supportsSystemDarkMode = false;
	bool supportsRoundedCorners = false;
	bool supportsSystemSystemBackdrop = false;
	bool supportsLegacyMica = false;
	bool supportsAcrylic = false;
};

QPoint toLocalPos(const QPoint &globalPos, const QRect &nativeWindowRect, int logicalWidth, int logicalHeight);
int hitFromEdges(Qt::Edges edges);
void syncNativeWindowStyles(void *hwnd, bool includeExStyle);
uint32_t windowsBuildNumber();
WindowsCapabilities detectWindowsCapabilities();

} // namespace WinUtils