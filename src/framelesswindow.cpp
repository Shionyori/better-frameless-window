#include "framelesswindow.h"

#include "diagnostics.h"
#include "winutils.h"
#include "titlebar.h"

#include <QEvent>
#include <QGuiApplication>
#include <QLayout>
#include <QMouseEvent>
#include <QObject>
#include <QPainter>
#include <QColor>
#include <QPushButton>
#include <QScopedValueRollback>
#include <QScreen>
#include <QShowEvent>
#include <QStyle>
#include <QStyleOption>
#include <QTimer>
#include <QVBoxLayout>
#include <QWindow>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#include <windowsx.h>
#include <dwmapi.h>

#ifndef WM_NCUAHDRAWCAPTION
#define WM_NCUAHDRAWCAPTION 0x00AE
#endif

#ifndef WM_NCUAHDRAWFRAME
#define WM_NCUAHDRAWFRAME 0x00AF
#endif
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
    , m_aeroBlurEnabled(true)
    , m_applyingTheme(false)
    , m_lastAppliedStyleSheet()
    , m_lastTranslucentBackground(false)
    , m_loggedNullWindowHandle(false)
    , m_pendingStateVisualRefresh(false)
{
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

void FramelessWindow::setAeroBlurEnabled(bool enabled)
{
    if (m_aeroBlurEnabled == enabled) {
        return;
    }

    m_aeroBlurEnabled = enabled;
    applyTheme();
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

void FramelessWindow::setBackgroundImagePath(const QString &imagePath)
{
    if (m_themeManager.backgroundImagePath() == imagePath) {
        return;
    }

    m_themeManager.setBackgroundImagePath(imagePath);
    if (m_themeManager.backgroundMode() == ThemeManager::BackgroundMode::Image) {
        applyTheme();
    }
}

void FramelessWindow::setCentralWidget(QWidget *widget)
{
    setContentWidget(widget);
}

QWidget *FramelessWindow::centralWidget() const
{
    return contentWidget();
}

QWidget *FramelessWindow::takeCentralWidget()
{
    return takeContentWidget();
}

void FramelessWindow::setContentWidget(QWidget *widget)
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

QWidget *FramelessWindow::contentWidget() const
{
    return m_userContentWidget;
}

QWidget *FramelessWindow::takeContentWidget()
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

bool FramelessWindow::isAeroBlurEnabled() const
{
    return m_aeroBlurEnabled;
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

QString FramelessWindow::backgroundImagePath() const
{
    return m_themeManager.backgroundImagePath();
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

    if (handleNativeWindowsMessage(message, result)) {
        return true;
    }

    return QWidget::nativeEvent(eventType, message, result);
}

bool FramelessWindow::handleNativeWindowsMessage(void *message, qintptr *result)
{
    const MSG *msg = static_cast<MSG *>(message);
    if (msg == nullptr) {
        Diagnostics::logWarning(QStringLiteral("nativeEvent received null MSG pointer"));
        return false;
    }

    switch (msg->message) {
    case WM_NCHITTEST:
    {
        const HWND hwnd = reinterpret_cast<HWND>(winId());
        if (hwnd != nullptr) {
            LRESULT dwmResult = 0;
            if (DwmDefWindowProc(hwnd,
                                 msg->message,
                                 msg->wParam,
                                 msg->lParam,
                                 &dwmResult)) {
                *result = static_cast<qintptr>(dwmResult);
                return true;
            }
        }
        return handleNcHitTestMessage(static_cast<qintptr>(msg->lParam), result);
    }
    case WM_NCPAINT:
    case WM_NCUAHDRAWCAPTION:
    case WM_NCUAHDRAWFRAME:
        *result = 0;
        return true;
    case WM_NCACTIVATE:
        *result = TRUE;
        return true;
    case WM_NCLBUTTONDOWN:
    case WM_NCLBUTTONDBLCLK:
    case WM_NCLBUTTONUP:
        return handleNcButtonMessage(msg->message,
                                     static_cast<quintptr>(msg->wParam),
                                     result);
    case WM_NCMOUSELEAVE:
    case WM_MOUSELEAVE:
    case WM_CAPTURECHANGED:
    case WM_KILLFOCUS:
        clearMaximizeButtonNativeHover();
        return false;
    case WM_SIZE:
        clearMaximizeButtonNativeHover();
        syncNativeWindowFrame();
        if (msg->wParam == SIZE_RESTORED) {
            QTimer::singleShot(50, this, [this]() {
                scheduleStateVisualRefresh();
            });
        } else if (msg->wParam == SIZE_MAXIMIZED) {
            scheduleStateVisualRefresh();
        }
        return false;
    case WM_ACTIVATE:
        if (LOWORD(msg->wParam) == WA_INACTIVE) {
            clearMaximizeButtonNativeHover();
        }
        return false;
    case WM_NCCALCSIZE:
        if (msg->wParam == TRUE) {
            *result = 0;
            return true;
        }
        return false;
    case WM_GETMINMAXINFO:
        return handleGetMinMaxInfoMessage(reinterpret_cast<void *>(msg->lParam), result);
    case WM_NCRBUTTONUP:
        return handleNcRightButtonUpMessage(static_cast<quintptr>(msg->wParam), static_cast<qintptr>(msg->lParam), result);
    default:
        return false;
    }
}

bool FramelessWindow::handleNcHitTestMessage(qintptr lParam, qintptr *result)
{
    const QPoint globalPos(GET_X_LPARAM(static_cast<LPARAM>(lParam)), GET_Y_LPARAM(static_cast<LPARAM>(lParam)));
    const int hit = hitTest(globalPos);

    WinUtils::setMaximizeButtonNativeHover(m_titleBar, hit == HTMAXBUTTON);

    if (hit != HTCLIENT) {
        *result = hit;
        return true;
    }

    return false;
}

bool FramelessWindow::handleNcButtonMessage(quint32 messageId, quintptr wParam, qintptr *result)
{
    if (wParam != HTMAXBUTTON && wParam != HTMINBUTTON && wParam != HTCLOSE) {
        return false;
    }

    if (messageId == WM_NCLBUTTONUP) {
        *result = 0;
        return true;
    }

    clearMaximizeButtonNativeHover();

    if (messageId == WM_NCLBUTTONDOWN) {
        if (wParam == HTMAXBUTTON) {
            toggleMaximizeRestore();
        } else if (wParam == HTMINBUTTON) {
            showMinimized();
        } else if (wParam == HTCLOSE) {
            close();
        }
    }

    *result = 0;
    return true;
}

bool FramelessWindow::handleGetMinMaxInfoMessage(void *lParam, qintptr *result)
{
    auto *mmi = reinterpret_cast<MINMAXINFO *>(lParam);
    if (mmi == nullptr) {
        Diagnostics::logWarning(QStringLiteral("WM_GETMINMAXINFO received null MINMAXINFO"));
        return false;
    }

    const QScreen *screen = windowHandle() ? windowHandle()->screen() : QGuiApplication::primaryScreen();
    if (screen != nullptr) {
        const QRect available = screen->availableGeometry();
        const QRect screenGeometry = screen->geometry();
        mmi->ptMaxPosition.x = available.x() - screenGeometry.x();
        mmi->ptMaxPosition.y = available.y() - screenGeometry.y();
        mmi->ptMaxSize.x = available.width();
        mmi->ptMaxSize.y = available.height();
    }

    *result = 0;
    return true;
}

bool FramelessWindow::handleNcRightButtonUpMessage(quintptr wParam, qintptr lParam, qintptr *result)
{
    if (wParam != HTCAPTION && wParam != HTSYSMENU) {
        return false;
    }

    const QPoint globalPos(GET_X_LPARAM(static_cast<LPARAM>(lParam)), GET_Y_LPARAM(static_cast<LPARAM>(lParam)));
    showSystemMenu(globalPos);
    *result = 0;
    return true;
}

void FramelessWindow::clearMaximizeButtonNativeHover()
{
    WinUtils::setMaximizeButtonNativeHover(m_titleBar, false);
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
        QTimer::singleShot(200, this, [this]() {
            if (!isVisible()) {
                return;
            }
#ifdef Q_OS_WIN
            const HWND hwndInsurance = reinterpret_cast<HWND>(winId());
            if (hwndInsurance != nullptr) {
                SetWindowPos(hwndInsurance, nullptr, 0, 0, 0, 0,
                             SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
                if (m_backdropEnabled
                    && m_backdropPreference == WindowEffectWin::BackdropPreference::Acrylic) {
                    applyVisualEffects();
                }
            }
#endif
        });
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
    if (m_pendingStateVisualRefresh) {
        return;
    }

    m_pendingStateVisualRefresh = true;

    const bool shouldBeTranslucent = shouldUseTranslucentBackground();
    const bool needsReset = shouldBeTranslucent;

    if (needsReset) {
        setAttribute(Qt::WA_TranslucentBackground, false);

#ifdef Q_OS_WIN
        const HWND hwnd = reinterpret_cast<HWND>(winId());
        if (hwnd != nullptr) {
            m_windowEffect.applyBackdropEffects(reinterpret_cast<void *>(hwnd),
                                                false,
                                                shouldUseDarkMode(),
                                                isMaximized(),
                                                isMinimized(),
                                                m_aeroBlurEnabled,
                                                m_backdropPreference);
            SetWindowPos(hwnd,
                         nullptr,
                         0,
                         0,
                         0,
                         0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        }
#endif
    }

    QTimer::singleShot(50, this, [this, shouldBeTranslucent, needsReset]() {
        if (shouldBeTranslucent) {
            setAttribute(Qt::WA_TranslucentBackground, true);
        }

        m_pendingStateVisualRefresh = false;
        applyTheme();
        syncNativeWindowFrame();
        applyVisualEffects();
        updateMaximizeButtonState();
        if (needsReset) {
            forceNativeDwmRefresh();
        }
        update();

        QTimer::singleShot(150, this, [this, shouldBeTranslucent]() {
            if (!isVisible()) {
                return;
            }

            if (shouldBeTranslucent != testAttribute(Qt::WA_TranslucentBackground)) {
                setAttribute(Qt::WA_TranslucentBackground, shouldBeTranslucent);
            }

            applyTheme();
            syncNativeWindowFrame();
            applyVisualEffects();
            forceNativeDwmRefresh();
            update();
            repaint();

            QTimer::singleShot(100, this, [this]() {
                if (!isVisible()) {
                    return;
                }

                update();
                repaint();
#ifdef Q_OS_WIN
                const HWND hwnd = reinterpret_cast<HWND>(winId());
                if (hwnd != nullptr) {
                    RedrawWindow(hwnd,
                                 nullptr,
                                 nullptr,
                                 RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME | RDW_ALLCHILDREN);
                }
#endif
            });
        });
    });
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

    const WinUtils::TitleBarButtonType buttonType = WinUtils::titleBarButtonTypeAt(m_titleBar, this, localPos);
    if (buttonType == WinUtils::TitleBarButtonType::Maximize) {
        if (GetKeyState(VK_LBUTTON) < 0) {
            return HTCLIENT;
        }
        return HTMAXBUTTON;
    }

    if (buttonType != WinUtils::TitleBarButtonType::None) {
        return HTCLIENT;
    }

    const bool onTitleBarCaptionArea = WinUtils::isOnTitleBarCaptionArea(m_titleBar, this, localPos);

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
    WinUtils::setMaximizeButtonNativeHover(m_titleBar, false);

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

    const bool wasTranslucentBefore = testAttribute(Qt::WA_TranslucentBackground);
    if (wasTranslucentBefore) {
        m_windowEffect.applyBackdropEffects(reinterpret_cast<void *>(hwnd),
                                            false,
                                            shouldUseDarkMode(),
                                            isMaximized(),
                                            isMinimized(),
                                            m_aeroBlurEnabled,
                                            m_backdropPreference);
        setAttribute(Qt::WA_TranslucentBackground, false);
        SetWindowPos(hwnd,
                     nullptr,
                     0,
                     0,
                     0,
                     0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }

    const WPARAM command = isMaximized() ? SC_RESTORE : SC_MAXIMIZE;
    SendMessage(hwnd, WM_SYSCOMMAND, command, 0);

    if (wasTranslucentBefore) {
        QTimer::singleShot(300, this, [this]() {
            if (!isVisible()) {
                return;
            }

            if (shouldUseTranslucentBackground() && !testAttribute(Qt::WA_TranslucentBackground)) {
                setAttribute(Qt::WA_TranslucentBackground, true);
            }
            applyTheme();
            syncNativeWindowFrame();
            applyVisualEffects();
            const HWND refreshHwnd = reinterpret_cast<HWND>(winId());
            if (refreshHwnd != nullptr) {
                SetWindowPos(refreshHwnd, nullptr, 0, 0, 0, 0,
                             SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
                RedrawWindow(refreshHwnd,
                             nullptr,
                             nullptr,
                             RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME | RDW_ALLCHILDREN);
            }

            QTimer::singleShot(100, this, [this]() {
                if (!isVisible() || !shouldUseTranslucentBackground()) {
                    return;
                }

                if (!testAttribute(Qt::WA_TranslucentBackground)) {
                    setAttribute(Qt::WA_TranslucentBackground, true);
                }

                applyVisualEffects();
                update();
                repaint();
            });
        });
    }
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

    WindowEffectWin::VisualEffectOptions options;
    options.shadowEnabled = m_shadowEnabled;
    options.backdropEnabled = m_backdropEnabled;
    options.backdropPreference = m_backdropPreference;
    options.roundedCornersEnabled = m_roundedCornersEnabled;
    options.immersiveDarkModeEnabled = m_immersiveDarkModeEnabled;
    options.aeroBlurEnabled = m_aeroBlurEnabled;
    options.useDarkMode = shouldUseDarkMode();
    options.maximized = isMaximized();
    options.minimized = isMinimized();
    options.borderColor = preferredBorderColor();

    void *hwnd = reinterpret_cast<void *>(winId());
    if (hwnd == nullptr) {
        Diagnostics::logWarning(QStringLiteral("applyVisualEffects skipped: winId returned null handle"));
        return;
    }

    m_windowEffect.applyVisualEffects(hwnd, options);
}

bool FramelessWindow::shouldUseDarkMode() const
{
    return m_themeManager.isDarkMode();
}

bool FramelessWindow::shouldUseTranslucentBackground() const
{
#ifdef Q_OS_WIN
    if (!m_backdropEnabled || isMinimized()) {
        return false;
    }

    const WinUtils::WindowsCapabilities caps = WinUtils::detectWindowsCapabilities();
    const bool autoChainAvailable = caps.supportsSystemBackdrop
                                    || caps.supportsLegacyMica
                                    || caps.supportsAcrylic
                                    || (m_aeroBlurEnabled && caps.supportsAeroBlur);

    switch (m_backdropPreference) {
    case WindowEffectWin::BackdropPreference::Auto:
        return autoChainAvailable;
    case WindowEffectWin::BackdropPreference::None:
        return false;
    case WindowEffectWin::BackdropPreference::MicaSystem:
        return caps.supportsSystemBackdrop ? true : autoChainAvailable;
    case WindowEffectWin::BackdropPreference::MicaLegacy:
        return caps.supportsLegacyMica ? true : autoChainAvailable;
    case WindowEffectWin::BackdropPreference::Acrylic:
        return caps.supportsAcrylic ? true : autoChainAvailable;
    case WindowEffectWin::BackdropPreference::Aero:
        return (m_aeroBlurEnabled && caps.supportsAeroBlur) ? true : autoChainAvailable;
    }

    return false;
#else
    return false;
#endif
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
