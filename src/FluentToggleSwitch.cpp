#include "Fluent/FluentToggleSwitch.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"

#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QKeyEvent>

namespace Fluent {

FluentToggleSwitch::FluentToggleSwitch(QWidget *parent)
    : QWidget(parent)
{
    setCursor(Qt::PointingHandCursor);
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);
    setFocusPolicy(Qt::StrongFocus);

    m_progressAnim = new QPropertyAnimation(this, "progress", this);
    m_progressAnim->setDuration(200);
    m_progressAnim->setEasingCurve(QEasingCurve::OutQuad);

    m_hoverAnim = new QPropertyAnimation(this, "hoverLevel", this);
    m_hoverAnim->setDuration(150);
    m_hoverAnim->setEasingCurve(QEasingCurve::OutQuad);

    m_focusAnim = new QPropertyAnimation(this, "focusLevel", this);
    m_focusAnim->setDuration(200);
    m_focusAnim->setEasingCurve(QEasingCurve::OutQuad);

    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentToggleSwitch::applyTheme);
}

FluentToggleSwitch::FluentToggleSwitch(const QString &text, QWidget *parent)
    : QWidget(parent)
    , m_text(text)
{
    setCursor(Qt::PointingHandCursor);
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);
    setFocusPolicy(Qt::StrongFocus);

    m_progressAnim = new QPropertyAnimation(this, "progress", this);
    m_progressAnim->setDuration(200);
    m_progressAnim->setEasingCurve(QEasingCurve::OutQuad);

    m_hoverAnim = new QPropertyAnimation(this, "hoverLevel", this);
    m_hoverAnim->setDuration(150);
    m_hoverAnim->setEasingCurve(QEasingCurve::OutQuad);

    m_focusAnim = new QPropertyAnimation(this, "focusLevel", this);
    m_focusAnim->setDuration(200);
    m_focusAnim->setEasingCurve(QEasingCurve::OutQuad);

    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentToggleSwitch::applyTheme);
}

bool FluentToggleSwitch::isChecked() const
{
    return m_checked;
}

void FluentToggleSwitch::setChecked(bool checked)
{
    if (m_checked == checked) {
        return;
    }

    m_checked = checked;
    m_progressAnim->stop();
    m_progressAnim->setStartValue(m_progress);
    m_progressAnim->setEndValue(m_checked ? 1.0 : 0.0);
    m_progressAnim->start();
    emit toggled(m_checked);
}

void FluentToggleSwitch::setText(const QString &text)
{
    m_text = text;
    update();
}

QSize FluentToggleSwitch::sizeHint() const
{
    const auto m = Style::metrics();
    const QFontMetrics fm(font());
    const int trackWidth = 40;
    const int gap = 10;
    const int rightPad = 6;
    const int textW = m_text.isEmpty() ? 0 : fm.horizontalAdvance(m_text);
    const int w = trackWidth + (m_text.isEmpty() ? 0 : (gap + textW)) + rightPad;
    return QSize(qMax(120, w), m.height);
}

QSize FluentToggleSwitch::minimumSizeHint() const
{
    return sizeHint();
}

qreal FluentToggleSwitch::progress() const
{
    return m_progress;
}

void FluentToggleSwitch::setProgress(qreal value)
{
    m_progress = qBound(0.0, value, 1.0);
    update();
}

qreal FluentToggleSwitch::hoverLevel() const
{
    return m_hoverLevel;
}

void FluentToggleSwitch::setHoverLevel(qreal value)
{
    m_hoverLevel = qBound(0.0, value, 1.0);
    update();
}

qreal FluentToggleSwitch::focusLevel() const
{
    return m_focusLevel;
}

void FluentToggleSwitch::setFocusLevel(qreal value)
{
    m_focusLevel = qBound(0.0, value, 1.0);
    update();
}

void FluentToggleSwitch::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) {
        applyTheme();
    }
}

void FluentToggleSwitch::applyTheme()
{
    update();
}

