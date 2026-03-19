#include "framelesswindow.h"

#include "diagnostics.h"
#include "windowvisualstate.h"
#include "winnativemessagerouter.h"
#include "winutils.h"
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

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

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
    , m_applyingTheme(false)
    , m_lastAppliedStyleSheet()
    , m_lastTranslucentBackground(false)
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
            applyTheme();
            syncNativeWindowFrame();
            applyVisualEffects();
            updateMaximizeButtonState();
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
    applyVisualEffects();
}

void FramelessWindow::setBackdropEnabled(bool enabled)
{
    if (m_backdropEnabled == enabled) {
        return;
    }

    m_backdropEnabled = enabled;
    applyTheme();
    applyVisualEffects();
}

void FramelessWindow::setBackdropPreference(WindowEffectWin::BackdropPreference preference)
{
    if (m_backdropPreference == preference) {
        return;
    }

    m_backdropPreference = preference;
    applyTheme();
    applyVisualEffects();
}

void FramelessWindow::setRoundedCornersEnabled(bool enabled)
{
    if (m_roundedCornersEnabled == enabled) {
        return;
    }

    m_roundedCornersEnabled = enabled;
    applyVisualEffects();
}

void FramelessWindow::setImmersiveDarkModeEnabled(bool enabled)
{
    if (m_immersiveDarkModeEnabled == enabled) {
        return;
    }

    m_immersiveDarkModeEnabled = enabled;
    applyVisualEffects();
}

void FramelessWindow::setThemeMode(ThemeManager::ThemeMode mode)
{
    if (m_themeManager.themeMode() == mode) {
        return;
    }

    m_themeManager.setThemeMode(mode);
    applyTheme();
    applyVisualEffects();
}

void FramelessWindow::setAccentColor(const QColor &accentColor)
{
    if (!accentColor.isValid() || m_themeManager.accentColor() == accentColor) {
        return;
    }

    m_themeManager.setAccentColor(accentColor);
    applyTheme();
    applyVisualEffects();
}

void FramelessWindow::setBackgroundMode(ThemeManager::BackgroundMode mode)
{
    if (m_themeManager.backgroundMode() == mode) {
        return;
    }

    m_themeManager.setBackgroundMode(mode);
    applyTheme();
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
        scheduleStateVisualRefresh();
    } else if (event->type() == QEvent::ActivationChange) {
        if (isActiveWindow()) {
            scheduleStateVisualRefresh();
        }
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
                                                    m_backdropPreference,
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
    m_lastTranslucentBackground = useTranslucentBackground;

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
#ifdef Q_OS_WIN
    const HWND hwnd = reinterpret_cast<HWND>(winId());
    if (hwnd == nullptr) {
        return;
    }

    SetWindowPos(hwnd,
                 nullptr,
                 0,
                 0,
                 0,
                 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    RedrawWindow(hwnd,
                 nullptr,
                 nullptr,
                 RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME | RDW_ALLCHILDREN);
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
    const HWND hwnd = reinterpret_cast<HWND>(winId());
    if (hwnd == nullptr) {
        return HTCLIENT;
    }

    RECT windowRect{};
    if (!GetWindowRect(hwnd, &windowRect)) {
        return HTCLIENT;
    }

    const UINT dpi = GetDpiForWindow(hwnd);
    const int frame = GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi);
    const int padded = GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
    const int border = qBound(4, frame + padded, 12);
    const int corner = border + 2;

    const int left = windowRect.left;
    const int top = windowRect.top;
    const int right = windowRect.right - 1;
    const int bottom = windowRect.bottom - 1;
    const int x = globalPos.x();
    const int y = globalPos.y();

    if (x < left || x > right || y < top || y > bottom) {
        return HTCLIENT;
    }

    const int logicalWidth = qMax(1, width());
    const int logicalHeight = qMax(1, height());
    const QRect nativeWindowRect(left, top, right - left + 1, bottom - top + 1);
    const QPoint localPos = WinUtils::toLocalPos(globalPos, nativeWindowRect, logicalWidth, logicalHeight);

    TitleBar::HitRegion titleBarHitRegion = TitleBar::HitRegion::None;
    if (m_titleBar != nullptr && m_titleBar->geometry().contains(localPos)) {
        titleBarHitRegion = m_titleBar->hitRegionAt(m_titleBar->mapFrom(this, localPos));
    }

    if (titleBarHitRegion == TitleBar::HitRegion::MaximizeButton) {
        if (GetKeyState(VK_LBUTTON) < 0) {
            return HTCLIENT;
        }
        return HTMAXBUTTON;
    }

    if (titleBarHitRegion == TitleBar::HitRegion::MinimizeButton
        || titleBarHitRegion == TitleBar::HitRegion::CloseButton
        || titleBarHitRegion == TitleBar::HitRegion::OtherInteractive) {
        return HTCLIENT;
    }

    const bool onTitleBarCaptionArea = titleBarHitRegion == TitleBar::HitRegion::Caption;

    if (isMaximized()) {
        return onTitleBarCaptionArea ? HTCAPTION : HTCLIENT;
    }

    Qt::Edges edges;

    const bool onLeftCorner = (x >= left - corner && x < left + corner);
    const bool onRightCorner = (x <= right + corner && x > right - corner);
    const bool onTopCorner = (y >= top - corner && y < top + corner);
    const bool onBottomCorner = (y <= bottom + corner && y > bottom - corner);

    if (onLeftCorner && onTopCorner)
        edges = Qt::TopEdge | Qt::LeftEdge;
    else if (onRightCorner && onTopCorner)
        edges = Qt::TopEdge | Qt::RightEdge;
    else if (onLeftCorner && onBottomCorner)
        edges = Qt::BottomEdge | Qt::LeftEdge;
    else if (onRightCorner && onBottomCorner)
        edges = Qt::BottomEdge | Qt::RightEdge;
    else {
        if (x >= left - border && x < left + border)
            edges |= Qt::LeftEdge;
        if (x <= right + border && x > right - border)
            edges |= Qt::RightEdge;
        if (y >= top - border && y < top + border)
            edges |= Qt::TopEdge;
        if (y <= bottom + border && y > bottom - border)
            edges |= Qt::BottomEdge;
    }

    if (edges != Qt::Edges()) {
        return WinUtils::hitFromEdges(edges);
    }

    if (onTitleBarCaptionArea) {
        return HTCAPTION;
    }
#endif

    return HTCLIENT;
}

