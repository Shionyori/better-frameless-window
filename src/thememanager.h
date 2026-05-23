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
        Solid,
        Transparent
    };

    ThemeManager() = default;

    void setThemeMode(ThemeMode mode);
    ThemeMode themeMode() const;

    void setAccentColor(const QColor &accentColor);
    QColor accentColor() const;

    bool isDarkMode() const;

    void startTransition(ThemeMode targetMode);
    void setTransitionProgress(qreal progress);
    qreal transitionProgress() const;
    bool isTransitioning() const;
    ThemeMode previousMode() const;

    QString buildStyleSheet(bool transparentWindowBackground = false) const;

private:
    QColor lerpColor(const QColor &a, const QColor &b, qreal t) const;
    QColor themeColor(const QColor &lightValue, const QColor &darkValue) const;

    ThemeMode m_themeMode = ThemeMode::Light;
    ThemeMode m_previousMode = ThemeMode::Light;
    qreal m_transitionProgress = 1.0;
    QColor m_accentColor = QColor(0, 120, 215);
};
