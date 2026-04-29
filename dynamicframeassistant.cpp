#include "dynamicframeassistant.h"
#include <QResizeEvent>
#include <QDebug>
#include "dynamicframeassistant.h"
#include <QResizeEvent>
#include <QDebug>



DynamicFrameAssistant::DynamicFrameAssistant(QWidget* parent) :
    QObject(parent),
    m_parentWidget(parent)
{
    m_eventFilter = new LambdaEventFilter(
        [this](QObject*, QEvent* event) {
            if (event->type() == QEvent::Resize && !childWidgetBlocked()) {
                this->handleResize();
            }
            return false;
        },
        this);

    if (m_parentWidget) {
        m_parentWidget->installEventFilter(m_eventFilter);
    }
}


DynamicFrameAssistant::~DynamicFrameAssistant()
{
    // FIX 2: Safely detach filter from parent before the filter or this object dies
    if (m_eventFilter && m_parentWidget) {
        m_parentWidget->removeEventFilter(m_eventFilter);
    }

    for (auto it = m_childWidgets.begin(); it != m_childWidgets.end(); ++it) {
        if (it.value().animation) {
            it.value().animation->stop();
            delete it.value().animation;
        }
    }
    m_childWidgets.clear();
}


void DynamicFrameAssistant::removeChildWidget(QWidget* widget)
{
    if (!widget || !m_childWidgets.contains(widget)) return;

    // Disconnect the destruction cleanup to prevent double-removal attempts
    // and clean up animation resources.
    ChildWidgetInfo& info = m_childWidgets[widget];
    if (info.animation) {
        info.animation->stop();
        delete info.animation;
    }

    m_childWidgets.remove(widget);
}

// FIX 5: Optimized lookup from O(N) to O(log N) / O(1)
bool DynamicFrameAssistant::isPopped(const QWidget *widget) const
{
    if (!widget) return false;

    for (auto it = m_childWidgets.begin(); it != m_childWidgets.end(); ++it) {
        if (it.value().visible) {
            return true;
        }
    }
    return false;
}


void DynamicFrameAssistant::setupWidgetAnimation(QWidget* widget)
{
    if (!m_childWidgets.contains(widget)) return;

    ChildWidgetInfo& info = m_childWidgets[widget];
    if (!info.animation) {
        info.animation = new QPropertyAnimation(widget, "geometry", this);
        info.animation->setEasingCurve(QEasingCurve::OutQuad);

        connect(info.animation, &QPropertyAnimation::finished, this, [this, widget]() {
            if (!m_childWidgets.contains(widget)) return;
            // No hide() call — widget stays in the tree, just parked off-screen.
            // This avoids any layout-triggered geometry reset that hide/show can cause.
            emit animationFinished(widget, m_childWidgets[widget].visible);
        });
    }
}

void DynamicFrameAssistant::addChildWidget(QWidget* widget, Qt::Alignment alignment, int margin)
{
    if (!widget) return;

    connect(widget, &QObject::destroyed, this, [this, widget]() {
        m_childWidgets.remove(widget);
    });

    ChildWidgetInfo info;
    info.widget    = widget;
    info.alignment = alignment;
    info.margin    = margin;
    info.visible   = true;
    info.animation = nullptr;

    if (widget->parentWidget() != m_parentWidget)
        widget->setParent(m_parentWidget);

    // Ensure widget is shown — we manage visibility purely by position
    widget->show();

    m_childWidgets.insert(widget, info);
    setupWidgetAnimation(widget);
    updateWidgetPosition(widget);
    update();
}


void DynamicFrameAssistant::popWidget(QWidget* widget, bool show, int duration)
{
    if (!widget || !m_childWidgets.contains(widget)) return;

    ChildWidgetInfo& info = m_childWidgets[widget];

    // Stop any running animation to allow mid-animation reversal
    if (info.animation && info.animation->state() == QAbstractAnimation::Running) {
        info.animation->stop();
    } else if (show == info.visible) {
        return;
    }

    if (!info.animation)
        setupWidgetAnimation(widget);

    if (show) {
        // Recalculate target in case parent was resized while widget was off-screen
        info.originalPosition = calculateVisiblePosition(widget, info.alignment, info.margin);

        QRect endRect(info.originalPosition, widget->size());
        QRect startRect(endRect);
        startRect.moveTopLeft(calculateHiddenPosition(widget, info.alignment));

        info.animation->setDuration(duration);
        info.animation->setStartValue(startRect);
        info.animation->setEndValue(endRect);
    } else {
        QRect startRect(widget->pos(), widget->size());
        QRect endRect(startRect);
        endRect.moveTopLeft(calculateHiddenPosition(widget, info.alignment));

        info.animation->setDuration(duration);
        info.animation->setStartValue(startRect);
        info.animation->setEndValue(endRect);
    }

    info.animation->start();
    info.visible = show;
}