int FramelessWindow::resizeBorderThickness() const
{
#ifdef Q_OS_WIN
    const HWND hwnd = reinterpret_cast<HWND>(winId());
    const UINT dpi = hwnd ? GetDpiForWindow(hwnd) : 96;
    const int frame = GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi);
    const int padded = GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
    const int nativeBorder = frame + padded;
    return qBound(4, nativeBorder, 6);
#else
    return 4;
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
    const HWND hwnd = reinterpret_cast<HWND>(winId());
    if (hwnd == nullptr) {
        Diagnostics::logWarning(QStringLiteral("toggleMaximizeRestore: null HWND, fallback to QWidget state switch"));
        if (isMaximized()) {
            showNormal();
        } else {
            showMaximized();
        }
        return;
    }

    const WPARAM command = isMaximized() ? SC_RESTORE : SC_MAXIMIZE;
    SendMessage(hwnd, WM_SYSCOMMAND, command, 0);
    scheduleStateVisualRefresh();
    QTimer::singleShot(120, this, [this]() {
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
#ifdef Q_OS_WIN
    const HWND hwnd = reinterpret_cast<HWND>(winId());
    if (hwnd == nullptr) {
        Diagnostics::logWarning(QStringLiteral("showSystemMenu: null HWND"));
        return;
    }

    HMENU menu = GetSystemMenu(hwnd, FALSE);
    if (menu == nullptr) {
        Diagnostics::logWarning(QStringLiteral("showSystemMenu: GetSystemMenu returned null"));
        return;
    }

    updateSystemMenuState(reinterpret_cast<void *>(menu));
    const POINT cursorPos{globalPos.x(), globalPos.y()};

    UINT flags = TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON;
    const int command = TrackPopupMenu(menu, flags, cursorPos.x, cursorPos.y, 0, hwnd, nullptr);
    if (command != 0) {
        const UINT sysCommand = static_cast<UINT>(command) & 0xFFF0;

        if (sysCommand == SC_MOVE || sysCommand == SC_SIZE) {
            ReleaseCapture();
        }

        SendMessage(hwnd, WM_SYSCOMMAND, static_cast<WPARAM>(sysCommand), 0);
    }
#else
    Q_UNUSED(globalPos)
#endif
}

void FramelessWindow::updateSystemMenuState(void *menuHandle) const
{
#ifdef Q_OS_WIN
    if (menuHandle == nullptr) {
        return;
    }

    HMENU menu = static_cast<HMENU>(menuHandle);
    const bool maximized = isMaximized();
    const bool minimized = isMinimized();

    auto setItemEnabled = [menu](UINT item, bool enabled) {
        EnableMenuItem(menu, item, MF_BYCOMMAND | (enabled ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
    };

    setItemEnabled(SC_RESTORE, maximized || minimized);
    setItemEnabled(SC_MOVE, !maximized && !minimized);
    setItemEnabled(SC_SIZE, !maximized && !minimized);
    setItemEnabled(SC_MINIMIZE, !minimized);
    setItemEnabled(SC_MAXIMIZE, !maximized);

    DrawMenuBar(reinterpret_cast<HWND>(winId()));
#else
    Q_UNUSED(menuHandle)
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
    void *hwnd = reinterpret_cast<void *>(winId());
    WinUtils::syncNativeWindowStyles(hwnd, true);
#endif
}

void FramelessWindow::syncNativeWindowFrame()
{
#ifdef Q_OS_WIN
    void *hwnd = reinterpret_cast<void *>(winId());
    // Use full style sync (including ex-style and WS_POPUP cleanup) every time.
    // WA_TranslucentBackground/state transitions may recreate or mutate native
    // styles, and partial sync can leave the window in a popup-like state where
    // Snap Layout (HTMAXBUTTON path) and DWM effects become inconsistent.
    WinUtils::syncNativeWindowStyles(hwnd, true);
#endif
}

void FramelessWindow::applyVisualEffects()
{
    if (windowHandle() == nullptr) {
        if (!m_loggedNullWindowHandle) {
            Diagnostics::logWarning(QStringLiteral("applyVisualEffects skipped: windowHandle is null"));
            m_loggedNullWindowHandle = true;
        }
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
        m_backdropPreference,
        m_roundedCornersEnabled,
        m_immersiveDarkModeEnabled,
        m_themeManager.themeMode(),
        isMaximized(),
        isMinimized(),
        preferredBorderColor());

    void *hwnd = reinterpret_cast<void *>(winId());
    if (hwnd == nullptr) {
        Diagnostics::logWarning(QStringLiteral("applyVisualEffects skipped: winId returned null handle"));
        return;
    }

    m_windowEffect.applyVisualEffects(hwnd, options);
}

bool FramelessWindow::shouldUseDarkMode() const
{
    return WindowVisualState::shouldUseDarkMode(m_themeManager.themeMode());
}

bool FramelessWindow::shouldUseTranslucentBackground() const
{
    return WindowVisualState::shouldUseTranslucentBackground(m_backdropEnabled,
                                                              isMinimized(),
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
