#include "windoweffect.h"

#include "core/windowvisualstate.h"
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

#ifndef DWMWA_COLOR_DEFAULT
#define DWMWA_COLOR_DEFAULT 0xFFFFFFFF
#endif

#ifndef DWMWA_COLOR_NONE
#define DWMWA_COLOR_NONE 0xFFFFFFFE
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
    applyDarkModeHint(hwnd, options.darkMode);
    // The border is a frame attribute and has to be settled before the
    // backdrop is applied; see applyBorderColor() for the Acrylic exception.
    applyBorderColor(hwnd, options.backdrop, options.borderColor);
    applyBackdrop(hwnd, options.backdrop, options.darkMode);
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
    if (m_ncRenderingPolicyValid) {
        const HRESULT policyHr = DwmSetWindowAttribute(win,
                                                       DWMWA_NCRENDERING_POLICY,
                                                       &policy,
                                                       sizeof(policy));
        if (FAILED(policyHr)) {
            m_ncRenderingPolicyValid = false;
            Diagnostics::logWarning(QStringLiteral("applyShadow: DWMWA_NCRENDERING_POLICY failed (hr=0x%1), disable subsequent calls")
                                        .arg(QString::number(static_cast<qulonglong>(policyHr), 16)));
        }
    }

    const MARGINS margins = enableShadow ? MARGINS{1, 1, 1, 1} : MARGINS{0, 0, 0, 0};
    if (m_extendFrameValid) {
        const HRESULT marginsHr = DwmExtendFrameIntoClientArea(win, &margins);
        if (FAILED(marginsHr)) {
            m_extendFrameValid = false;
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

    if (!m_roundedCornersAttributeValid) {
        return;
    }

    const bool enableRoundedCorners = enabled && !maximized && !minimized;
    const DWORD corner = enableRoundedCorners ? DWMWCP_ROUND : DWMWCP_DONOTROUND;
    const HRESULT hr = DwmSetWindowAttribute(win,
                                             DWMWA_WINDOW_CORNER_PREFERENCE,
                                             &corner,
                                             sizeof(corner));
    if (FAILED(hr)) {
        m_roundedCornersAttributeValid = false;
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

void WindowEffect::applyDarkModeHint(void *hwnd, bool darkMode) const
{
#ifdef Q_OS_WIN
    const HWND win = static_cast<HWND>(hwnd);
    if (win == nullptr) {
        Diagnostics::logWarning(QStringLiteral("applyDarkModeHint: null HWND"));
        return;
    }

    if (!m_darkMode20Valid && !m_darkMode19Valid) {
        return;
    }

    // DWM reads this attribute both for its own dark chrome and as the light /
    // dark selector of Mica, so it has to follow the app theme. Leaving it at
    // light while the theme is dark paints the dark wash over light Mica, which
    // turns the window flat grey.
    const BOOL darkEnabled = darkMode ? TRUE : FALSE;
    if (m_darkMode20Valid) {
        const HRESULT result = DwmSetWindowAttribute(win,
                                                     DWMWA_USE_SYSTEM_DARK_MODE,
                                                     &darkEnabled,
                                                     sizeof(darkEnabled));
        if (SUCCEEDED(result)) {
            return;
        }

        m_darkMode20Valid = false;
        Diagnostics::logWarning(QStringLiteral("applyDarkModeHint: DWMWA_USE_SYSTEM_DARK_MODE failed (hr=0x%1), fallback to legacy attribute")
                                    .arg(QString::number(static_cast<qulonglong>(result), 16)));
    }

    if (m_darkMode19Valid) {
        const HRESULT legacyResult = DwmSetWindowAttribute(win,
                                                            DWMWA_USE_SYSTEM_DARK_MODE_BEFORE_20H1,
                                                            &darkEnabled,
                                                            sizeof(darkEnabled));
        if (FAILED(legacyResult)) {
            m_darkMode19Valid = false;
            Diagnostics::logWarning(QStringLiteral("applyDarkModeHint: legacy dark mode attribute failed (hr=0x%1), disable subsequent calls")
                                        .arg(QString::number(static_cast<qulonglong>(legacyResult), 16)));
        }
    }
#else
    (void) hwnd;
    (void) darkMode;
#endif
}

void WindowEffect::applyBackdrop(void *hwnd,
                                 BackdropKind kind,
                                 bool darkMode,
                                 bool forceRebind) const
{
#ifdef Q_OS_WIN
    const HWND win = static_cast<HWND>(hwnd);
    if (win == nullptr) {
        Diagnostics::logWarning(QStringLiteral("applyBackdrop: null HWND"));
        return;
    }

    // Per-instance caching: skip redundant DWM calls during frequent visual
    // refreshes (window move, focus change) when nothing actually changed.
    if (!forceRebind
        && WindowVisualState::isBackdropApplyRedundant(kind,
                                                      darkMode,
                                                      m_lastAppliedKind,
                                                      m_lastAppliedDarkMode,
                                                      m_lastAppliedSuccessfully)) {
        return;
    }

    auto setBackdropType = [&](DWORD type, const QString &reason) -> HRESULT {
        if (!m_systemBackdropAttributeSupported) {
            return E_NOTIMPL;
        }

        const HRESULT hr = DwmSetWindowAttribute(win,
                                                 DWMWA_SYSTEMBACKDROP_TYPE,
                                                 &type,
                                                 sizeof(type));
        if (hr == E_INVALIDARG) {
            m_systemBackdropAttributeSupported = false;
            Diagnostics::logWarning(QStringLiteral("applyBackdrop: DWMWA_SYSTEMBACKDROP_TYPE unsupported (hr=0x%1, reason=%2)")
                                        .arg(QString::number(static_cast<qulonglong>(hr), 16), reason));
        } else if (FAILED(hr)) {
            Diagnostics::logWarning(QStringLiteral("applyBackdrop: DWMWA_SYSTEMBACKDROP_TYPE failed (hr=0x%1, reason=%2)")
                                        .arg(QString::number(static_cast<qulonglong>(hr), 16), reason));
        }
        return hr;
    };

    // Phase 1: reset the previously applied backdrop when DWM stores it as a
    // window attribute (Mica), so that re-applying the same kind is recognized
    // as a genuine state change. Acrylic lives in the accent policy instead and
    // is idempotent, so it is re-applied in place: clearing it first drops the
    // blur for a frame, which is the flicker seen after a maximize/restore
    // transition.
    if (WindowVisualState::shouldClearBackdropBeforeApply(m_lastAppliedKind, kind)) {
        if (m_lastAppliedKind == BackdropKind::Mica) {
            setBackdropType(DWMSBT_NONE, QStringLiteral("clear-mica"));
        } else if (m_lastAppliedKind == BackdropKind::Acrylic) {
            applyAcrylicAccent(win, false, darkMode);
        }
    }

    m_lastAppliedKind = BackdropKind::None;
    m_lastAppliedSuccessfully = false;

    if (kind == BackdropKind::None) {
        m_lastAppliedSuccessfully = true;
        m_lastAppliedDarkMode = darkMode;
        return;
    }

    const Utils::WindowsCapabilities caps = Utils::detectWindowsCapabilities();

    // Phase 2: apply the resolved kind, falling back to Acrylic when Mica is
    // rejected by the running OS.
    bool applied = false;
    if (kind == BackdropKind::Mica) {
        const HRESULT hr = setBackdropType(DWMSBT_MAINWINDOW, QStringLiteral("apply-mica"));
        if (SUCCEEDED(hr)) {
            applied = true;
        } else if (caps.supportsAcrylic && getSetWindowCompositionAttribute() != nullptr) {
            kind = BackdropKind::Acrylic;
        }
    }

    if (!applied && kind == BackdropKind::Acrylic) {
        applyAcrylicAccent(win, true, darkMode);
        applied = true;
    }

    if (applied) {
        m_lastAppliedKind = kind;
        m_lastAppliedSuccessfully = true;
        m_lastAppliedDarkMode = darkMode;

        if (kind != BackdropKind::Acrylic) {
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
    (void) kind;
    (void) darkMode;
    (void) forceRebind;
#endif
}

void WindowEffect::applyBorderColor(void *hwnd, BackdropKind backdrop, const QColor &borderColor) const
{
#ifdef Q_OS_WIN
    const HWND win = static_cast<HWND>(hwnd);
    if (win == nullptr) {
        return;
    }

    if (!m_borderColorAttributeValid) {
        return;
    }

    const Utils::WindowsCapabilities caps = Utils::detectWindowsCapabilities();
    if (!caps.supportsRoundedCorners) {
        return;
    }

    // The legacy accent-policy Acrylic paints the whole window as frame, so a
    // visible border - a custom colour or the system default - is drawn across
    // the client area and the window becomes a solid rectangle. Suppress the
    // border entirely while it owns the surface.
    const bool acrylicOwnsSurface = backdrop == BackdropKind::Acrylic;
    const DWORD desired = acrylicOwnsSurface
                              ? DWMWA_COLOR_NONE
                              : borderColor.isValid()
                                    ? RGB(borderColor.red(), borderColor.green(), borderColor.blue())
                                    : DWMWA_COLOR_DEFAULT;

    if (m_borderAttributeApplied && m_borderAttributeValue == desired) {
        return;
    }

    DWORD value = desired;
    const HRESULT hr = DwmSetWindowAttribute(win,
                                             DWMWA_BORDER_COLOR,
                                             &value,
                                             sizeof(value));
    if (SUCCEEDED(hr)) {
        m_borderAttributeApplied = true;
        m_borderAttributeValue = desired;
        return;
    }

    if (hr == E_INVALIDARG) {
        m_borderColorAttributeValid = false;
    }
    Diagnostics::logWarning(QStringLiteral("applyBorderColor: DWMWA_BORDER_COLOR failed (hr=0x%1)")
                                .arg(QString::number(static_cast<qulonglong>(hr), 16)));
#else
    (void) hwnd;
    (void) backdrop;
    (void) borderColor;
#endif
}
