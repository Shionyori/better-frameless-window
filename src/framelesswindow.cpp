#include "framelesswindow.h"

#include "diagnostics.h"
#include "core/windowgeometry.h"
#include "core/windowvisualstate.h"
// Platform-neutral: reports failure instead of doing anything off Windows.
#include "win32/windowcommand.h"
#ifdef Q_OS_WIN
#include <qt_windows.h>
#include "win32/systemmenu.h"
#include "win32/windowhittest.h"
#include "win32/windowframe.h"
#include "win32/nativemessagerouter.h"
#include "win32/utils.h"
#endif
#include "titlebar.h"

#include <QEvent>
#include <QLayout>
#include <QMoveEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QShortcut>
#include <QPropertyAnimation>
#include <QObject>
#include <QPainter>
#include <QCloseEvent>
#include <QColor>
#include <QGuiApplication>
#include <QPushButton>
#include <QScreen>
#include <QScopedValueRollback>
#include <QSettings>
#include <QShowEvent>
#include <QStyle>
#include <QStyleOption>
#include <QTimer>
#include <QVBoxLayout>
#include <QWindow>
#include <QtGlobal>

namespace {

// Windows' own maximize/restore transition is roughly 200ms; 170ms reads as the
// same gesture without the window lingering in an in-between state.
constexpr int kWindowTransitionDurationMs = 170;

} // namespace

FramelessWindow::FramelessWindow(QWidget *parent)
    : QWidget(parent)
    , m_titleBar(nullptr)
    , m_contentPanel(nullptr)
    , m_userContentWidget(nullptr)
    , m_layout(nullptr)
    , m_shadowEnabled(true)
    , m_backdropMode(WindowEffect::BackdropMode::Auto)
    , m_roundedCornersEnabled(true)
    , m_snapLayoutEnabled(true)
    , m_backdropTransitionGuardActive(false)
    , m_backdropTransitionEpoch(0)
    , m_lastNativeSizeMaximized(false)
    , m_applyingTheme(false)
    , m_lastAppliedStyleSheet()
    , m_loggedNullWindowHandle(false)
    , m_borderColor()
    , m_loggedNullWinId(false)
    , m_loggedTranslucentShadow(false)
    , m_visualRefreshCoordinator(this)
    , m_followSystemTheme(false)
{
    m_visualRefreshCoordinator.configure(
        [this]() {
            return currentVisualStateToken();
        },
        [this]() {
            return isVisible();
        },
        [this]() {
            performVisualRefreshPass();
        },
        [this](quint64 previousToken, quint64 tokenBefore, quint64 tokenAfter) {
            if (tokenAfter != previousToken || tokenAfter != tokenBefore) {
                forceNativeDwmRefresh();
            }
        },
        [this]() {
            update();
        });

    initWindow();
    initLayout();
}

FramelessWindow::~FramelessWindow() = default;

void FramelessWindow::setWindowSizeLimits(const QSize &minimumSize, const QSize &maximumSize)
{
    if (minimumSize.isValid() && minimumSize != QWidget::minimumSize()) {
        QWidget::setMinimumSize(minimumSize);
    }

    if (maximumSize.isValid() && maximumSize != QWidget::maximumSize()) {
        QWidget::setMaximumSize(maximumSize);
    }
}

QSize FramelessWindow::minimumWindowSize() const
{
    return QWidget::minimumSize();
}

QSize FramelessWindow::maximumWindowSize() const
{
    return QWidget::maximumSize();
}

void FramelessWindow::setShadowEnabled(bool enabled)
{
    if (m_shadowEnabled == enabled) {
        return;
    }

    m_shadowEnabled = enabled;
    requestVisualRefresh();
}

void FramelessWindow::setBackdropMode(WindowEffect::BackdropMode mode)
{
    if (m_backdropMode == mode) {
        return;
    }

    m_backdropMode = mode;
    requestVisualRefresh();
}

void FramelessWindow::setRoundedCornersEnabled(bool enabled)
{
    if (m_roundedCornersEnabled == enabled) {
        return;
    }

    m_roundedCornersEnabled = enabled;
    requestVisualRefresh();
}

void FramelessWindow::setSnapLayoutEnabled(bool enabled)
{
    m_snapLayoutEnabled = enabled;
}

void FramelessWindow::setBackgroundImage(const QPixmap &image, BackgroundImageMode mode)
{
    m_backgroundImage = image;
    m_backgroundImageMode = mode;

    // The presence of an image changes the resolved surface: an image needs an
    // opaque window, so a live backdrop has to be dropped. The refresh below
    // recomputes the theme style sheet and the DWM effects from that policy.
    requestVisualRefresh();
}

void FramelessWindow::clearBackgroundImage()
{
    m_backgroundImage = QPixmap();
    // Dropping the image lets the requested backdrop take the surface again.
    requestVisualRefresh();
}

QPixmap FramelessWindow::backgroundImage() const
{
    return m_backgroundImage;
}

FramelessWindow::BackgroundImageMode FramelessWindow::backgroundImageMode() const
{
    return m_backgroundImageMode;
}

void FramelessWindow::setBackgroundOpacity(qreal opacity)
{
    m_backgroundOpacity = qBound(0.0, opacity, 1.0);
    update();
}

qreal FramelessWindow::backgroundOpacity() const
{
    return m_backgroundOpacity;
}

void FramelessWindow::setTitleBarVisible(bool visible)
{
    if (m_titleBar != nullptr) {
        m_titleBar->setVisible(visible);
    }
}

bool FramelessWindow::isTitleBarVisible() const
{
    return m_titleBar != nullptr && m_titleBar->isVisible();
}

