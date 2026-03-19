#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class QMouseEvent;
class QHBoxLayout;

class TitleBar : public QWidget
{
    Q_OBJECT
public:
    explicit TitleBar(QWidget *parent = nullptr);

    int heightHint() const;
    void setMaximized(bool maximized);
    void addCenterWidget(QWidget *widget);
    void clearCenterWidgets();

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
    bool isOnControlButton(const QPoint &pos) const;
    void updateButtonVisualState(QPushButton *button, const char *state);
    void resetButtonVisualStates();

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
};
