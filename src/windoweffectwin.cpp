#include "windoweffectwin.h"

void WindowEffectWin::applyShadow(void *hwnd, bool enabled) const
{
    // DWM-based shadow for Windows 10 and later, but it seem doesn't work
    // Temporarily put aside, maybe fix later

    // if (hwnd == nullptr) {
    //     return;
    // }

    // const HWND win = static_cast<HWND>(hwnd);

    // BOOL compositionEnabled = FALSE;
    // const HRESULT compositionHr = DwmIsCompositionEnabled(&compositionEnabled);

    // if (SUCCEEDED(compositionHr) && compositionEnabled) {
    //     DWMNCRENDERINGPOLICY policy = enabled ? DWMNCRP_ENABLED : DWMNCRP_DISABLED;
    //     DwmSetWindowAttribute(win, DWMWA_NCRENDERING_POLICY, &policy, sizeof(policy));

    //     const MARGINS margins = enabled ? MARGINS{-1, -1, -1, -1} : MARGINS{0, 0, 0, 0};
    //     DwmExtendFrameIntoClientArea(win, &margins);
    //     return;
    // }

    // LONG_PTR classStyle = GetClassLongPtr(win, GCL_STYLE);
    // if (enabled) {
    //     classStyle |= CS_DROPSHADOW;
    // } else {
    //     classStyle &= ~static_cast<LONG_PTR>(CS_DROPSHADOW);
    // }
    // SetClassLongPtr(win, GCL_STYLE, classStyle);

    // SetWindowPos(win, nullptr, 0, 0, 0, 0,
    //              SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

    (void) hwnd;
    (void) enabled;
}
