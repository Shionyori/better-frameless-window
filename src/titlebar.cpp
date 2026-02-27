#include "titlebar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>

TitleBar::TitleBar(QWidget *parent)
    : QWidget(parent)
    , m_titleLabel(new QLabel("Better Frameless Window", this))
    , m_minimizeButton(new QPushButton("-", this))
    , m_maximizeButton(new QPushButton("□", this))
    , m_closeButton(new QPushButton("×", this))
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

int TitleBar::heightHint() const
{
    return height();
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit maximizeRestoreRequested();
        event->accept();
        return;
    }

    QWidget::mouseDoubleClickEvent(event);
}

void TitleBar::mousePressEvent(QMouseEvent *event)
{
    if (childAt(event->pos()) != nullptr && qobject_cast<QPushButton *>(childAt(event->pos())) != nullptr) {
        QWidget::mousePressEvent(event);
        return;
    }

    if (event->button() == Qt::LeftButton) {
        emit systemMoveRequested(event->globalPos());
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
