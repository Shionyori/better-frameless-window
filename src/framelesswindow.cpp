#include "framelesswindow.h"

#include "titlebar.h"

#include <QApplication>
#include <QEvent>
#include <QGuiApplication>
#include <QLabel>
#include <QLayout>
#include <QPainter>
#include <QPushButton>
#include <QScreen>
#include <QStyle>
#include <QStyleOption>
#include <QVBoxLayout>
#include <QWindow>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#include <windowsx.h>
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
            background-color: #f7f7f7;
            border: 1px solid #d9d9d9;
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

    const MSG *msg = static_cast<MSG *>(message);

    switch (msg->message) {
    case WM_NCHITTEST: {
        const auto globalPos = QPoint(GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam));
        const QPoint localPos = mapFromGlobal(globalPos);
        const int hit = hitTest(localPos);
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
        updateMaximizeButtonState();
    }
}

int FramelessWindow::hitTest(const QPoint &localPos) const
{
#ifdef Q_OS_WIN
    const int border = resizeBorderThickness();
    const QRect frameRect = rect();

    const bool left = localPos.x() >= frameRect.left() && localPos.x() < frameRect.left() + border;
    const bool right = localPos.x() <= frameRect.right() && localPos.x() > frameRect.right() - border;
    const bool top = localPos.y() >= frameRect.top() && localPos.y() < frameRect.top() + border;
    const bool bottom = localPos.y() <= frameRect.bottom() && localPos.y() > frameRect.bottom() - border;

    if (!isMaximized()) {
        if (top && left)
            return HTTOPLEFT;
        if (top && right)
            return HTTOPRIGHT;
        if (bottom && left)
            return HTBOTTOMLEFT;
        if (bottom && right)
            return HTBOTTOMRIGHT;
        if (left)
            return HTLEFT;
        if (right)
            return HTRIGHT;
        if (top)
            return HTTOP;
        if (bottom)
            return HTBOTTOM;
    }

    if (m_titleBar != nullptr && m_titleBar->geometry().contains(localPos)) {
        QWidget *child = childAt(localPos);
        if (qobject_cast<QPushButton *>(child) == nullptr) {
            return HTCAPTION;
        }
    }
#endif

    return HTCLIENT;
}

int FramelessWindow::resizeBorderThickness() const
{
    const qreal dpr = devicePixelRatioF();
    const int logical = 7;
    return qMax(5, qRound(logical * dpr));
}

void FramelessWindow::toggleMaximizeRestore()
{
    if (isMaximized()) {
        showNormal();
    } else {
        showMaximized();
    }
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
    HMENU menu = GetSystemMenu(hwnd, FALSE);
    if (menu == nullptr) {
        return;
    }

    UINT flags = TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN;
    const int command = TrackPopupMenu(menu, flags, globalPos.x(), globalPos.y(), 0, hwnd, nullptr);
    if (command != 0) {
        PostMessage(hwnd, WM_SYSCOMMAND, static_cast<WPARAM>(command), 0);
    }
#else
    Q_UNUSED(globalPos)
#endif
}

void FramelessWindow::updateMaximizeButtonState()
{
    if (m_titleBar == nullptr) {
        return;
    }

    setWindowTitle(isMaximized() ? QStringLiteral("Better Frameless Window (Maximized)") : QStringLiteral("Better Frameless Window"));
}
