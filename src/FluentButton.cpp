#include "Fluent/FluentButton.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"

#include <QEvent>
#include <QPainter>
#include <QStyleOptionButton>
#include <QVariantAnimation>
#include <QEasingCurve>

namespace Fluent {

FluentButton::FluentButton(QWidget *parent)
    : QPushButton(parent)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);
    setAutoDefault(false);
    setMinimumHeight(Style::metrics().height);

    m_hoverAnim = new QVariantAnimation(this);
    m_hoverAnim->setDuration(150);
    m_hoverAnim->setEasingCurve(QEasingCurve::OutQuad);
    connect(m_hoverAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_hoverLevel = value.toReal();
        update();
    });

    m_pressAnim = new QVariantAnimation(this);
    m_pressAnim->setDuration(100);
    m_pressAnim->setEasingCurve(QEasingCurve::OutQuad);
    connect(m_pressAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_pressLevel = value.toReal();
        update();
    });

    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentButton::applyTheme);
    connect(this, &QAbstractButton::toggled, this, QOverload<>::of(&FluentButton::update));
}

FluentButton::FluentButton(const QString &text, QWidget *parent)
    : QPushButton(text, parent)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);
    setAutoDefault(false);
    setMinimumHeight(Style::metrics().height);

    m_hoverAnim = new QVariantAnimation(this);
    m_hoverAnim->setDuration(150);
    m_hoverAnim->setEasingCurve(QEasingCurve::OutQuad);
    connect(m_hoverAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_hoverLevel = value.toReal();
        update();
    });

    m_pressAnim = new QVariantAnimation(this);
    m_pressAnim->setDuration(100);
    m_pressAnim->setEasingCurve(QEasingCurve::OutQuad);
    connect(m_pressAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_pressLevel = value.toReal();
        update();
    });

    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentButton::applyTheme);
    connect(this, &QAbstractButton::toggled, this, QOverload<>::of(&FluentButton::update));
}

bool FluentButton::isPrimary() const
{
    return m_primary;
}

void FluentButton::setPrimary(bool primary)
{
    if (m_primary == primary) {
        return;
    }

    m_primary = primary;
    applyTheme();
}

qreal FluentButton::hoverLevel() const
{
    return m_hoverLevel;
}

void FluentButton::setHoverLevel(qreal value)
{
    m_hoverLevel = qBound(0.0, value, 1.0);
    update();
}

qreal FluentButton::pressLevel() const
{
    return m_pressLevel;
}

void FluentButton::setPressLevel(qreal value)
{
    m_pressLevel = qBound(0.0, value, 1.0);
    update();
}

void FluentButton::changeEvent(QEvent *event)
{
    QPushButton::changeEvent(event);

    if (event->type() == QEvent::EnabledChange) {
        applyTheme();
    }
}

void FluentButton::applyTheme()
{
    update();
}

void FluentButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    const auto &colors = ThemeManager::instance().colors();

    const bool checked = isCheckable() && isChecked();

    QColor base;
    QColor hover;
    QColor pressed;
    QColor border;
    QColor textColor;

    if (m_primary) {
        // Primary buttons: accent-filled. In checked state, stay accent but appear "selected".
        base = checked ? colors.accent.darker(125) : colors.accent;
        hover = base.lighter(118);
        pressed = base.darker(118);
        border = base.darker(110);
        textColor = QColor("#FFFFFF");
    } else {
        // Secondary buttons: neutral surface. In checked state, use a subtle accent tint and accent border.
        const QColor accentTint = Style::mix(colors.surface, colors.accent, 0.12);
        base = checked ? accentTint : colors.surface;
        hover = checked
                    ? Style::mix(accentTint, colors.accent, 0.10)
                    : Style::mix(colors.surface, colors.hover, 0.88);
        pressed = checked
                      ? Style::mix(accentTint, colors.accent, 0.18)
                      : Style::mix(colors.surface, colors.pressed, 0.92);
        border = checked ? Style::mix(colors.border, colors.accent, 0.85) : colors.border;
        // Fluent-like toggle detail: checked secondary button uses accent-tinted text.
        textColor = checked ? Style::mix(colors.text, colors.accent, 0.82) : colors.text;
    }

    QColor fill = Style::mix(base, hover, m_hoverLevel);
    fill = Style::mix(fill, pressed, m_pressLevel);

    if (!isEnabled()) {
        fill = Style::mix(colors.surface, colors.hover, 0.45);
        border = Style::mix(colors.border, colors.disabledText, 0.25);
        textColor = colors.disabledText;
    }

    QPainter painter(this);
    if (!painter.isActive()) {
        return;
    }
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    
    const auto m = Style::metrics();
    QRectF rect = QRectF(this->rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    const qreal radius = m.radius;

    painter.setPen(QPen(border, 1.0));
    painter.setBrush(fill);
    painter.drawRoundedRect(rect, radius, radius);

    // Fluent-like checked detail (without indicator bar): add a subtle inner highlight so
    // the selected state is still obvious on accent-filled primary buttons.
    if (checked && m_primary && isEnabled()) {
        QColor inner = QColor(255, 255, 255, 115);
        painter.setPen(QPen(inner, 1.0));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(rect.adjusted(1.0, 1.0, -1.0, -1.0), radius - 1, radius - 1);
    }

    if (hasFocus() && isEnabled()) {
        QColor focus = colors.focus;
        focus.setAlpha(230);
        painter.setPen(QPen(focus, 2.0));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(rect.adjusted(1, 1, -1, -1), radius - 1, radius - 1);
    }

    // Draw text
    painter.setPen(textColor);
    painter.setFont(this->font());
    
    // Calculate text and icon layout
    QRect contentRect = rect.toRect();
    
    if (!icon().isNull()) {
        // Button has icon - draw icon on the left
        QIcon::Mode mode = isEnabled() ? QIcon::Normal : QIcon::Disabled;
        QPixmap pixmap = icon().pixmap(iconSize(), mode);
        
        const int gap = 8;
        int totalWidth = iconSize().width() + gap + fontMetrics().horizontalAdvance(text());
        int startX = contentRect.center().x() - totalWidth / 2;
        
        QRect iconRect(startX, contentRect.center().y() - iconSize().height() / 2,
                      iconSize().width(), iconSize().height());
        painter.drawPixmap(iconRect, pixmap);
        
        QRect textRect(startX + iconSize().width() + gap, contentRect.top(),
                  contentRect.width() - (startX + iconSize().width() + gap), contentRect.height());
        painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic, text());
    } else {
        // No icon - center text
        painter.drawText(contentRect, Qt::AlignCenter | Qt::AlignVCenter | Qt::TextShowMnemonic, text());
    }
}

void FluentButton::enterEvent(QEvent *event)
{
    QPushButton::enterEvent(event);
    startHoverAnimation(1.0);
}

void FluentButton::leaveEvent(QEvent *event)
{
    QPushButton::leaveEvent(event);
    startHoverAnimation(0.0);
}

void FluentButton::mousePressEvent(QMouseEvent *event)
{
    startPressAnimation(1.0);
    QPushButton::mousePressEvent(event);
}

void FluentButton::mouseReleaseEvent(QMouseEvent *event)
{
    startPressAnimation(0.0);
    QPushButton::mouseReleaseEvent(event);
}

void FluentButton::startHoverAnimation(qreal endValue)
{
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverLevel);
    m_hoverAnim->setEndValue(endValue);
    m_hoverAnim->start();
}

void FluentButton::startPressAnimation(qreal endValue)
{
    m_pressAnim->stop();
    m_pressAnim->setStartValue(m_pressLevel);
    m_pressAnim->setEndValue(endValue);
    m_pressAnim->start();
}

} // namespace Fluent
