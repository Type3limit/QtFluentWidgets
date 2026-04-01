#include "Fluent/FluentDial.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"

#include <QFocusEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QVariantAnimation>
#include <QWheelEvent>
#include <QEasingCurve>
#include <QtMath>
#include <cmath>

namespace Fluent {

namespace {
static QPointF localPosF(QMouseEvent *e)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return e->position();
#else
    return e->localPos();
#endif
}
} // namespace

FluentDial::FluentDial(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(38, 38);
    setCursor(Qt::PointingHandCursor);
    setMouseTracking(true);
    setFocusPolicy(Qt::WheelFocus);
    setToolTip(tr("拖拽旋转调整角度"));

    m_hoverAnim = new QVariantAnimation(this);
    m_hoverAnim->setDuration(120);
    m_hoverAnim->setEasingCurve(QEasingCurve::OutQuad);
    connect(m_hoverAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_hoverLevel = value.toReal();
        update();
    });

    m_focusAnim = new QVariantAnimation(this);
    m_focusAnim->setDuration(160);
    m_focusAnim->setEasingCurve(QEasingCurve::OutQuad);
    connect(m_focusAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_focusLevel = value.toReal();
        update();
    });
}

QSize FluentDial::sizeHint() const
{
    return {38, 38};
}

QSize FluentDial::minimumSizeHint() const
{
    return sizeHint();
}

void FluentDial::setValue(int angleDegrees)
{
    const int v = ((angleDegrees % 360) + 360) % 360;
    if (m_value == v) return;
    m_value = v;
    update();
}

void FluentDial::setTicksVisible(bool visible)
{
    if (m_ticksVisible == visible)
        return;
    m_ticksVisible = visible;
    update();
}

void FluentDial::setTickStep(int stepDegrees)
{
    stepDegrees = qMax(1, stepDegrees);
    if (m_tickStep == stepDegrees)
        return;
    m_tickStep = stepDegrees;
    update();
}

void FluentDial::setMajorTickStep(int stepDegrees)
{
    stepDegrees = qMax(1, stepDegrees);
    if (m_majorTickStep == stepDegrees)
        return;
    m_majorTickStep = stepDegrees;
    update();
}

void FluentDial::setPointerVisible(bool visible)
{
    if (m_pointerVisible == visible)
        return;
    m_pointerVisible = visible;
    update();
}

void FluentDial::enterEvent(FluentEnterEvent *event)
{
    QWidget::enterEvent(event);
    if (m_hoverAnim) {
        m_hoverAnim->stop();
        m_hoverAnim->setStartValue(m_hoverLevel);
        m_hoverAnim->setEndValue(1.0);
        m_hoverAnim->start();
    }
}

void FluentDial::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    if (m_hoverAnim) {
        m_hoverAnim->stop();
        m_hoverAnim->setStartValue(m_hoverLevel);
        m_hoverAnim->setEndValue(0.0);
        m_hoverAnim->start();
    }
}

void FluentDial::focusInEvent(QFocusEvent *event)
{
    QWidget::focusInEvent(event);
    if (m_focusAnim) {
        m_focusAnim->stop();
        m_focusAnim->setStartValue(m_focusLevel);
        m_focusAnim->setEndValue(1.0);
        m_focusAnim->start();
    }
}

void FluentDial::focusOutEvent(QFocusEvent *event)
{
    QWidget::focusOutEvent(event);
    if (m_focusAnim) {
        m_focusAnim->stop();
        m_focusAnim->setStartValue(m_focusLevel);
        m_focusAnim->setEndValue(0.0);
        m_focusAnim->start();
    }
}

void FluentDial::updateFromPos(const QPointF &pos, bool emitSignal)
{
    const QPointF c = QRectF(rect()).center();
    const QPointF d = pos - c;
    if (std::abs(d.x()) < 1.0 && std::abs(d.y()) < 1.0) return; // ignore centre clicks

    qreal angle = qRadiansToDegrees(std::atan2(d.y(), d.x()));
    if (angle < 0.0) angle += 360.0;
    const int newVal = qBound(0, qRound(angle) % 360, 359);
    if (newVal == m_value) return;
    m_value = newVal;
    update();
    if (emitSignal)
        emit valueChanged(m_value);
}

void FluentDial::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        updateFromPos(localPosF(event));
    }
    QWidget::mousePressEvent(event);
}

void FluentDial::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton))
        updateFromPos(localPosF(event));
    QWidget::mouseMoveEvent(event);
}

