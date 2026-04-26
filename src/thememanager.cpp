#include "thememanager.h"

namespace {
QString colorToCss(const QColor &color)
{
    return color.name(QColor::HexRgb);
}

QString buildWindowBackgroundRule(const QColor &windowBg,
                                  bool transparentWindowBackground)
{
    // Keep a stable translucent layer so Mica remains visible without introducing heavy blending artifacts.
    const QString translucentBase = QStringLiteral("rgba(255, 255, 255, 0.30)");

    if (transparentWindowBackground) {
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

    const QColor windowBg = dark ? QColor(32, 32, 32) : QColor(243, 244, 246);
    const QColor windowBorder = dark ? QColor(77, 77, 77) : QColor(185, 192, 202);
    const QColor titleBg = dark ? QColor(43, 43, 43) : QColor(255, 255, 255);
    const QColor titleBorder = dark ? QColor(66, 66, 66) : QColor(199, 206, 216);
    const QColor textColor = dark ? QColor(241, 241, 241) : QColor(34, 34, 34);
    const QColor contentColor = dark ? QColor(210, 210, 210) : QColor(68, 68, 68);
    const QColor buttonHover = dark ? QColor(68, 68, 68) : QColor(232, 232, 232);
    const QColor buttonPressed = dark ? QColor(84, 84, 84) : QColor(216, 216, 216);
    const QColor disabledColor = textColor.lighter(160);
    const QColor closeHover = QColor(232, 17, 35);

    const QString windowBackgroundRule = buildWindowBackgroundRule(windowBg,
                                                                   transparentWindowBackground);

    return QStringLiteral(R"(
        #FramelessWindow {
            %1
            border: none;
        }
        #FramelessContentPanel {
            background: transparent;
            border: none;
        }
        TitleBar {
            background-color: %2;
            border-bottom: 1px solid %3;
        }
        #TitleBarLabel {
            color: %4;
            font-family: "Segoe UI";
            font-size: 13px;
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
               colorToCss(closeHover.darker(115)));
}