void FramelessWindow::setTitleBarHeight(int height)
{
    if (m_titleBar != nullptr) {
        m_titleBar->setHeight(height);
    }
}

int FramelessWindow::titleBarHeight() const
{
    return m_titleBar != nullptr ? m_titleBar->height() : 0;
}

void FramelessWindow::setMinimizeButtonVisible(bool visible)
{
    if (m_titleBar != nullptr) {
        m_titleBar->setMinimizeButtonVisible(visible);
    }
}

bool FramelessWindow::isMinimizeButtonVisible() const
{
    return m_titleBar != nullptr && m_titleBar->isMinimizeButtonVisible();
}

void FramelessWindow::setMaximizeButtonVisible(bool visible)
{
    if (m_titleBar != nullptr) {
        m_titleBar->setMaximizeButtonVisible(visible);
    }
}

bool FramelessWindow::isMaximizeButtonVisible() const
{
    return m_titleBar != nullptr && m_titleBar->isMaximizeButtonVisible();
}

void FramelessWindow::setCloseButtonVisible(bool visible)
{
    if (m_titleBar != nullptr) {
        m_titleBar->setCloseButtonVisible(visible);
    }
}

bool FramelessWindow::isCloseButtonVisible() const
{
    return m_titleBar != nullptr && m_titleBar->isCloseButtonVisible();
}

void FramelessWindow::setThemeMode(ThemeManager::ThemeMode mode, bool persist, bool animated)
{
    if (m_themeManager.themeMode() == mode) {
        return;
    }

    if (persist) {
        QSettings settings(QStringLiteral("better-frameless-window"), QStringLiteral("settings"));
        settings.setValue(QStringLiteral("theme/mode"),
                          mode == ThemeManager::ThemeMode::Dark
                              ? QStringLiteral("dark")
                              : QStringLiteral("light"));
    }

    if (animated && isVisible()) {
        m_themeManager.startTransition(mode);
        auto *anim = new QPropertyAnimation(this, "themeTransitionProgress", this);
        anim->setDuration(300);
        anim->setEasingCurve(QEasingCurve::InOutCubic);
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->start(QAbstractAnimation::DeleteWhenStopped);

        QTimer::singleShot(350, this, [this]() {
            forceBackdropRebind();
        });
    } else {
        m_themeManager.setThemeMode(mode);
        m_themeManager.setTransitionProgress(1.0);
        requestVisualRefresh();

        QTimer::singleShot(50, this, [this]() {
            forceBackdropRebind();
        });
    }
}

qreal FramelessWindow::themeTransitionProgress() const
{
    return m_themeManager.transitionProgress();
}

void FramelessWindow::setThemeTransitionProgress(qreal progress)
{
    m_themeManager.setTransitionProgress(progress);
    requestVisualRefresh();
}

void FramelessWindow::setAccentColor(const QColor &accentColor)
{
    if (!accentColor.isValid() || m_themeManager.accentColor() == accentColor) {
        return;
    }

    m_themeManager.setAccentColor(accentColor);
    requestVisualRefresh();
}

void FramelessWindow::requestVisualRefresh()
{
    if (!isVisible()) {
        performVisualRefreshPass();
        update();
        return;
    }

    scheduleStateVisualRefresh();
}

void FramelessWindow::performVisualRefreshPass()
{
    applyTheme();

    // Suppress native frame sync and DWM visual effects during an active
    // system resize (WM_ENTERSIZEMOVE → WM_EXITSIZEMOVE) and while the
    // maximize/restore transition animates the geometry. Calling
    // SetWindowPos(SWP_FRAMECHANGED) or DwmSetWindowAttribute mid-transition
    // fights the moving frame and causes visible jitter.
    if (!m_resizing && !m_animatingWindowTransition) {
        syncNativeWindowFrame();
        applyVisualEffects();
    }

    updateMaximizeButtonState();
}

void FramelessWindow::setCentralWidget(QWidget *widget)
{
    if (m_contentPanel == nullptr) {
        return;
    }

    QLayout *contentLayout = m_contentPanel->layout();
    if (contentLayout == nullptr) {
        return;
    }

    if (m_userContentWidget == widget) {
        return;
    }

    if (m_userContentWidget != nullptr) {
        detachContentEventFilters(m_userContentWidget);
        contentLayout->removeWidget(m_userContentWidget);
        m_userContentWidget->setParent(nullptr);
    }

    m_userContentWidget = widget;
    if (m_userContentWidget == nullptr) {
        return;
    }

    m_userContentWidget->setParent(m_contentPanel);
    attachContentEventFilters(m_userContentWidget);
    contentLayout->addWidget(m_userContentWidget);
}

QWidget *FramelessWindow::centralWidget() const
{
    return m_userContentWidget;
}

QWidget *FramelessWindow::takeCentralWidget()
{
    if (m_contentPanel == nullptr || m_userContentWidget == nullptr) {
        return nullptr;
    }

    QLayout *contentLayout = m_contentPanel->layout();
    if (contentLayout != nullptr) {
        contentLayout->removeWidget(m_userContentWidget);
    }

    QWidget *currentContent = m_userContentWidget;
    detachContentEventFilters(currentContent);
    currentContent->setParent(nullptr);
    m_userContentWidget = nullptr;
    return currentContent;
}

void FramelessWindow::addTitleBarWidget(QWidget *widget)
{
    if (m_titleBar == nullptr || widget == nullptr) {
        return;
    }

    m_titleBar->addCenterWidget(widget);
    widget->installEventFilter(this);
}

