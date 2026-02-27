#pragma once

#include <QWidget>

class QVBoxLayout;
class QLabel;
class TitleBar;

class FramelessWindow : public QWidget
{
    Q_OBJECT
public:
    explicit FramelessWindow(QWidget *parent = nullptr);
    ~FramelessWindow();

protected:
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
    void paintEvent(QPaintEvent *e) override;
    void changeEvent(QEvent *event) override;

    void initWindow();
    void initLayout();
    int hitTest(const QPoint &localPos) const;
    int resizeBorderThickness() const;
    void toggleMaximizeRestore();
    void startSystemMove();
    void showSystemMenu(const QPoint &globalPos);
    void updateMaximizeButtonState();

private:
    TitleBar *m_titleBar;
    QLabel *m_contentLabel;
    QVBoxLayout *m_layout;
};
