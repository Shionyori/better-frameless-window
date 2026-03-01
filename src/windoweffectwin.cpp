#include "windoweffectwin.h"

#include "diagnostics.h"

#ifdef Q_OS_WIN
#include <qt_windows.h>
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

#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif

#ifndef DWMWA_MICA_EFFECT
#define DWMWA_MICA_EFFECT 1029
#endif

#ifndef DWMSBT_AUTO
#define DWMSBT_AUTO 0
#define DWMSBT_NONE 1
#define DWMSBT_MAINWINDOW 2
#define DWMSBT_TRANSIENTWINDOW 3
#define DWMSBT_TABBEDWINDOW 4
#endif

#ifndef DWMWCP_DEFAULT
#define DWMWCP_DEFAULT 0
#define DWMWCP_DONOTROUND 1
#define DWMWCP_ROUND 2
#define DWMWCP_ROUNDSMALL 3
#endif

namespace {
enum ACCENT_STATE {
    ACCENT_DISABLED = 0,
    ACCENT_ENABLE_GRADIENT = 1,
    ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
    ACCENT_ENABLE_BLURBEHIND = 3,
    ACCENT_ENABLE_ACRYLICBLURBEHIND = 4
};

struct ACCENT_POLICY {
    int AccentState;
    int AccentFlags;
    int GradientColor;
    int AnimationId;
};

enum WINDOWCOMPOSITIONATTRIB {
    WCA_ACCENT_POLICY = 19
};

struct WINDOWCOMPOSITIONATTRIBDATA {
    WINDOWCOMPOSITIONATTRIB Attrib;
    PVOID pvData;
    SIZE_T cbData;
};

using SetWindowCompositionAttributePtr = BOOL(WINAPI *)(HWND, WINDOWCOMPOSITIONATTRIBDATA *);

DWORD windowsBuildNumber()
{
    static DWORD cachedBuild = 0;
    static bool initialized = false;
    if (initialized) {
        return cachedBuild;
    }

    initialized = true;
    using RtlGetVersionPtr = LONG(WINAPI *)(PRTL_OSVERSIONINFOW);
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (ntdll == nullptr) {
        return cachedBuild;
    }

    const auto rtlGetVersion = reinterpret_cast<RtlGetVersionPtr>(GetProcAddress(ntdll, "RtlGetVersion"));
    if (rtlGetVersion == nullptr) {
        return cachedBuild;
    }

    RTL_OSVERSIONINFOW ver{};
    ver.dwOSVersionInfoSize = sizeof(ver);
    if (rtlGetVersion(&ver) == 0) {
        cachedBuild = ver.dwBuildNumber;
    }

    return cachedBuild;
}

SetWindowCompositionAttributePtr getSetWindowCompositionAttribute()
{
    static SetWindowCompositionAttributePtr fn = nullptr;
    static bool initialized = false;
    if (initialized) {
        return fn;
    }

    initialized = true;
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32 == nullptr) {
        return nullptr;
    }

    fn = reinterpret_cast<SetWindowCompositionAttributePtr>(GetProcAddress(user32, "SetWindowCompositionAttribute"));
    return fn;
}

void applyAcrylicAccent(HWND hwnd, bool enable, bool useDarkMode)
{
    const auto setWindowCompositionAttribute = getSetWindowCompositionAttribute();
    if (setWindowCompositionAttribute == nullptr) {
        return;
    }

    ACCENT_POLICY accent{};
    if (enable) {
        accent.AccentState = ACCENT_ENABLE_ACRYLICBLURBEHIND;
        accent.AccentFlags = 2;
        const int alpha = 0xCC;
        const int red = useDarkMode ? 0x20 : 0xF3;
        const int green = useDarkMode ? 0x20 : 0xF4;
        const int blue = useDarkMode ? 0x20 : 0xF6;
        accent.GradientColor = (alpha << 24) | (blue << 16) | (green << 8) | red;
    } else {
        accent.AccentState = ACCENT_DISABLED;
    }

    WINDOWCOMPOSITIONATTRIBDATA data{};
    data.Attrib = WCA_ACCENT_POLICY;
    data.pvData = &accent;
    data.cbData = sizeof(accent);
    setWindowCompositionAttribute(hwnd, &data);
}
}
#endif