void FramelessWindow::clearTitleBarWidgets()
{
    if (m_titleBar == nullptr) {
        return;
    }

    m_titleBar->clearCenterWidgets();
}

void FramelessWindow::setDiagnosticsEnabled(bool enabled)
{
    Diagnostics::setEnabled(enabled);
}

void FramelessWindow::setResizing(bool resizing)
{
    m_resizing = resizing;
}

bool FramelessWindow::isShadowEnabled() const
{
    return m_shadowEnabled;
}

WindowEffect::BackdropMode FramelessWindow::backdropMode() const
{
    return m_backdropMode;
}

bool FramelessWindow::isRoundedCornersEnabled() const
{
    return m_roundedCornersEnabled;
}

bool FramelessWindow::isSnapLayoutEnabled() const
{
    return m_snapLayoutEnabled;
}

bool FramelessWindow::isDiagnosticsEnabled() const
{
    return Diagnostics::isEnabled();
}

ThemeManager::ThemeMode FramelessWindow::themeMode() const
{
    return m_themeManager.themeMode();
}

void FramelessWindow::setFollowSystemTheme(bool enabled)
{
    if (m_followSystemTheme == enabled) {
        return;
    }

    m_followSystemTheme = enabled;

#ifdef Q_OS_WIN
    if (enabled) {
        const bool systemDark = Utils::systemPrefersDarkTheme();
        setThemeMode(systemDark ? ThemeManager::ThemeMode::Dark
                                : ThemeManager::ThemeMode::Light,
                     /*persist=*/false,
                     /*animated=*/false);
    }
#endif
}

bool FramelessWindow::followsSystemTheme() const
{
    return m_followSystemTheme;
}

void FramelessWindow::syncThemeWithSystemIfFollowing()
{
#ifdef Q_OS_WIN
    if (!m_followSystemTheme) {
        return;
    }

    const bool systemDark = Utils::systemPrefersDarkTheme();
    const ThemeManager::ThemeMode systemMode = systemDark
        ? ThemeManager::ThemeMode::Dark
        : ThemeManager::ThemeMode::Light;

    if (m_themeManager.themeMode() != systemMode) {
        setThemeMode(systemMode, /*persist=*/false, /*animated=*/false);
    }
#endif
}

QColor FramelessWindow::accentColor() const
{
    return m_themeManager.accentColor();
}

void FramelessWindow::setBorderColor(const QColor &color)
{
    if (m_borderColor == color) {
        return;
    }

    m_borderColor = color;
    requestVisualRefresh();
}

QColor FramelessWindow::borderColor() const
{
    return m_borderColor;
}

void FramelessWindow::initWindow()
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint);
    setMinimumSize(480, 320);
    resize(960, 640);
    setAttribute(Qt::WA_StyledBackground, true);

    setObjectName("FramelessWindow");

    // Restore persisted theme preference (only when not following system)
    if (!m_followSystemTheme) {
        QSettings settings(QStringLiteral("better-frameless-window"), QStringLiteral("settings"));
        const QString savedMode = settings.value(QStringLiteral("theme/mode")).toString();
        if (savedMode == QStringLiteral("dark")) {
            m_themeManager.setThemeMode(ThemeManager::ThemeMode::Dark);
        } else if (savedMode == QStringLiteral("light")) {
            m_themeManager.setThemeMode(ThemeManager::ThemeMode::Light);
        }
        // If no saved value, keep default (Light)
    }

    applyTheme();

    auto *altSpaceShortcut = new QShortcut(QKeySequence(Qt::ALT | Qt::Key_Space), this);
    altSpaceShortcut->setContext(Qt::WindowShortcut);
    connect(altSpaceShortcut, &QShortcut::activated, this, [this]() {
        showSystemMenu(mapToGlobal(QPoint(0, 0)));
    });
}

void FramelessWindow::initLayout()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setSpacing(0);
    m_layout->setContentsMargins(0, 0, 0, 0);

    m_titleBar = new TitleBar(this);
    m_contentPanel = new QWidget(this);
    m_contentPanel->setObjectName("FramelessContentPanel");
    // WA_TranslucentBackground on a child widget inside an already-layered
    // top-level window breaks nested transparency compositing on Windows,
    // causing child widgets (like user content labels) to render invisibly.
    // The stylesheet #FramelessContentPanel { background: transparent; }
    // already provides the desired visual transparency.
    m_contentPanel->setAttribute(Qt::WA_TranslucentBackground, false);

    auto *contentLayout = new QVBoxLayout(m_contentPanel);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    m_layout->addWidget(m_titleBar);
    m_layout->addWidget(m_contentPanel, 1);

    initMouseTracking();

    connect(m_titleBar, &TitleBar::minimizeRequested, this, &FramelessWindow::showMinimized);
    connect(m_titleBar, &TitleBar::maximizeRestoreRequested, this, &FramelessWindow::toggleMaximizeRestore);
    connect(m_titleBar, &TitleBar::closeRequested, this, &FramelessWindow::close);
    connect(m_titleBar, &TitleBar::systemMoveRequested, this, &FramelessWindow::startSystemMove);
    connect(m_titleBar, &TitleBar::systemMenuRequested, this, &FramelessWindow::showSystemMenu);

    updateMaximizeButtonState();
}

#ifdef Q_OS_WIN
bool FramelessWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
    if (eventType != "windows_generic_MSG" && eventType != "windows_dispatcher_MSG") {
        return QWidget::nativeEvent(eventType, message, result);
    }

    if (NativeMessageRouter::handle(*this, message, result)) {
        return true;
    }

    return QWidget::nativeEvent(eventType, message, result);
}
#else
bool FramelessWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
    Q_UNUSED(eventType)
    Q_UNUSED(message)
    Q_UNUSED(result)
    return false;
}
#endif

void FramelessWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    QStyleOption opt;
    opt.initFrom(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

    if (m_backgroundImage.isNull()) {
        return;
    }

    // Fill with opaque theme color so the image blends toward it as
    // opacity decreases, rather than revealing the desktop behind.
    painter.fillRect(rect(), m_themeManager.windowBackgroundColor());

    painter.setOpacity(m_backgroundOpacity);

    const QRect r = rect();
    const QSize imgSize = m_backgroundImage.size();

    switch (m_backgroundImageMode) {
    case BackgroundImageMode::Cover: {
        const qreal scaleX = static_cast<qreal>(r.width()) / imgSize.width();
        const qreal scaleY = static_cast<qreal>(r.height()) / imgSize.height();
        const qreal scale = qMax(scaleX, scaleY);
        const int sw = qRound(imgSize.width() * scale);
        const int sh = qRound(imgSize.height() * scale);
        const int x = (r.width() - sw) / 2;
        const int y = (r.height() - sh) / 2;
        painter.drawPixmap(QRect(x, y, sw, sh), m_backgroundImage);
        break;
    }
    case BackgroundImageMode::Stretch:
        painter.drawPixmap(r, m_backgroundImage);
        break;
    case BackgroundImageMode::Fit: {
        const QSize scaled = imgSize.scaled(r.size(), Qt::KeepAspectRatio);
        const int x = (r.width() - scaled.width()) / 2;
        const int y = (r.height() - scaled.height()) / 2;
        painter.drawPixmap(QRect(x, y, scaled.width(), scaled.height()), m_backgroundImage);
        break;
    }
    case BackgroundImageMode::Tile:
        painter.drawTiledPixmap(r, m_backgroundImage);
        break;
    case BackgroundImageMode::Center: {
        const int x = (r.width() - imgSize.width()) / 2;
        const int y = (r.height() - imgSize.height()) / 2;
        painter.drawPixmap(x, y, m_backgroundImage);
        break;
    }
    }
}

void FramelessWindow::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::WindowTitleChange) {
        if (m_titleBar != nullptr) {
            m_titleBar->setTitle(windowTitle());
        }
    } else if (event->type() == QEvent::WindowIconChange) {
        if (m_titleBar != nullptr) {
            m_titleBar->setIcon(windowIcon());
        }
    } else if (event->type() == QEvent::WindowStateChange) {
        // Keep maximize/restore edge tracking owned by WM_SIZE handling.
        // Mixing Qt-state writes here can clear the native transition flag
        // before SIZE_RESTORED arrives, which may skip systemBackdrop rebind guard.
        if (m_titleBar != nullptr) {
            m_titleBar->setVisible(!isFullScreen());
        }
        scheduleStateVisualRefresh();
    } else if (event->type() == QEvent::ActivationChange) {
        if (isActiveWindow()) {
            scheduleStateVisualRefresh();
        }
    } else if (event->type() == QEvent::WinIdChange) {
        ensureNativeResizeStyle();
        scheduleStateVisualRefresh();
        QTimer::singleShot(0, this, [this]() {
            if (!isVisible()) {
                return;
            }

            scheduleStateVisualRefresh();
        });
    } else if (event->type() == QEvent::ApplicationPaletteChange
               || event->type() == QEvent::PaletteChange) {
        applyVisualEffects();
    }
}

void FramelessWindow::scheduleStateVisualRefresh()
{
    m_visualRefreshCoordinator.requestRefresh();
}

quint64 FramelessWindow::currentVisualStateToken() const
{
    return WindowVisualState::buildVisualStateToken(isVisible(),
                                                    isMaximized(),
                                                    isMinimized(),
                                                    isActiveWindow(),
                                                    m_shadowEnabled,
                                                    m_roundedCornersEnabled,
                                                    surfacePolicy().backdrop,
                                                    m_themeManager.themeMode(),
                                                    usesTranslucentSurface());
}

bool FramelessWindow::eventFilter(QObject *watched, QEvent *event)
{
    QWidget *watchedWidget = qobject_cast<QWidget *>(watched);

    if (event->type() == QEvent::MouseButtonPress) {
        const bool inContentPanel = watchedWidget != nullptr
                                    && m_contentPanel != nullptr
                                    && m_contentPanel->isAncestorOf(watchedWidget);
        const bool canInitiateResize = watched == this
                                       || watched == m_titleBar
                                       || inContentPanel;
        if (!canInitiateResize || qobject_cast<QPushButton *>(watched) != nullptr) {
            return QWidget::eventFilter(watched, event);
        }

        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            const bool started = tryStartSystemResizeAtGlobalPos(mouseEvent->globalPosition().toPoint());
            if (started) {
                mouseEvent->accept();
                return true;
            }
        }
    }

    if (event->type() == QEvent::MouseMove) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        updateCursorForPosition(mapFromGlobal(mouseEvent->globalPosition().toPoint()));
    } else if (event->type() == QEvent::Leave) {
        const QPoint localPos = mapFromGlobal(QCursor::pos());
        updateCursorForPosition(localPos);
    }

    return QWidget::eventFilter(watched, event);
}

void FramelessWindow::initMouseTracking()
{
    setMouseTracking(true);
    const auto widgets = findChildren<QWidget *>();
    for (QWidget *widget : widgets) {
        widget->setMouseTracking(true);
        widget->installEventFilter(this);
    }
}

