#include "thememanager.h"

namespace {
QString colorToCss(const QColor &color)
{
    if (color.alpha() == 255) {
        return color.name(QColor::HexRgb);
    }
    return QStringLiteral("rgba(%1, %2, %3, %4)")
        .arg(color.red())
        .arg(color.green())
        .arg(color.blue())
        .arg(color.alphaF(), 0, 'f', 2);
}

QString buildWindowBackgroundRule(const QColor &windowBg,
                                  bool transparentWindowBackground,
                                  bool dark)
{
    if (transparentWindowBackground) {
        const QString translucentBase = dark
            ? QStringLiteral("rgba(0, 0, 0, 0.35)")
            : QStringLiteral("rgba(255, 255, 255, 0.15)");
        return QStringLiteral("background-color: %1; background-image: none;")
            .arg(translucentBase);
    }

    return QStringLiteral("background-color: %1;").arg(colorToCss(windowBg));
}
}

void ThemeManager::setThemeMode(ThemeMode mode)
{
    m_themeMode = mode;
}

ThemeManager::ThemeMode ThemeManager::themeMode() const
{
    return m_themeMode;
}

void ThemeManager::setAccentColor(const QColor &accentColor)
{
    if (accentColor.isValid()) {
        m_accentColor = accentColor;
    }
}

QColor ThemeManager::accentColor() const
{
    return m_accentColor;
}

bool ThemeManager::isDarkMode() const
{
    return m_themeMode == ThemeMode::Dark;
}

QString ThemeManager::buildStyleSheet(bool transparentWindowBackground) const
{
    const bool dark = isDarkMode();

    const QColor windowBg = dark ? QColor(25, 25, 25) : QColor(245, 245, 245);
    const QColor titleBg = dark ? QColor(45, 45, 45) : QColor(250, 250, 250);
    const QColor titleBorder = dark ? QColor(42, 42, 42) : QColor(224, 224, 224);
    const QColor textColor = dark ? QColor(250, 250, 250) : QColor(26, 26, 26);
    const QColor contentColor = dark ? QColor(224, 224, 224) : QColor(58, 58, 58);
    const QColor buttonHover = dark ? QColor(255, 255, 255, 20) : QColor(0, 0, 0, 10);
    const QColor buttonPressed = dark ? QColor(255, 255, 255, 31) : QColor(0, 0, 0, 20);
    const QColor disabledColor = dark ? QColor(255, 255, 255, 89) : QColor(0, 0, 0, 64);
    const QColor closeHover = QColor(196, 43, 28);
    const QColor buttonBg = dark ? QColor(255, 255, 255, 12) : QColor(0, 0, 0, 6);

    const QString windowBackgroundRule = buildWindowBackgroundRule(windowBg,
                                                                   transparentWindowBackground,
                                                                   dark);

    return QStringLiteral(R"(
        #FramelessWindow {
            %1
            border: none;
        }
        #FramelessContentPanel {
            background: transparent;
            border: none;
        }
        QPushButton {
            color: %4;
            background: %11;
            border: none;
            border-radius: 5px;
            padding: 5px 14px;
            font-family: "Segoe UI";
            font-size: 12px;
        }
        QPushButton:hover {
            background: %6;
        }
        QPushButton:pressed {
            background: %7;
        }
        QPushButton:disabled {
            color: %8;
        }
        TitleBar {
            background-color: %2;
            border-bottom: 1px solid %3;
        }
        #TitleBarLabel {
            color: %4;
            font-family: "Segoe UI";
            font-size: 14px;
            font-weight: 600;
            letter-spacing: 0.2px;
        }
        #ContentLabel {
            color: %5;
            font-size: 18px;
        }
        #TitleBarMinimizeButton,
        #TitleBarMaximizeButton,
        #TitleBarCloseButton {
            border: none;
            border-radius: 5px;
            background: transparent;
            color: %4;
            font-family: "Segoe MDL2 Assets", "Segoe UI";
            font-size: 10px;
            padding: 0px;
        }
        #TitleBarMinimizeButton[btnState="hover"],
        #TitleBarMaximizeButton[btnState="hover"] {
            background: %6;
        }
        #TitleBarMinimizeButton[btnState="pressed"],
        #TitleBarMaximizeButton[btnState="pressed"] {
            background: %7;
        }
        #TitleBarMinimizeButton[btnState="disabled"],
        #TitleBarMaximizeButton[btnState="disabled"],
        #TitleBarCloseButton[btnState="disabled"] {
            color: %8;
        }
        #TitleBarCloseButton[btnState="hover"] {
            background: %9;
            color: white;
        }
        #TitleBarCloseButton[btnState="pressed"] {
            background: %10;
            color: white;
        }
    )")
           .arg(windowBackgroundRule,
               colorToCss(titleBg),
               colorToCss(titleBorder),
               colorToCss(textColor),
               colorToCss(contentColor),
               colorToCss(buttonHover),
               colorToCss(buttonPressed),
               colorToCss(disabledColor),
               colorToCss(closeHover),
               colorToCss(closeHover.darker(115)),
               colorToCss(buttonBg));
}
