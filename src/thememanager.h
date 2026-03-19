#pragma once

#include <QColor>
#include <QString>

class ThemeManager
{
public:
    enum class ThemeMode {
        Light,
        Dark
    };

    enum class BackgroundMode {
        Solid
    };

    ThemeManager() = default;

    void setThemeMode(ThemeMode mode);
    ThemeMode themeMode() const;

    void setAccentColor(const QColor &accentColor);
    QColor accentColor() const;

    void setBackgroundMode(BackgroundMode mode);
    BackgroundMode backgroundMode() const;

    bool isDarkMode() const;
    QString buildStyleSheet(bool transparentWindowBackground = false) const;

private:
    ThemeMode m_themeMode = ThemeMode::Light;
    QColor m_accentColor = QColor(0, 120, 215);
    BackgroundMode m_backgroundMode = BackgroundMode::Solid;
};
