#include <QApplication>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

#include "framelesswindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    FramelessWindow window;
    window.setDiagnosticsEnabled(true); // enable diagnostics logging for visual effect debugging
    window.setWindowOpacityLevel(0.96);
    window.setThemeMode(ThemeManager::ThemeMode::Light);
    window.setBackgroundMode(ThemeManager::BackgroundMode::Solid);
    window.setShadowEnabled(true);
    window.setNativeEffectsEnabled(false);
    window.setNativeBackdropPreference(WindowEffectWin::BackdropPreference::None);
    window.setRoundedCornersEnabled(true);
    window.setImmersiveDarkModeEnabled(true);
    window.setWindowSizeLimits(QSize(520, 360), QSize());

    window.show();
    return app.exec();
}