#pragma once

#include "thememanager.h"
#include "core/visualrefreshcoordinator.h"
#include "core/windowvisualstate.h"
#include "win32/windoweffect.h"

#include <QColor>
#include <QPixmap>
#include <QSize>
#include <QWidget>

class QMoveEvent;
class QPropertyAnimation;
class QResizeEvent;
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

    void setWindowSizeLimits(const QSize &minimumSize, const QSize &maximumSize = QSize());
    QSize minimumWindowSize() const;
    QSize maximumWindowSize() const;

    void setShadowEnabled(bool enabled);
    void setRoundedCornersEnabled(bool enabled);
    // BackdropMode::None is the "off" switch; there is no separate enabled flag.
    void setBackdropMode(WindowEffect::BackdropMode mode);
    // True while a material (Mica/Acrylic) owns the surface. The application
    // background (colour/image/opacity) is only painted when this is false.
    bool isBackdropActive() const;
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

    // Background colour / image / opacity describe the application-painted
    // surface, i.e. BackdropMode::None. A material (Mica/Acrylic) owns its own
    // background and these have no effect while one is active.
    void setBackgroundColor(const QColor &color);
    QColor backgroundColor() const;
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
    void setMinimizeButtonVisible(bool visible);
    bool isMinimizeButtonVisible() const;
    void setMaximizeButtonVisible(bool visible);
    bool isMaximizeButtonVisible() const;
    void setCloseButtonVisible(bool visible);
    bool isCloseButtonVisible() const;

    void saveWindowGeometry();
    void restoreWindowGeometry();

    void setDiagnosticsEnabled(bool enabled);
    bool isShadowEnabled() const;
    WindowEffect::BackdropMode backdropMode() const;
    bool isRoundedCornersEnabled() const;
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
    void moveEvent(QMoveEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void leaveEvent(QEvent *event) override;

    // Called by NativeMessageRouter on WM_ENTERSIZEMOVE / WM_EXITSIZEMOVE.
    void setResizing(bool resizing);
    void initWindow();
    void initLayout();
    void initMouseTracking();
    void applyTheme();
    int hitTest(const QPoint &globalPos) const;
    int resizeBorderThickness() const;
    Qt::Edges edgesForLocalPos(const QPoint &localPos) const;
    void toggleMaximizeRestore();
    // Animated, idempotent versions used by the native SC_MAXIMIZE/SC_RESTORE
    // path (Aero snap to the top, Win+Up / Win+Down, system menu).
    // Return false when the requested state does not apply (already maximized,
    // not maximized, no usable target) so the caller can fall back to the
    // native command.
    bool maximizeWithTransition();
    bool restoreWithTransition();
    // Aero snap to the top (and any external maximize) resizes the window to the
    // work area before the state changes, which leaves nothing to animate. The
    // rectangle is in native pixels.
    bool interceptExternalMaximize(const QRect &requestedNativeRect);
    // Windows computes the maximized rectangle by subtracting the sizing frame,
    // and WM_NCCALCSIZE keeps the client equal to the window, so a maximize can
    // land inside the work area. Only correct that case: an equal or larger
    // rectangle is left alone, otherwise the correction and the window manager
    // fight over a pixel.
    void settleMaximizedGeometry();
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
    WindowVisualState::SurfacePolicy surfacePolicy() const;
    bool usesTranslucentSurface() const;
    bool shouldUseDarkMode() const;
    QColor preferredBorderColor() const;
    Qt::CursorShape cursorForEdges(Qt::Edges edges) const;

private:
    bool shouldStartRestoreTransitionFromSizeState(bool isMaximizedState, bool isRestoredState);
    void beginBackdropTransitionGuard();
    void performVisualRefreshPass();
    void requestVisualRefresh();
    void forceBackdropRebind();
    void rememberRestoreGeometry();
    void startWindowTransition(const QRect &target, bool maximizeAtEnd);
    void finishWindowTransition();
    void completeWindowTransition();
    void attachContentEventFilters(QWidget *widget);
    void detachContentEventFilters(QWidget *widget);
    friend class NativeMessageRouter;
    friend class TestFramelessWindow;

    TitleBar *m_titleBar;
    QWidget *m_contentPanel;
    QWidget *m_userContentWidget;
    QVBoxLayout *m_layout;
    bool m_shadowEnabled;
    WindowEffect::BackdropMode m_backdropMode;
    bool m_roundedCornersEnabled;
    bool m_snapLayoutEnabled;
    QColor m_backgroundColor;
    QPixmap m_backgroundImage;
    BackgroundImageMode m_backgroundImageMode = BackgroundImageMode::Cover;
    qreal m_backgroundOpacity = 1.0;
    bool m_backdropTransitionGuardActive;
    quint64 m_backdropTransitionEpoch;
    bool m_lastNativeSizeMaximized;
    bool m_applyingTheme;
    QString m_lastAppliedStyleSheet;
    QColor m_borderColor;
    bool m_loggedNullWindowHandle;
    bool m_loggedNullWinId;
    bool m_loggedTranslucentShadow;
    bool m_resizing = false;
    WindowVisualState::SurfaceKind m_lastAppliedSurfaceKind = WindowVisualState::SurfaceKind::Opaque;
    QPropertyAnimation *m_windowTransition = nullptr;
    bool m_windowTransitionMaximizes = false;
    bool m_animatingWindowTransition = false;
    // Geometry the window returns to on restore. Tracked while normal so a
    // maximize that did not come from this class (Win+Up, snap) still animates.
    QRect m_restoreGeometry;
    VisualRefreshCoordinator m_visualRefreshCoordinator;
    ThemeManager m_themeManager;
    bool m_followSystemTheme;
    WindowEffect m_windowEffect;
};
