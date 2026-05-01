#include "winnativemessagerouter.h"

#include "diagnostics.h"
#include "framelesswindow.h"

#include <QGuiApplication>
#include <QScreen>
#include <QTimer>
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

bool NativeWindowsMessageRouter::handleNcHitTestMessage(FramelessWindow &window, qintptr lParam, qintptr *result)
{
    const QPoint globalPos(GET_X_LPARAM(static_cast<LPARAM>(lParam)), GET_Y_LPARAM(static_cast<LPARAM>(lParam)));
    const int hit = window.hitTest(globalPos);

    if (hit != HTCLIENT) {
        *result = hit;
        return true;
    }

    return false;
}

bool NativeWindowsMessageRouter::handleNcButtonMessage(FramelessWindow &window,
                                                       quint32 messageId,
                                                       quintptr wParam,
                                                       qintptr *result)
{
    if (wParam != HTMAXBUTTON && wParam != HTMINBUTTON && wParam != HTCLOSE) {
        return false;
    }

    if (messageId == WM_NCLBUTTONUP) {
        *result = 0;
        return true;
    }

    if (messageId == WM_NCLBUTTONDOWN) {
        if (wParam == HTMAXBUTTON) {
            window.toggleMaximizeRestore();
        } else if (wParam == HTMINBUTTON) {
            window.showMinimized();
        } else if (wParam == HTCLOSE) {
            window.close();
        }
    }

    *result = 0;
    return true;
}

bool NativeWindowsMessageRouter::handleGetMinMaxInfoMessage(FramelessWindow &window,
                                                            void *lParam,
                                                            qintptr *result)
{
    auto *mmi = reinterpret_cast<MINMAXINFO *>(lParam);
    if (mmi == nullptr) {
        Diagnostics::logWarning(QStringLiteral("WM_GETMINMAXINFO received null MINMAXINFO"));
        return false;
    }

    const QScreen *screen = window.windowHandle() ? window.windowHandle()->screen() : QGuiApplication::primaryScreen();
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

bool NativeWindowsMessageRouter::handleNcRightButtonUpMessage(FramelessWindow &window,
                                                              quintptr wParam,
                                                              qintptr lParam,
                                                              qintptr *result)
{
    if (wParam != HTCAPTION && wParam != HTSYSMENU) {
        return false;
    }

    const QPoint globalPos(GET_X_LPARAM(static_cast<LPARAM>(lParam)), GET_Y_LPARAM(static_cast<LPARAM>(lParam)));
    window.showSystemMenu(globalPos);
    *result = 0;
    return true;
}
#endif

bool NativeWindowsMessageRouter::handle(FramelessWindow &window, void *message, qintptr *result)
{
#ifdef Q_OS_WIN
    const MSG *msg = static_cast<MSG *>(message);
    if (msg == nullptr) {
        Diagnostics::logWarning(QStringLiteral("nativeEvent received null MSG pointer"));
        return false;
    }

    switch (msg->message) {
    case WM_NCHITTEST:
    {
        const HWND hwnd = reinterpret_cast<HWND>(window.winId());
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

        return NativeWindowsMessageRouter::handleNcHitTestMessage(window,
                                      static_cast<qintptr>(msg->lParam),
                                      result);
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
        return NativeWindowsMessageRouter::handleNcButtonMessage(window,
                                     msg->message,
                                     static_cast<quintptr>(msg->wParam),
                                     result);
    case WM_NCMOUSELEAVE:
    case WM_MOUSELEAVE:
    case WM_CAPTURECHANGED:
    case WM_KILLFOCUS:
        return false;
    case WM_SIZE:
    {
        const bool isMaximizedSize = msg->wParam == SIZE_MAXIMIZED;
        const bool isRestoredSize = msg->wParam == SIZE_RESTORED;
        const bool restoredFromMaximized = window.shouldStartRestoreTransitionFromSizeState(isMaximizedSize,
                                                                                            isRestoredSize);
        window.syncNativeWindowFrame();
        if (restoredFromMaximized) {
            window.beginBackdropTransitionGuard();
            window.scheduleStateVisualRefresh();
        } else if (isMaximizedSize) {
            window.scheduleStateVisualRefresh();
        } else if (isRestoredSize) {
            window.scheduleStateVisualRefresh();
        }

        if (restoredFromMaximized) {
            QTimer::singleShot(80, &window, [&window]() {
                if (!window.isVisible()) {
                    return;
                }

                // Early restore nudge: poke compositor and queue a refresh,
                // while final backdrop rebind is handled by guard release.
                window.scheduleStateVisualRefresh();
                window.forceNativeDwmRefresh();
            });
        }
        return false;
    }
    case WM_EXITSIZEMOVE:
        window.scheduleStateVisualRefresh();
        return false;
    case WM_WINDOWPOSCHANGED:
    {
        const auto *windowPos = reinterpret_cast<const WINDOWPOS *>(msg->lParam);
        if (windowPos == nullptr) {
            window.scheduleStateVisualRefresh();
            return false;
        }

        const bool sizeChanged = (windowPos->flags & SWP_NOSIZE) == 0;
        const bool posChanged = (windowPos->flags & SWP_NOMOVE) == 0;

        if (sizeChanged || posChanged) {
            window.scheduleStateVisualRefresh();
        }
        return false;
    }
    case WM_ACTIVATE:
        if (LOWORD(msg->wParam) != WA_INACTIVE) {
            window.scheduleStateVisualRefresh();
        }
        return false;
    case WM_NCCALCSIZE:
        if (msg->wParam == TRUE) {
            *result = 0;
            return true;
        }
        return false;
    case WM_GETMINMAXINFO:
        return NativeWindowsMessageRouter::handleGetMinMaxInfoMessage(window,
                                          reinterpret_cast<void *>(msg->lParam),
                                          result);
    case WM_NCRBUTTONUP:
        return NativeWindowsMessageRouter::handleNcRightButtonUpMessage(window,
                                        static_cast<quintptr>(msg->wParam),
                                        static_cast<qintptr>(msg->lParam),
                                        result);
    default:
        return false;
    }
#else
    Q_UNUSED(window)
    Q_UNUSED(message)
    Q_UNUSED(result)
    return false;
#endif
}
