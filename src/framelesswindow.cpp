#include "framelesswindow.h"

#include "diagnostics.h"
#include "core/windowvisualstate.h"
#include "platform/win32/windowcommandwin.h"
#include "platform/win32/systemmenuwin.h"
#include "platform/win32/windowhittestwin.h"
#include "platform/win32/windowframewin.h"
#include "platform/win32/winnativemessagerouter.h"
#include "titlebar.h"

#include <QEvent>
#include <QLayout>
#include <QMouseEvent>
#include <QObject>
#include <QPainter>
#include <QColor>
#include <QPushButton>
#include <QScopedValueRollback>
#include <QShowEvent>
#include <QStyle>
#include <QStyleOption>
#include <QTimer>
#include <QVBoxLayout>
#include <QWindow>

FramelessWindow::FramelessWindow(QWidget *parent)
    : QWidget(parent)
    , m_titleBar(nullptr)
    , m_contentPanel(nullptr)
    , m_userContentWidget(nullptr)
    , m_layout(nullptr)
    , m_shadowEnabled(true)
    , m_backdropEnabled(true)
    , m_backdropPreference(WindowEffectWin::BackdropPreference::Auto)
    , m_roundedCornersEnabled(true)
    , m_immersiveDarkModeEnabled(true)
    , m_backdropTransitionGuardActive(false)
    , m_backdropTransitionEpoch(0)
    , m_lastNativeSizeMaximized(false)
    , m_applyingTheme(false)
    , m_lastAppliedStyleSheet()
    , m_loggedNullWindowHandle(false)
    , m_visualRefreshCoordinator(this)
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

void FramelessWindow::setShadowEnabled(bool enabled)
{
    if (m_shadowEnabled == enabled) {
        return;
    }

    m_shadowEnabled = enabled;
    requestVisualRefresh();
}

void FramelessWindow::setBackdropEnabled(bool enabled)
{
    if (m_backdropEnabled == enabled) {
        return;
    }

    m_backdropEnabled = enabled;
    requestVisualRefresh();
}

void FramelessWindow::setBackdropPreference(WindowEffectWin::BackdropPreference preference)
{
    if (m_backdropPreference == preference) {
        return;
    }

    m_backdropPreference = preference;
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

void FramelessWindow::setImmersiveDarkModeEnabled(bool enabled)
{
    if (m_immersiveDarkModeEnabled == enabled) {
        return;
    }

    m_immersiveDarkModeEnabled = enabled;
    requestVisualRefresh();
}

void FramelessWindow::setThemeMode(ThemeManager::ThemeMode mode)
{
    if (m_themeManager.themeMode() == mode) {
        return;
    }

    m_themeManager.setThemeMode(mode);
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

void FramelessWindow::setBackgroundMode(ThemeManager::BackgroundMode mode)
{
    if (m_themeManager.backgroundMode() == mode) {
        return;
    }

    m_themeManager.setBackgroundMode(mode);
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
    syncNativeWindowFrame();
    applyVisualEffects();
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

bool FramelessWindow::isShadowEnabled() const
{
    return m_shadowEnabled;
}

bool FramelessWindow::isBackdropEnabled() const
{
    return m_backdropEnabled;
}

WindowEffectWin::BackdropPreference FramelessWindow::backdropPreference() const
{
    return m_backdropPreference;
}

bool FramelessWindow::isRoundedCornersEnabled() const
{
    return m_roundedCornersEnabled;
}

bool FramelessWindow::isImmersiveDarkModeEnabled() const
{
    return m_immersiveDarkModeEnabled;
}

bool FramelessWindow::isDiagnosticsEnabled() const
{
    return Diagnostics::isEnabled();
}

ThemeManager::ThemeMode FramelessWindow::themeMode() const
{
    return m_themeManager.themeMode();
}

QColor FramelessWindow::accentColor() const
{
    return m_themeManager.accentColor();
}

ThemeManager::BackgroundMode FramelessWindow::backgroundMode() const
{
    return m_themeManager.backgroundMode();
}

void FramelessWindow::initWindow()
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint);
    setMinimumSize(480, 320);
    resize(960, 640);
    setAttribute(Qt::WA_StyledBackground, true);

    setObjectName("FramelessWindow");
    applyTheme();
}

void FramelessWindow::initLayout()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setSpacing(0);
    m_layout->setContentsMargins(0, 0, 0, 0);

    m_titleBar = new TitleBar(this);
    m_contentPanel = new QWidget(this);
    m_contentPanel->setObjectName("FramelessContentPanel");
    m_contentPanel->setAttribute(Qt::WA_TranslucentBackground, true);

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

    if (NativeWindowsMessageRouter::handle(*this, message, result)) {
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
}

void FramelessWindow::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange) {
        // Keep maximize/restore edge tracking owned by WM_SIZE handling.
        // Mixing Qt-state writes here can clear the native transition flag
        // before SIZE_RESTORED arrives, which may skip backdrop rebind guard.
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
                                                    m_backdropEnabled,
                                                    m_roundedCornersEnabled,
                                                    m_immersiveDarkModeEnabled,
                                                    effectiveBackdropPreference(),
                                                    m_themeManager.themeMode(),
                                                    shouldUseTranslucentBackground());
}

