#include "Fluent/FluentCheckBox.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"

#include <QEvent>
#include <QPainter>
#include <QVariantAnimation>
#include <QEasingCurve>
#include <QPainterPath>
namespace Fluent {

FluentCheckBox::FluentCheckBox(QWidget *parent)
    : QCheckBox(parent)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);
    setFocusPolicy(Qt::StrongFocus);

    m_hoverAnim = new QVariantAnimation(this);
    m_hoverAnim->setDuration(150);
    m_hoverAnim->setEasingCurve(QEasingCurve::OutQuad);
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
    m_checkAnim->setEasingCurve(QEasingCurve::OutQuad);
    connect(m_checkAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_checkLevel = value.toReal();
        update();
    });
    connect(this, &QCheckBox::stateChanged, this, [this](int state) {
        m_checkAnim->stop();
        m_checkAnim->setStartValue(m_checkLevel);
        m_checkAnim->setEndValue(state == Qt::Checked ? 1.0 : 0.0);
        m_checkAnim->start();
    });
    m_checkLevel = isChecked() ? 1.0 : 0.0;

    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentCheckBox::applyTheme);
}

FluentCheckBox::FluentCheckBox(const QString &text, QWidget *parent)
    : QCheckBox(text, parent)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);
    setFocusPolicy(Qt::StrongFocus);

    m_hoverAnim = new QVariantAnimation(this);
    m_hoverAnim->setDuration(150);
    m_hoverAnim->setEasingCurve(QEasingCurve::OutQuad);
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
    m_checkAnim->setEasingCurve(QEasingCurve::OutQuad);
    connect(m_checkAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_checkLevel = value.toReal();
        update();
    });
    connect(this, &QCheckBox::stateChanged, this, [this](int state) {
        m_checkAnim->stop();
        m_checkAnim->setStartValue(m_checkLevel);
        m_checkAnim->setEndValue(state == Qt::Checked ? 1.0 : 0.0);
        m_checkAnim->start();
    });
    m_checkLevel = isChecked() ? 1.0 : 0.0;

    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentCheckBox::applyTheme);
}

QSize FluentCheckBox::sizeHint() const
{
    const QFontMetrics fm(font());
    const int boxSize = 20;
    const int left = 2;
    const int gap = 10;
    const int rightPad = 6;
    const int w = left + boxSize + gap + fm.horizontalAdvance(text()) + rightPad;
    const int h = qMax(boxSize + 4, fm.height() + 8);
    return {w, h};
}

QSize FluentCheckBox::minimumSizeHint() const
{
    return sizeHint();
}

qreal FluentCheckBox::hoverLevel() const
{
    return m_hoverLevel;
}

void FluentCheckBox::setHoverLevel(qreal value)
{
    m_hoverLevel = qBound(0.0, value, 1.0);
    update();
}

qreal FluentCheckBox::focusLevel() const
{
    return m_focusLevel;
}

void FluentCheckBox::setFocusLevel(qreal value)
{
    m_focusLevel = qBound(0.0, value, 1.0);
    update();
}

void FluentCheckBox::changeEvent(QEvent *event)
{
    QCheckBox::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) {
        applyTheme();
    }
}

void FluentCheckBox::applyTheme()
{
    update();
}

