#include "windoweffect.h"

#include "diagnostics.h"
#include "utils.h"

#ifdef Q_OS_WIN
#include <qt_windows.h>
#include <dwmapi.h>

#include <unordered_map>

#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif

#ifndef DWMWA_BORDER_COLOR
#define DWMWA_BORDER_COLOR 34
#endif

#ifndef DWMWA_USE_SYSTEM_DARK_MODE
#define DWMWA_USE_SYSTEM_DARK_MODE 20
#endif

#ifndef DWMWA_USE_SYSTEM_DARK_MODE_BEFORE_20H1
#define DWMWA_USE_SYSTEM_DARK_MODE_BEFORE_20H1 19
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
        const int alpha = useDarkMode ? 0x78 : 0x90;
        const int red = useDarkMode ? 0x1B : 0xF3;
        const int green = useDarkMode ? 0x1B : 0xF3;
        const int blue = useDarkMode ? 0x1B : 0xF3;
        accent.GradientColor = (alpha << 24) | (blue << 16) | (green << 8) | red;
    } else {
        accent.AccentState = ACCENT_DISABLED;
    }

    WINDOWCOMPOSITIONATTRIBDATA data{};
    data.Attrib = WCA_ACCENT_POLICY;
    data.pvData = &accent;
    data.cbData = sizeof(accent);
    const BOOL ok = setWindowCompositionAttribute(hwnd, &data);
    if (ok) {
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
                     RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
    }
}
}
#endif

void WindowEffect::applyVisualEffects(void *hwnd, const VisualEffectOptions &options) const
{
#ifdef Q_OS_WIN
    applyShadow(hwnd, options.shadowEnabled, options.maximized, options.minimized);
    applyRoundedCorners(hwnd, options.roundedCornersEnabled, options.maximized, options.minimized);
    applySystemDarkMode(hwnd, options.systemDarkModeEnabled, options.useDarkMode);
    applySystemBackdropEffects(hwnd,
                         options.systemBackdropEnabled,
                         options.useDarkMode,
                         options.maximized,
                         options.minimized,
                         options.systemBackdropPreference);
    applyBorderColor(hwnd, options.borderColor);
#else
    (void) hwnd;
    (void) options;
#endif
}

void WindowEffect::applyShadow(void *hwnd, bool enabled, bool maximized, bool minimized) const
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

void WindowEffect::applyRoundedCorners(void *hwnd, bool enabled, bool maximized, bool minimized) const
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

void WindowEffect::applySystemDarkMode(void *hwnd, bool enabled, bool useDarkMode) const
{
#ifdef Q_OS_WIN
    const HWND win = static_cast<HWND>(hwnd);
    if (win == nullptr) {
        Diagnostics::logWarning(QStringLiteral("applySystemDarkMode: null HWND"));
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
                                                     DWMWA_USE_SYSTEM_DARK_MODE,
                                                     &darkEnabled,
                                                     sizeof(darkEnabled));
        if (SUCCEEDED(result)) {
            return;
        }

        darkMode20Valid = false;
        Diagnostics::logWarning(QStringLiteral("applySystemDarkMode: DWMWA_USE_SYSTEM_DARK_MODE failed (hr=0x%1), fallback to legacy attribute")
                                    .arg(QString::number(static_cast<qulonglong>(result), 16)));
    }

    if (darkMode19Valid) {
        const HRESULT legacyResult = DwmSetWindowAttribute(win,
                                                            DWMWA_USE_SYSTEM_DARK_MODE_BEFORE_20H1,
                                                            &darkEnabled,
                                                            sizeof(darkEnabled));
        if (FAILED(legacyResult)) {
            darkMode19Valid = false;
            Diagnostics::logWarning(QStringLiteral("applySystemDarkMode: legacy dark mode attribute failed (hr=0x%1), disable subsequent calls")
                                        .arg(QString::number(static_cast<qulonglong>(legacyResult), 16)));
        }
    }
#else
    (void) hwnd;
    (void) enabled;
    (void) useDarkMode;
#endif
}

