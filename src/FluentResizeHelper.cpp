#include "Fluent/FluentResizeHelper.h"

#include <QEvent>
#include <QHoverEvent>
#include <QMouseEvent>
#include <QWidget>

namespace Fluent {

FluentResizeHelper::FluentResizeHelper(QWidget *target)
    : QObject(target)
    , m_target(target)
{
    if (m_target) {
        m_target->setMouseTracking(true);
        m_target->setAttribute(Qt::WA_Hover, true);
        m_target->installEventFilter(this);
    }
}

void FluentResizeHelper::setEnabled(bool enabled)
{
    m_enabled = enabled;
    if (!m_target) {
        return;
    }

    if (!m_enabled) {
        m_leftButtonPressed = false;
        m_pressEdges = None;
        if (m_cursorChanged) {
            m_target->unsetCursor();
            m_cursorChanged = false;
        }
    }
}

bool FluentResizeHelper::isEnabled() const
{
    return m_enabled;
}

void FluentResizeHelper::setBorderWidth(int width)
{
    m_borderWidth = qMax(1, width);
}

int FluentResizeHelper::borderWidth() const
{
    return m_borderWidth;
}

bool FluentResizeHelper::eventFilter(QObject *watched, QEvent *event)
{
    if (!m_enabled || !m_target) {
        return QObject::eventFilter(watched, event);
    }

    if (m_target->isFullScreen() || m_target->isMaximized()) {
        if (m_cursorChanged) {
            m_target->unsetCursor();
            m_cursorChanged = false;
        }
        return QObject::eventFilter(watched, event);
    }

    switch (event->type()) {
    case QEvent::HoverMove:
        mouseHover(static_cast<QHoverEvent *>(event));
        return true;
    case QEvent::Leave:
        mouseLeave();
        return false;
    case QEvent::MouseButtonPress:
        mousePress(static_cast<QMouseEvent *>(event));
        // Only consume if we're starting a resize.
        return !m_pressEdges.testFlag(None);
    case QEvent::MouseButtonRelease:
        mouseRelease(static_cast<QMouseEvent *>(event));
        return false;
    case QEvent::MouseMove:
        mouseMove(static_cast<QMouseEvent *>(event));
        // Consume moves only while resizing.
        return m_leftButtonPressed && !m_pressEdges.testFlag(None);
    default:
        break;
    }

    return QObject::eventFilter(watched, event);
}

void FluentResizeHelper::mouseHover(QHoverEvent *event)
{
    if (!event) {
        return;
    }
    updateCursorShape(m_target->mapToGlobal(event->pos()));
}

void FluentResizeHelper::mouseLeave()
{
    if (!m_leftButtonPressed && m_cursorChanged) {
        m_target->unsetCursor();
        m_cursorChanged = false;
    }
}

void FluentResizeHelper::mousePress(QMouseEvent *event)
{
    if (!event || event->button() != Qt::LeftButton) {
        return;
    }

    m_leftButtonPressed = true;
    m_pressGlobalPos = event->globalPos();
    m_pressGeometry = m_target->geometry();
    m_pressEdges = calculateEdges(m_pressGlobalPos);
}

void FluentResizeHelper::mouseRelease(QMouseEvent *event)
{
    if (!event || event->button() != Qt::LeftButton) {
        return;
    }

    m_leftButtonPressed = false;
    m_pressEdges = None;
}

void FluentResizeHelper::mouseMove(QMouseEvent *event)
{
    if (!event) {
        return;
    }

    if (!m_leftButtonPressed || m_pressEdges.testFlag(None)) {
        updateCursorShape(event->globalPos());
        return;
    }

    QRect g = adjustedGeometryForDrag(event->globalPos());

    // Respect min/max size.
    const QSize minS = m_target->minimumSize();
    const QSize maxS = m_target->maximumSize();

    if (g.width() < minS.width()) {
        g.setWidth(minS.width());
        if (m_pressEdges.testFlag(Left) || m_pressEdges.testFlag(TopLeft) || m_pressEdges.testFlag(BottomLeft)) {
            g.moveLeft(m_pressGeometry.right() - g.width() + 1);
        }
    }
    if (g.height() < minS.height()) {
        g.setHeight(minS.height());
        if (m_pressEdges.testFlag(Top) || m_pressEdges.testFlag(TopLeft) || m_pressEdges.testFlag(TopRight)) {
            g.moveTop(m_pressGeometry.bottom() - g.height() + 1);
        }
    }

    if (maxS.width() < QWIDGETSIZE_MAX && g.width() > maxS.width()) {
        g.setWidth(maxS.width());
        if (m_pressEdges.testFlag(Left) || m_pressEdges.testFlag(TopLeft) || m_pressEdges.testFlag(BottomLeft)) {
            g.moveLeft(m_pressGeometry.right() - g.width() + 1);
        }
    }
    if (maxS.height() < QWIDGETSIZE_MAX && g.height() > maxS.height()) {
        g.setHeight(maxS.height());
        if (m_pressEdges.testFlag(Top) || m_pressEdges.testFlag(TopLeft) || m_pressEdges.testFlag(TopRight)) {
            g.moveTop(m_pressGeometry.bottom() - g.height() + 1);
        }
    }

    m_target->setGeometry(g.normalized());
}

void FluentResizeHelper::updateCursorShape(const QPoint &globalPos)
{
    if (!m_target || m_leftButtonPressed) {
        return;
    }

    const Edges edges = calculateEdges(globalPos);
    if (edges.testFlag(Top) || edges.testFlag(Bottom)) {
        m_target->setCursor(Qt::SizeVerCursor);
        m_cursorChanged = true;
    } else if (edges.testFlag(Left) || edges.testFlag(Right)) {
        m_target->setCursor(Qt::SizeHorCursor);
        m_cursorChanged = true;
    } else if (edges.testFlag(TopLeft) || edges.testFlag(BottomRight)) {
        m_target->setCursor(Qt::SizeFDiagCursor);
        m_cursorChanged = true;
    } else if (edges.testFlag(TopRight) || edges.testFlag(BottomLeft)) {
        m_target->setCursor(Qt::SizeBDiagCursor);
        m_cursorChanged = true;
    } else if (m_cursorChanged) {
        m_target->unsetCursor();
        m_cursorChanged = false;
    }
}

FluentResizeHelper::Edges FluentResizeHelper::calculateEdges(const QPoint &globalPos) const
{
    if (!m_target) {
        return None;
    }

    const QPoint local = m_target->mapFromGlobal(globalPos);
    const QRect r = m_target->rect();
    const int bw = m_borderWidth;

    const bool onLeft = local.x() >= r.left() && local.x() <= r.left() + bw;
    const bool onRight = local.x() <= r.right() && local.x() >= r.right() - bw;
    const bool onTop = local.y() >= r.top() && local.y() <= r.top() + bw;
    const bool onBottom = local.y() <= r.bottom() && local.y() >= r.bottom() - bw;

    if (onTop && onLeft) {
        return TopLeft;
    }
    if (onTop && onRight) {
        return TopRight;
    }
    if (onBottom && onLeft) {
        return BottomLeft;
    }
    if (onBottom && onRight) {
        return BottomRight;
    }
    if (onTop) {
        return Top;
    }
    if (onBottom) {
        return Bottom;
    }
    if (onLeft) {
        return Left;
    }
    if (onRight) {
        return Right;
    }

    return None;
}

QRect FluentResizeHelper::adjustedGeometryForDrag(const QPoint &globalPos) const
{
    QRect g = m_pressGeometry;

    const int dx = globalPos.x() - m_pressGlobalPos.x();
    const int dy = globalPos.y() - m_pressGlobalPos.y();

    if (m_pressEdges == Left) {
        g.setLeft(m_pressGeometry.left() + dx);
    } else if (m_pressEdges == Right) {
        g.setRight(m_pressGeometry.right() + dx);
    } else if (m_pressEdges == Top) {
        g.setTop(m_pressGeometry.top() + dy);
    } else if (m_pressEdges == Bottom) {
        g.setBottom(m_pressGeometry.bottom() + dy);
    } else if (m_pressEdges == TopLeft) {
        g.setTop(m_pressGeometry.top() + dy);
        g.setLeft(m_pressGeometry.left() + dx);
    } else if (m_pressEdges == TopRight) {
        g.setTop(m_pressGeometry.top() + dy);
        g.setRight(m_pressGeometry.right() + dx);
    } else if (m_pressEdges == BottomLeft) {
        g.setBottom(m_pressGeometry.bottom() + dy);
        g.setLeft(m_pressGeometry.left() + dx);
    } else if (m_pressEdges == BottomRight) {
        g.setBottom(m_pressGeometry.bottom() + dy);
        g.setRight(m_pressGeometry.right() + dx);
    }

    return g;
}

} // namespace Fluent
