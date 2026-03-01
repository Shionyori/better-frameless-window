# better-frameless-window

一个可复用的 **Qt6 Windows 无边框基础窗口**，只提供窗口壳能力：
- 去原生边框 + 自定义标题栏
- 标题栏拖拽、双击最大化/还原、右键系统菜单
- 8 向边缘缩放、HiDPI 命中修正
- Win10/11 视觉能力（阴影、圆角、深色、Mica/Acrylic）及降级

项目不内置业务 UI。你可以把它当作基础窗体，在内容区挂载你自己的页面。

## 最小接入

```cpp
#include <QApplication>
#include <QLabel>
#include "framelesswindow.h"

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	FramelessWindow window;
	auto *page = new QLabel("Your App Content");
	page->setAlignment(Qt::AlignCenter);
	window.setContentWidget(page);

	window.show();
	return app.exec();
}
```

## 基础 API

- `setCentralWidget(QWidget*)` / `centralWidget()` / `takeCentralWidget()`：主内容区接口（推荐，语义对齐 `QMainWindow`）。
- `setContentWidget(QWidget*)` / `contentWidget()` / `takeContentWidget()`：与 `centralWidget` 等价的兼容接口。
- `addTitleBarWidget(QWidget*)`：向标题栏中部插入自定义控件。
- `clearTitleBarWidgets()`：清空标题栏插槽控件。
- `setThemeMode(...)` / `setAccentColor(...)`：主题与强调色。
- `setBackdropPreference(...)`：特效模式偏好，支持 `Auto / None / MicaSystem / MicaLegacy / Acrylic / Aero`。
	- 当指定模式在当前系统不可用时，会自动回退到 `Auto` 可用链（除 `None`）。
- `setShadowEnabled(...)` / `setBackdropEnabled(...)` / `setRoundedCornersEnabled(...)` / `setImmersiveDarkModeEnabled(...)` / `setAeroBlurEnabled(...)`：视觉能力开关。

## 构建

```bash
cmake --preset windows-msvc-local-debug
cmake --build --preset windows-msvc-local-debug
```