void WindowEffectWin::applyVisualEffects(void *hwnd, const VisualEffectOptions &options) const
{
#ifdef Q_OS_WIN
    applyShadow(hwnd, options.shadowEnabled, options.maximized, options.minimized);
    applyRoundedCorners(hwnd, options.roundedCornersEnabled, options.maximized, options.minimized);
    applyImmersiveDarkMode(hwnd, options.immersiveDarkModeEnabled, options.useDarkMode);
    applyBackdropEffects(hwnd,
                         options.backdropEnabled,
                         options.useDarkMode,
                         options.maximized,
                         options.minimized,
                         options.aeroBlurEnabled);
    applyBorderColor(hwnd, options.borderColor);
#else
    (void) hwnd;
    (void) options;
#endif
}

void WindowEffectWin::applyShadow(void *hwnd, bool enabled, bool maximized, bool minimized) const
{
#ifdef Q_OS_WIN
    const HWND win = static_cast<HWND>(hwnd);
    if (win == nullptr) {
        Diagnostics::logWarning(QStringLiteral("applyShadow: null HWND"));
        return;
    }

    BOOL compositionEnabled = FALSE;
    if (FAILED(DwmIsCompositionEnabled(&compositionEnabled)) || !compositionEnabled) {
        Diagnostics::logWarning(QStringLiteral("applyShadow skipped: DWM composition disabled or unavailable"));
        return;
    }

    const bool enableShadow = enabled && !maximized && !minimized;
    const DWMNCRENDERINGPOLICY policy = enableShadow ? DWMNCRP_ENABLED : DWMNCRP_DISABLED;
    DwmSetWindowAttribute(win,
                          DWMWA_NCRENDERING_POLICY,
                          &policy,
                          sizeof(policy));

    const MARGINS margins = enableShadow ? MARGINS{1, 1, 1, 1} : MARGINS{0, 0, 0, 0};
    DwmExtendFrameIntoClientArea(win, &margins);
#else
    (void) hwnd;
    (void) enabled;
    (void) maximized;
    (void) minimized;
#endif
}

void WindowEffectWin::applyRoundedCorners(void *hwnd, bool enabled, bool maximized, bool minimized) const
{
#ifdef Q_OS_WIN
    const HWND win = static_cast<HWND>(hwnd);
    if (win == nullptr) {
        Diagnostics::logWarning(QStringLiteral("applyRoundedCorners: null HWND"));
        return;
    }

    const bool enableRoundedCorners = enabled && !maximized && !minimized;
    const DWORD corner = enableRoundedCorners ? DWMWCP_ROUND : DWMWCP_DONOTROUND;
    DwmSetWindowAttribute(win,
                          DWMWA_WINDOW_CORNER_PREFERENCE,
                          &corner,
                          sizeof(corner));
#else
    (void) hwnd;
    (void) enabled;
    (void) maximized;
    (void) minimized;
#endif
}

void WindowEffectWin::applyImmersiveDarkMode(void *hwnd, bool enabled, bool useDarkMode) const
{
#ifdef Q_OS_WIN
    const HWND win = static_cast<HWND>(hwnd);
    if (win == nullptr) {
        Diagnostics::logWarning(QStringLiteral("applyImmersiveDarkMode: null HWND"));
        return;
    }

    const BOOL darkEnabled = (enabled && useDarkMode) ? TRUE : FALSE;
    HRESULT result = DwmSetWindowAttribute(win,
                                           DWMWA_USE_IMMERSIVE_DARK_MODE,
                                           &darkEnabled,
                                           sizeof(darkEnabled));
    if (FAILED(result)) {
        Diagnostics::logWarning(QStringLiteral("applyImmersiveDarkMode: DWMWA_USE_IMMERSIVE_DARK_MODE failed, fallback to legacy attribute"));
        DwmSetWindowAttribute(win,
                              DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1,
                              &darkEnabled,
                              sizeof(darkEnabled));
    }
#else
    (void) hwnd;
    (void) enabled;
    (void) useDarkMode;
#endif
}

WindowEffectWin::BackdropMode WindowEffectWin::selectBackdropMode(void *hwnd,
                                                                  bool enabled,
                                                                  bool maximized,
                                                                  bool minimized,
                                                                  bool aeroBlurEnabled) const
{
#ifdef Q_OS_WIN
    if (!enabled || hwnd == nullptr || minimized || maximized) {
        return BackdropMode::None;
    }

    const DWORD build = windowsBuildNumber();
    if (build >= 22621) {
        return BackdropMode::MicaSystem;
    }

    if (build >= 22000) {
        return BackdropMode::MicaLegacy;
    }

    if (build >= 17763 && getSetWindowCompositionAttribute() != nullptr) {
        return BackdropMode::Acrylic;
    }

    if (aeroBlurEnabled && build >= 7600 && build < 9200) {
        return BackdropMode::Aero;
    }
#else
    (void) hwnd;
    (void) enabled;
    (void) maximized;
    (void) minimized;
    (void) aeroBlurEnabled;
#endif

    return BackdropMode::None;
}

