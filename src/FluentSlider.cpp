#include "Fluent/FluentSlider.h"
#include "Fluent/FluentTheme.h"

#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QStyle>

namespace Fluent {

FluentSlider::FluentSlider(Qt::Orientation orientation, QWidget *parent)
    : QSlider(orientation, parent)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);
    setFocusPolicy(Qt::StrongFocus);

    m_positionAnim = new QPropertyAnimation(this, "handlePos", this);
    m_positionAnim->setDuration(100);
    m_positionAnim->setEasingCurve(QEasingCurve::OutCubic);

    m_hoverAnim = new QPropertyAnimation(this, "hoverLevel", this);
    m_hoverAnim->setDuration(150);
    m_hoverAnim->setEasingCurve(QEasingCurve::OutCubic);

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
        m_positionAnim->stop();
        m_positionAnim->setStartValue(m_handlePos);
        m_positionAnim->setEndValue(target);
        m_positionAnim->start();
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
    update();
}

void FluentSlider::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    const auto &colors = ThemeManager::instance().colors();

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
        painter.setBrush(colors.border);
        painter.drawRoundedRect(QRectF(trackLeft, trackY, trackWidth, thickness), radius, radius);

        // Fill
        const qreal fillWidth = (trackWidth * m_handlePos);
        painter.setBrush(colors.accent);
        painter.drawRoundedRect(QRectF(trackLeft, trackY, fillWidth, thickness), radius, radius);

        // Handle
        painter.setBrush(QColor("#FFFFFF"));
        // Handle border logic:
        // Outer ring: Accent (visible on hover/press)
        // Inner circle: Accent filled
        // Let's stick to simple Fluent style: Solid dot with border
        
        QColor handleBorder = colors.border;
        if (m_hoverLevel > 0.1) handleBorder = colors.accent;
        
        painter.setPen(QPen(handleBorder, 1));
        
        // Outer white knob
        painter.setBrush(Qt::white);
        QRectF handleRect(handleX, handleY, handleSize, handleSize);
        painter.drawEllipse(handleRect.adjusted(0.5, 0.5, -0.5, -0.5));
        
        // Inner accent dot
        qreal innerSize = handleSize * 0.6;
        if (m_hoverLevel > 0.01) {
             painter.setPen(Qt::NoPen);
             painter.setBrush(colors.accent);
             QRectF innerRect(
                 handleX + (handleSize - innerSize) / 2.0,
                 handleY + (handleSize - innerSize) / 2.0,
                 innerSize, innerSize
             );
             painter.drawEllipse(innerRect);
        }

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
        painter.setBrush(colors.border);
        painter.drawRoundedRect(QRectF(trackX, trackTop, thickness, trackHeight), radius, radius);

        // Fill
        const qreal fillHeight = (trackHeight * m_handlePos);
        painter.setBrush(colors.accent);
        // Fill from bottom up
        painter.drawRoundedRect(QRectF(trackX, trackBottom - fillHeight, thickness, fillHeight), radius, radius);

        // Handle
        QColor handleBorder = colors.border;
        if (m_hoverLevel > 0.1) handleBorder = colors.accent;
        
        painter.setPen(QPen(handleBorder, 1));
        painter.setBrush(Qt::white);
        QRectF handleRect(handleX, handleY, handleSize, handleSize);
        painter.drawEllipse(handleRect.adjusted(0.5, 0.5, -0.5, -0.5));
        
        // Inner accent dot
        qreal innerSize = handleSize * 0.6;
        if (m_hoverLevel > 0.01) {
             painter.setPen(Qt::NoPen);
             painter.setBrush(colors.accent);
             QRectF innerRect(
                 handleX + (handleSize - innerSize) / 2.0,
                 handleY + (handleSize - innerSize) / 2.0,
                 innerSize, innerSize
             );
             painter.drawEllipse(innerRect);
        }
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

void FluentSlider::enterEvent(QEvent *event)
{
    QSlider::enterEvent(event);
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverLevel);
    m_hoverAnim->setEndValue(1.0);
    m_hoverAnim->start();
}

void FluentSlider::leaveEvent(QEvent *event)
{
    QSlider::leaveEvent(event);
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverLevel);
    m_hoverAnim->setEndValue(0.0);
    m_hoverAnim->start();
}

} // namespace Fluent
