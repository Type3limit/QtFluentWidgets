#include "Fluent/FluentRadioButton.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"

#include <QEvent>
#include <QPainter>
#include <QVariantAnimation>
#include <QEasingCurve>

namespace Fluent {

FluentRadioButton::FluentRadioButton(QWidget *parent)
    : QRadioButton(parent)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);
    setFocusPolicy(Qt::StrongFocus);

    m_hoverAnim = new QVariantAnimation(this);
    m_hoverAnim->setDuration(150);
    m_hoverAnim->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_hoverAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_hoverLevel = value.toReal();
        update();
    });

    m_focusAnim = new QVariantAnimation(this);
    m_focusAnim->setDuration(200);
    m_focusAnim->setEasingCurve(QEasingCurve::OutQuad);
    connect(m_focusAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_focusLevel = value.toReal();
        update();
    });

    m_checkAnim = new QVariantAnimation(this);
    m_checkAnim->setDuration(200);
    m_checkAnim->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_checkAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_checkLevel = value.toReal();
        update();
    });
    connect(this, &QRadioButton::toggled, this, [this](bool checked) {
        m_checkAnim->stop();
        m_checkAnim->setStartValue(m_checkLevel);
        m_checkAnim->setEndValue(checked ? 1.0 : 0.0);
        m_checkAnim->start();
    });
    m_checkLevel = isChecked() ? 1.0 : 0.0;

    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentRadioButton::applyTheme);
}

FluentRadioButton::FluentRadioButton(const QString &text, QWidget *parent)
    : QRadioButton(text, parent)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);
    setFocusPolicy(Qt::StrongFocus);

    m_hoverAnim = new QVariantAnimation(this);
    m_hoverAnim->setDuration(150);
    m_hoverAnim->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_hoverAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_hoverLevel = value.toReal();
        update();
    });

    m_focusAnim = new QVariantAnimation(this);
    m_focusAnim->setDuration(200);
    m_focusAnim->setEasingCurve(QEasingCurve::OutQuad);
    connect(m_focusAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_focusLevel = value.toReal();
        update();
    });

    m_checkAnim = new QVariantAnimation(this);
    m_checkAnim->setDuration(200);
    m_checkAnim->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_checkAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_checkLevel = value.toReal();
        update();
    });
    connect(this, &QRadioButton::toggled, this, [this](bool checked) {
        m_checkAnim->stop();
        m_checkAnim->setStartValue(m_checkLevel);
        m_checkAnim->setEndValue(checked ? 1.0 : 0.0);
        m_checkAnim->start();
    });
    m_checkLevel = isChecked() ? 1.0 : 0.0;

    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentRadioButton::applyTheme);
}

QSize FluentRadioButton::sizeHint() const
{
    const QFontMetrics fm(font());
    const int circle = 18;
    const int leftPad = 2;
    const int gap = 8;
    const int rightPad = 6;
    const int w = leftPad + circle + gap + fm.horizontalAdvance(text()) + rightPad;
    const int h = qMax(circle + 4, fm.height() + 8);
    return {w, h};
}

QSize FluentRadioButton::minimumSizeHint() const
{
    return sizeHint();
}

qreal FluentRadioButton::hoverLevel() const
{
    return m_hoverLevel;
}

void FluentRadioButton::setHoverLevel(qreal value)
{
    m_hoverLevel = qBound(0.0, value, 1.0);
    update();
}

qreal FluentRadioButton::focusLevel() const
{
    return m_focusLevel;
}

void FluentRadioButton::setFocusLevel(qreal value)
{
    m_focusLevel = qBound(0.0, value, 1.0);
    update();
}

void FluentRadioButton::changeEvent(QEvent *event)
{
    QRadioButton::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) {
        applyTheme();
    }
}

void FluentRadioButton::applyTheme()
{
    update();
}

void FluentRadioButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    const auto &colors = ThemeManager::instance().colors();

    QPainter painter(this);
    if (!painter.isActive()) {
        return;
    }
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Hover background (full row highlight)
    if (m_hoverLevel > 0.01 && isEnabled()) {
        painter.setPen(Qt::NoPen);
        QColor hoverBg = Style::withAlpha(colors.hover, static_cast<int>(90 * m_hoverLevel));
        painter.setBrush(hoverBg);
        const QRectF hoverRect = QRectF(this->rect()).adjusted(1.0, 2.0, -1.0, -2.0);
        painter.drawRoundedRect(hoverRect, 6.0, 6.0);
    }

    const qreal size = 18.0;
    const qreal leftPad = 2.0;
    QRectF circleRect(leftPad, (height() - size) / 2.0, size, size);
    QRectF textRect = QRectF(this->rect()).adjusted(static_cast<int>(leftPad + size + 8), 0, -4, 0);
    
    // Adjust for border sharpness
    QRectF drawRect = circleRect.adjusted(0.5, 0.5, -0.5, -0.5);

    QColor border = colors.border;
    QColor fill = colors.surface;
    if (!isEnabled()) {
        fill = colors.hover;
        drawRect = circleRect;
    }

    painter.setPen(QPen(border, 1));
    painter.setBrush(fill);
    painter.drawEllipse(drawRect);

    // Focus ring (keyboard focus)
    if (isEnabled() && m_focusLevel > 0.01) {
        QColor focus = colors.focus;
        focus.setAlphaF(0.9 * m_focusLevel);
        painter.setPen(QPen(focus, 2.0));
        painter.setBrush(Qt::NoBrush);
        const QRectF ring = circleRect.adjusted(1.0, 1.0, -1.0, -1.0);
        painter.drawEllipse(ring);
    }

    // Indicator hover is handled by the full-row highlight above.

    if (m_checkLevel > 0.01) {
        // Animation: Scale dot from 0 to normal size
        // Outline becomes Accent?
        // Standard Radio:
        // Unchecked: Border Gray, Dot None
        // Checked: Border Accent, Dot Accent.
        
        // Let's animate Border color interpolation?
        // We already drew the base border. 
        // We can draw Access border on top with opacity?
        
        QColor accentBorder = colors.accent;
        accentBorder.setAlphaF(m_checkLevel);
        painter.setPen(QPen(accentBorder, 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(drawRect); // Overlay accent border
        
        // Dot scaling
        qreal dotSize = 10.0 * m_checkLevel; // Max dot size approx 10
        if (dotSize > 0.5) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(colors.accent);
            
            QRectF dotRect(
                circleRect.center().x() - dotSize / 2.0,
                circleRect.center().y() - dotSize / 2.0,
                dotSize, dotSize
            );
            painter.drawEllipse(dotRect);
        }
    }

    painter.setPen(isEnabled() ? colors.text : colors.disabledText);
    const QString elided = fontMetrics().elidedText(text(), Qt::ElideRight, static_cast<int>(textRect.width()));
    painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, elided);
}

void FluentRadioButton::enterEvent(QEvent *event)
{
    QRadioButton::enterEvent(event);
    startHoverAnimation(1.0);
}

void FluentRadioButton::leaveEvent(QEvent *event)
{
    QRadioButton::leaveEvent(event);
    startHoverAnimation(0.0);
}

void FluentRadioButton::focusInEvent(QFocusEvent *event)
{
    QRadioButton::focusInEvent(event);
    startFocusAnimation(1.0);
}

void FluentRadioButton::focusOutEvent(QFocusEvent *event)
{
    QRadioButton::focusOutEvent(event);
    startFocusAnimation(0.0);
}

void FluentRadioButton::startHoverAnimation(qreal endValue)
{
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverLevel);
    m_hoverAnim->setEndValue(endValue);
    m_hoverAnim->start();
}

void FluentRadioButton::startFocusAnimation(qreal endValue)
{
    m_focusAnim->stop();
    m_focusAnim->setStartValue(m_focusLevel);
    m_focusAnim->setEndValue(endValue);
    m_focusAnim->start();
}

} // namespace Fluent