void FluentDial::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_dragging) {
        updateFromPos(localPosF(event));
        m_dragging = false;
    }
    QWidget::mouseReleaseEvent(event);
}

void FluentDial::wheelEvent(QWheelEvent *event)
{
    // One wheel notch = ±1°
    const int delta = (event->angleDelta().y() > 0) ? 1 : -1;
    const int newVal = ((m_value + delta) % 360 + 360) % 360;
    if (newVal != m_value) {
        m_value = newVal;
        update();
        emit valueChanged(m_value);
    }
    event->accept();
}

void FluentDial::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const auto &tc = ThemeManager::instance().colors();

    // Geometry
    const QRectF outer = QRectF(rect()).adjusted(4.0, 4.0, -4.0, -4.0);
    const QPointF center = outer.center();
    const qreal radius   = outer.width() / 2.0;

    if (m_hoverLevel > 0.0 || m_focusLevel > 0.0) {
        QColor halo = tc.accent;
        halo.setAlphaF(qBound<qreal>(0.0, 0.08 + 0.12 * m_hoverLevel + 0.18 * m_focusLevel, 0.34));
        p.setPen(QPen(halo, 1.8));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(outer.adjusted(-1.5, -1.5, 1.5, 1.5));
    }

    // ── Track ring ────────────────────────────────────────────────────────
    QColor fill = tc.surface;
    if (m_hoverLevel > 0.0)
        fill = Style::mix(tc.surface, tc.hover, 0.30 * m_hoverLevel);
    QColor stroke = Style::mix(tc.border, tc.accent, 0.18 * m_hoverLevel + 0.15 * m_focusLevel);
    p.setPen(QPen(stroke, 2.5, Qt::SolidLine, Qt::RoundCap));
    p.setBrush(fill);
    p.drawEllipse(outer);


    // ── Filled arc from 0° to current angle ──────────────────────────────
    // Qt arcs start at 3-o'clock (0°) and go counter-clockwise in angle units.
    // Our convention: 0° = east, clockwise positive → negate for Qt.
    if (m_value > 0) {
        QColor arcColor = m_dragging ? tc.accent.lighter(112)
                                     : Style::mix(tc.accent, tc.focus, 0.22 * m_focusLevel);
        QPen arcPen(arcColor, 2.7, Qt::SolidLine, Qt::RoundCap);
        p.setPen(arcPen);
        p.setBrush(Qt::NoBrush);
        // Qt drawArc: startAngle in 1/16ths of a degree, counter-clockwise
        const int startAngle = 0;
        const int spanAngle  = -qRound(m_value * 16.0); // negative = clockwise
        p.drawArc(outer.adjusted(1.25, 1.25, -1.25, -1.25), startAngle, spanAngle);
    }

    // ── Pointer + indicator dot ───────────────────────────────────────────
    const qreal angleRad = qDegreesToRadians((qreal)m_value);
    const qreal dotCx = center.x() + (radius - 1.5) * std::cos(angleRad);
    const qreal dotCy = center.y() + (radius - 1.5) * std::sin(angleRad);

    if (m_pointerVisible) {
        const qreal innerLen = 5.0;
        const qreal pointerLen = radius - 6.5;
        const QPointF from(center.x() + innerLen * std::cos(angleRad),
                           center.y() + innerLen * std::sin(angleRad));
        const QPointF to(center.x() + pointerLen * std::cos(angleRad),
                         center.y() + pointerLen * std::sin(angleRad));
        QColor pointerColor = m_dragging ? tc.accent.lighter(115)
                                         : Style::mix(tc.accent, tc.focus, 0.18 * m_focusLevel);
        p.setPen(QPen(pointerColor, 2.2, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(from, to);
    }

    // Shadow
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 40));
    p.drawEllipse(QPointF(dotCx, dotCy + 1.0), 5.0, 5.0);

    // Dot
    QColor dotColor = m_dragging ? tc.accent.lighter(115)
                                 : Style::mix(tc.accent, tc.focus, 0.20 * m_focusLevel);
    p.setBrush(dotColor);
    p.setPen(QPen(tc.surface, 1.5));
    p.drawEllipse(QPointF(dotCx, dotCy), 5.0, 5.0);

    // ── Centre dot ───────────────────────────────────────────────────────
    QColor centerDot = Style::mix(tc.border, tc.accent, 0.30 * m_focusLevel);
    p.setBrush(centerDot);
    p.setPen(Qt::NoPen);
    p.drawEllipse(center, 2.5, 2.5);
}

} // namespace Fluent

