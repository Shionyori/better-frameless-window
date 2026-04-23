#include "titlebar.h"

#include <QApplication>
#include <QCursor>
#include <QEasingCurve>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QEvent>
#include <QFont>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QStyle>

TitleBar::TitleBar(QWidget *parent)
    : QWidget(parent)
    , m_layout(new QHBoxLayout(this))
    , m_centerContainer(new QWidget(this))
    , m_centerLayout(new QHBoxLayout(m_centerContainer))
    , m_titleLabel(new QLabel("Better Frameless Window", this))
    , m_minimizeButton(new QPushButton(this))
    , m_maximizeButton(new QPushButton(this))
    , m_closeButton(new QPushButton(this))
    , m_leftPressed(false)
    , m_dragInitiated(false)
    , m_visualMaximized(false)
{
    setFixedHeight(36);
    setMouseTracking(true);

    m_layout->setContentsMargins(10, 0, 6, 0);
    m_layout->setSpacing(0);

    m_centerContainer->setObjectName("TitleBarCenterContainer");
    m_centerLayout->setContentsMargins(0, 0, 0, 0);
    m_centerLayout->setSpacing(6);

    m_titleLabel->setObjectName("TitleBarLabel");
    m_minimizeButton->setObjectName("TitleBarMinimizeButton");
    m_maximizeButton->setObjectName("TitleBarMaximizeButton");
    m_closeButton->setObjectName("TitleBarCloseButton");

    QFont titleFont(QStringLiteral("Segoe UI"));
    titleFont.setPixelSize(13);
    titleFont.setWeight(QFont::DemiBold);
    m_titleLabel->setFont(titleFont);

    initControlButton(m_minimizeButton, "min");
    initControlButton(m_maximizeButton, "max");
    initControlButton(m_closeButton, "close");
    updateControlButtonGlyphs();
    resetButtonVisualStates();

    m_layout->addWidget(m_titleLabel);
    m_layout->addWidget(m_centerContainer, 0);
    m_layout->addStretch();
    m_layout->addSpacing(2);
    m_layout->addWidget(m_minimizeButton);
    m_layout->addWidget(m_maximizeButton);
    m_layout->addWidget(m_closeButton);

    connect(m_minimizeButton, &QPushButton::clicked, this, &TitleBar::minimizeRequested);
    connect(m_maximizeButton, &QPushButton::clicked, this, &TitleBar::maximizeRestoreRequested);
    connect(m_closeButton, &QPushButton::clicked, this, &TitleBar::closeRequested);
}

void TitleBar::initControlButton(QPushButton *button, const char *role)
{
    if (button == nullptr) {
        return;
    }

    button->setFixedSize(40, 25);
    button->setFlat(true);
    button->setAutoDefault(false);
    button->setDefault(false);
    button->setFocusPolicy(Qt::NoFocus);
    button->setMouseTracking(true);
    button->setAttribute(Qt::WA_Hover, true);

    QFont iconFont(QStringLiteral("Segoe MDL2 Assets"));
    iconFont.setPixelSize(9);
    button->setFont(iconFont);

    button->setProperty("btnRole", role);
    button->setProperty("btnState", "normal");
    button->installEventFilter(this);

    auto *effect = new QGraphicsOpacityEffect(button);
    effect->setOpacity(0.92);
    button->setGraphicsEffect(effect);

    auto *animation = new QPropertyAnimation(effect, "opacity", button);
    animation->setDuration(90);
    animation->setEasingCurve(QEasingCurve::OutCubic);

    ButtonVisualFx fx;
    fx.effect = effect;
    fx.animation = animation;
    m_buttonFx.insert(button, fx);
}

void TitleBar::updateControlButtonGlyphs()
{
    m_minimizeButton->setText(QStringLiteral("\uE921"));
    m_maximizeButton->setText(m_visualMaximized ? QStringLiteral("\uE923") : QStringLiteral("\uE922"));
    m_closeButton->setText(QStringLiteral("\uE8BB"));
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
    animateButtonOpacity(button, stateValue);
    button->style()->unpolish(button);
    button->style()->polish(button);
    button->update();
}

void TitleBar::animateButtonOpacity(QPushButton *button, const QString &state)
{
    const auto it = m_buttonFx.constFind(button);
    if (it == m_buttonFx.constEnd()) {
        return;
    }

    const ButtonVisualFx fx = it.value();
    if (fx.effect == nullptr || fx.animation == nullptr) {
        return;
    }

    qreal targetOpacity = 0.92;
    if (state == QStringLiteral("hover")) {
        targetOpacity = 1.0;
    } else if (state == QStringLiteral("pressed")) {
        targetOpacity = 0.78;
    } else if (state == QStringLiteral("disabled")) {
        targetOpacity = 0.45;
    }

    if (qFuzzyCompare(fx.effect->opacity(), targetOpacity)) {
        return;
    }

    fx.animation->stop();
    fx.animation->setStartValue(fx.effect->opacity());
    fx.animation->setEndValue(targetOpacity);
    fx.animation->start();
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
    updateControlButtonGlyphs();
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
