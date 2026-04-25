# better-frameless-window

**Qt6 Windows 无边框基础窗口**：
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

## API 总览

### FramelessWindow

#### 窗口本体

- `setWindowOpacityLevel(...)` / `windowOpacityLevel()`：窗口透明度
- `setWindowSizeLimits(...)` / `minimumWindowSize()` / `maximumWindowSize()`：窗口尺寸约束
- `setShadowEnabled(...)` / `isShadowEnabled()`：阴影开关
- `setCentralWidget(...)` / `centralWidget()` / `takeCentralWidget()`：主内容区
- `addTitleBarWidget(...)` / `clearTitleBarWidgets()`：标题栏中部插槽
- `setDiagnosticsEnabled(...)` / `isDiagnosticsEnabled()`：诊断日志开关

#### 原生效果

- `setNativeEffectsEnabled(...)` / `isNativeEffectsEnabled()`：原生效果总开关，默认关闭
- `setNativeBackdropPreference(...)` / `nativeBackdropPreference()`：原生背景偏好，支持 `Auto / None / MicaSystem / MicaLegacy / Acrylic`
- `setRoundedCornersEnabled(...)` / `isRoundedCornersEnabled()`：圆角开关
- `setImmersiveDarkModeEnabled(...)` / `isImmersiveDarkModeEnabled()`：深色模式开关
- `setThemeMode(...)` / `themeMode()`：主题模式
- `setAccentColor(...)` / `accentColor()`：强调色
- `setBackgroundMode(...)` / `backgroundMode()`：背景模式

### TitleBar

- `TitleBar(QWidget *parent = nullptr)`：标题栏组件
- `heightHint()`：标题栏建议高度
- `setMaximized(bool)`：同步最大化状态
- `addCenterWidget(QWidget*)`：在标题栏中部插入控件
- `clearCenterWidgets()`：清空中部控件
- `hitRegionAt(const QPoint &)`：命中区域识别
- `minimizeRequested()` / `maximizeRestoreRequested()` / `closeRequested()` / `systemMoveRequested()` / `systemMenuRequested(const QPoint &)`：交互信号

### WindowEffectWin

- `BackdropPreference`：`Auto / None / MicaSystem / MicaLegacy / Acrylic`
- `BackdropMode`：底层选中的实际背景模式
- `VisualEffectOptions`：视觉效果参数包，`nativeEffectsEnabled` 默认关闭
- `applyVisualEffects(...)`：一次性应用所有窗口效果
- `applyShadow(...)`：阴影
- `applyRoundedCorners(...)`：圆角
- `applyImmersiveDarkMode(...)`：深色模式
- `applyNativeBackdropEffects(...)`：背景材质
- `applyBorderColor(...)`：边框色

### 内部辅助模块

- `VisualRefreshCoordinator::configure(...)` / `requestRefresh()`
- `Diagnostics::setEnabled(...)` / `isEnabled()` / `logWarning(...)`
- `WinUtils::toLocalPos(...)` / `hitFromEdges(...)` / `syncNativeWindowStyles(...)` / `windowsBuildNumber()` / `detectWindowsCapabilities()`
- `WindowFrameWin::syncWindowFrame(...)` / `forceDwmRefresh(...)`
- `WindowCommandWin::toggleMaximizeRestore(...)`
- `SystemMenuWin::WindowState` / `SystemMenuWin::showForWindow(...)`
- `WindowHitTestWin::TitleRegion` / `WindowHitTestWin::Context` / `resizeBorderThickness(...)` / `nonClientHitTest(...)`
- `NativeWindowsMessageRouter::handle(...)`
