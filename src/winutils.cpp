#include "winutils.h"

#include "titlebar.h"

#include <QPushButton>
#include <QWidget>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

namespace {
constexpr auto kMaximizeButtonObjectName = "TitleBarMaximizeButton";
}

namespace WinUtils {

void setMaximizeButtonNativeHover(TitleBar *titleBar, bool hovered)
{
    if (titleBar != nullptr) {
        titleBar->setMaximizeButtonNativeHover(hovered);
    }
}

QPoint toLocalPos(const QPoint &globalPos, const QRect &nativeWindowRect, int logicalWidth, int logicalHeight)
{
    if (nativeWindowRect.isEmpty()) {
        return QPoint(0, 0);
    }

    const int nativeWidth = qMax(1, nativeWindowRect.width());
    const int nativeHeight = qMax(1, nativeWindowRect.height());

    return QPoint(
        qBound(0, ((globalPos.x() - nativeWindowRect.left()) * logicalWidth) / nativeWidth, logicalWidth - 1),
        qBound(0, ((globalPos.y() - nativeWindowRect.top()) * logicalHeight) / nativeHeight, logicalHeight - 1));
}

int hitFromEdges(Qt::Edges edges)
{
#ifdef Q_OS_WIN
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

    return HTCLIENT;
#else
    Q_UNUSED(edges)
    return 0;
#endif
}

void syncNativeWindowStyles(void *hwnd, bool includeExStyle)
{
#ifdef Q_OS_WIN
    const HWND win = static_cast<HWND>(hwnd);
    if (win == nullptr) {
        return;
    }

    LONG_PTR style = GetWindowLongPtr(win, GWL_STYLE);
    if (includeExStyle) {
        style &= ~static_cast<LONG_PTR>(WS_POPUP);
    }
    style |= WS_OVERLAPPED | WS_THICKFRAME | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU;
    SetWindowLongPtr(win, GWL_STYLE, style);

    if (includeExStyle) {
        LONG_PTR exStyle = GetWindowLongPtr(win, GWL_EXSTYLE);
        exStyle &= ~static_cast<LONG_PTR>(WS_EX_TOOLWINDOW);
        exStyle |= WS_EX_APPWINDOW;
        SetWindowLongPtr(win, GWL_EXSTYLE, exStyle);
    }

    SetWindowPos(win, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
#else
    Q_UNUSED(hwnd)
    Q_UNUSED(includeExStyle)
#endif
}

bool isOnMaximizeButton(const TitleBar *titleBar, const QWidget *hostWidget, const QPoint &localPos)
{
    if (titleBar == nullptr || hostWidget == nullptr || !titleBar->geometry().contains(localPos)) {
        return false;
    }

    const QPoint titleBarPos = titleBar->mapFrom(hostWidget, localPos);
    QWidget *hovered = titleBar->childAt(titleBarPos);
    const auto *button = qobject_cast<QPushButton *>(hovered);
    return button != nullptr && button->objectName() == QString::fromLatin1(kMaximizeButtonObjectName);
}

bool isOnTitleBarCaptionArea(const TitleBar *titleBar, const QWidget *hostWidget, const QPoint &localPos)
{
    if (titleBar == nullptr || hostWidget == nullptr || !titleBar->geometry().contains(localPos)) {
        return false;
    }

    QWidget *hovered = hostWidget->childAt(localPos);
    return qobject_cast<QPushButton *>(hovered) == nullptr;
}

} // namespace WinUtils
