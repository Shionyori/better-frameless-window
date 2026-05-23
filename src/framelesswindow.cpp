#include "framelesswindow.h"

#include "diagnostics.h"
#include "core/windowvisualstate.h"
#ifdef Q_OS_WIN
#include <qt_windows.h>
#include "win32/windowcommand.h"
#include "win32/systemmenu.h"
#include "win32/windowhittest.h"
#include "win32/windowframe.h"
#include "win32/nativemessagerouter.h"
#include "win32/utils.h"
#endif
#include "titlebar.h"

#include <QEvent>
#include <QLayout>
#include <QMouseEvent>
#include <QShortcut>
#include <QGraphicsOpacityEffect>
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

FramelessWindow::FramelessWindow(QWidget *parent)
    : QWidget(parent)
    , m_titleBar(nullptr)
    , m_contentPanel(nullptr)
    , m_userContentWidget(nullptr)
    , m_layout(nullptr)
    , m_shadowEnabled(true)
    , m_systemBackdropEnabled(true)
    , m_systemBackdropPreference(WindowEffect::SystemBackdropPreference::Auto)
    , m_roundedCornersEnabled(true)
    , m_systemDarkModeEnabled(true)
    , m_snapLayoutEnabled(true)
    , m_systemBackdropTransitionGuardActive(false)
    , m_systemBackdropTransitionEpoch(0)
    , m_lastNativeSizeMaximized(false)
    , m_applyingTheme(false)
    , m_lastAppliedStyleSheet()
    , m_loggedNullWindowHandle(false)
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

void FramelessWindow::setWindowOpacity(qreal opacity)
{
    const qreal normalizedOpacity = qBound<qreal>(0.0, opacity, 1.0);
    if (qFuzzyCompare(QWidget::windowOpacity(), normalizedOpacity)) {
        return;
    }

    QWidget::setWindowOpacity(normalizedOpacity);
}

qreal FramelessWindow::windowOpacity() const
{
    return QWidget::windowOpacity();
}

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

void FramelessWindow::setSystemShadowEnabled(bool enabled)
{
    if (m_shadowEnabled == enabled) {
        return;
    }

    m_shadowEnabled = enabled;
    requestVisualRefresh();
}

void FramelessWindow::setSystemBackdropEnabled(bool enabled)
{
    if (m_systemBackdropEnabled == enabled) {
        return;
    }

    m_systemBackdropEnabled = enabled;
    requestVisualRefresh();
}