void FramelessWindow::applyTheme()
{
    if (m_applyingTheme) {
        return;
    }

    QScopedValueRollback<bool> applyingGuard(m_applyingTheme, true);
    const bool useTranslucentBackground = usesTranslucentSurface();
    setAttribute(Qt::WA_TranslucentBackground, useTranslucentBackground);

    const QString styleSheet = m_themeManager.buildStyleSheet(useTranslucentBackground);
    if (m_lastAppliedStyleSheet != styleSheet) {
        setStyleSheet(styleSheet);
        m_lastAppliedStyleSheet = styleSheet;
    }
}

void FramelessWindow::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    // Native window handle may have been destroyed and recreated during
    // hide/show cycles. Reset null-handle logging guards so a null handle
    // in the new window is logged rather than silently suppressed.
    m_loggedNullWindowHandle = false;
    m_loggedNullWinId = false;

    if (m_titleBar != nullptr) {
        m_titleBar->setTitle(windowTitle());
        m_titleBar->setIcon(windowIcon());
    }
    ensureNativeResizeStyle();
    scheduleStateVisualRefresh();
}

void FramelessWindow::closeEvent(QCloseEvent *event)
{
    saveWindowGeometry();
    QWidget::closeEvent(event);
}

void FramelessWindow::saveWindowGeometry()
{
    if (isFullScreen() || isMinimized()) {
        return;
    }

    const bool maximized = isMaximized();
    QSettings settings(QStringLiteral("better-frameless-window"), QStringLiteral("settings"));
    settings.setValue(QStringLiteral("geometry/maximized"), maximized);

    // Persist the restore rectangle even while maximized: otherwise closing a
    // maximized window would keep whatever was saved the last time the window
    // happened to be restored, which can be arbitrarily stale.
    //
    // The tracked rectangle is preferred over the window manager's idea of it:
    // the maximize transition animates the geometry up to the work area before
    // the state flips, so GetWindowPlacement() would report the maximized
    // rectangle as the "normal" one.
    QRect rect = geometry();
    if (maximized) {
        rect = m_restoreGeometry;
    }

    if (!rect.isValid()) {
#ifdef Q_OS_WIN
        const QRect nativeRestore = WindowFrame::restoreGeometry(reinterpret_cast<void *>(winId()));
        if (!nativeRestore.isValid()) {
            return;
        }

        const qreal dpr = devicePixelRatioF();
        rect = dpr > 0.0
                   ? QRect(qRound(nativeRestore.x() / dpr),
                           qRound(nativeRestore.y() / dpr),
                           qRound(nativeRestore.width() / dpr),
                           qRound(nativeRestore.height() / dpr))
                   : nativeRestore;
#else
        // No source for the restore rectangle on this platform: keep whatever
        // was persisted rather than writing the maximized size.
        return;
#endif
    }
    settings.setValue(QStringLiteral("geometry/x"), rect.x());
    settings.setValue(QStringLiteral("geometry/y"), rect.y());
    settings.setValue(QStringLiteral("geometry/width"), rect.width());
    settings.setValue(QStringLiteral("geometry/height"), rect.height());
}

void FramelessWindow::restoreWindowGeometry()
{
    QSettings settings(QStringLiteral("better-frameless-window"), QStringLiteral("settings"));

    if (!settings.contains(QStringLiteral("geometry/width"))) {
        return;
    }

    const int x = settings.value(QStringLiteral("geometry/x"), 0).toInt();
    const int y = settings.value(QStringLiteral("geometry/y"), 0).toInt();
    const int w = settings.value(QStringLiteral("geometry/width"), 960).toInt();
    const int h = settings.value(QStringLiteral("geometry/height"), 640).toInt();
    const bool wasMaximized = settings.value(QStringLiteral("geometry/maximized"), false).toBool();

    QList<QRect> available;
    const auto screens = QGuiApplication::screens();
    available.reserve(screens.size());
    for (const QScreen *screen : screens) {
        available.append(screen->availableGeometry());
    }

    // A rectangle that only touches a monitor by a few pixels is not usable, so
    // pull it back onto the nearest screen instead of dropping it entirely.
    const QRect target = WindowGeometry::clampToNearestScreen(QRect(x, y, qMax(1, w), qMax(1, h)),
                                                              available);
    setGeometry(target);

    if (wasMaximized) {
        showMaximized();
    }
}

void FramelessWindow::forceNativeDwmRefresh()
{
#ifdef Q_OS_WIN
    WindowFrame::forceDwmRefresh(reinterpret_cast<void *>(winId()));
#else
    Q_UNUSED(0)
#endif
}

void FramelessWindow::mousePressEvent(QMouseEvent *event)
{
    // A press takes over from a running maximize/restore transition.
    finishWindowTransition();

    if (event->button() == Qt::LeftButton) {
        const bool started = tryStartSystemResizeAtLocalPos(event->pos());
        if (started) {
            event->accept();
            return;
        }
    }

    QWidget::mousePressEvent(event);
}

void FramelessWindow::mouseMoveEvent(QMouseEvent *event)
{
    updateCursorForPosition(event->pos());
    QWidget::mouseMoveEvent(event);
}

void FramelessWindow::moveEvent(QMoveEvent *event)
{
    QWidget::moveEvent(event);
    rememberRestoreGeometry();
}

void FramelessWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    rememberRestoreGeometry();
}

void FramelessWindow::leaveEvent(QEvent *event)
{
    unsetCursor();
    QWidget::leaveEvent(event);
}