void WindowEffectWin::applyBackdropEffects(void *hwnd,
                                           bool enabled,
                                           bool useDarkMode,
                                           bool maximized,
                                           bool minimized,
                                           bool aeroBlurEnabled) const
{
#ifdef Q_OS_WIN
    const HWND win = static_cast<HWND>(hwnd);
    if (win == nullptr) {
        Diagnostics::logWarning(QStringLiteral("applyBackdropEffects: null HWND"));
        return;
    }

    BackdropMode mode = selectBackdropMode(hwnd, enabled, maximized, minimized, aeroBlurEnabled);
    if (mode == BackdropMode::MicaSystem) {
        const DWORD backdrop = DWMSBT_MAINWINDOW;
        const HRESULT hr = DwmSetWindowAttribute(win,
                                                 DWMWA_SYSTEMBACKDROP_TYPE,
                                                 &backdrop,
                                                 sizeof(backdrop));
        if (SUCCEEDED(hr)) {
            const BOOL disableLegacyMica = FALSE;
            DwmSetWindowAttribute(win,
                                  DWMWA_MICA_EFFECT,
                                  &disableLegacyMica,
                                  sizeof(disableLegacyMica));
            applyAcrylicAccent(win, false, useDarkMode);
            applyAeroBlur(hwnd, false);
            return;
        }

        Diagnostics::logWarning(QStringLiteral("applyBackdropEffects: DWMWA_SYSTEMBACKDROP_TYPE failed, fallback to legacy Mica"));
        mode = BackdropMode::MicaLegacy;
    }

    const DWORD noneBackdrop = DWMSBT_NONE;
    DwmSetWindowAttribute(win,
                          DWMWA_SYSTEMBACKDROP_TYPE,
                          &noneBackdrop,
                          sizeof(noneBackdrop));

    const BOOL enableLegacyMica = (mode == BackdropMode::MicaLegacy) ? TRUE : FALSE;
    DwmSetWindowAttribute(win,
                          DWMWA_MICA_EFFECT,
                          &enableLegacyMica,
                          sizeof(enableLegacyMica));

    if (mode == BackdropMode::Acrylic) {
        applyAcrylicAccent(win, true, useDarkMode);
    } else {
        applyAcrylicAccent(win, false, useDarkMode);
    }

    applyAeroBlur(hwnd, mode == BackdropMode::Aero);
#else
    (void) hwnd;
    (void) enabled;
    (void) useDarkMode;
    (void) maximized;
    (void) minimized;
    (void) aeroBlurEnabled;
#endif
}

void WindowEffectWin::applyAeroBlur(void *hwnd, bool enabled) const
{
#ifdef Q_OS_WIN
    const HWND win = static_cast<HWND>(hwnd);
    if (win == nullptr) {
        Diagnostics::logWarning(QStringLiteral("applyAeroBlur: null HWND"));
        return;
    }

    BOOL compositionEnabled = FALSE;
    if (FAILED(DwmIsCompositionEnabled(&compositionEnabled)) || !compositionEnabled) {
        Diagnostics::logWarning(QStringLiteral("applyAeroBlur skipped: DWM composition disabled or unavailable"));
        return;
    }

    DWM_BLURBEHIND blur{};
    blur.dwFlags = DWM_BB_ENABLE;
    blur.fEnable = enabled ? TRUE : FALSE;
    DwmEnableBlurBehindWindow(win, &blur);
#else
    (void) hwnd;
    (void) enabled;
#endif
}

void WindowEffectWin::applyBorderColor(void *hwnd, const QColor &borderColor) const
{
#ifdef Q_OS_WIN
    const HWND win = static_cast<HWND>(hwnd);
    if (win == nullptr || !borderColor.isValid()) {
        return;
    }

    const COLORREF colorRef = RGB(borderColor.red(), borderColor.green(), borderColor.blue());
    DwmSetWindowAttribute(win,
                          DWMWA_BORDER_COLOR,
                          &colorRef,
                          sizeof(colorRef));
#else
    (void) hwnd;
    (void) borderColor;
#endif
}
