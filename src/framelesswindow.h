#pragma once

#include "thememanager.h"
#include "windoweffectwin.h"

#include <QWidget>

class QVBoxLayout;
class QLabel;
class TitleBar;
class QColor;

class FramelessWindow : public QWidget
{
    Q_OBJECT
public:
    explicit FramelessWindow(QWidget *parent = nullptr);
    ~FramelessWindow();

    void setShadowEnabled(bool enabled);
    void setBackdropEnabled(bool enabled);
    void setRoundedCornersEnabled(bool enabled);
    void setImmersiveDarkModeEnabled(bool enabled);
    void setAeroBlurEnabled(bool enabled);
    void setThemeMode(ThemeManager::ThemeMode mode);
    void setAccentColor(const QColor &accentColor);
    void addTitleBarWidget(QWidget *widget);
    void clearTitleBarWidgets();
    void setDiagnosticsEnabled(bool enabled);
    bool isShadowEnabled() const;
    bool isBackdropEnabled() const;
    bool isRoundedCornersEnabled() const;
    bool isImmersiveDarkModeEnabled() const;
    bool isAeroBlurEnabled() const;
    bool isDiagnosticsEnabled() const;
    ThemeManager::ThemeMode themeMode() const;
    QColor accentColor() const;

protected:
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void paintEvent(QPaintEvent *e) override;
    void changeEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

    void initWindow();
    void initLayout();
    void initMouseTracking();
    void applyTheme();
    int hitTest(const QPoint &globalPos) const;
    int resizeBorderThickness() const;
    Qt::Edges edgesForLocalPos(const QPoint &localPos) const;
    void toggleMaximizeRestore();
    void startSystemMove();
    void showSystemMenu(const QPoint &globalPos);
    void updateSystemMenuState(void *menuHandle) const;
    void updateMaximizeButtonState();
    void updateCursorForPosition(const QPoint &localPos);
    bool tryStartSystemResizeAtLocalPos(const QPoint &localPos);
    bool tryStartSystemResizeAtGlobalPos(const QPoint &globalPos);
    void ensureNativeResizeStyle();
    void syncNativeWindowFrame();
    void applyVisualEffects();
    bool shouldUseDarkMode() const;
    QColor preferredBorderColor() const;
    Qt::CursorShape cursorForEdges(Qt::Edges edges) const;

private:
#ifdef Q_OS_WIN
    bool handleNativeWindowsMessage(void *message, qintptr *result);
    bool handleNcHitTestMessage(qintptr lParam, qintptr *result);
    bool handleNcButtonMessage(quint32 messageId, quintptr wParam, qintptr *result);
    bool handleGetMinMaxInfoMessage(void *lParam, qintptr *result);
    bool handleNcRightButtonUpMessage(quintptr wParam, qintptr lParam, qintptr *result);
    void clearMaximizeButtonNativeHover();
#endif

    TitleBar *m_titleBar;
    QLabel *m_contentLabel;
    QVBoxLayout *m_layout;
    bool m_shadowEnabled;
    bool m_backdropEnabled;
    bool m_roundedCornersEnabled;
    bool m_immersiveDarkModeEnabled;
    bool m_aeroBlurEnabled;
    bool m_applyingTheme;
    ThemeManager m_themeManager;
    WindowEffectWin m_windowEffect;
};
