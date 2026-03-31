#pragma once

#include <QtGlobal>
#include <QEvent>
#include <QMouseEvent>
#include <QWheelEvent>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QEnterEvent>
#endif

namespace Fluent {

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
using FluentEnterEvent = QEnterEvent;

inline QPointF mousePositionF(const QMouseEvent *event)
{
    return event->position();
}

inline QPoint globalMousePosition(const QMouseEvent *event)
{
    return event->globalPosition().toPoint();
}

inline QPointF wheelPositionF(const QWheelEvent *event)
{
    return event->position();
}
#else
using FluentEnterEvent = QEvent;

inline QPointF mousePositionF(const QMouseEvent *event)
{
    return event->localPos();
}

inline QPoint globalMousePosition(const QMouseEvent *event)
{
    return event->globalPos();
}

inline QPointF wheelPositionF(const QWheelEvent *event)
{
    return event->posF();
}
#endif

} // namespace Fluent