void FluentToggleSwitch::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    const auto &colors = ThemeManager::instance().colors();

    QPainter painter(this);
    if (!painter.isActive()) {
        return;
    }
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const auto m = Style::metrics();

    // Fluent Design: 40x20 track
    const qreal trackWidth = 40.0;
    const qreal trackHeight = 20.0;
    const qreal radius = trackHeight / 2.0;
    const qreal x = 2.0;
    const qreal y = (height() - trackHeight) / 2.0;

    // Hover background (full row highlight)
    if (m_hoverLevel > 0.01 && isEnabled()) {
        painter.setPen(Qt::NoPen);
        QColor hoverBg = Style::withAlpha(colors.hover, static_cast<int>(90 * m_hoverLevel));
        painter.setBrush(hoverBg);
        const QRectF hoverRect = QRectF(this->rect()).adjusted(1.0, 2.0, -1.0, -2.0);
        painter.drawRoundedRect(hoverRect, 6.0, 6.0);
    }

    // Track colors
    QColor trackColor;
    if (!isEnabled()) {
        trackColor = Style::mix(colors.surface, colors.hover, 0.55);
    } else if (m_checked) {
        trackColor = colors.accent;
    } else {
        trackColor = Style::mix(colors.border, colors.hover, 0.25);
    }

    // Focus ring
    if (isEnabled() && m_focusLevel > 0.01) {
        QColor focus = colors.focus;
        focus.setAlphaF(0.9 * m_focusLevel);
        painter.setPen(QPen(focus, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);
        const QRectF ring(x + 1.5, y + 1.5, trackWidth - 3.0, trackHeight - 3.0);
        painter.drawRoundedRect(ring, (trackHeight - 2.0) / 2.0, (trackHeight - 2.0) / 2.0);
    }

    // Draw track
    painter.setPen(Qt::NoPen);
    painter.setBrush(trackColor);
    painter.drawRoundedRect(QRectF(x, y, trackWidth, trackHeight), radius, radius);

    // Knob parameters - Islands style
    const qreal knobSize = 14.0;
    const qreal knobPadding = 3.0;
    
    // Calculate knob position
    const qreal knobMinX = x + knobPadding;
    const qreal knobMaxX = x + trackWidth - knobPadding - knobSize;
    const qreal knobX = knobMinX + (knobMaxX - knobMinX) * m_progress;
    const qreal knobY = y + (trackHeight - knobSize) / 2.0;
    
    // Draw knob with border (Islands style - no shadow for performance)
    painter.setBrush(Qt::white);
    painter.setPen(QPen(Style::withAlpha(colors.border, 160), 1.0));
    painter.drawEllipse(QRectF(knobX, knobY, knobSize, knobSize));

    // Draw hover ring
    if (m_hoverLevel > 0.01 && isEnabled()) {
        QColor hoverRing = colors.accent;
        hoverRing.setAlphaF(m_hoverLevel * 0.3);
        painter.setPen(Qt::NoPen);
        painter.setBrush(hoverRing);
        const qreal expandSize = 2.0 * m_hoverLevel;
        painter.drawEllipse(QRectF(knobX - expandSize, knobY - expandSize, 
                                   knobSize + expandSize * 2, knobSize + expandSize * 2));
    }

    // Draw text label
    if (!m_text.isEmpty()) {
        painter.setPen(isEnabled() ? colors.text : colors.disabledText);
        painter.setFont(this->font());
        const QRect textRect(static_cast<int>(x + trackWidth + 10), 0, width() - static_cast<int>(x + trackWidth + 10), height());
        const QString elided = fontMetrics().elidedText(m_text, Qt::ElideRight, textRect.width());
        painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, elided);
    }
}

void FluentToggleSwitch::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        setChecked(!m_checked);
    }

    QWidget::mousePressEvent(event);
}

void FluentToggleSwitch::focusInEvent(QFocusEvent *event)
{
    QWidget::focusInEvent(event);
    startFocusAnimation(1.0);
}

void FluentToggleSwitch::focusOutEvent(QFocusEvent *event)
{
    QWidget::focusOutEvent(event);
    startFocusAnimation(0.0);
}

void FluentToggleSwitch::startFocusAnimation(qreal endValue)
{
    m_focusAnim->stop();
    m_focusAnim->setStartValue(m_focusLevel);
    m_focusAnim->setEndValue(endValue);
    m_focusAnim->start();
}

void FluentToggleSwitch::enterEvent(QEvent *event)
{
    QWidget::enterEvent(event);
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverLevel);
    m_hoverAnim->setEndValue(1.0);
    m_hoverAnim->start();
}

void FluentToggleSwitch::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverLevel);
    m_hoverAnim->setEndValue(0.0);
    m_hoverAnim->start();
}

} // namespace Fluent
