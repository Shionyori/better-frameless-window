#include "windoweffectwin.h"

#include "diagnostics.h"
#include "winutils.h"

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
QString backdropModeToString(WindowEffectWin::BackdropMode mode)
{
    switch (mode) {
    case WindowEffectWin::BackdropMode::None:
        return QStringLiteral("None");
    case WindowEffectWin::BackdropMode::MicaSystem:
        return QStringLiteral("MicaSystem");
    case WindowEffectWin::BackdropMode::MicaLegacy:
        return QStringLiteral("MicaLegacy");
    case WindowEffectWin::BackdropMode::Acrylic:
        return QStringLiteral("Acrylic");
    case WindowEffectWin::BackdropMode::Aero:
        return QStringLiteral("Aero");
    }

    return QStringLiteral("Unknown");
}

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
        const int alpha = useDarkMode ? 0x74 : 0x88;
        const int red = useDarkMode ? 0x18 : 0xF0;
        const int green = useDarkMode ? 0x18 : 0xF0;
        const int blue = useDarkMode ? 0x18 : 0xF0;
        accent.GradientColor = (alpha << 24) | (blue << 16) | (green << 8) | red;
    } else {
        accent.AccentState = ACCENT_DISABLED;
    }

    WINDOWCOMPOSITIONATTRIBDATA data{};
    data.Attrib = WCA_ACCENT_POLICY;
    data.pvData = &accent;
    data.cbData = sizeof(accent);
    const BOOL ok = setWindowCompositionAttribute(hwnd, &data);
    if (ok && enable) {
        SetWindowPos(hwnd,
                     nullptr,
                     0,
                     0,
                     0,
                     0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }
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
                         options.aeroBlurEnabled,
                         options.backdropPreference);
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
    static bool ncRenderingPolicyValid = true;
    if (ncRenderingPolicyValid) {
        const HRESULT policyHr = DwmSetWindowAttribute(win,
                                                       DWMWA_NCRENDERING_POLICY,
                                                       &policy,
                                                       sizeof(policy));
        if (FAILED(policyHr)) {
            ncRenderingPolicyValid = false;
            Diagnostics::logWarning(QStringLiteral("applyShadow: DWMWA_NCRENDERING_POLICY failed (hr=0x%1), disable subsequent calls")
                                        .arg(QString::number(static_cast<qulonglong>(policyHr), 16)));
        }
    }

    const MARGINS margins = enableShadow ? MARGINS{1, 1, 1, 1} : MARGINS{0, 0, 0, 0};
    static bool extendFrameValid = true;
    if (extendFrameValid) {
        const HRESULT marginsHr = DwmExtendFrameIntoClientArea(win, &margins);
        if (FAILED(marginsHr)) {
            extendFrameValid = false;
            Diagnostics::logWarning(QStringLiteral("applyShadow: DwmExtendFrameIntoClientArea failed (hr=0x%1), disable subsequent calls")
                                        .arg(QString::number(static_cast<qulonglong>(marginsHr), 16)));
        }
    }
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

    static bool roundedCornersAttributeValid = true;
    if (!roundedCornersAttributeValid) {
        return;
    }

    const bool enableRoundedCorners = enabled && !maximized && !minimized;
    const DWORD corner = enableRoundedCorners ? DWMWCP_ROUND : DWMWCP_DONOTROUND;
    const HRESULT hr = DwmSetWindowAttribute(win,
                                             DWMWA_WINDOW_CORNER_PREFERENCE,
                                             &corner,
                                             sizeof(corner));
    if (FAILED(hr)) {
        roundedCornersAttributeValid = false;
        Diagnostics::logWarning(QStringLiteral("applyRoundedCorners: DWMWA_WINDOW_CORNER_PREFERENCE failed (hr=0x%1), disable subsequent calls")
                                    .arg(QString::number(static_cast<qulonglong>(hr), 16)));
    }
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

    static bool darkMode20Valid = true;
    static bool darkMode19Valid = true;
    if (!darkMode20Valid && !darkMode19Valid) {
        return;
    }

    const BOOL darkEnabled = (enabled && useDarkMode) ? TRUE : FALSE;
    if (darkMode20Valid) {
        const HRESULT result = DwmSetWindowAttribute(win,
                                                     DWMWA_USE_IMMERSIVE_DARK_MODE,
                                                     &darkEnabled,
                                                     sizeof(darkEnabled));
        if (SUCCEEDED(result)) {
            return;
        }

        darkMode20Valid = false;
        Diagnostics::logWarning(QStringLiteral("applyImmersiveDarkMode: DWMWA_USE_IMMERSIVE_DARK_MODE failed (hr=0x%1), fallback to legacy attribute")
                                    .arg(QString::number(static_cast<qulonglong>(result), 16)));
    }

    if (darkMode19Valid) {
        const HRESULT legacyResult = DwmSetWindowAttribute(win,
                                                            DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1,
                                                            &darkEnabled,
                                                            sizeof(darkEnabled));
        if (FAILED(legacyResult)) {
            darkMode19Valid = false;
            Diagnostics::logWarning(QStringLiteral("applyImmersiveDarkMode: legacy dark mode attribute failed (hr=0x%1), disable subsequent calls")
                                        .arg(QString::number(static_cast<qulonglong>(legacyResult), 16)));
        }
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
                                                                  bool aeroBlurEnabled,
                                                                  BackdropPreference backdropPreference) const
{
#ifdef Q_OS_WIN
    Q_UNUSED(maximized)

    if (!enabled || hwnd == nullptr || minimized) {
        return BackdropMode::None;
    }

    const WinUtils::WindowsCapabilities caps = WinUtils::detectWindowsCapabilities();
    if (backdropPreference != BackdropPreference::Auto) {
        switch (backdropPreference) {
        case BackdropPreference::None:
            return BackdropMode::None;
        case BackdropPreference::MicaSystem:
            if (caps.supportsSystemBackdrop) {
                return BackdropMode::MicaSystem;
            }
            Diagnostics::logWarning(QStringLiteral("selectBackdropMode: requested MicaSystem not supported, fallback to Auto chain"));
            break;
        case BackdropPreference::MicaLegacy:
            if (caps.supportsLegacyMica) {
                return BackdropMode::MicaLegacy;
            }
            Diagnostics::logWarning(QStringLiteral("selectBackdropMode: requested MicaLegacy not supported, fallback to Auto chain"));
            break;
        case BackdropPreference::Acrylic:
            if (caps.supportsAcrylic && getSetWindowCompositionAttribute() != nullptr) {
                return BackdropMode::Acrylic;
            }
            Diagnostics::logWarning(QStringLiteral("selectBackdropMode: requested Acrylic not supported, fallback to Auto chain"));
            break;
        case BackdropPreference::Aero:
            if (aeroBlurEnabled && caps.supportsAeroBlur) {
                return BackdropMode::Aero;
            }
            Diagnostics::logWarning(QStringLiteral("selectBackdropMode: requested Aero not supported, fallback to Auto chain"));
            break;
        case BackdropPreference::Auto:
            break;
        }
    }

    if (caps.supportsSystemBackdrop) {
        return BackdropMode::MicaSystem;
    }

    if (caps.supportsLegacyMica) {
        return BackdropMode::MicaLegacy;
    }

    if (caps.supportsAcrylic && getSetWindowCompositionAttribute() != nullptr) {
        return BackdropMode::Acrylic;
    }

    if (aeroBlurEnabled && caps.supportsAeroBlur) {
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
                                           bool aeroBlurEnabled,
                                           BackdropPreference backdropPreference) const
{
#ifdef Q_OS_WIN
    const HWND win = static_cast<HWND>(hwnd);
    if (win == nullptr) {
        Diagnostics::logWarning(QStringLiteral("applyBackdropEffects: null HWND"));
        return;
    }

    Q_UNUSED(maximized)

    if (minimized) {
        applyAcrylicAccent(win, false, useDarkMode);
        applyAeroBlur(hwnd, false);
        return;
    }

    const WinUtils::WindowsCapabilities caps = WinUtils::detectWindowsCapabilities();
    const bool acrylicAvailable = caps.supportsAcrylic && getSetWindowCompositionAttribute() != nullptr;
    const bool wantsBackdrop = enabled && !minimized && backdropPreference != BackdropPreference::None;

    if (!wantsBackdrop) {
        applyAcrylicAccent(win, false, useDarkMode);
        applyAeroBlur(hwnd, false);
        return;
    }

    if (backdropPreference == BackdropPreference::Aero && aeroBlurEnabled && caps.supportsAeroBlur) {
        applyAcrylicAccent(win, false, useDarkMode);
        applyAeroBlur(hwnd, true);
        return;
    }

    if (acrylicAvailable) {
        applyAcrylicAccent(win, true, useDarkMode);
        applyAeroBlur(hwnd, false);
        // Extra RedrawWindow to ensure DWM immediately resolves the Acrylic layer
        RedrawWindow(win, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME | RDW_ALLCHILDREN);
        return;
    }

    applyAcrylicAccent(win, false, useDarkMode);
    applyAeroBlur(hwnd, false);
#else
    (void) hwnd;
    (void) enabled;
    (void) useDarkMode;
    (void) maximized;
    (void) minimized;
    (void) aeroBlurEnabled;
    (void) backdropPreference;
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

    static bool borderColorAttributeValid = true;
    if (!borderColorAttributeValid) {
        return;
    }

    const WinUtils::WindowsCapabilities caps = WinUtils::detectWindowsCapabilities();
    if (!caps.supportsLegacyMica && !caps.supportsSystemBackdrop) {
        return;
    }

    const COLORREF colorRef = RGB(borderColor.red(), borderColor.green(), borderColor.blue());
    const HRESULT hr = DwmSetWindowAttribute(win,
                                             DWMWA_BORDER_COLOR,
                                             &colorRef,
                                             sizeof(colorRef));
    if (FAILED(hr)) {
        borderColorAttributeValid = false;
        Diagnostics::logWarning(QStringLiteral("applyBorderColor: DWMWA_BORDER_COLOR failed (hr=0x%1), disable subsequent calls")
                                    .arg(QString::number(static_cast<qulonglong>(hr), 16)));
    }
#else
    (void) hwnd;
    (void) borderColor;
#endif
}
