#include "framelesswindow.h"

#include "titlebar.h"

#include <QApplication>
#include <QEvent>
#include <QGuiApplication>
#include <QLabel>
#include <QLayout>
#include <QMouseEvent>
#include <QObject>
#include <QPainter>
#include <QColor>
#include <QPalette>
#include <QPushButton>
#include <QScreen>
#include <QShowEvent>
#include <QStyle>
#include <QStyleOption>
#include <QVBoxLayout>
#include <QWindow>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#include <windowsx.h>
#include <dwmapi.h>

#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif

#ifndef DWMWA_BORDER_COLOR
#define DWMWA_BORDER_COLOR 34
#endif

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1
#define DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 19
#endif

#ifndef DWMWCP_DEFAULT
#define DWMWCP_DEFAULT 0
#define DWMWCP_DONOTROUND 1
#define DWMWCP_ROUND 2
#define DWMWCP_ROUNDSMALL 3
#endif
#endif

FramelessWindow::FramelessWindow(QWidget *parent)
    : QWidget(parent)
    , m_titleBar(nullptr)
    , m_contentLabel(nullptr)
    , m_layout(nullptr)
{
    initWindow();
    initLayout();
}

FramelessWindow::~FramelessWindow() = default;

void FramelessWindow::initWindow()
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint);
    setMinimumSize(480, 320);
    resize(960, 640);

    setObjectName("FramelessWindow");
    setStyleSheet(R"(
        #FramelessWindow {
            background-color: #f3f4f6;
            border: 1px solid #b9c0ca;
        }
        TitleBar {
            background-color: #ffffff;
            border-bottom: 1px solid #c7ced8;
        }
        #TitleBarLabel {
            color: #222;
            font-weight: 600;
        }
        #TitleBarMinimizeButton,
        #TitleBarMaximizeButton,
        #TitleBarCloseButton {
            border: none;
            background: transparent;
            color: #222;
            font-size: 14px;
        }
        #TitleBarMinimizeButton:hover,
        #TitleBarMaximizeButton:hover {
            background: #e8e8e8;
        }
        #TitleBarCloseButton:hover {
            background: #e81123;
            color: white;
        }
    )");
}

void FramelessWindow::initLayout()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setSpacing(0);
    m_layout->setContentsMargins(0, 0, 0, 0);

    m_titleBar = new TitleBar(this);
    m_contentLabel = new QLabel("Content Area\nDrag title bar to move\nDrag edges to resize", this);
    m_contentLabel->setAlignment(Qt::AlignCenter);
    m_contentLabel->setStyleSheet("font-size: 18px; color: #444;");

    m_layout->addWidget(m_titleBar);
    m_layout->addWidget(m_contentLabel, 1);

    initMouseTracking();

    connect(m_titleBar, &TitleBar::minimizeRequested, this, &FramelessWindow::showMinimized);
    connect(m_titleBar, &TitleBar::maximizeRestoreRequested, this, &FramelessWindow::toggleMaximizeRestore);
    connect(m_titleBar, &TitleBar::closeRequested, this, &FramelessWindow::close);
    connect(m_titleBar, &TitleBar::systemMoveRequested, this, [this](const QPoint &globalPos) {
        if (isMaximized() || windowHandle() == nullptr) {
            startSystemMove();
            return;
        }

        const QPoint localPos = mapFromGlobal(globalPos);
        const Qt::Edges edges = edgesForLocalPos(localPos);
        if (edges != Qt::Edges()) {
            windowHandle()->startSystemResize(edges);
            return;
        }

        startSystemMove();
    });
    connect(m_titleBar, &TitleBar::systemMenuRequested, this, &FramelessWindow::showSystemMenu);

    updateMaximizeButtonState();
}

#ifdef Q_OS_WIN
bool FramelessWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
    if (eventType != "windows_generic_MSG" && eventType != "windows_dispatcher_MSG") {
        return QWidget::nativeEvent(eventType, message, result);
    }

    const MSG *msg = static_cast<MSG *>(message);

    switch (msg->message) {
    case WM_NCHITTEST: {
        const QPoint globalPos(GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam));
        const int hit = hitTest(globalPos);
        if (hit != HTCLIENT) {
            *result = hit;
            return true;
        }
        break;
    }
    case WM_NCCALCSIZE:
        if (msg->wParam == TRUE) {
            *result = 0;
            return true;
        }
        break;
    case WM_GETMINMAXINFO: {
        auto *mmi = reinterpret_cast<MINMAXINFO *>(msg->lParam);
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
    case WM_NCRBUTTONUP: {
        if (msg->wParam == HTCAPTION || msg->wParam == HTSYSMENU) {
            const QPoint globalPos(GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam));
            showSystemMenu(globalPos);
            *result = 0;
            return true;
        }
        break;
    }
    default:
        break;
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
    QWidget::paintEvent(event);

    QPainter painter(this);
    QStyleOption opt;
    opt.initFrom(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);
}

void FramelessWindow::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange) {
        syncNativeWindowFrame();
        applyRoundedCorners();
        applyImmersiveDarkMode();
        applyBorderColor();
        updateMaximizeButtonState();
    } else if (event->type() == QEvent::ApplicationPaletteChange
               || event->type() == QEvent::PaletteChange) {
        applyImmersiveDarkMode();
        applyBorderColor();
    }
}

bool FramelessWindow::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched)

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

void FramelessWindow::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    ensureNativeResizeStyle();
    syncNativeWindowFrame();
    applyRoundedCorners();
    applyImmersiveDarkMode();
    applyBorderColor();
}

void FramelessWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && windowHandle() != nullptr && !isMaximized()) {
        const Qt::Edges edges = edgesForLocalPos(event->pos());
        if (edges != Qt::Edges()) {
            windowHandle()->startSystemResize(edges);
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
    const int border = resizeBorderThickness();
    const int corner = border + 2;
    const QRect globalRect = frameGeometry();

    const int left = globalRect.left();
    const int top = globalRect.top();
    const int right = globalRect.right();
    const int bottom = globalRect.bottom();
    const int x = globalPos.x();
    const int y = globalPos.y();

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
        if (edges == (Qt::TopEdge | Qt::LeftEdge))
            return HTTOPLEFT;
        if (edges == (Qt::TopEdge | Qt::RightEdge))
            return HTTOPRIGHT;
        if (edges == (Qt::BottomEdge | Qt::LeftEdge))
            return HTBOTTOMLEFT;
        if (edges == (Qt::BottomEdge | Qt::RightEdge))
            return HTBOTTOMRIGHT;
        if (edges == Qt::TopEdge)
            return HTTOP;
        if (edges == Qt::BottomEdge)
            return HTBOTTOM;
        if (edges == Qt::LeftEdge)
            return HTLEFT;
        if (edges == Qt::RightEdge)
            return HTRIGHT;
    }

    if (!globalRect.contains(globalPos)) {
        return HTCLIENT;
    }

    const QPoint localPos = mapFromGlobal(globalPos);

    if (m_titleBar != nullptr && m_titleBar->geometry().contains(localPos)) {
        QWidget *hovered = childAt(localPos);
        const auto *button = qobject_cast<QPushButton *>(hovered);
        const bool onButton = button != nullptr;

        if (onButton && button->objectName() == QStringLiteral("TitleBarMaximizeButton")) {
            return HTMAXBUTTON;
        }

        if (!onButton) {
            return HTCAPTION;
        }
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
    const WPARAM command = isMaximized() ? SC_RESTORE : SC_MAXIMIZE;
    PostMessage(hwnd, WM_SYSCOMMAND, command, 0);
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
    Q_UNUSED(globalPos)

    const HWND hwnd = reinterpret_cast<HWND>(winId());
    HMENU menu = GetSystemMenu(hwnd, FALSE);
    if (menu == nullptr) {
        return;
    }

    updateSystemMenuState(reinterpret_cast<void *>(menu));

    POINT cursorPos{};
    GetCursorPos(&cursorPos);

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

    setWindowTitle(isMaximized() ? QStringLiteral("Better Frameless Window (Maximized)") : QStringLiteral("Better Frameless Window"));
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

void FramelessWindow::ensureNativeResizeStyle()
{
#ifdef Q_OS_WIN
    const HWND hwnd = reinterpret_cast<HWND>(winId());
    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
    style &= ~static_cast<LONG_PTR>(WS_POPUP);
    style |= WS_OVERLAPPED | WS_THICKFRAME | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU;

    LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
    exStyle &= ~static_cast<LONG_PTR>(WS_EX_TOOLWINDOW);
    exStyle |= WS_EX_APPWINDOW;

    SetWindowLongPtr(hwnd, GWL_STYLE, style);
    SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);

    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
#endif
}

void FramelessWindow::syncNativeWindowFrame()
{
#ifdef Q_OS_WIN
    const HWND hwnd = reinterpret_cast<HWND>(winId());

    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
    style |= WS_OVERLAPPED | WS_THICKFRAME | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU;
    SetWindowLongPtr(hwnd, GWL_STYLE, style);

    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
#endif
}

void FramelessWindow::applyRoundedCorners()
{
#ifdef Q_OS_WIN
    const HWND hwnd = reinterpret_cast<HWND>(winId());

    if (hwnd == nullptr || isMaximized() || isMinimized()) {
        return;
    }

    const DWORD corner = DWMWCP_ROUND;
    DwmSetWindowAttribute(hwnd,
                          DWMWA_WINDOW_CORNER_PREFERENCE,
                          &corner,
                          sizeof(corner));
#endif
}

void FramelessWindow::applyImmersiveDarkMode()
{
#ifdef Q_OS_WIN
    const HWND hwnd = reinterpret_cast<HWND>(winId());
    if (hwnd == nullptr) {
        return;
    }

    const BOOL enabled = shouldUseDarkMode() ? TRUE : FALSE;
    HRESULT result = DwmSetWindowAttribute(hwnd,
                                           DWMWA_USE_IMMERSIVE_DARK_MODE,
                                           &enabled,
                                           sizeof(enabled));
    if (FAILED(result)) {
        DwmSetWindowAttribute(hwnd,
                              DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1,
                              &enabled,
                              sizeof(enabled));
    }
#endif
}

bool FramelessWindow::shouldUseDarkMode() const
{
    const QColor windowColor = palette().color(QPalette::Window);
    return windowColor.lightness() < 128;
}

void FramelessWindow::applyBorderColor()
{
#ifdef Q_OS_WIN
    const HWND hwnd = reinterpret_cast<HWND>(winId());
    if (hwnd == nullptr) {
        return;
    }

    const QColor border = preferredBorderColor();
    const COLORREF colorRef = RGB(border.red(), border.green(), border.blue());
    DwmSetWindowAttribute(hwnd,
                          DWMWA_BORDER_COLOR,
                          &colorRef,
                          sizeof(colorRef));
#endif
}

QColor FramelessWindow::preferredBorderColor() const
{
    const QPalette pal = palette();
    QColor border = pal.color(QPalette::Mid);

    if (shouldUseDarkMode()) {
        border = border.lighter(120);
    }

    if (!border.isValid()) {
        border = QColor(185, 192, 202);
    }

    return border;
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
