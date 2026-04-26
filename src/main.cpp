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

    window.setThemeMode(ThemeManager::ThemeMode::Light);
    window.setShadowEnabled(true);
    window.setSystemBackdrop(WindowEffectWin::BackdropMode::MicaLegacy);
    window.setRoundedCornersEnabled(true);
    window.setSystemDarkModeEnabled(true);
    window.setWindowSizeLimits(QSize(520, 360), QSize());

    window.show();
    return app.exec();
}