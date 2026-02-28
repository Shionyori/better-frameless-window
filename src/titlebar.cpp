#include "titlebar.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QStyle>

TitleBar::TitleBar(QWidget *parent)
    : QWidget(parent)
    , m_titleLabel(new QLabel("Better Frameless Window", this))
    , m_minimizeButton(new QPushButton("-", this))
    , m_maximizeButton(new QPushButton("□", this))
    , m_closeButton(new QPushButton("×", this))
    , m_leftPressed(false)
    , m_dragInitiated(false)
{
    setFixedHeight(36);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 0, 6, 0);
    layout->setSpacing(4);

    m_titleLabel->setObjectName("TitleBarLabel");
    m_minimizeButton->setObjectName("TitleBarMinimizeButton");
    m_maximizeButton->setObjectName("TitleBarMaximizeButton");
    m_closeButton->setObjectName("TitleBarCloseButton");

    const QList<QPushButton *> buttons = {m_minimizeButton, m_maximizeButton, m_closeButton};
    for (auto *button : buttons) {
        button->setFixedSize(32, 28);
        button->setFlat(true);
    }

    layout->addWidget(m_titleLabel);
    layout->addStretch();
    layout->addWidget(m_minimizeButton);
    layout->addWidget(m_maximizeButton);
    layout->addWidget(m_closeButton);

    connect(m_minimizeButton, &QPushButton::clicked, this, &TitleBar::minimizeRequested);
    connect(m_maximizeButton, &QPushButton::clicked, this, &TitleBar::maximizeRestoreRequested);
    connect(m_closeButton, &QPushButton::clicked, this, &TitleBar::closeRequested);
}

bool TitleBar::isOnControlButton(const QPoint &pos) const
{
    QWidget *hovered = childAt(pos);
    return hovered != nullptr && qobject_cast<QPushButton *>(hovered) != nullptr;
}

int TitleBar::heightHint() const
{
    return height();
}

void TitleBar::setMaximizeButtonNativeHover(bool hovered)
{
    if (m_maximizeButton->property("nativeHover").toBool() == hovered) {
        return;
    }

    m_maximizeButton->setProperty("nativeHover", hovered);
    m_maximizeButton->style()->unpolish(m_maximizeButton);
    m_maximizeButton->style()->polish(m_maximizeButton);
    m_maximizeButton->update();
}

void TitleBar::setMaximized(bool maximized)
{
    m_minimizeButton->setEnabled(true);
    m_maximizeButton->setEnabled(true);
    m_closeButton->setEnabled(true);
    setMaximizeButtonNativeHover(false);
    m_maximizeButton->setText(maximized ? "❐" : "□");
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    m_leftPressed = false;
    m_dragInitiated = false;

    if (event->button() == Qt::LeftButton && !isOnControlButton(event->pos())) {
        emit maximizeRestoreRequested();
        event->accept();
        return;
    }

    QWidget::mouseDoubleClickEvent(event);
}

void TitleBar::mousePressEvent(QMouseEvent *event)
{
    if (isOnControlButton(event->pos())) {
        QWidget::mousePressEvent(event);
        return;
    }

    if (event->button() == Qt::LeftButton) {
        m_leftPressed = true;
        m_dragInitiated = false;
        m_pressPos = event->pos();
        event->accept();
        return;
    }

    if (event->button() == Qt::RightButton) {
        emit systemMenuRequested(event->globalPos());
        event->accept();
        return;
    }

    QWidget::mousePressEvent(event);
}

void TitleBar::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_leftPressed || (event->buttons() & Qt::LeftButton) == 0) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    if (!m_dragInitiated) {
        const int distance = (event->pos() - m_pressPos).manhattanLength();
        if (distance >= QApplication::startDragDistance()) {
            m_dragInitiated = true;
            emit systemMoveRequested();
        }
    }

    event->accept();
}

void TitleBar::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_leftPressed = false;
        m_dragInitiated = false;
        event->accept();
        return;
    }

    QWidget::mouseReleaseEvent(event);
}

void TitleBar::leaveEvent(QEvent *event)
{
    if (QApplication::mouseButtons().testFlag(Qt::LeftButton) == false) {
        m_leftPressed = false;
        m_dragInitiated = false;
    }

    QWidget::leaveEvent(event);
}
