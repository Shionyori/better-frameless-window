#pragma once

#include <QtGlobal>

class FramelessWindow;

class NativeMessageRouter
{
public:
    static bool handle(FramelessWindow &window, void *message, qintptr *result);

private:
#ifdef Q_OS_WIN
    static bool handleNcHitTestMessage(FramelessWindow &window, qintptr lParam, qintptr *result);
    static bool handleNcButtonMessage(FramelessWindow &window, quint32 messageId, quintptr wParam, qintptr *result);
    static bool handleGetMinMaxInfoMessage(FramelessWindow &window, void *lParam, qintptr *result);
    static bool handleNcRightButtonUpMessage(FramelessWindow &window, quintptr wParam, qintptr lParam, qintptr *result);
#endif
};