bool FramelessWindow::eventFilter(QObject *watched, QEvent *event)
{
    QWidget *watchedWidget = qobject_cast<QWidget *>(watched);

    if (event->type() == QEvent::MouseButtonPress) {
        const bool inContentPanel = watchedWidget != nullptr
                                    && m_contentPanel != nullptr
                                    && (watchedWidget == m_contentPanel || m_contentPanel->isAncestorOf(watchedWidget));
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
    ensureNativeResizeStyle();
    scheduleStateVisualRefresh();
}

void FramelessWindow::forceNativeDwmRefresh()
{
    WindowFrameWin::forceDwmRefresh(reinterpret_cast<void *>(winId()));
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
    WindowHitTestWin::Context context;
    context.hwnd = reinterpret_cast<void *>(winId());
    context.logicalWidth = width();
    context.logicalHeight = height();
    context.maximized = isMaximized();
    context.titleRegionResolver = [this](const QPoint &localPos) {
        if (m_titleBar == nullptr || !m_titleBar->geometry().contains(localPos)) {
            return WindowHitTestWin::TitleRegion::None;
        }

        const TitleBar::HitRegion hit = m_titleBar->hitRegionAt(m_titleBar->mapFrom(this, localPos));
        switch (hit) {
        case TitleBar::HitRegion::Caption:
            return WindowHitTestWin::TitleRegion::Caption;
        case TitleBar::HitRegion::MinimizeButton:
            return WindowHitTestWin::TitleRegion::MinimizeButton;
        case TitleBar::HitRegion::MaximizeButton:
            return WindowHitTestWin::TitleRegion::MaximizeButton;
        case TitleBar::HitRegion::CloseButton:
            return WindowHitTestWin::TitleRegion::CloseButton;
        case TitleBar::HitRegion::OtherInteractive:
            return WindowHitTestWin::TitleRegion::OtherInteractive;
        case TitleBar::HitRegion::None:
        default:
            return WindowHitTestWin::TitleRegion::None;
        }
    };

    return WindowHitTestWin::nonClientHitTest(context, globalPos);
#endif

    return 0;
}

int FramelessWindow::resizeBorderThickness() const
{
    return WindowHitTestWin::resizeBorderThickness(reinterpret_cast<void *>(winId()));
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
    if (!WindowCommandWin::toggleMaximizeRestore(reinterpret_cast<void *>(winId()), isMaximized())) {
        Diagnostics::logWarning(QStringLiteral("toggleMaximizeRestore: null HWND, fallback to QWidget state switch"));
        if (isMaximized()) {
            showNormal();
        } else {
            showMaximized();
        }
        return;
    }

    scheduleStateVisualRefresh();
    QTimer::singleShot(90, this, [this]() {
        if (!isVisible()) {
            return;
        }

        scheduleStateVisualRefresh();
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
    SystemMenuWin::WindowState state;
    state.maximized = isMaximized();
    state.minimized = isMinimized();
    SystemMenuWin::showForWindow(reinterpret_cast<void *>(winId()), globalPos, state);
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
    WindowFrameWin::syncWindowFrame(reinterpret_cast<void *>(winId()));
}

void FramelessWindow::syncNativeWindowFrame()
{
    WindowFrameWin::syncWindowFrame(reinterpret_cast<void *>(winId()));
}

void FramelessWindow::applyVisualEffects()
{
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

    const WindowEffectWin::VisualEffectOptions options = WindowVisualState::buildVisualEffectOptions(
        m_shadowEnabled,
        m_backdropEnabled,
        effectiveBackdropPreference(),
        m_roundedCornersEnabled,
        m_immersiveDarkModeEnabled,
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
}

void FramelessWindow::forceBackdropRebind()
{
    if (!m_backdropEnabled || windowHandle() == nullptr) {
        return;
    }

    void *hwnd = reinterpret_cast<void *>(winId());
    if (hwnd == nullptr) {
        return;
    }

    const bool useDarkMode = shouldUseDarkMode();
    const bool maximized = isMaximized();
    const bool minimized = isMinimized();

    // Re-assert the target backdrop mode without inserting a temporary
    // visual-off state, so maximize/restore keeps a more consistent look.
    m_windowEffect.applyBackdropEffects(hwnd,
                                        true,
                                        useDarkMode,
                                        maximized,
                                        minimized,
                                        m_backdropPreference);
    m_windowEffect.applyBackdropEffects(hwnd,
                                        true,
                                        useDarkMode,
                                        maximized,
                                        minimized,
                                        m_backdropPreference);
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

WindowEffectWin::BackdropPreference FramelessWindow::effectiveBackdropPreference() const
{
    return m_backdropPreference;
}

void FramelessWindow::beginBackdropTransitionGuard()
{
    if (!m_backdropEnabled) {
        m_backdropTransitionGuardActive = false;
        return;
    }

    m_backdropTransitionGuardActive = true;
    const quint64 epoch = ++m_backdropTransitionEpoch;

    // Keep a short restore guard window for delayed re-assertion timings.
    // Backdrop preference itself stays stable to avoid transient effect loss.
    QTimer::singleShot(160, this, [this, epoch]() {
        if (epoch != m_backdropTransitionEpoch) {
            return;
        }

        m_backdropTransitionGuardActive = false;
        requestVisualRefresh();
        forceBackdropRebind();
        forceNativeDwmRefresh();

        // Fallback pass: some DWM composition paths may drop the first
        // restored-material bind. Re-apply once more after settle.
        QTimer::singleShot(220, this, [this, epoch]() {
            if (epoch != m_backdropTransitionEpoch || m_backdropTransitionGuardActive || !isVisible()) {
                return;
            }

            requestVisualRefresh();
            forceBackdropRebind();
            forceNativeDwmRefresh();
        });
    });
}

bool FramelessWindow::shouldUseDarkMode() const
{
    return WindowVisualState::shouldUseDarkMode(m_themeManager.themeMode());
}

bool FramelessWindow::shouldUseTranslucentBackground() const
{
    // Keep Qt translucent-surface policy stable across maximize/restore guard.
    // Native backdrop can be temporarily forced to None, but coupling that
    // transient state to QWidget translucency may cause delayed composition
    // recovery on Windows after restore.
    return WindowVisualState::shouldUseTranslucentBackground(m_backdropEnabled,
                                                              false,
                                                              m_backdropPreference);
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
