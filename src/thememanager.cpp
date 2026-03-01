#include "thememanager.h"

#include <QUrl>

namespace {
QString colorToCss(const QColor &color)
{
    return color.name(QColor::HexRgb);
}

QString cssUrlFromLocalPath(const QString &path)
{
    return QUrl::fromLocalFile(path).toString(QUrl::FullyEncoded);
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

void ThemeManager::setBackgroundMode(BackgroundMode mode)
{
    m_backgroundMode = mode;
}

ThemeManager::BackgroundMode ThemeManager::backgroundMode() const
{
    return m_backgroundMode;
}

void ThemeManager::setBackgroundImagePath(const QString &imagePath)
{
    m_backgroundImagePath = imagePath;
}

QString ThemeManager::backgroundImagePath() const
{
    return m_backgroundImagePath;
}

bool ThemeManager::isDarkMode() const
{
    return m_themeMode == ThemeMode::Dark;
}

QString ThemeManager::buildStyleSheet() const
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
    const QColor accentHover = m_accentColor;
    const QColor closeHover = QColor(232, 17, 35);

    QString windowBackgroundRule = QStringLiteral("background-color: %1;").arg(colorToCss(windowBg));
    if (m_backgroundMode == BackgroundMode::Gradient) {
        const QColor gradStart = dark ? windowBg.lighter(120) : windowBg.lighter(106);
        const QColor gradEnd = dark ? windowBg.darker(125) : windowBg.darker(108);
        windowBackgroundRule = QStringLiteral(
            "background-color: %1;"
            "background-image: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %2, stop:1 %3);")
                                   .arg(colorToCss(windowBg),
                                        colorToCss(gradStart),
                                        colorToCss(gradEnd));
    } else if (m_backgroundMode == BackgroundMode::Image && !m_backgroundImagePath.trimmed().isEmpty()) {
        windowBackgroundRule = QStringLiteral(
            "background-color: %1;"
            "background-image: url(\"%2\");"
            "background-repeat: no-repeat;"
            "background-position: center;")
                                   .arg(colorToCss(windowBg), cssUrlFromLocalPath(m_backgroundImagePath));
    }

    return QStringLiteral(R"(
        #FramelessWindow {
            %1
            border: 1px solid %2;
        }
        TitleBar {
            background-color: %3;
            border-bottom: 1px solid %4;
        }
        #TitleBarLabel {
            color: %5;
            font-weight: 600;
        }
        #ContentLabel {
            color: %6;
            font-size: 18px;
        }
        #TitleBarMinimizeButton,
        #TitleBarMaximizeButton,
        #TitleBarCloseButton {
            border: none;
            background: transparent;
            color: %5;
            font-size: 14px;
        }
        #TitleBarMinimizeButton:hover,
        #TitleBarMaximizeButton:hover,
        #TitleBarMaximizeButton[nativeHover="true"] {
            background: %7;
        }
        #TitleBarMinimizeButton:pressed,
        #TitleBarMaximizeButton:pressed {
            background: %8;
        }
        #TitleBarMinimizeButton:disabled,
        #TitleBarMaximizeButton:disabled,
        #TitleBarCloseButton:disabled {
            color: %9;
        }
        #TitleBarCloseButton:hover {
            background: %10;
            color: white;
        }
        #TitleBarCloseButton:pressed {
            background: %11;
            color: white;
        }
    )")
        .arg(windowBackgroundRule,
             colorToCss(windowBorder),
             colorToCss(titleBg),
             colorToCss(titleBorder),
             colorToCss(textColor),
             colorToCss(contentColor),
             colorToCss(buttonHover),
             colorToCss(buttonPressed),
             colorToCss(accentHover),
             colorToCss(closeHover),
             colorToCss(closeHover.darker(115)));
}
