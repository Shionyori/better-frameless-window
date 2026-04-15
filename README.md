# better-frameless-window

一个**Qt6 Windows 无边框基础窗口**：
- 去原生边框 + 自定义标题栏
- 标题栏拖拽、双击最大化/还原、右键系统菜单
- 8 向边缘缩放、HiDPI 命中修正
- Win10/11 视觉效果（阴影、圆角、深色、Mica/Acrylic）


## 现阶段问题

基本功能已经初具雏形，但还是存在不少问题：

- 深色模式还没有实现
- Mica/Acrylic 视觉效果在最小化窗口后会暂时丢失

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
	window.setCentralWidget(page);

	window.show();
	return app.exec();
}
```

## 基础 API

- `setCentralWidget(QWidget*)` / `centralWidget()` / `takeCentralWidget()`：主内容区接口
- `addTitleBarWidget(QWidget*)`：向标题栏中部插入自定义控件
- `clearTitleBarWidgets()`：清空标题栏插槽控件
- `setThemeMode(...)` / `setAccentColor(...)`：主题与强调色
- `setBackdropPreference(...)`：特效模式偏好，参数可选 `Auto / None / MicaSystem / MicaLegacy / Acrylic`
- `setShadowEnabled(...)` / `setBackdropEnabled(...)` / `setRoundedCornersEnabled(...)` / `setImmersiveDarkModeEnabled(...)`：视觉效果开关
