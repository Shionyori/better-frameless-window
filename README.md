# better-frameless-window

**Qt6 Windows 无边框基础窗口**：
- 去原生边框 + 自定义标题栏
- 标题栏拖拽、双击最大化/还原、右键系统菜单
- 8 向边缘缩放、HiDPI 命中修正
- Win10/11 视觉效果（阴影、圆角、深色、Mica/Acrylic）


## 现阶段问题

基本功能已经初具雏形，但还是存在不少问题

已知/搁置的问题：

- 深色模式还没有实现
- Mica/Acrylic 视觉效果在最小化窗口后会暂时丢失

下一步计划：

- 修复 FramelessWindow初始化backdropMode 与 setSystemBackdrop() 之间的冲突问题
- 添加 BackgroundMode（Solid/Transparent）设置接口，修改当前的 setWindowOpacity() 接口以适应新的 BackgroundMode 设置
- 调整目录结构，分离平台相关代码和核心功能代码

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

## API 总览（面向使用者）

### FramelessWindow

#### 窗口属性

- `setWindowOpacity(qreal opacity)` - 设置窗口整体不透明度（0.0 - 1.0）
- `setWindowSizeLimits(...)` / `minimumWindowSize()` / `maximumWindowSize()`：窗口尺寸约束
- `setShadowEnabled(...)`：窗口阴影
- `setCentralWidget(...)` / `centralWidget()` / `takeCentralWidget()`：主内容区
- `addTitleBarWidget(...)` / `clearTitleBarWidgets()`：标题栏中部插槽

#### 视觉效果

- `setRoundedCornersEnabled(bool enabled)` - 启用/禁用圆角
- `setSystemDarkModeEnabled(bool enabled)` - 启用/禁用系统深色模式适配
- `setThemeMode(ThemeManager::ThemeMode mode)` - 设置主题模式（Light/Dark/System）
- `setAccentColor(...)` / `accentColor()`：设置/获取系统强调色
- `setSystemEffectsEnabled(bool enabled)` - 启用/禁用系统视觉效果（如 Mica/Acrylic）
- `setSystemBackdrop(WindowEffectWin::BackdropMode mode)` - 设置系统背景效果模式（None/Mica/MicaLegacy/Acrylic）

#### 其他

- `setDiagnosticsEnabled(bool enabled)` - 启用/禁用诊断日志输出（用于调试视觉效果问题）
