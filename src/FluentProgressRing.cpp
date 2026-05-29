#include "Fluent/FluentProgressRing.h"
#include "Fluent/FluentMotion.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"
#include "FluentPaintSupport.h"

#include <QEvent>
#include <QHideEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QShowEvent>
#include <QTimerEvent>
#include <cmath>

namespace Fluent {

FluentProgressRing::FluentProgressRing(QWidget *parent)
    : QProgressBar(parent)
{
    setTextVisible(false);
    setRange(0, 100);
    setValue(0);
    setMinimumSize(32, 32);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    m_valueAnim = new QPropertyAnimation(this, "displayValue", this);

    connect(this, &QProgressBar::valueChanged, this, [this](int newValue) {
        if (isIndeterminate()) {
            update();
            return;
        }
        m_valueAnim->stop();
        const qreal target = static_cast<qreal>(newValue);
        if (m_valueAnim->duration() <= 0) {
            setDisplayValue(target);
        } else {
            m_valueAnim->setStartValue(m_displayValue);
            m_valueAnim->setEndValue(target);
            m_valueAnim->start();
        }
    });

    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentProgressRing::applyTheme);
}

qreal FluentProgressRing::displayValue() const
{
    return m_displayValue;
}

void FluentProgressRing::setDisplayValue(qreal value)
{
    m_displayValue = value;
    update();
}

qreal FluentProgressRing::rotationAngle() const
{
    return m_rotationAngle;
}

void FluentProgressRing::setRotationAngle(qreal angle)
{
    m_rotationAngle = std::fmod(angle, 360.0);
    if (m_rotationAngle < 0.0) {
        m_rotationAngle += 360.0;
    }
    update();
}

qreal FluentProgressRing::ringWidth() const
{
    return m_ringWidth;
}

void FluentProgressRing::setRingWidth(qreal width)
{
    m_ringWidth = qBound<qreal>(2.0, width, 12.0);
    update();
}

bool FluentProgressRing::isIndeterminate() const
{
    return m_indeterminate || (minimum() == 0 && maximum() == 0);
}

void FluentProgressRing::setIndeterminate(bool indeterminate)
{
    if (m_indeterminate == indeterminate) {
        return;
    }
    m_indeterminate = indeterminate;
    syncSpinTimer();
    update();
}

void FluentProgressRing::changeEvent(QEvent *event)
{
    QProgressBar::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) {
        syncSpinTimer();
        applyTheme();
    }
}

void FluentProgressRing::hideEvent(QHideEvent *event)
{
    QProgressBar::hideEvent(event);
    syncSpinTimer();
}

void FluentProgressRing::showEvent(QShowEvent *event)
{
    QProgressBar::showEvent(event);
    syncSpinTimer();
}

void FluentProgressRing::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_spinTimer.timerId()) {
        setRotationAngle(m_rotationAngle + 7.5);
        return;
    }
    QProgressBar::timerEvent(event);
}

void FluentProgressRing::applyTheme()
{
    FluentMotion::configure(m_valueAnim, FluentMotionRole::Selection);
    syncSpinTimer();
    update();
}

void FluentProgressRing::syncSpinTimer()
{
    const bool shouldSpin = isVisible()
        && isEnabled()
        && isIndeterminate()
        && ThemeManager::instance().animationsEnabled();
    if (shouldSpin && !m_spinTimer.isActive()) {
        m_spinTimer.start(16, this);
    } else if (!shouldSpin && m_spinTimer.isActive()) {
        m_spinTimer.stop();
    }
}

void FluentProgressRing::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    if (!Detail::canBeginWidgetPainter(this)) {
        return;
    }

    const auto &colors = ThemeManager::instance().colors();

    QPainter painter(this);
    if (!painter.isActive()) {
        return;
    }

    painter.setRenderHint(QPainter::Antialiasing, true);

    const qreal side = qMin(width(), height()) - m_ringWidth - 1.0;
    if (side <= 0.0) {
        return;
    }

    QRectF ringRect((width() - side) / 2.0, (height() - side) / 2.0, side, side);

    QColor track = colors.border;
    track.setAlpha(isEnabled() ? 145 : 90);
    QColor accent = isEnabled() ? colors.accent : colors.disabledText;

    painter.setPen(QPen(track, m_ringWidth, Qt::SolidLine, Qt::RoundCap));
    painter.setBrush(Qt::NoBrush);
    painter.drawArc(ringRect, 0, 360 * 16);

    painter.setPen(QPen(accent, m_ringWidth, Qt::SolidLine, Qt::RoundCap));
    if (isIndeterminate()) {
        const int start = static_cast<int>((90.0 - m_rotationAngle) * 16.0);
        painter.drawArc(ringRect, start, -105 * 16);
        return;
    }

    const int minVal = minimum();
    const int maxVal = maximum();
    const qreal range = qMax(1, maxVal - minVal);
    const qreal progress = qBound<qreal>(0.0, (m_displayValue - minVal) / range, 1.0);
    painter.drawArc(ringRect, 90 * 16, static_cast<int>(-progress * 360.0 * 16.0));
}

} // namespace Fluent
