#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class QMouseEvent;

class TitleBar : public QWidget
{
    Q_OBJECT
public:
    explicit TitleBar(QWidget *parent = nullptr);

    int heightHint() const;

signals:
    void minimizeRequested();
    void maximizeRestoreRequested();
    void closeRequested();
    void systemMoveRequested(const QPoint &globalPos);
    void systemMenuRequested(const QPoint &globalPos);

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    QLabel *m_titleLabel;
    QPushButton *m_minimizeButton;
    QPushButton *m_maximizeButton;
    QPushButton *m_closeButton;
};
