#include "Fluent/FluentToolButton.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"

#include <QEvent>
#include <QPainter>
#include <QStyleOptionToolButton>
#include <QVariantAnimation>

namespace Fluent {

FluentToolButton::FluentToolButton(QWidget *parent)
    : QToolButton(parent)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);
    setAutoRaise(true);
    setMinimumHeight(Style::metrics().height);

    m_hoverAnim = new QVariantAnimation(this);
    m_hoverAnim->setDuration(120);
    connect(m_hoverAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_hoverLevel = value.toReal();
        update();
    });

    m_pressAnim = new QVariantAnimation(this);
    m_pressAnim->setDuration(90);
    connect(m_pressAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_pressLevel = value.toReal();
        update();
    });

    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentToolButton::applyTheme);
    connect(this, &QAbstractButton::toggled, this, QOverload<>::of(&FluentToolButton::update));
}

FluentToolButton::FluentToolButton(const QString &text, QWidget *parent)
    : QToolButton(parent)
{
    setText(text);
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);
    setAutoRaise(true);
    setMinimumHeight(Style::metrics().height);

    m_hoverAnim = new QVariantAnimation(this);
    m_hoverAnim->setDuration(120);
    connect(m_hoverAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_hoverLevel = value.toReal();
        update();
    });

    m_pressAnim = new QVariantAnimation(this);
    m_pressAnim->setDuration(90);
    connect(m_pressAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_pressLevel = value.toReal();
        update();
    });

    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentToolButton::applyTheme);
    connect(this, &QAbstractButton::toggled, this, QOverload<>::of(&FluentToolButton::update));
}

qreal FluentToolButton::hoverLevel() const
{
    return m_hoverLevel;
}

void FluentToolButton::setHoverLevel(qreal value)
{
    m_hoverLevel = qBound(0.0, value, 1.0);
    update();
}

qreal FluentToolButton::pressLevel() const
{
    return m_pressLevel;
}

void FluentToolButton::setPressLevel(qreal value)
{
    m_pressLevel = qBound(0.0, value, 1.0);
    update();
}

void FluentToolButton::changeEvent(QEvent *event)
{
    QToolButton::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) {
        applyTheme();
    }
}

void FluentToolButton::applyTheme()
{
    update();
}

void FluentToolButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    const auto &colors = ThemeManager::instance().colors();

    const auto m = Style::metrics();

    QPainter painter(this);
    if (!painter.isActive()) {
        return;
    }
    painter.setRenderHint(QPainter::Antialiasing, true);

    QRect rect = this->rect().adjusted(0, 0, -1, -1);

    // Caption / window buttons (used by FluentMainWindow)
    const QVariant glyphVar = property("fluentWindowGlyph");
    if (glyphVar.isValid()) {
        const int glyph = glyphVar.toInt();
        const bool isClose = (glyph == 3);

        const qreal hover = isEnabled() ? m_hoverLevel : 0.0;
        const qreal press = isEnabled() ? m_pressLevel : 0.0;

        QColor fill = Qt::transparent;
        if (isClose) {
            // Win11-like close hover: vivid red hover, darker pressed.
            QColor danger = colors.error.isValid() ? colors.error : QColor("#E81123");
            QColor dangerPressed = danger.darker(125);
            danger.setAlphaF(0.92);
            dangerPressed.setAlphaF(0.97);
            fill = Style::mix(fill, danger, hover);
            fill = Style::mix(fill, dangerPressed, press);
        } else {
            // Subtle hover/press like caption buttons (lighter than normal controls)
            QColor h = colors.hover;
            h.setAlphaF(0.58);
            QColor pr = colors.pressed;
            pr.setAlphaF(0.78);
            fill = Style::mix(fill, h, hover);
            fill = Style::mix(fill, pr, press);
        }

        painter.setPen(Qt::NoPen);
        painter.setBrush(fill);
        painter.drawRoundedRect(QRectF(rect).adjusted(0.5, 0.5, -0.5, -0.5), 4.0, 4.0);

        QColor glyphColor = Style::withAlpha(colors.text, 235);
        if (isClose && (hover > 0.01 || press > 0.01)) {
            glyphColor = QColor(255, 255, 255, 245);
        }

        const qreal strokeW = 1.6;
        painter.setPen(QPen(glyphColor, strokeW, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);

        // Use a fixed square glyph box so icons stay 16x16 and never look squashed
        // when the caption button is wide.
        const QPointF center = QRectF(rect).center();
        const QRectF glyphBox(center.x() - 8.0, center.y() - 8.0, 16.0, 16.0);
        const qreal pad = 3.0;
        const QRectF r = glyphBox.adjusted(pad, pad, -pad, -pad);

        switch (glyph) {
        case 0: // minimize
            painter.drawLine(QPointF(r.left(), r.bottom()), QPointF(r.right(), r.bottom()));
            break;
        case 1: // maximize
            painter.drawRoundedRect(r.adjusted(0.7, 0.7, -0.7, -0.7), 1.2, 1.2);
            break;
        case 2: // restore
            {
                // Draw two windows with a visible offset so the glyph doesn't look like
                // perfectly stacked squares.
                const qreal offset = 1.6;
                const qreal shrink = 0.9;
                const QRectF base = r.adjusted(shrink, shrink, -shrink, -shrink);
                const QRectF back = base.translated(offset, -offset);
                const QRectF front = base.translated(-offset, offset);
                painter.drawRoundedRect(back, 1.2, 1.2);
                painter.drawRoundedRect(front, 1.2, 1.2);
            }
            break;
        case 3: // close
            painter.drawLine(QPointF(r.left(), r.top()), QPointF(r.right(), r.bottom()));
            painter.drawLine(QPointF(r.right(), r.top()), QPointF(r.left(), r.bottom()));
            break;
        default:
            break;
        }
        return;
    }

    QColor base = colors.surface;
    QColor hover = Style::mix(colors.surface, colors.hover, 0.88);
    QColor pressed = Style::mix(colors.surface, colors.pressed, 0.92);
    const bool checked = isCheckable() && isChecked();
    if (checked) {
        // Fluent-like toggle toolbutton: subtle accent tint + accent border.
        const QColor accentTint = Style::mix(colors.surface, colors.accent, 0.16);
        base = accentTint;
        hover = Style::mix(accentTint, colors.accent, 0.10);
        pressed = Style::mix(accentTint, colors.accent, 0.18);
    }
    QColor fill = Style::mix(base, hover, m_hoverLevel);
    fill = Style::mix(fill, pressed, m_pressLevel);
    QColor border = checked ? Style::mix(colors.border, colors.accent, 0.92) : colors.border;

    if (!isEnabled()) {
        fill = Style::mix(colors.surface, colors.hover, 0.45);
        border = Style::mix(colors.border, colors.disabledText, 0.25);
    }

    painter.setPen(QPen(border, 1));
    painter.setBrush(fill);
    painter.drawRoundedRect(QRectF(rect).adjusted(0.5, 0.5, -0.5, -0.5), m.radius, m.radius);

    if (hasFocus() && isEnabled()) {
        QColor focus = colors.focus;
        focus.setAlpha(230);
        painter.setPen(QPen(focus, 2.0));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(QRectF(rect).adjusted(1.0, 1.0, -1.0, -1.0), m.radius - 1, m.radius - 1);
    }

    QStyleOptionToolButton opt;
    initStyleOption(&opt);
    opt.state &= ~QStyle::State_MouseOver;
    opt.state &= ~QStyle::State_Sunken;
    opt.state &= ~QStyle::State_On;
    if (checked && isEnabled()) {
        // Make the checked state more obvious by tinting label color.
        opt.palette.setColor(QPalette::ButtonText, Style::mix(colors.text, colors.accent, 0.90));
        opt.palette.setColor(QPalette::Text, Style::mix(colors.text, colors.accent, 0.90));
    }
    style()->drawControl(QStyle::CE_ToolButtonLabel, &opt, &painter, this);
}

void FluentToolButton::enterEvent(QEvent *event)
{
    QToolButton::enterEvent(event);
    startHoverAnimation(1.0);
}

void FluentToolButton::leaveEvent(QEvent *event)
{
    QToolButton::leaveEvent(event);
    startHoverAnimation(0.0);
}

void FluentToolButton::mousePressEvent(QMouseEvent *event)
{
    startPressAnimation(1.0);
    QToolButton::mousePressEvent(event);
}

void FluentToolButton::mouseReleaseEvent(QMouseEvent *event)
{
    startPressAnimation(0.0);
    QToolButton::mouseReleaseEvent(event);
}

void FluentToolButton::startHoverAnimation(qreal endValue)
{
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverLevel);
    m_hoverAnim->setEndValue(endValue);
    m_hoverAnim->start();
}

void FluentToolButton::startPressAnimation(qreal endValue)
{
    m_pressAnim->stop();
    m_pressAnim->setStartValue(m_pressLevel);
    m_pressAnim->setEndValue(endValue);
    m_pressAnim->start();
}

} // namespace Fluent
