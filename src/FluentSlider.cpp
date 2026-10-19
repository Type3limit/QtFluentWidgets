#include "Fluent/FluentSlider.h"
#include "Fluent/FluentMotion.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"

#include <QAbstractAnimation>
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QStyle>

namespace Fluent {

namespace {

struct SliderPaintColors {
    QColor track;
    QColor fill;
    QColor handleFill;
    QColor handleBorder;
};

SliderPaintColors sliderPaintColors(qreal hoverLevel, bool enabled)
{
    const auto &colors = ThemeManager::instance().colors();
    const auto tokens = ThemeManager::instance().tokens();
    const qreal hover = qBound<qreal>(0.0, hoverLevel, 1.0);

    SliderPaintColors result;
    result.track = Style::mix(tokens.neutral.strokeSubtle,
                              tokens.neutral.fillTertiary,
                              tokens.dark ? 0.54 : 0.38);
    result.fill = tokens.accent.base;
    result.handleFill = tokens.dark
        ? Style::mix(tokens.neutral.cardHover, colors.text, 0.10 + 0.04 * hover)
        : tokens.neutral.layer;
    // WinUI keeps the thumb ring neutral; hover feedback comes from the inner dot.
    result.handleBorder = tokens.neutral.stroke;

    if (!enabled) {
        result.track = tokens.neutral.strokeSubtle;
        result.fill = tokens.neutral.strokeStrong;
        result.fill.setAlpha(145);
        result.handleFill = Style::mix(tokens.neutral.card, tokens.neutral.background, tokens.dark ? 0.48 : 0.34);
        result.handleBorder = tokens.neutral.strokeSubtle;
    }

    return result;
}

// Thumb: neutral raised ring with an accent dot that is always visible
// (WinUI style): grows on hover, shrinks while dragging.
void paintSliderHandle(QPainter &painter, const QRectF &handleRect, const SliderPaintColors &paint, qreal hoverLevel, bool pressed)
{
    painter.setPen(QPen(paint.handleBorder, 1));
    painter.setBrush(paint.handleFill);
    painter.drawEllipse(handleRect.adjusted(0.5, 0.5, -0.5, -0.5));

    const qreal hover = qBound<qreal>(0.0, hoverLevel, 1.0);
    const qreal ratio = pressed ? 0.44 : 0.52 + 0.18 * hover;
    const qreal innerSize = handleRect.width() * ratio;
    painter.setPen(Qt::NoPen);
    painter.setBrush(paint.fill);
    painter.drawEllipse(QRectF(handleRect.center().x() - innerSize / 2.0,
                               handleRect.center().y() - innerSize / 2.0,
                               innerSize, innerSize));
}

} // namespace

FluentSlider::FluentSlider(Qt::Orientation orientation, QWidget *parent)
    : QSlider(orientation, parent)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);
    setFocusPolicy(Qt::StrongFocus);

    m_positionAnim = new QPropertyAnimation(this, "handlePos", this);

    m_hoverAnim = new QPropertyAnimation(this, "hoverLevel", this);

    setHandlePos((value() - minimum()) * 1.0 / qMax(1, maximum() - minimum()));

    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentSlider::applyTheme);
    connect(this, &QSlider::sliderPressed, this, [this]() {
        m_positionAnim->stop();
    });
    connect(this, &QSlider::sliderMoved, this, [this](int newValue) {
        const qreal target = (newValue - minimum()) * 1.0 / qMax(1, maximum() - minimum());
        setHandlePos(target);
    });
    connect(this, &QSlider::valueChanged, this, [this](int newValue) {
        const qreal target = (newValue - minimum()) * 1.0 / qMax(1, maximum() - minimum());
        if (isSliderDown()) {
            setHandlePos(target);
            return;
        }
        FluentMotion::configure(m_positionAnim, FluentMotionRole::Selection);
        m_positionAnim->stop();
        if (m_positionAnim->duration() <= 0) {
            setHandlePos(target);
        } else {
            m_positionAnim->setStartValue(m_handlePos);
            m_positionAnim->setEndValue(target);
            m_positionAnim->start();
        }
    });
}

QSize FluentSlider::sizeHint() const
{
    return orientation() == Qt::Horizontal ? QSize(220, 30) : QSize(30, 220);
}

qreal FluentSlider::handlePos() const
{
    return m_handlePos;
}

void FluentSlider::setHandlePos(qreal value)
{
    m_handlePos = qBound(0.0, value, 1.0);
    update();
}

qreal FluentSlider::hoverLevel() const
{
    return m_hoverLevel;
}

void FluentSlider::setHoverLevel(qreal value)
{
    m_hoverLevel = qBound(0.0, value, 1.0);
    update();
}

void FluentSlider::changeEvent(QEvent *event)
{
    QSlider::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) {
        applyTheme();
    }
}