WindowEffect::SystemBackdropMode WindowEffect::selectSystemBackdropMode(bool enabled,
                                                                  bool minimized,
                                                                  SystemBackdropPreference systemBackdropPreference) const
{
#ifdef Q_OS_WIN
    if (!enabled || minimized) {
        return SystemBackdropMode::None;
    }

    const Utils::WindowsCapabilities caps = Utils::detectWindowsCapabilities();
    const bool allowAcrylic = true;
    if (systemBackdropPreference != SystemBackdropPreference::Auto) {
        switch (systemBackdropPreference) {
        case SystemBackdropPreference::None:
            return SystemBackdropMode::None;
        case SystemBackdropPreference::Mica:
            if (caps.supportsSystemSystemBackdrop) {
                return SystemBackdropMode::Mica;
            }
            Diagnostics::logWarning(QStringLiteral("selectSystemBackdropMode: requested Mica not supported, fallback to Auto chain"));
            break;
        case SystemBackdropPreference::MicaLegacy:
            if (caps.supportsLegacyMica) {
                return SystemBackdropMode::MicaLegacy;
            }
            Diagnostics::logWarning(QStringLiteral("selectSystemBackdropMode: requested MicaLegacy not supported, fallback to Auto chain"));
            break;
        case SystemBackdropPreference::Acrylic:
            if (allowAcrylic && caps.supportsAcrylic && getSetWindowCompositionAttribute() != nullptr) {
                return SystemBackdropMode::Acrylic;
            }
            Diagnostics::logWarning(QStringLiteral("selectSystemBackdropMode: requested Acrylic not supported now, fallback to Mica/Auto chain"));
            break;
        case SystemBackdropPreference::Auto:
            break;
        }
    }

    if (caps.supportsSystemSystemBackdrop) {
        return SystemBackdropMode::Mica;
    }

    if (caps.supportsLegacyMica) {
        return SystemBackdropMode::MicaLegacy;
    }

    if (allowAcrylic && caps.supportsAcrylic && getSetWindowCompositionAttribute() != nullptr) {
        return SystemBackdropMode::Acrylic;
    }
#else
    (void) enabled;
    (void) minimized;
    (void) systemBackdropPreference;
#endif

    return SystemBackdropMode::None;
}

