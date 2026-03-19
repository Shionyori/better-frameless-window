#pragma once

#include "thememanager.h"
#include "core/visualrefreshcoordinator.h"
#include "windoweffectwin.h"

#include <QWidget>

class QVBoxLayout;
class TitleBar;
class QColor;
class QString;
class NativeWindowsMessageRouter;

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
    void setThemeMode(ThemeManager::ThemeMode mode);
    void setAccentColor(const QColor &accentColor);
    void setBackgroundMode(ThemeManager::BackgroundMode mode);

    void setCentralWidget(QWidget *widget);
    QWidget *centralWidget() const;
    QWidget *takeCentralWidget();
    void addTitleBarWidget(QWidget *widget);
    void clearTitleBarWidgets();
    void setDiagnosticsEnabled(bool enabled);
    bool isShadowEnabled() const;
    bool isBackdropEnabled() const;
    WindowEffectWin::BackdropPreference backdropPreference() const;
    bool isRoundedCornersEnabled() const;
    bool isImmersiveDarkModeEnabled() const;
    bool isDiagnosticsEnabled() const;
    ThemeManager::ThemeMode themeMode() const;
    QColor accentColor() const;
    ThemeManager::BackgroundMode backgroundMode() const;

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
    void updateMaximizeButtonState();
    void updateCursorForPosition(const QPoint &localPos);
    bool tryStartSystemResizeAtLocalPos(const QPoint &localPos);
    bool tryStartSystemResizeAtGlobalPos(const QPoint &globalPos);
    void ensureNativeResizeStyle();
    void syncNativeWindowFrame();
    void forceNativeDwmRefresh();
    void applyVisualEffects();
    void scheduleStateVisualRefresh();
    quint64 currentVisualStateToken() const;
    bool shouldUseTranslucentBackground() const;
    bool shouldUseDarkMode() const;
    QColor preferredBorderColor() const;
    Qt::CursorShape cursorForEdges(Qt::Edges edges) const;

private:
    bool shouldStartRestoreTransitionFromSizeState(bool isMaximizedState, bool isRestoredState);
    WindowEffectWin::BackdropPreference effectiveBackdropPreference() const;
    void beginBackdropTransitionGuard();
    void performVisualRefreshPass();
    void requestVisualRefresh();
    void forceBackdropRebind();
    void attachContentEventFilters(QWidget *widget);
    void detachContentEventFilters(QWidget *widget);
    friend class NativeWindowsMessageRouter;

    TitleBar *m_titleBar;
    QWidget *m_contentPanel;
    QWidget *m_userContentWidget;
    QVBoxLayout *m_layout;
    bool m_shadowEnabled;
    bool m_backdropEnabled;
    WindowEffectWin::BackdropPreference m_backdropPreference;
    bool m_roundedCornersEnabled;
    bool m_immersiveDarkModeEnabled;
    bool m_backdropTransitionGuardActive;
    quint64 m_backdropTransitionEpoch;
    bool m_lastNativeSizeMaximized;
    bool m_applyingTheme;
    QString m_lastAppliedStyleSheet;
    bool m_loggedNullWindowHandle;
    VisualRefreshCoordinator m_visualRefreshCoordinator;
    ThemeManager m_themeManager;
    WindowEffectWin m_windowEffect;
};
