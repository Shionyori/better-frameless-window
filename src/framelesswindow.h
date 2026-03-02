#pragma once

#include "thememanager.h"
#include "windoweffectwin.h"

#include <QWidget>

class QVBoxLayout;
class TitleBar;
class QColor;
class QString;

class FramelessWindow : public QWidget
{
    Q_OBJECT
public:
    explicit FramelessWindow(QWidget *parent = nullptr);
    ~FramelessWindow();

    void setShadowEnabled(bool enabled);
    void setBackdropEnabled(bool enabled);
    void setBackdropPreference(WindowEffectWin::BackdropPreference preference);
    void setRoundedCornersEnabled(bool enabled);
    void setImmersiveDarkModeEnabled(bool enabled);
    void setAeroBlurEnabled(bool enabled);
    void setThemeMode(ThemeManager::ThemeMode mode);
    void setAccentColor(const QColor &accentColor);
    void setBackgroundMode(ThemeManager::BackgroundMode mode);
    void setBackgroundImagePath(const QString &imagePath);

    void setCentralWidget(QWidget *widget);
    QWidget *centralWidget() const;
    QWidget *takeCentralWidget();

    void setContentWidget(QWidget *widget);
    QWidget *contentWidget() const;
    QWidget *takeContentWidget();
    void addTitleBarWidget(QWidget *widget);
    void clearTitleBarWidgets();
    void setDiagnosticsEnabled(bool enabled);
    bool isShadowEnabled() const;
    bool isBackdropEnabled() const;
    WindowEffectWin::BackdropPreference backdropPreference() const;
    bool isRoundedCornersEnabled() const;
    bool isImmersiveDarkModeEnabled() const;
    bool isAeroBlurEnabled() const;
    bool isDiagnosticsEnabled() const;
    ThemeManager::ThemeMode themeMode() const;
    QColor accentColor() const;
    ThemeManager::BackgroundMode backgroundMode() const;
    QString backgroundImagePath() const;

protected:
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void paintEvent(QPaintEvent *e) override;
    void changeEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

    void initWindow();
    void initLayout();
    void initMouseTracking();
    void applyTheme();
    int hitTest(const QPoint &globalPos) const;
    int resizeBorderThickness() const;
    Qt::Edges edgesForLocalPos(const QPoint &localPos) const;
    void toggleMaximizeRestore();
    void startSystemMove();
    void showSystemMenu(const QPoint &globalPos);
    void updateSystemMenuState(void *menuHandle) const;
    void updateMaximizeButtonState();
    void updateCursorForPosition(const QPoint &localPos);
    bool tryStartSystemResizeAtLocalPos(const QPoint &localPos);
    bool tryStartSystemResizeAtGlobalPos(const QPoint &globalPos);
    void ensureNativeResizeStyle();
    void syncNativeWindowFrame();
    void applyVisualEffects();
    void scheduleStateVisualRefresh();
    bool shouldUseTranslucentBackground() const;
    bool shouldUseDarkMode() const;
    QColor preferredBorderColor() const;
    Qt::CursorShape cursorForEdges(Qt::Edges edges) const;

private:
    void attachContentEventFilters(QWidget *widget);
    void detachContentEventFilters(QWidget *widget);

#ifdef Q_OS_WIN
    bool handleNativeWindowsMessage(void *message, qintptr *result);
    bool handleNcHitTestMessage(qintptr lParam, qintptr *result);
    bool handleNcButtonMessage(quint32 messageId, quintptr wParam, qintptr *result);
    bool handleGetMinMaxInfoMessage(void *lParam, qintptr *result);
    bool handleNcRightButtonUpMessage(quintptr wParam, qintptr lParam, qintptr *result);
    void clearMaximizeButtonNativeHover();
#endif

    TitleBar *m_titleBar;
    QWidget *m_contentPanel;
    QWidget *m_userContentWidget;
    QVBoxLayout *m_layout;
    bool m_shadowEnabled;
    bool m_backdropEnabled;
    WindowEffectWin::BackdropPreference m_backdropPreference;
    bool m_roundedCornersEnabled;
    bool m_immersiveDarkModeEnabled;
    bool m_aeroBlurEnabled;
    bool m_applyingTheme;
    QString m_lastAppliedStyleSheet;
    bool m_lastTranslucentBackground;
    bool m_visualEffectsStateCached;
    bool m_lastVisualShadowEnabled;
    bool m_lastVisualBackdropEnabled;
    WindowEffectWin::BackdropPreference m_lastVisualBackdropPreference;
    bool m_lastVisualRoundedCornersEnabled;
    bool m_lastVisualImmersiveDarkModeEnabled;
    bool m_lastVisualAeroBlurEnabled;
    bool m_lastVisualUseDarkMode;
    bool m_lastVisualMaximized;
    bool m_lastVisualMinimized;
    QColor m_lastVisualBorderColor;
    bool m_loggedNullWindowHandle;
    bool m_pendingStateVisualRefresh;
    ThemeManager m_themeManager;
    WindowEffectWin m_windowEffect;
};