void FluentSlider::applyTheme()
{
    const bool snapPosition = m_positionAnim &&
        m_positionAnim->state() == QAbstractAnimation::Running &&
        FluentMotion::duration(FluentMotionRole::Selection) <= 0;
    const bool snapHover = m_hoverAnim &&
        m_hoverAnim->state() == QAbstractAnimation::Running &&
        FluentMotion::duration(FluentMotionRole::Hover) <= 0;
    const qreal targetPosition = m_positionAnim ? m_positionAnim->endValue().toReal() : m_handlePos;
    const qreal targetHover = m_hoverAnim ? m_hoverAnim->endValue().toReal() : m_hoverLevel;

    FluentMotion::configure(m_positionAnim, FluentMotionRole::Selection);
    FluentMotion::configure(m_hoverAnim, FluentMotionRole::Hover);
    if (snapPosition) {
        m_positionAnim->stop();
        setHandlePos(targetPosition);
    }
    if (snapHover) {
        m_hoverAnim->stop();
        setHoverLevel(targetHover);
    }
    update();
}

void FluentSlider::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    const SliderPaintColors paint = sliderPaintColors(m_hoverLevel, isEnabled());

    QPainter painter(this);
    if (!painter.isActive()) {
        return;
    }
    painter.setRenderHint(QPainter::Antialiasing, true);

    const int thickness = 4;
    const qreal radius = thickness / 2.0;
    const qreal trackMargin = 12.0;

    if (orientation() == Qt::Horizontal) {
        const qreal trackLeft = trackMargin;
        const qreal trackRight = width() - trackMargin;
        const qreal trackY = (height() - thickness) / 2.0;
        const qreal trackWidth = trackRight - trackLeft;

        const qreal handleSize = 20.0 + (4.0 * m_hoverLevel); // Start slightly bigger
        const qreal handleX = trackLeft + (trackWidth * m_handlePos) - handleSize / 2.0;
        const qreal handleY = (height() - handleSize) / 2.0;
        
        // Track
        painter.setPen(Qt::NoPen);
        painter.setBrush(paint.track);
        painter.drawRoundedRect(QRectF(trackLeft, trackY, trackWidth, thickness), radius, radius);

        // Fill
        const qreal fillWidth = (trackWidth * m_handlePos);
        painter.setBrush(paint.fill);
        painter.drawRoundedRect(QRectF(trackLeft, trackY, fillWidth, thickness), radius, radius);

        QRectF handleRect(handleX, handleY, handleSize, handleSize);
        paintSliderHandle(painter, handleRect, paint, m_hoverLevel, isSliderDown());

    } else {
        const qreal trackTop = trackMargin;
        const qreal trackBottom = height() - trackMargin;
        const qreal trackX = (width() - thickness) / 2.0;
        const qreal trackHeight = trackBottom - trackTop;

        const qreal handleSize = 20.0 + (4.0 * m_hoverLevel);
        const qreal handleY = trackBottom - (trackHeight * m_handlePos) - handleSize / 2.0;
        const qreal handleX = (width() - handleSize) / 2.0;

        // Track
        painter.setPen(Qt::NoPen);
        painter.setBrush(paint.track);
        painter.drawRoundedRect(QRectF(trackX, trackTop, thickness, trackHeight), radius, radius);

        // Fill
        const qreal fillHeight = (trackHeight * m_handlePos);
        painter.setBrush(paint.fill);
        // Fill from bottom up
        painter.drawRoundedRect(QRectF(trackX, trackBottom - fillHeight, thickness, fillHeight), radius, radius);

        QRectF handleRect(handleX, handleY, handleSize, handleSize);
        paintSliderHandle(painter, handleRect, paint, m_hoverLevel, isSliderDown());
    }
}