void FramelessWindow::setSystemBackdropPreference(WindowEffect::SystemBackdropPreference preference)
{
    if (m_systemBackdropPreference == preference) {
        return;
    }

    m_systemBackdropPreference = preference;
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

void FramelessWindow::setSystemDarkModeEnabled(bool enabled)
{
    if (m_systemDarkModeEnabled == enabled) {
        return;
    }

    m_systemDarkModeEnabled = enabled;
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
    update();
}

void FramelessWindow::clearBackgroundImage()
{
    m_backgroundImage = QPixmap();
    update();
}

QPixmap FramelessWindow::backgroundImage() const
{
    return m_backgroundImage;
}

FramelessWindow::BackgroundImageMode FramelessWindow::backgroundImageMode() const
{
    return m_backgroundImageMode;
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
            forceSystemBackdropRebind();
        });
    } else {
        m_themeManager.setThemeMode(mode);
        m_themeManager.setTransitionProgress(1.0);
        requestVisualRefresh();

        QTimer::singleShot(50, this, [this]() {
            forceSystemBackdropRebind();
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
    // system resize (WM_ENTERSIZEMOVE → WM_EXITSIZEMOVE). Calling
    // SetWindowPos(SWP_FRAMECHANGED) or DwmSetWindowAttribute mid-resize
    // fights the system's own resize handling and causes visible jitter.
    if (!m_resizing) {
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

bool FramelessWindow::isSystemBackdropEnabled() const
{
    return m_systemBackdropEnabled;
}

WindowEffect::SystemBackdropPreference FramelessWindow::systemBackdropPreference() const
{
    return m_systemBackdropPreference;
}

bool FramelessWindow::isRoundedCornersEnabled() const
{
    return m_roundedCornersEnabled;
}

bool FramelessWindow::isSystemDarkModeEnabled() const
{
    return m_systemDarkModeEnabled;
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
        const bool systemDark = Utils::isSystemDarkModeEnabled();
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

    const bool systemDark = Utils::isSystemDarkModeEnabled();
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
                                                    m_systemBackdropEnabled,
                                                    m_roundedCornersEnabled,
                                                    m_systemDarkModeEnabled,
                                                    effectiveSystemBackdropPreference(),
                                                    m_themeManager.themeMode(),
                                                    shouldUseTranslucentBackground());
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
    const bool useTranslucentBackground = shouldUseTranslucentBackground();
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

    QSettings settings(QStringLiteral("better-frameless-window"), QStringLiteral("settings"));
    settings.setValue(QStringLiteral("geometry/maximized"), isMaximized());

    if (!isMaximized()) {
        settings.setValue(QStringLiteral("geometry/x"), x());
        settings.setValue(QStringLiteral("geometry/y"), y());
        settings.setValue(QStringLiteral("geometry/width"), width());
        settings.setValue(QStringLiteral("geometry/height"), height());
    }
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

    const QRect savedRect(x, y, w, h);
    bool onScreen = false;
    for (QScreen *screen : QGuiApplication::screens()) {
        if (screen->availableGeometry().intersects(savedRect)) {
            onScreen = true;
            break;
        }
    }

    if (onScreen) {
        setGeometry(x, y, w, h);
    }

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
    if (isMaximized()) {
        return Qt::Edges();
    }

    const int border = resizeBorderThickness();
    const int corner = border + 2;
    const int x = localPos.x();
    const int y = localPos.y();
    const int w = width();
    const int h = height();

    if (x < 0 || y < 0 || x >= w || y >= h) {
        return Qt::Edges();
    }

    QWidget *hovered = childAt(localPos);
    const bool onButton = qobject_cast<QPushButton *>(hovered) != nullptr;
    if (onButton) {
        return Qt::Edges();
    }

    Qt::Edges edges;

    if (x < corner && y < corner)
        return Qt::TopEdge | Qt::LeftEdge;
    if (x >= w - corner && y < corner)
        return Qt::TopEdge | Qt::RightEdge;
    if (x < corner && y >= h - corner)
        return Qt::BottomEdge | Qt::LeftEdge;
    if (x >= w - corner && y >= h - corner)
        return Qt::BottomEdge | Qt::RightEdge;

    if (x < border)
        edges |= Qt::LeftEdge;
    if (x >= w - border)
        edges |= Qt::RightEdge;
    if (y < border)
        edges |= Qt::TopEdge;
    if (y >= h - border)
        edges |= Qt::BottomEdge;

    return edges;
}

void FramelessWindow::toggleMaximizeRestore()
{
#ifdef Q_OS_WIN
    if (m_contentPanel == nullptr) {
        if (isMaximized()) {
            showNormal();
        } else {
            showMaximized();
        }
        return;
    }

    // Ensure content has an opacity effect
    auto *contentEffect = qobject_cast<QGraphicsOpacityEffect *>(m_contentPanel->graphicsEffect());
    if (contentEffect == nullptr) {
        contentEffect = new QGraphicsOpacityEffect(m_contentPanel);
        contentEffect->setOpacity(1.0);
        m_contentPanel->setGraphicsEffect(contentEffect);
    }

    // Fade content out, toggle during invisibility, fade back in.
    // No setUpdatesEnabled freeze needed — the opacity effect hides
    // the layout change while the DWM animation runs.
    auto *fadeOut = new QPropertyAnimation(contentEffect, "opacity", this);
    fadeOut->setDuration(60);
    fadeOut->setStartValue(contentEffect->opacity());
    fadeOut->setEndValue(0.0);
    fadeOut->start(QAbstractAnimation::DeleteWhenStopped);

    QTimer::singleShot(60, this, [this]() {
        if (!WindowCommand::toggleMaximizeRestore(reinterpret_cast<void *>(winId()), isMaximized())) {
            auto *fx = qobject_cast<QGraphicsOpacityEffect *>(m_contentPanel->graphicsEffect());
            if (fx != nullptr) {
                fx->setOpacity(1.0);
            }
            Diagnostics::logWarning(QStringLiteral("toggleMaximizeRestore: null HWND, fallback"));
            if (isMaximized()) {
                showNormal();
            } else {
                showMaximized();
            }
            return;
        }

        scheduleStateVisualRefresh();

        // Fade content back in after the DWM animation settles
        QTimer::singleShot(200, this, [this]() {
            if (!isVisible()) {
                return;
            }

            scheduleStateVisualRefresh();

            auto *fx = qobject_cast<QGraphicsOpacityEffect *>(m_contentPanel->graphicsEffect());
            if (fx != nullptr) {
                auto *fadeIn = new QPropertyAnimation(fx, "opacity", this);
                fadeIn->setDuration(100);
                fadeIn->setEasingCurve(QEasingCurve::OutCubic);
                fadeIn->setStartValue(0.0);
                fadeIn->setEndValue(1.0);
                fadeIn->start(QAbstractAnimation::DeleteWhenStopped);
            }
        });
    });
#else
    if (isMaximized()) {
        showNormal();
    } else {
        showMaximized();
    }
#endif
}

void FramelessWindow::startSystemMove()
{
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

    // Sync WA_TranslucentBackground with expected state before applying native effects.
    // Guards against direct applyVisualEffects() calls that bypass applyTheme().
    const bool shouldBeTranslucent = shouldUseTranslucentBackground();
    if (shouldBeTranslucent != testAttribute(Qt::WA_TranslucentBackground)) {
        setAttribute(Qt::WA_TranslucentBackground, shouldBeTranslucent);
    }

    const WindowEffect::VisualEffectOptions options = WindowVisualState::buildVisualEffectOptions(
        m_shadowEnabled,
        m_systemBackdropEnabled,
        effectiveSystemBackdropPreference(),
        m_roundedCornersEnabled,
        m_systemDarkModeEnabled,
        m_themeManager.themeMode(),
        isMaximized(),
        isMinimized(),
        preferredBorderColor());

    void *hwnd = reinterpret_cast<void *>(winId());
    if (hwnd == nullptr) {
        Diagnostics::logWarning(QStringLiteral("applyVisualEffects skipped: winId returned null handle"));
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

void FramelessWindow::forceSystemBackdropRebind()
{
#ifdef Q_OS_WIN
    if (!m_systemBackdropEnabled || windowHandle() == nullptr) {
        return;
    }

    void *hwnd = reinterpret_cast<void *>(winId());
    if (hwnd == nullptr) {
        return;
    }

    const bool useDarkMode = shouldUseDarkMode();
    const bool maximized = isMaximized();
    const bool minimized = isMinimized();

    // Force DWM to see a genuine state transition by clearing to None
    // first, then re-applying the target. This bypasses the per-instance
    // cache in applySystemBackdropEffects and ensures backdrop recovery
    // after maximize/restore transitions.
    m_windowEffect.applySystemBackdropEffects(hwnd,
                                        false,
                                        useDarkMode,
                                        maximized,
                                        minimized,
                                        m_systemBackdropPreference);
    m_windowEffect.applySystemBackdropEffects(hwnd,
                                        true,
                                        useDarkMode,
                                        maximized,
                                        minimized,
                                        m_systemBackdropPreference);
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

WindowEffect::SystemBackdropPreference FramelessWindow::effectiveSystemBackdropPreference() const
{
    return m_systemBackdropPreference;
}

void FramelessWindow::beginSystemBackdropTransitionGuard()
{
    if (!m_systemBackdropEnabled) {
        m_systemBackdropTransitionGuardActive = false;
        return;
    }

    m_systemBackdropTransitionGuardActive = true;
    const quint64 epoch = ++m_systemBackdropTransitionEpoch;

    // The new applySystemBackdropEffects always clears effects before applying,
    // so a single recovery pass reliably restores the backdrop after restore.
    QTimer::singleShot(160, this, [this, epoch]() {
        if (epoch != m_systemBackdropTransitionEpoch) {
            return;
        }

        m_systemBackdropTransitionGuardActive = false;
        requestVisualRefresh();
        forceSystemBackdropRebind();
        forceNativeDwmRefresh();
    });
}

bool FramelessWindow::shouldUseDarkMode() const
{
    return WindowVisualState::shouldUseDarkMode(m_themeManager.themeMode());
}

bool FramelessWindow::shouldUseTranslucentBackground() const
{
    // Keep Qt translucent-surface policy stable across maximize/restore guard.
    // Native systemBackdrop can be temporarily forced to None, but coupling that
    // transient state to QWidget translucency may cause delayed composition
    // recovery on Windows after restore.
    return WindowVisualState::shouldUseTranslucentBackground(m_systemBackdropEnabled,
                                                              false,
                                                              m_systemBackdropPreference);
}

QColor FramelessWindow::preferredBorderColor() const
{
    return QColor();
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