void FluentCheckBox::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    const auto &colors = ThemeManager::instance().colors();

    QPainter painter(this);
    if (!painter.isActive()) {
        return;
    }
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // Fluent Design: 20x20px checkbox
    const qreal boxSize = 20.0;
    const qreal cornerRadius = 3.0;
    const qreal leftPad = 2.0;
    
    // Left-align with proper spacing
    QRectF boxRect(leftPad, (height() - boxSize) / 2.0, boxSize, boxSize);
    QRectF textRect = QRectF(this->rect()).adjusted(static_cast<int>(leftPad + boxSize + 10), 0, -4, 0);

    // Hover background (full row highlight)
    if (m_hoverLevel > 0.01 && isEnabled()) {
        painter.setPen(Qt::NoPen);
        QColor hoverBg = Style::withAlpha(colors.hover, static_cast<int>(90 * m_hoverLevel));
        painter.setBrush(hoverBg);
        const QRectF hoverRect = QRectF(this->rect()).adjusted(1.0, 2.0, -1.0, -2.0);
        painter.drawRoundedRect(hoverRect, 6.0, 6.0);
    }

    // Determine checkbox state colors
    QColor borderColor = colors.border;
    QColor fillColor = colors.surface;
    
    if (!isEnabled()) {
        borderColor = colors.border;
        fillColor = colors.hover;
    } else if (m_checkLevel > 0.01) {
        // Checked state: use accent color
        borderColor = colors.accent;
        fillColor = colors.accent;
    }

    // Draw checkbox box
    painter.setPen(QPen(borderColor, 1.5));
    
    if (m_checkLevel > 0.01) {
        // Checked: filled with accent
        QColor accentFill = fillColor;
        accentFill.setAlphaF(m_checkLevel);
        painter.setBrush(accentFill);
    } else {
        // Unchecked: white background
        painter.setBrush(fillColor);
    }
    
    painter.drawRoundedRect(boxRect.adjusted(0.75, 0.75, -0.75, -0.75), cornerRadius, cornerRadius);

    // Focus ring (keyboard focus)
    if (isEnabled() && m_focusLevel > 0.01) {
        QColor focus = colors.focus;
        focus.setAlphaF(0.9 * m_focusLevel);
        painter.setPen(QPen(focus, 2.0));
        painter.setBrush(Qt::NoBrush);
        const QRectF ring = boxRect.adjusted(1.0, 1.0, -1.0, -1.0);
        painter.drawRoundedRect(ring, cornerRadius, cornerRadius);
    }

    // Draw checkmark with animation
    if (m_checkLevel > 0.01) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(Qt::NoBrush);
        
        QColor checkColor = Qt::white;
        checkColor.setAlphaF(m_checkLevel);
        painter.setPen(QPen(checkColor, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        
        // Fluent-style checkmark path
        QPainterPath checkPath;
        QPointF p1(boxRect.left() + boxSize * 0.25, boxRect.top() + boxSize * 0.50);
        QPointF p2(boxRect.left() + boxSize * 0.45, boxRect.top() + boxSize * 0.70);
        QPointF p3(boxRect.left() + boxSize * 0.75, boxRect.top() + boxSize * 0.30);
        
        checkPath.moveTo(p1);
        checkPath.lineTo(p2);
        checkPath.lineTo(p3);
        
        painter.drawPath(checkPath);
    }

    // Draw text label
    if (!text().isEmpty()) {
        painter.setPen(isEnabled() ? colors.text : colors.disabledText);
        painter.setBrush(Qt::NoBrush);
        QFont font = this->font();
        painter.setFont(font);
        const QString elided = fontMetrics().elidedText(text(), Qt::ElideRight, static_cast<int>(textRect.width()));
        painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft | Qt::TextShowMnemonic, elided);
    }
}

void FluentCheckBox::enterEvent(QEvent *event)
{
    QCheckBox::enterEvent(event);
    startHoverAnimation(1.0);
}

void FluentCheckBox::leaveEvent(QEvent *event)
{
    QCheckBox::leaveEvent(event);
    startHoverAnimation(0.0);
}

void FluentCheckBox::focusInEvent(QFocusEvent *event)
{
    QCheckBox::focusInEvent(event);
    startFocusAnimation(1.0);
}

void FluentCheckBox::focusOutEvent(QFocusEvent *event)
{
    QCheckBox::focusOutEvent(event);
    startFocusAnimation(0.0);
}

void FluentCheckBox::startHoverAnimation(qreal endValue)
{
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverLevel);
    m_hoverAnim->setEndValue(endValue);
    m_hoverAnim->start();
}

void FluentCheckBox::startFocusAnimation(qreal endValue)
{
    m_focusAnim->stop();
    m_focusAnim->setStartValue(m_focusLevel);
    m_focusAnim->setEndValue(endValue);
    m_focusAnim->start();
}

} // namespace Fluent