void DynamicFrameAssistant::popAll(bool show, int duration)
{
    for (auto& info : m_childWidgets)
        popWidget(info.widget, show, duration);
}

// Center-aligned widgets always hide toward the top.
// This is intentional and consistent: they appear to "collapse upward"
// on dismiss and "drop in from above" on show — a natural gravity metaphor.
// No randomness needed; predictable direction is better UX for repeated toggles.
QPoint DynamicFrameAssistant::calculateHiddenPosition(QWidget* widget, const Qt::Alignment& alignment)
{
    if (!m_parentWidget || !widget) return QPoint();

    QPoint hiddenPos = m_childWidgets[widget].originalPosition;
    QSize  childSize = widget->size();

    // Vertical axis
    if (alignment & Qt::AlignTop) {
        hiddenPos.setY(-childSize.height() - m_defaultMargin);
    } else if (alignment & Qt::AlignBottom) {
        hiddenPos.setY(m_parentWidget->rect().height() + m_defaultMargin);
    } else {
        // AlignVCenter or bare AlignHCenter — slide off the top
        hiddenPos.setY(-childSize.height() - m_defaultMargin);
    }

    // Horizontal axis
    if (alignment & Qt::AlignLeft) {
        hiddenPos.setX(-childSize.width() - m_defaultMargin);
    } else if (alignment & Qt::AlignRight) {
        hiddenPos.setX(m_parentWidget->rect().width() + m_defaultMargin);
    } else {
        // AlignHCenter — keep horizontal position, only move vertically
        // (the vertical axis already handles the exit direction above)
    }

    return hiddenPos;
}

QPoint DynamicFrameAssistant::calculateVisiblePosition(QWidget* widget, const Qt::Alignment& alignment, int margin)
{
    if (!m_parentWidget || !widget) return QPoint();

    QRect  availableRect = m_parentWidget->rect();
    QSize  childSize     = widget->size();
    QPoint newPos;

    if (alignment & Qt::AlignLeft) {
        newPos.setX(availableRect.left() + margin);
    } else if (alignment & Qt::AlignRight) {
        newPos.setX(availableRect.right() - childSize.width() - margin);
    } else {
        newPos.setX(availableRect.left() + (availableRect.width() - childSize.width()) / 2);
    }

    if (alignment & Qt::AlignTop) {
        newPos.setY(availableRect.top() + margin);
    } else if (alignment & Qt::AlignBottom) {
        newPos.setY(availableRect.bottom() - childSize.height() - margin);
    } else {
        newPos.setY(availableRect.top() + (availableRect.height() - childSize.height()) / 2);
    }

    return newPos;
}

void DynamicFrameAssistant::setParentWidget(QWidget* widget)
{
    if (m_eventFilter && m_parentWidget)
        m_parentWidget->removeEventFilter(m_eventFilter);

    m_parentWidget = widget;

    if (m_parentWidget) {
        if (!m_eventFilter) {
            m_eventFilter = new LambdaEventFilter(
                [this](QObject*, QEvent* event) {
                    if (event->type() == QEvent::Resize && !childWidgetBlocked()) {
                        this->handleResize();
                    }
                    return false;
                },
                this);
        }
        m_parentWidget->installEventFilter(m_eventFilter);

        for (auto& info : m_childWidgets) {
            if (info.widget)
                info.widget->setParent(m_parentWidget);
        }
        updatePositions();
    }
}

void DynamicFrameAssistant::setChildWidgetAlignment(QWidget* widget, Qt::Alignment alignment)
{
    if (!widget || !m_childWidgets.contains(widget)) return;
    m_childWidgets[widget].alignment = alignment;
    updateWidgetPosition(widget);
}

void DynamicFrameAssistant::setChildWidgetMargin(QWidget* widget, int margin)
{
    if (!widget || !m_childWidgets.contains(widget)) return;
    m_childWidgets[widget].margin = margin;
    updateWidgetPosition(widget);
}

bool DynamicFrameAssistant::childWidgetBlocked() const { return m_block; }
void DynamicFrameAssistant::setChildWidgetBlock(bool newBlock) { m_block = newBlock; }
void DynamicFrameAssistant::setBlock(bool newBlock) { m_block = newBlock; }
void DynamicFrameAssistant::update() { updatePositions(); }


void DynamicFrameAssistant::updatePositions()
{
    for (auto& info : m_childWidgets)
        updateWidgetPosition(info.widget);
}

void DynamicFrameAssistant::updateWidgetPosition(QWidget* widget)
{
    // Safety check for null/removed widgets
    if (!m_parentWidget || !widget || !m_childWidgets.contains(widget)) return;

    ChildWidgetInfo& info = m_childWidgets[widget];
    info.originalPosition = calculateVisiblePosition(widget, info.alignment, info.margin);

    if (info.visible) {
        widget->move(info.originalPosition);
    } else {
        widget->move(calculateHiddenPosition(widget, info.alignment));
    }
}


void DynamicFrameAssistant::handleResize() { updatePositions(); }