void WindowEffect::applySystemBackdropEffects(void *hwnd,
                                           bool enabled,
                                           bool useDarkMode,
                                           bool maximized,
                                           bool minimized,
                                           SystemBackdropPreference systemBackdropPreference) const
{
#ifdef Q_OS_WIN
    const HWND win = static_cast<HWND>(hwnd);
    if (win == nullptr) {
        Diagnostics::logWarning(QStringLiteral("applySystemBackdropEffects: null HWND"));
        return;
    }

    const Utils::WindowsCapabilities caps = Utils::detectWindowsCapabilities();
    SystemBackdropMode mode = selectSystemBackdropMode(enabled, minimized, systemBackdropPreference);

    // Per-instance caching: skip redundant DWM calls during frequent visual
    // refreshes (window move, focus change) when nothing actually changed.
    if (mode == m_lastAppliedMode && m_lastAppliedSuccessfully) {
        return;
    }

    static bool systemBackdropAttributeSupported = true;
    static bool legacyMicaAttributeSupported = true;

    auto setSystemBackdropType = [&](DWORD type, const QString &reason) -> HRESULT {
        if (!systemBackdropAttributeSupported) {
            return E_NOTIMPL;
        }

        const HRESULT hr = DwmSetWindowAttribute(win,
                                                 DWMWA_SYSTEMBACKDROP_TYPE,
                                                 &type,
                                                 sizeof(type));
        if (hr == E_INVALIDARG) {
            systemBackdropAttributeSupported = false;
            Diagnostics::logWarning(QStringLiteral("applySystemBackdropEffects: DWMWA_SYSTEMBACKDROP_TYPE unsupported (hr=0x%1, reason=%2)")
                                        .arg(QString::number(static_cast<qulonglong>(hr), 16), reason));
        } else if (FAILED(hr)) {
            Diagnostics::logWarning(QStringLiteral("applySystemBackdropEffects: DWMWA_SYSTEMBACKDROP_TYPE failed (hr=0x%1, reason=%2)")
                                        .arg(QString::number(static_cast<qulonglong>(hr), 16), reason));
        }
        return hr;
    };

    auto setLegacyMica = [&](BOOL value, const QString &reason) -> HRESULT {
        if (!legacyMicaAttributeSupported) {
            return E_NOTIMPL;
        }

        const HRESULT hr = DwmSetWindowAttribute(win,
                                                 DWMWA_MICA_EFFECT,
                                                 &value,
                                                 sizeof(value));
        if (hr == E_INVALIDARG) {
            legacyMicaAttributeSupported = false;
            Diagnostics::logWarning(QStringLiteral("applySystemBackdropEffects: DWMWA_MICA_EFFECT unsupported (hr=0x%1, reason=%2)")
                                        .arg(QString::number(static_cast<qulonglong>(hr), 16), reason));
        } else if (FAILED(hr)) {
            Diagnostics::logWarning(QStringLiteral("applySystemBackdropEffects: DWMWA_MICA_EFFECT failed (hr=0x%1, reason=%2)")
                                        .arg(QString::number(static_cast<qulonglong>(hr), 16), reason));
        }
        return hr;
    };

    // Phase 1: Clear the previously applied effect so that re-applying the
    // same backdrop type is recognized by DWM as a genuine state change.
    if (m_lastAppliedMode == SystemBackdropMode::Acrylic) {
        applyAcrylicAccent(win, false, useDarkMode);
    } else if (m_lastAppliedMode == SystemBackdropMode::Mica) {
        setSystemBackdropType(DWMSBT_NONE, QStringLiteral("clear-mica"));
    } else if (m_lastAppliedMode == SystemBackdropMode::MicaLegacy) {
        setLegacyMica(FALSE, QStringLiteral("clear-legacy-mica"));
    }

    m_lastAppliedMode = SystemBackdropMode::None;
    m_lastAppliedSuccessfully = false;

    // Phase 2: If no backdrop is desired, done after clearing.
    if (mode == SystemBackdropMode::None) {
        m_lastAppliedSuccessfully = true;
        return;
    }

    // Phase 3: Apply the selected mode with linear fallback.
    bool applied = false;

    if (mode == SystemBackdropMode::Mica) {
        const HRESULT hr = setSystemBackdropType(DWMSBT_MAINWINDOW, QStringLiteral("apply-mica"));
        if (SUCCEEDED(hr)) {
            applied = true;
        } else if (caps.supportsLegacyMica && legacyMicaAttributeSupported) {
            mode = SystemBackdropMode::MicaLegacy;
        } else if (caps.supportsAcrylic && getSetWindowCompositionAttribute() != nullptr) {
            mode = SystemBackdropMode::Acrylic;
        }
    }

    if (!applied && mode == SystemBackdropMode::MicaLegacy) {
        const HRESULT hr = setLegacyMica(TRUE, QStringLiteral("apply-legacy-mica"));
        if (SUCCEEDED(hr)) {
            applied = true;
        } else if (caps.supportsAcrylic && getSetWindowCompositionAttribute() != nullptr) {
            mode = SystemBackdropMode::Acrylic;
        }
    }

    if (!applied && mode == SystemBackdropMode::Acrylic) {
        applyAcrylicAccent(win, true, useDarkMode);
        applied = true;
    }

    if (applied) {
        m_lastAppliedMode = mode;
        m_lastAppliedSuccessfully = true;

        if (mode != SystemBackdropMode::Acrylic) {
            SetWindowPos(win,
                         nullptr,
                         0,
                         0,
                         0,
                         0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
            RedrawWindow(win,
                         nullptr,
                         nullptr,
                         RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
        }
    }

#else
    (void) hwnd;
    (void) useDarkMode;
    (void) maximized;
    (void) minimized;
    (void) systemBackdropPreference;
#endif
}

void WindowEffect::applyBorderColor(void *hwnd, const QColor &borderColor) const
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

    const Utils::WindowsCapabilities caps = Utils::detectWindowsCapabilities();
    if (!caps.supportsLegacyMica && !caps.supportsSystemSystemBackdrop) {
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