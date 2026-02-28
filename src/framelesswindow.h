#pragma once

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
    bool isShadowEnabled() const;
    bool isBackdropEnabled() const;
    bool isRoundedCornersEnabled() const;
    bool isImmersiveDarkModeEnabled() const;

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
    void applyRoundedCorners();
    void applyNativeShadow();
    void applyImmersiveDarkMode();
    void applyBackdropEffects();
    void applyVisualEffects();
    void applyBorderColor();
    bool shouldUseDarkMode() const;
    QColor preferredBorderColor() const;
    Qt::CursorShape cursorForEdges(Qt::Edges edges) const;

private:
    TitleBar *m_titleBar;
    QLabel *m_contentLabel;
    QVBoxLayout *m_layout;
    bool m_shadowEnabled;
    bool m_backdropEnabled;
    bool m_roundedCornersEnabled;
    bool m_immersiveDarkModeEnabled;
    WindowEffectWin m_windowEffect;
};
