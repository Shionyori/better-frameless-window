#include "titlebar.h"

#include <QApplication>
#include <QCursor>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QEvent>
#include <QStyle>

TitleBar::TitleBar(QWidget *parent)
    : QWidget(parent)
    , m_layout(new QHBoxLayout(this))
    , m_centerContainer(new QWidget(this))
    , m_centerLayout(new QHBoxLayout(m_centerContainer))
    , m_titleLabel(new QLabel("Better Frameless Window", this))
    , m_minimizeButton(new QPushButton("-", this))
    , m_maximizeButton(new QPushButton("□", this))
    , m_closeButton(new QPushButton("×", this))
    , m_leftPressed(false)
    , m_dragInitiated(false)
    , m_visualMaximized(false)
{
    setFixedHeight(36);
    setMouseTracking(true);

    m_layout->setContentsMargins(10, 0, 6, 0);
    m_layout->setSpacing(4);

    m_centerContainer->setObjectName("TitleBarCenterContainer");
    m_centerLayout->setContentsMargins(0, 0, 0, 0);
    m_centerLayout->setSpacing(6);

    m_titleLabel->setObjectName("TitleBarLabel");
    m_minimizeButton->setObjectName("TitleBarMinimizeButton");
    m_maximizeButton->setObjectName("TitleBarMaximizeButton");
    m_closeButton->setObjectName("TitleBarCloseButton");

    const QList<QPushButton *> buttons = {m_minimizeButton, m_maximizeButton, m_closeButton};
    for (auto *button : buttons) {
        button->setFixedSize(32, 28);
        button->setFlat(true);
        button->setMouseTracking(true);
        button->setAttribute(Qt::WA_Hover, true);
        button->installEventFilter(this);
    }

    m_minimizeButton->setProperty("btnRole", "min");
    m_maximizeButton->setProperty("btnRole", "max");
    m_closeButton->setProperty("btnRole", "close");
    resetButtonVisualStates();

    m_layout->addWidget(m_titleLabel);
    m_layout->addWidget(m_centerContainer, 0);
    m_layout->addStretch();
    m_layout->addWidget(m_minimizeButton);
    m_layout->addWidget(m_maximizeButton);
    m_layout->addWidget(m_closeButton);

    connect(m_minimizeButton, &QPushButton::clicked, this, &TitleBar::minimizeRequested);
    connect(m_maximizeButton, &QPushButton::clicked, this, &TitleBar::maximizeRestoreRequested);
    connect(m_closeButton, &QPushButton::clicked, this, &TitleBar::closeRequested);
}

void TitleBar::addCenterWidget(QWidget *widget)
{
    if (widget == nullptr) {
        return;
    }

    widget->setParent(m_centerContainer);
    widget->setMouseTracking(true);
    m_centerLayout->addWidget(widget);
}

void TitleBar::clearCenterWidgets()
{
    while (m_centerLayout->count() > 0) {
        QLayoutItem *item = m_centerLayout->takeAt(0);
        if (item == nullptr) {
            continue;
        }

        QWidget *widget = item->widget();
        if (widget != nullptr) {
            widget->deleteLater();
        }

        delete item;
    }
}

TitleBar::HitRegion TitleBar::hitRegionAt(const QPoint &pos) const
{
    if (!rect().contains(pos)) {
        return HitRegion::None;
    }

    auto hitVisibleButton = [pos](const QPushButton *button) {
        return button != nullptr && button->isVisible() && button->geometry().contains(pos);
    };

    if (hitVisibleButton(m_minimizeButton)) {
        return HitRegion::MinimizeButton;
    }

    if (hitVisibleButton(m_maximizeButton)) {
        return HitRegion::MaximizeButton;
    }

    if (hitVisibleButton(m_closeButton)) {
        return HitRegion::CloseButton;
    }

    QWidget *hovered = childAt(pos);
    if (qobject_cast<QPushButton *>(hovered) != nullptr) {
        return HitRegion::OtherInteractive;
    }

    return HitRegion::Caption;
}

bool TitleBar::isOnControlButton(const QPoint &pos) const
{
    return controlButtonAt(pos) != nullptr;
}

QPushButton *TitleBar::controlButtonAt(const QPoint &pos) const
{
    if (m_minimizeButton != nullptr && m_minimizeButton->isVisible() && m_minimizeButton->geometry().contains(pos)) {
        return m_minimizeButton;
    }

    if (m_maximizeButton != nullptr && m_maximizeButton->isVisible() && m_maximizeButton->geometry().contains(pos)) {
        return m_maximizeButton;
    }

    if (m_closeButton != nullptr && m_closeButton->isVisible() && m_closeButton->geometry().contains(pos)) {
        return m_closeButton;
    }

    return nullptr;
}

int TitleBar::heightHint() const
{
    return height();
}

bool TitleBar::eventFilter(QObject *watched, QEvent *event)
{
    auto *button = qobject_cast<QPushButton *>(watched);
    if (button == nullptr) {
        return QWidget::eventFilter(watched, event);
    }

    switch (event->type()) {
    case QEvent::Enter:
    case QEvent::HoverEnter:
    case QEvent::Leave:
    case QEvent::HoverLeave:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::HoverMove:
    case QEvent::MouseMove:
    case QEvent::EnabledChange:
        syncButtonVisualStatesFromCursor();
        break;
    default:
        break;
    }

    return QWidget::eventFilter(watched, event);
}

void TitleBar::updateButtonVisualState(QPushButton *button, const char *state)
{
    if (button == nullptr) {
        return;
    }

    const QString stateValue = QString::fromLatin1(state);
    if (button->property("btnState").toString() == stateValue) {
        return;
    }

    button->setProperty("btnState", stateValue);
    button->style()->unpolish(button);
    button->style()->polish(button);
    button->update();
}

void TitleBar::resetButtonVisualStates()
{
    syncButtonVisualStatesFromCursor();
}

void TitleBar::syncButtonVisualStatesFromCursor()
{
    const QList<QPushButton *> buttons = {m_minimizeButton, m_maximizeButton, m_closeButton};
    for (QPushButton *button : buttons) {
        if (button == nullptr) {
            continue;
        }

        updateButtonVisualState(button, button->isEnabled() ? "normal" : "disabled");
    }

    const QPoint localPos = mapFromGlobal(QCursor::pos());
    if (!rect().contains(localPos)) {
        return;
    }

    QPushButton *hoveredButton = controlButtonAt(localPos);
    if (hoveredButton == nullptr || !hoveredButton->isEnabled()) {
        return;
    }

    if (QApplication::mouseButtons().testFlag(Qt::LeftButton)) {
        updateButtonVisualState(hoveredButton, "pressed");
    } else {
        updateButtonVisualState(hoveredButton, "hover");
    }
}

void TitleBar::setMaximized(bool maximized)
{
    if (m_visualMaximized == maximized
        && m_minimizeButton->isEnabled()
        && m_maximizeButton->isEnabled()
        && m_closeButton->isEnabled()) {
        syncButtonVisualStatesFromCursor();
        return;
    }

    m_visualMaximized = maximized;
    m_minimizeButton->setEnabled(true);
    m_maximizeButton->setEnabled(true);
    m_closeButton->setEnabled(true);
    m_maximizeButton->setText(maximized ? "❐" : "□");
    resetButtonVisualStates();
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
    syncButtonVisualStatesFromCursor();

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
        syncButtonVisualStatesFromCursor();
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

    syncButtonVisualStatesFromCursor();

    QWidget::leaveEvent(event);
}