int FramelessWindow::hitTest(const QPoint &globalPos) const
{
#ifdef Q_OS_WIN
    if (isFullScreen()) {
        return HTCLIENT;
    }

    WindowHitTest::Context context;
    context.hwnd = reinterpret_cast<void *>(winId());
    context.logicalWidth = width();
    context.logicalHeight = height();
    context.maximized = isMaximized();
    context.fullScreen = isFullScreen();
    context.snapLayoutEnabled = m_snapLayoutEnabled;
    context.titleRegionResolver = [this](const QPoint &localPos) {
        if (m_titleBar == nullptr || !m_titleBar->geometry().contains(localPos)) {
            return WindowHitTest::TitleRegion::None;
        }

        const TitleBar::HitRegion hit = m_titleBar->hitRegionAt(m_titleBar->mapFrom(this, localPos));
        switch (hit) {
        case TitleBar::HitRegion::Caption:
            return WindowHitTest::TitleRegion::Caption;
        case TitleBar::HitRegion::MinimizeButton:
            return WindowHitTest::TitleRegion::MinimizeButton;
        case TitleBar::HitRegion::MaximizeButton:
            return WindowHitTest::TitleRegion::MaximizeButton;
        case TitleBar::HitRegion::CloseButton:
            return WindowHitTest::TitleRegion::CloseButton;
        case TitleBar::HitRegion::OtherInteractive:
            return WindowHitTest::TitleRegion::OtherInteractive;
        case TitleBar::HitRegion::None:
        default:
            return WindowHitTest::TitleRegion::None;
        }
    };

    return WindowHitTest::nonClientHitTest(context, globalPos);
#endif

    return 0;
}

int FramelessWindow::resizeBorderThickness() const
{
#ifdef Q_OS_WIN
    return WindowHitTest::resizeBorderThickness(reinterpret_cast<void *>(winId()));
#else
    return 0;
#endif
}

Qt::Edges FramelessWindow::edgesForLocalPos(const QPoint &localPos) const
{
#ifdef Q_OS_WIN
    if (isMaximized() || isFullScreen()) {
        return Qt::Edges();
    }

    // Buttons must not be swallowed by the resize border.
    QWidget *hovered = childAt(localPos);
    if (qobject_cast<QPushButton *>(hovered) != nullptr) {
        return Qt::Edges();
    }

    return WindowHitTest::edgesAtLocalPos(localPos,
                                          size(),
                                          resizeBorderThickness(),
                                          false,
                                          false);
#else
    Q_UNUSED(localPos)
    return Qt::Edges();
#endif
}

void FramelessWindow::toggleMaximizeRestore()
{
#ifdef Q_OS_WIN
    // Windows never animates this window: the native transition belongs to the
    // frame, and the frame is ours (WS_CAPTION is cleared to keep Snap Layout
    // working), so SC_MAXIMIZE/SC_RESTORE land instantly. Animate the geometry
    // ourselves and only flip the window state once the rectangle arrives.
    if (m_windowTransition != nullptr) {
        // A second click while the transition runs must not stack animations.
        finishWindowTransition();
        return;
    }

    if (isMaximized()) {
        QRect target = m_restoreGeometry;
        if (!target.isValid()) {
            target = normalGeometry();
        }
        if (target.isValid()) {
            startWindowTransition(target, false);
            return;
        }
    } else if (const QScreen *targetScreen = screen()) {
        // The rectangle to come back to, captured before the animation walks
        // the geometry up to the work area.
        m_restoreGeometry = geometry();
        startWindowTransition(targetScreen->availableGeometry(), true);
        return;
    }

    // No usable target (for example a window that was maximized before it was
    // ever shown): fall back to an instant state change, which is abrupt but
    // always correct.
    if (!WindowCommand::setMaximized(reinterpret_cast<void *>(winId()), !isMaximized())) {
        Diagnostics::logWarning(QStringLiteral("toggleMaximizeRestore: native state change failed, fallback"));
        if (isMaximized()) {
            showNormal();
        } else {
            showMaximized();
        }
    }
#else
    if (isMaximized()) {
        showNormal();
    } else {
        showMaximized();
    }
#endif
}

