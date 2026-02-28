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
    void ensureNativeResizeStyle();
    void syncNativeWindowFrame();
    void applyRoundedCorners();
    Qt::CursorShape cursorForEdges(Qt::Edges edges) const;

private:
    TitleBar *m_titleBar;
    QLabel *m_contentLabel;
    QVBoxLayout *m_layout;
};
