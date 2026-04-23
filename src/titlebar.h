#pragma once

#include <QWidget>
#include <QHash>

class QLabel;
class QPushButton;
class QMouseEvent;
class QHBoxLayout;
class QGraphicsOpacityEffect;
class QPropertyAnimation;

class TitleBar : public QWidget
{
    Q_OBJECT
public:
    enum class HitRegion {
        None,
        Caption,
        MinimizeButton,
        MaximizeButton,
        CloseButton,
        OtherInteractive
    };

    explicit TitleBar(QWidget *parent = nullptr);

    int heightHint() const;
    void setMaximized(bool maximized);
    void addCenterWidget(QWidget *widget);
    void clearCenterWidgets();
    HitRegion hitRegionAt(const QPoint &pos) const;

signals:
    void minimizeRequested();
    void maximizeRestoreRequested();
    void closeRequested();
    void systemMoveRequested();
    void systemMenuRequested(const QPoint &globalPos);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    struct ButtonVisualFx {
        QGraphicsOpacityEffect *effect = nullptr;
        QPropertyAnimation *animation = nullptr;
    };

    bool isOnControlButton(const QPoint &pos) const;
    QPushButton *controlButtonAt(const QPoint &pos) const;
    void initControlButton(QPushButton *button, const char *role);
    void updateControlButtonGlyphs();
    void updateButtonVisualState(QPushButton *button, const char *state);
    void resetButtonVisualStates();
    void syncButtonVisualStatesFromCursor();
    void animateButtonOpacity(QPushButton *button, const QString &state);

    QHBoxLayout *m_layout;
    QWidget *m_centerContainer;
    QHBoxLayout *m_centerLayout;
    QLabel *m_titleLabel;
    QPushButton *m_minimizeButton;
    QPushButton *m_maximizeButton;
    QPushButton *m_closeButton;
    bool m_leftPressed;
    bool m_dragInitiated;
    QPoint m_pressPos;
    bool m_visualMaximized;
    QHash<QPushButton *, ButtonVisualFx> m_buttonFx;
};