void FramelessWindow::startWindowTransition(const QRect &target, bool maximizeAtEnd)
{
    const QRect from = geometry();
    m_windowTransitionMaximizes = maximizeAtEnd;
    m_animatingWindowTransition = true;

    if (!maximizeAtEnd) {
        // A maximized window cannot be moved or resized by the platform, so drop
        // the state first and animate back out of the old rectangle. This goes
        // through the platform rather than QWidget::showNormal(): the window
        // state then comes back from WM_SIZE, which keeps Qt's idea of the state
        // and the OS's in sync.
        WindowCommand::setMaximized(reinterpret_cast<void *>(winId()), false);
    }

    auto *animation = new QPropertyAnimation(this, "geometry", this);
    animation->setDuration(kWindowTransitionDurationMs);
    animation->setEasingCurve(QEasingCurve::InOutCubic);
    animation->setStartValue(from);
    animation->setEndValue(target);

    m_windowTransition = animation;
    connect(animation, &QPropertyAnimation::finished, this, [this]() {
        m_windowTransition = nullptr;
        completeWindowTransition();
    });

    // Maximize feedback is immediate in Windows: flip the glyph now and keep
    // updateMaximizeButtonState() from reverting it mid-transition.
    if (m_titleBar != nullptr) {
        m_titleBar->setMaximized(maximizeAtEnd);
    }

    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void FramelessWindow::finishWindowTransition()
{
    if (m_windowTransition == nullptr) {
        return;
    }

    // stop() does not emit finished(), so the end state is applied here.
    m_windowTransition->stop();
    m_windowTransition = nullptr;
    completeWindowTransition();
}

void FramelessWindow::completeWindowTransition()
{
    m_animatingWindowTransition = false;

    if (m_windowTransitionMaximizes) {
        m_windowTransitionMaximizes = false;
        // The rectangle already is the maximized one, so this only sets
        // WS_MAXIMIZE and lets WM_SIZE tell Qt about it.
        WindowCommand::setMaximized(reinterpret_cast<void *>(winId()), true);
    }

    // DWM dropped the backdrop while the geometry was moving; re-apply it now
    // that the rectangle has settled.
    beginBackdropTransitionGuard();
    scheduleStateVisualRefresh();
}

void FramelessWindow::rememberRestoreGeometry()
{
    // The geometry the window returns to. Tracked continuously so a maximize
    // that did not come from this class (Win+Up, snap) still animates, and
    // skipped while the transition animates through intermediate sizes.
    if (m_animatingWindowTransition || isMaximized() || isMinimized() || isFullScreen()) {
        return;
    }

#ifdef Q_OS_WIN
    // Qt learns the maximized state from WM_SIZE asynchronously, so during a
    // native maximize a move/resize event can arrive while isMaximized() is
    // still false although the geometry already is the work area. Ask the
    // window itself instead of relying on the event order.
    if (WindowFrame::isMaximized(reinterpret_cast<void *>(winId()))) {
        return;
    }
#endif

    m_restoreGeometry = geometry();
}

void FramelessWindow::startSystemMove()
{
    // Dragging the caption during a transition would fight the animation.
    finishWindowTransition();

    if (windowHandle() != nullptr && !isMaximized()) {
        windowHandle()->startSystemMove();
    }
}

void FramelessWindow::showSystemMenu(const QPoint &globalPos)
{
#ifdef Q_OS_WIN
    SystemMenu::WindowState state;
    state.maximized = isMaximized();
    state.minimized = isMinimized();
    SystemMenu::showForWindow(reinterpret_cast<void *>(winId()), globalPos, state);
#else
    Q_UNUSED(globalPos)
#endif
}

void FramelessWindow::updateMaximizeButtonState()
{
    if (m_titleBar == nullptr) {
        return;
    }

    // While the transition runs the window state does not match what the user
    // just asked for; startWindowTransition() already set the glyph.
    if (m_animatingWindowTransition) {
        return;
    }

    m_titleBar->setMaximized(isMaximized());
}

void FramelessWindow::updateCursorForPosition(const QPoint &localPos)
{
    const Qt::CursorShape shape = cursorForEdges(edgesForLocalPos(localPos));
    if (shape == Qt::ArrowCursor) {
        unsetCursor();
    } else {
        setCursor(shape);
    }
}

bool FramelessWindow::tryStartSystemResizeAtLocalPos(const QPoint &localPos)
{
    if (windowHandle() == nullptr || isMaximized()) {
        return false;
    }

    finishWindowTransition();

    const Qt::Edges edges = edgesForLocalPos(localPos);
    if (edges == Qt::Edges()) {
        return false;
    }

    windowHandle()->startSystemResize(edges);
    return true;
}

bool FramelessWindow::tryStartSystemResizeAtGlobalPos(const QPoint &globalPos)
{
    return tryStartSystemResizeAtLocalPos(mapFromGlobal(globalPos));
}

void FramelessWindow::ensureNativeResizeStyle()
{
#ifdef Q_OS_WIN
    WindowFrame::syncWindowFrame(reinterpret_cast<void *>(winId()));
#else
    Q_UNUSED(0)
#endif
}

void FramelessWindow::syncNativeWindowFrame()
{
#ifdef Q_OS_WIN
    if (m_animatingWindowTransition) {
        return;
    }

    WindowFrame::syncWindowFrame(reinterpret_cast<void *>(winId()));
#else
    Q_UNUSED(0)
#endif
}

void FramelessWindow::applyVisualEffects()
{
#ifdef Q_OS_WIN
    if (windowHandle() == nullptr) {
        if (!m_loggedNullWindowHandle) {
            Diagnostics::logWarning(QStringLiteral("applyVisualEffects skipped: windowHandle is null"));
            m_loggedNullWindowHandle = true;
        }

        QTimer::singleShot(0, this, [this]() {
            if (windowHandle() != nullptr) {
                scheduleStateVisualRefresh();
            }
        });
        return;
    }

    m_loggedNullWindowHandle = false;
    m_loggedNullWinId = false;

    // Sync WA_TranslucentBackground with the resolved surface before applying
    // native effects. Guards against direct applyVisualEffects() calls that
    // bypass applyTheme().
    const WindowVisualState::SurfacePolicy policy = surfacePolicy();
    const bool shouldBeTranslucent = WindowVisualState::usesTranslucentSurface(policy);
    if (shouldBeTranslucent != testAttribute(Qt::WA_TranslucentBackground)) {
        setAttribute(Qt::WA_TranslucentBackground, shouldBeTranslucent);
    }

    // DWM tears the backdrop down on minimize. Telling WindowEffect about it
    // keeps its cache honest, so the restore transition re-applies the backdrop
    // instead of being skipped as redundant.
    WindowVisualState::SurfacePolicy effectSurface = policy;
    if (isMinimized()) {
        effectSurface.backdrop = WindowEffect::BackdropKind::None;
    }

    if (m_shadowEnabled && shouldBeTranslucent && !m_loggedTranslucentShadow) {
        // DWM only paints its frame shadow for windows without per-pixel alpha.
        // A translucent surface is layered, so this flag has no visible effect
        // until the window switches to an opaque surface (BackdropMode::None).
        Diagnostics::logWarning(QStringLiteral("applyShadow: DWM draws no shadow for a translucent surface, "
                                               "shadow takes effect only when the window is opaque"));
        m_loggedTranslucentShadow = true;
    } else if (!m_shadowEnabled || !shouldBeTranslucent) {
        m_loggedTranslucentShadow = false;
    }

    const WindowEffect::VisualEffectOptions options = WindowVisualState::buildVisualEffectOptions(
        m_shadowEnabled,
        m_roundedCornersEnabled,
        m_themeManager.themeMode(),
        isMaximized(),
        isMinimized(),
        effectSurface,
        preferredBorderColor());

    void *hwnd = reinterpret_cast<void *>(winId());
    if (hwnd == nullptr) {
        if (!m_loggedNullWinId) {
            Diagnostics::logWarning(QStringLiteral("applyVisualEffects skipped: winId returned null handle"));
            m_loggedNullWinId = true;
        }

        QTimer::singleShot(0, this, [this]() {
            if (windowHandle() != nullptr) {
                scheduleStateVisualRefresh();
            }
        });
        return;
    }

    m_windowEffect.applyVisualEffects(hwnd, options);
#else
    Q_UNUSED(0)
#endif
}

void FramelessWindow::forceBackdropRebind()
{
#ifdef Q_OS_WIN
    const WindowEffect::BackdropKind backdrop = surfacePolicy().backdrop;
    if (backdrop == WindowEffect::BackdropKind::None || isMinimized() || windowHandle() == nullptr) {
        return;
    }

    void *hwnd = reinterpret_cast<void *>(winId());
    if (hwnd == nullptr) {
        return;
    }

    // Ask DWM to re-evaluate the backdrop after a maximize/restore or theme
    // transition. WindowEffect decides whether the previously applied effect
    // has to be reset first: Mica is a stored attribute and needs the reset,
    // while Acrylic is re-applied in place because clearing the accent policy
    // drops the blur for a frame.
    m_windowEffect.applyBackdrop(hwnd, backdrop, shouldUseDarkMode(), true);
#else
    Q_UNUSED(0)
#endif
}

bool FramelessWindow::shouldStartRestoreTransitionFromSizeState(bool isMaximizedState, bool isRestoredState)
{
    if (isMaximizedState) {
        m_lastNativeSizeMaximized = true;
        return false;
    }

    if (isRestoredState) {
        const bool restoredFromMaximized = m_lastNativeSizeMaximized;
        m_lastNativeSizeMaximized = false;
        return restoredFromMaximized;
    }

    return false;
}

void FramelessWindow::beginBackdropTransitionGuard()
{
    if (m_animatingWindowTransition) {
        // completeWindowTransition() arms the guard once the geometry settles.
        m_backdropTransitionGuardActive = false;
        return;
    }

    if (surfacePolicy().backdrop == WindowEffect::BackdropKind::None) {
        m_backdropTransitionGuardActive = false;
        return;
    }

    m_backdropTransitionGuardActive = true;
    const quint64 epoch = ++m_backdropTransitionEpoch;

    // applyBackdrop() always clears the previous backdrop before applying, so a
    // single recovery pass reliably restores it after a restore transition.
    QTimer::singleShot(160, this, [this, epoch]() {
        if (epoch != m_backdropTransitionEpoch) {
            return;
        }

        m_backdropTransitionGuardActive = false;
        requestVisualRefresh();
        forceBackdropRebind();
        forceNativeDwmRefresh();
    });
}

bool FramelessWindow::shouldUseDarkMode() const
{
    return WindowVisualState::shouldUseDarkMode(m_themeManager.themeMode());
}

WindowVisualState::SurfacePolicy FramelessWindow::surfacePolicy() const
{
    // The minimized flag is deliberately not part of the surface: Qt keeps the
    // translucent attribute stable across minimize/restore, because flipping it
    // mid-transition delays composition recovery on Windows. DWM drops the
    // backdrop on its own while the window is minimized.
    return WindowVisualState::resolveSurfacePolicy(m_backdropMode,
                                                   false,
                                                   !m_backgroundImage.isNull());
}

bool FramelessWindow::usesTranslucentSurface() const
{
    return WindowVisualState::usesTranslucentSurface(surfacePolicy());
}

QColor FramelessWindow::preferredBorderColor() const
{
    return m_borderColor;
}

Qt::CursorShape FramelessWindow::cursorForEdges(Qt::Edges edges) const
{
    if (edges == (Qt::TopEdge | Qt::LeftEdge) || edges == (Qt::BottomEdge | Qt::RightEdge)) {
        return Qt::SizeFDiagCursor;
    }

    if (edges == (Qt::TopEdge | Qt::RightEdge) || edges == (Qt::BottomEdge | Qt::LeftEdge)) {
        return Qt::SizeBDiagCursor;
    }

    if (edges == Qt::TopEdge || edges == Qt::BottomEdge) {
        return Qt::SizeVerCursor;
    }

    if (edges == Qt::LeftEdge || edges == Qt::RightEdge) {
        return Qt::SizeHorCursor;
    }

    return Qt::ArrowCursor;
}

void FramelessWindow::attachContentEventFilters(QWidget *widget)
{
    if (widget == nullptr) {
        return;
    }

    widget->setMouseTracking(true);
    widget->installEventFilter(this);

    const auto children = widget->findChildren<QWidget *>();
    for (QWidget *child : children) {
        child->setMouseTracking(true);
        child->installEventFilter(this);
    }
}

void FramelessWindow::detachContentEventFilters(QWidget *widget)
{
    if (widget == nullptr) {
        return;
    }

    const auto children = widget->findChildren<QWidget *>();
    for (QWidget *child : children) {
        child->removeEventFilter(this);
    }
    widget->removeEventFilter(this);
}