void FluentSlider::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // Allow dragging the handle; only click-to-jump when clicking on the track.
        const qreal trackMargin = 12.0;

        const qreal handleSize = 20.0 + (4.0 * m_hoverLevel);

        if (orientation() == Qt::Horizontal) {
            const qreal trackLeft = trackMargin;
            const qreal trackRight = width() - trackMargin;
            const qreal trackWidth = qMax<qreal>(1.0, trackRight - trackLeft);

            qreal handleCenterX = trackLeft + (trackWidth * m_handlePos);
            const qreal handleX = handleCenterX - handleSize / 2.0;
            const qreal handleY = (height() - handleSize) / 2.0;
            const QRectF handleRect(handleX, handleY, handleSize, handleSize);
            if (handleRect.contains(event->pos())) {
                // Start dragging based on our painted handle geometry.
                m_dragging = true;
                m_dragOffsetPx = static_cast<qreal>(event->pos().x()) - handleCenterX;
                setSliderDown(true);
                event->accept();
                return;
            }

            const qreal x = qBound(trackLeft, static_cast<qreal>(event->pos().x()), trackRight);
            const qreal pos = (x - trackLeft) / trackWidth;
            const int newValue = QStyle::sliderValueFromPosition(minimum(), maximum(), static_cast<int>(pos * 1000), 1000, false);
            // Click-to-jump, then immediately enter dragging so the user can keep holding
            // the mouse button and adjust without re-clicking the handle.
            setSliderDown(true);
            setValue(newValue);

            handleCenterX = trackLeft + (trackWidth * m_handlePos);
            m_dragging = true;
            m_dragOffsetPx = static_cast<qreal>(event->pos().x()) - handleCenterX;

            event->accept();
            return;
        }

        // Vertical: bottom = minimum visually? Our drawing fills from bottom up with m_handlePos
        const qreal trackTop = trackMargin;
        const qreal trackBottom = height() - trackMargin;
        const qreal trackHeight = qMax<qreal>(1.0, trackBottom - trackTop);

        qreal handleCenterY = trackBottom - (trackHeight * m_handlePos);
        const qreal handleY = handleCenterY - handleSize / 2.0;
        const qreal handleX = (width() - handleSize) / 2.0;
        const QRectF handleRect(handleX, handleY, handleSize, handleSize);
        if (handleRect.contains(event->pos())) {
            m_dragging = true;
            m_dragOffsetPx = static_cast<qreal>(event->pos().y()) - handleCenterY;
            setSliderDown(true);
            event->accept();
            return;
        }

        const qreal y = qBound(trackTop, static_cast<qreal>(event->pos().y()), trackBottom);
        const qreal posFromBottom = (trackBottom - y) / trackHeight;
        const int newValue = QStyle::sliderValueFromPosition(minimum(), maximum(), static_cast<int>(posFromBottom * 1000), 1000, false);
        setSliderDown(true);
        setValue(newValue);

        handleCenterY = trackBottom - (trackHeight * m_handlePos);
        m_dragging = true;
        m_dragOffsetPx = static_cast<qreal>(event->pos().y()) - handleCenterY;

        event->accept();
        return;
    }

    QSlider::mousePressEvent(event);
}

void FluentSlider::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_dragging) {
        QSlider::mouseMoveEvent(event);
        return;
    }

    const qreal trackMargin = 12.0;
    if (orientation() == Qt::Horizontal) {
        const qreal trackLeft = trackMargin;
        const qreal trackRight = width() - trackMargin;
        const qreal trackWidth = qMax<qreal>(1.0, trackRight - trackLeft);

        const qreal rawX = static_cast<qreal>(event->pos().x()) - m_dragOffsetPx;
        const qreal centerX = qBound(trackLeft, rawX, trackRight);
        const qreal pos = (centerX - trackLeft) / trackWidth;
        const int newValue = QStyle::sliderValueFromPosition(minimum(), maximum(), static_cast<int>(pos * 1000), 1000, false);
        setValue(newValue);
        event->accept();
        return;
    }

    const qreal trackTop = trackMargin;
    const qreal trackBottom = height() - trackMargin;
    const qreal trackHeight = qMax<qreal>(1.0, trackBottom - trackTop);

    const qreal rawY = static_cast<qreal>(event->pos().y()) - m_dragOffsetPx;
    const qreal centerY = qBound(trackTop, rawY, trackBottom);
    const qreal posFromBottom = (trackBottom - centerY) / trackHeight;
    const int newValue = QStyle::sliderValueFromPosition(minimum(), maximum(), static_cast<int>(posFromBottom * 1000), 1000, false);
    setValue(newValue);
    event->accept();
}

void FluentSlider::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_dragging && event->button() == Qt::LeftButton) {
        m_dragging = false;
        m_dragOffsetPx = 0.0;
        setSliderDown(false);
        event->accept();
        return;
    }

    QSlider::mouseReleaseEvent(event);
}

void FluentSlider::enterEvent(FluentEnterEvent *event)
{
    QSlider::enterEvent(event);
    FluentMotion::configure(m_hoverAnim, FluentMotionRole::Hover);
    m_hoverAnim->stop();
    if (m_hoverAnim->duration() <= 0) {
        setHoverLevel(1.0);
        return;
    }
    m_hoverAnim->setStartValue(m_hoverLevel);
    m_hoverAnim->setEndValue(1.0);
    m_hoverAnim->start();
}

void FluentSlider::leaveEvent(QEvent *event)
{
    QSlider::leaveEvent(event);
    FluentMotion::configure(m_hoverAnim, FluentMotionRole::Hover);
    m_hoverAnim->stop();
    if (m_hoverAnim->duration() <= 0) {
        setHoverLevel(0.0);
        return;
    }
    m_hoverAnim->setStartValue(m_hoverLevel);
    m_hoverAnim->setEndValue(0.0);
    m_hoverAnim->start();
}

} // namespace Fluent
