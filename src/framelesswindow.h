#pragma once

#include "thememanager.h"
#include "core/visualrefreshcoordinator.h"
#include "win32/windoweffect.h"

#include <QColor>
#include <QPixmap>
#include <QSize>
#include <QWidget>

class QVBoxLayout;
class TitleBar;
class QString;
class NativeMessageRouter;

class FramelessWindow : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal themeTransitionProgress READ themeTransitionProgress WRITE setThemeTransitionProgress)
public:
    explicit FramelessWindow(QWidget *parent = nullptr);
    ~FramelessWindow();

    void setWindowOpacity(qreal opacity);
    qreal windowOpacity() const;

    void setWindowSizeLimits(const QSize &minimumSize, const QSize &maximumSize = QSize());
    QSize minimumWindowSize() const;
    QSize maximumWindowSize() const;

    void setSystemShadowEnabled(bool enabled);
    void setSystemBackdropEnabled(bool enabled);
    void setSystemBackdropPreference(WindowEffect::SystemBackdropPreference preference);
    void setRoundedCornersEnabled(bool enabled);
    void setSystemDarkModeEnabled(bool enabled);
    void setSnapLayoutEnabled(bool enabled);
    void setThemeMode(ThemeManager::ThemeMode mode, bool persist = true, bool animated = true);
    ThemeManager::ThemeMode themeMode() const;
    qreal themeTransitionProgress() const;
    void setThemeTransitionProgress(qreal progress);
    void setFollowSystemTheme(bool enabled);
    bool followsSystemTheme() const;
    void syncThemeWithSystemIfFollowing();

    void setAccentColor(const QColor &accentColor);
    QColor accentColor() const;

    void setBorderColor(const QColor &color);
    QColor borderColor() const;

    void setCentralWidget(QWidget *widget);
    QWidget *centralWidget() const;
    QWidget *takeCentralWidget();

    void addTitleBarWidget(QWidget *widget);
    void clearTitleBarWidgets();

    enum class BackgroundImageMode {
        Cover,
        Stretch,
        Fit,
        Tile,
        Center
    };

    void setBackgroundImage(const QPixmap &image, BackgroundImageMode mode = BackgroundImageMode::Cover);
    void clearBackgroundImage();
    QPixmap backgroundImage() const;
    BackgroundImageMode backgroundImageMode() const;
    void setBackgroundOpacity(qreal opacity);
    qreal backgroundOpacity() const;

    void setTitleBarVisible(bool visible);
    bool isTitleBarVisible() const;
    void setTitleBarHeight(int height);
    int titleBarHeight() const;

    void saveWindowGeometry();
    void restoreWindowGeometry();

    void setDiagnosticsEnabled(bool enabled);
    void setResizing(bool resizing);
    bool isShadowEnabled() const;
    bool isSystemBackdropEnabled() const;
    WindowEffect::SystemBackdropPreference systemBackdropPreference() const;
    bool isRoundedCornersEnabled() const;
    bool isSystemDarkModeEnabled() const;
    bool isSnapLayoutEnabled() const;
    bool isDiagnosticsEnabled() const;

protected:
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void paintEvent(QPaintEvent *e) override;
    void changeEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
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
    WindowEffect::SystemBackdropPreference effectiveSystemBackdropPreference() const;
    void beginSystemBackdropTransitionGuard();
    void performVisualRefreshPass();
    void requestVisualRefresh();
    void forceSystemBackdropRebind();
    void attachContentEventFilters(QWidget *widget);
    void detachContentEventFilters(QWidget *widget);
    friend class NativeMessageRouter;

    TitleBar *m_titleBar;
    QWidget *m_contentPanel;
    QWidget *m_userContentWidget;
    QVBoxLayout *m_layout;
    bool m_shadowEnabled;
    bool m_systemBackdropEnabled;
    WindowEffect::SystemBackdropPreference m_systemBackdropPreference;
    bool m_roundedCornersEnabled;
    bool m_systemDarkModeEnabled;
    bool m_snapLayoutEnabled;
    QPixmap m_backgroundImage;
    BackgroundImageMode m_backgroundImageMode = BackgroundImageMode::Cover;
    qreal m_backgroundOpacity = 1.0;
    bool m_systemBackdropTransitionGuardActive;
    quint64 m_systemBackdropTransitionEpoch;
    bool m_lastNativeSizeMaximized;
    bool m_applyingTheme;
    QString m_lastAppliedStyleSheet;
    QColor m_borderColor;
    bool m_loggedNullWindowHandle;
    bool m_loggedNullWinId;
    bool m_resizing = false;
    VisualRefreshCoordinator m_visualRefreshCoordinator;
    ThemeManager m_themeManager;
    bool m_followSystemTheme;
    WindowEffect m_windowEffect;
};
