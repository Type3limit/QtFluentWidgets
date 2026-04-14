#include "Fluent/FluentTimePicker.h"

#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"
#include "datePicker/FluentWheelPickerSupport.h"

#include <QEvent>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QLocale>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>
#include <QVariantAnimation>

#include <climits>

namespace Fluent {

namespace {

QLocale defaultPickerLocale()
{
    return QLocale();
}

int nearestMinuteValue(int minute, int increment)
{
    const int clampedIncrement = qBound(1, increment, 59);
    int nearest = 0;
    int nearestDistance = INT_MAX;
    for (int value = 0; value < 60; value += clampedIncrement) {
        const int distance = qAbs(value - minute);
        if (distance < nearestDistance) {
            nearestDistance = distance;
            nearest = value;
        }
    }
    return nearest;
}

qreal timeFieldWeight(bool use24HourClock, int index)
{
    if (use24HourClock) {
        return index == 0 ? 1.0 : 1.0;
    }
    return index == 2 ? 0.9 : 1.0;
}

} // namespace

FluentTimePicker::FluentTimePicker(QWidget *parent)
    : QTimeEdit(parent)
{
    QTimeEdit::setDisplayFormat(QStringLiteral("h:mm AP"));
    setButtonSymbols(QAbstractSpinBox::NoButtons);
    setFrame(false);
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);
    setMinimumHeight(Style::metrics().height);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    setLocale(defaultPickerLocale());

    m_hourPlaceholderText = tr("时");
    m_minutePlaceholderText = tr("分");
    m_anteMeridiemText = tr("上午");
    m_postMeridiemText = tr("下午");

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

    connect(this, &QTimeEdit::timeChanged, this, [this](const QTime &) {
        if (m_blockTimeChangedMarker) {
            return;
        }
        m_hasTime = true;
        update();
    });

    QTimer::singleShot(0, this, [this]() {
        ensureEditorHidden();
    });

    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentTimePicker::applyTheme);
}

FluentTimePicker::~FluentTimePicker() = default;

qreal FluentTimePicker::hoverLevel() const
{
    return m_hoverLevel;
}

void FluentTimePicker::setHoverLevel(qreal value)
{
    m_hoverLevel = qBound(0.0, value, 1.0);
    update();
}

qreal FluentTimePicker::focusLevel() const
{
    return m_focusLevel;
}

void FluentTimePicker::setFocusLevel(qreal value)
{
    m_focusLevel = qBound(0.0, value, 1.0);
    update();
}

void FluentTimePicker::setTime(const QTime &time)
{
    if (!time.isValid()) {
        clearTime();
        return;
    }

    const bool hadTime = m_hasTime;
    const QTime previous = QTimeEdit::time();

    m_blockTimeChangedMarker = true;
    QTimeEdit::setTime(time);
    m_blockTimeChangedMarker = false;

    m_hasTime = true;
    update();

    if ((!hadTime || previous == time) && previous == time) {
        emit timeChanged(time);
    }
}

QTime FluentTimePicker::time() const
{
    return m_hasTime ? QTimeEdit::time() : QTime();
}

void FluentTimePicker::clearTime()
{
    if (!m_hasTime) {
        return;
    }

    m_hasTime = false;
    update();
}

bool FluentTimePicker::hasTime() const
{
    return m_hasTime;
}

void FluentTimePicker::setUse24HourClock(bool enabled)
{
    if (m_use24HourClock == enabled) {
        return;
    }

    m_use24HourClock = enabled;
    QTimeEdit::setDisplayFormat(m_use24HourClock ? QStringLiteral("HH:mm") : QStringLiteral("h:mm AP"));
    if (isPopupVisible()) {
        rebuildPopupColumns(popupTimeFromColumns());
    }
    updateGeometry();
    update();
}

bool FluentTimePicker::use24HourClock() const
{
    return m_use24HourClock;
}

void FluentTimePicker::setMinuteIncrement(int minutes)
{
    const int clamped = qBound(1, minutes, 59);
    if (m_minuteIncrement == clamped) {
        return;
    }

    m_minuteIncrement = clamped;
    if (isPopupVisible()) {
        rebuildPopupColumns(popupTimeFromColumns());
    }
    update();
}

int FluentTimePicker::minuteIncrement() const
{
    return m_minuteIncrement;
}

void FluentTimePicker::setHourPlaceholderText(const QString &text)
{
    if (m_hourPlaceholderText == text) {
        return;
    }

    m_hourPlaceholderText = text;
    update();
}

QString FluentTimePicker::hourPlaceholderText() const
{
    return m_hourPlaceholderText;
}

void FluentTimePicker::setMinutePlaceholderText(const QString &text)
{
    if (m_minutePlaceholderText == text) {
        return;
    }

    m_minutePlaceholderText = text;
    update();
}

QString FluentTimePicker::minutePlaceholderText() const
{
    return m_minutePlaceholderText;
}

void FluentTimePicker::setAnteMeridiemText(const QString &text)
{
    if (m_anteMeridiemText == text) {
        return;
    }

    m_anteMeridiemText = text;
    if (isPopupVisible()) {
        rebuildPopupColumns(popupTimeFromColumns());
    }
    update();
}

QString FluentTimePicker::anteMeridiemText() const
{
    return m_anteMeridiemText;
}

void FluentTimePicker::setPostMeridiemText(const QString &text)
{
    if (m_postMeridiemText == text) {
        return;
    }

    m_postMeridiemText = text;
    if (isPopupVisible()) {
        rebuildPopupColumns(popupTimeFromColumns());
    }
    update();
}

QString FluentTimePicker::postMeridiemText() const
{
    return m_postMeridiemText;
}

void FluentTimePicker::changeEvent(QEvent *event)
{
    QTimeEdit::changeEvent(event);

    if (!event) {
        return;
    }

    if (event->type() == QEvent::EnabledChange || event->type() == QEvent::LocaleChange) {
        applyTheme();
    }
}

void FluentTimePicker::applyTheme()
{
    const QString next = QStringLiteral("QTimeEdit { background: transparent; color: transparent; border: none; }"
                                        "QTimeEdit::up-button, QTimeEdit::down-button { width: 0px; border: none; }"
                                        "QTimeEdit::up-arrow, QTimeEdit::down-arrow { width: 0px; height: 0px; }");
    if (styleSheet() != next) {
        setStyleSheet(next);
    }

    ensureEditorHidden();
    if (m_popup) {
        m_popup->update();
    }
    update();
}

void FluentTimePicker::ensureEditorHidden()
{
    QLineEdit *le = findChild<QLineEdit *>();
    if (!le) {
        return;
    }

    if (m_editor != le) {
        m_editor = le;
    }
    m_editor->setFrame(false);
    m_editor->setReadOnly(true);
    m_editor->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_editor->hide();
}

void FluentTimePicker::resizeEvent(QResizeEvent *event)
{
    QTimeEdit::resizeEvent(event);
    ensureEditorHidden();
}

void FluentTimePicker::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    const auto &colors = ThemeManager::instance().colors();

    const bool enabled = isEnabled();

    QPainter painter(this);
    if (!painter.isActive()) {
        return;
    }
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF surfaceRect = QRectF(this->rect());
    Style::paintControlSurface(painter, surfaceRect, colors, m_hoverLevel, m_focusLevel, enabled, isPopupVisible());

    const auto m = Style::metrics();
    const QRect buttonRect(width() - m.iconAreaWidth, 0, m.iconAreaWidth, height());
    const int fieldCount = m_use24HourClock ? 2 : 3;
    const QRect contentRect = surfaceRect.toRect().adjusted(8, 4, -m.iconAreaWidth - 6, -4);

    QColor divider = colors.border;
    divider.setAlpha(80);
    painter.setPen(QPen(divider, 1));
    painter.drawLine(QPointF(buttonRect.left() + 0.5, buttonRect.top() + 8), QPointF(buttonRect.left() + 0.5, buttonRect.bottom() - 8));

    qreal totalWeight = 0.0;
    for (int i = 0; i < fieldCount; ++i) {
        totalWeight += timeFieldWeight(m_use24HourClock, i);
    }
    totalWeight = qMax<qreal>(1.0, totalWeight);

    int x = contentRect.left();
    for (int i = 0; i < fieldCount; ++i) {
        const int remainingWidth = contentRect.right() - x + 1;
        const int fieldWidth = (i + 1 == fieldCount)
                                   ? remainingWidth
                                   : qMax(38, int(contentRect.width() * (timeFieldWeight(m_use24HourClock, i) / totalWeight)));
        const QRect fieldRect(x, contentRect.top(), qMin(fieldWidth, remainingWidth), contentRect.height());

        QColor textColor = enabled ? (m_hasTime ? colors.text : colors.subText) : colors.disabledText;
        if (!m_hasTime) {
            textColor.setAlpha(210);
        }

        painter.setPen(textColor);
        const QString text = displayTextForIndex(i);
        painter.drawText(fieldRect.adjusted(10, 0, -10, 0),
                         (i == 2 && !m_use24HourClock ? Qt::AlignCenter : Qt::AlignLeft) | Qt::AlignVCenter,
                         painter.fontMetrics().elidedText(text, Qt::ElideRight, qMax(0, fieldRect.width() - 20)));

        x += fieldRect.width();
        if (i + 1 < fieldCount) {
            painter.setPen(QPen(divider, 1));
            painter.drawLine(QPointF(x + 0.5, contentRect.top() + 4), QPointF(x + 0.5, contentRect.bottom() - 4));
        }
    }

    const QColor iconColor = enabled ? colors.subText : colors.disabledText;
    Style::drawChevronDown(painter, buttonRect.center(), iconColor, 8.0, 1.6);
}

void FluentTimePicker::enterEvent(FluentEnterEvent *event)
{
    QTimeEdit::enterEvent(event);
    startHoverAnimation(1.0);
}

void FluentTimePicker::leaveEvent(QEvent *event)
{
    QTimeEdit::leaveEvent(event);
    startHoverAnimation(0.0);
}

void FluentTimePicker::focusInEvent(QFocusEvent *event)
{
    QTimeEdit::focusInEvent(event);
    startFocusAnimation(1.0);
}

void FluentTimePicker::focusOutEvent(QFocusEvent *event)
{
    QTimeEdit::focusOutEvent(event);
    if (!isPopupVisible()) {
        startFocusAnimation(0.0);
    }
}

void FluentTimePicker::startHoverAnimation(qreal endValue)
{
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverLevel);
    m_hoverAnim->setEndValue(endValue);
    m_hoverAnim->start();
}

void FluentTimePicker::startFocusAnimation(qreal endValue)
{
    m_focusAnim->stop();
    m_focusAnim->setStartValue(m_focusLevel);
    m_focusAnim->setEndValue(endValue);
    m_focusAnim->start();
}

void FluentTimePicker::mousePressEvent(QMouseEvent *event)
{
    QTimeEdit::mousePressEvent(event);
    if (!event || event->button() != Qt::LeftButton) {
        return;
    }

    QTimer::singleShot(0, this, [this]() {
        togglePopup();
    });
    event->accept();
}

void FluentTimePicker::keyPressEvent(QKeyEvent *event)
{
    if (!event) {
        QTimeEdit::keyPressEvent(event);
        return;
    }

    const bool altDown = (event->key() == Qt::Key_Down && (event->modifiers() & Qt::AltModifier));
    if (event->key() == Qt::Key_F4 || altDown || event->key() == Qt::Key_Space || event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        togglePopup();
        event->accept();
        return;
    }

    if (isPopupVisible() && event->key() == Qt::Key_Escape) {
        hidePopup();
        event->accept();
        return;
    }

    QTimeEdit::keyPressEvent(event);
}

QSize FluentTimePicker::sizeHint() const
{
    return QSize(m_use24HourClock ? 180 : 220, Style::metrics().height);
}

void FluentTimePicker::togglePopup()
{
    if (isPopupVisible()) {
        hidePopup();
    } else {
        showPopup();
    }
}

void FluentTimePicker::showPopup()
{
    if (!m_popup) {
        m_popup = new Detail::FluentWheelPickerPopup(this);
        m_popup->setAnchor(this);
        m_popup->onAccepted = [this]() {
            setTime(popupTimeFromColumns());
            setFocus();
        };
        m_popup->onDismissed = [this](bool) {
            update();
            if (!hasFocus()) {
                startFocusAnimation(0.0);
            }
        };
    }

    rebuildPopupColumns(popupSeedTime());
    m_popup->popup();
    update();
}

void FluentTimePicker::hidePopup()
{
    if (m_popup) {
        m_popup->dismiss(false, true);
    }
}

bool FluentTimePicker::isPopupVisible() const
{
    return m_popup && m_popup->isVisible();
}

void FluentTimePicker::rebuildPopupColumns(const QTime &selectedTime)
{
    if (!m_popup) {
        return;
    }

    const QTime seed = selectedTime.isValid() ? selectedTime : popupSeedTime();
    QVector<Detail::PickerColumnConfig> configs;

    Detail::PickerColumnConfig hourConfig;
    hourConfig.alignment = Qt::AlignCenter;
    hourConfig.width = 86;
    if (m_use24HourClock) {
        for (int hour = 0; hour < 24; ++hour) {
            hourConfig.options.push_back({ QStringLiteral("%1").arg(hour, 2, 10, QChar('0')), hour });
        }
        hourConfig.currentValue = seed.hour();
    } else {
        for (int hour = 1; hour <= 12; ++hour) {
            hourConfig.options.push_back({ QString::number(hour), hour });
        }
        int hour12 = seed.hour() % 12;
        if (hour12 == 0) {
            hour12 = 12;
        }
        hourConfig.currentValue = hour12;
    }
    configs.push_back(hourConfig);

    Detail::PickerColumnConfig minuteConfig;
    minuteConfig.alignment = Qt::AlignCenter;
    minuteConfig.width = 86;
    for (int minute = 0; minute < 60; minute += qBound(1, m_minuteIncrement, 59)) {
        minuteConfig.options.push_back({ QStringLiteral("%1").arg(minute, 2, 10, QChar('0')), minute });
    }
    minuteConfig.currentValue = nearestMinuteValue(seed.minute(), m_minuteIncrement);
    configs.push_back(minuteConfig);

    if (!m_use24HourClock) {
        Detail::PickerColumnConfig periodConfig;
        periodConfig.alignment = Qt::AlignCenter;
        periodConfig.width = 82;
        periodConfig.options.push_back({ periodText(false), 0 });
        periodConfig.options.push_back({ periodText(true), 1 });
        periodConfig.currentValue = seed.hour() >= 12 ? 1 : 0;
        configs.push_back(periodConfig);
    }

    m_popup->setColumnConfigs(configs);
}

QTime FluentTimePicker::popupSeedTime() const
{
    if (m_hasTime) {
        return QTimeEdit::time();
    }

    const QTime now = QTime::currentTime();
    return QTime(now.hour(), nearestMinuteValue(now.minute(), m_minuteIncrement));
}

QTime FluentTimePicker::popupTimeFromColumns() const
{
    if (!m_popup) {
        return popupSeedTime();
    }

    const int hourValue = m_popup->columnValue(0).toInt();
    const int minuteValue = m_popup->columnValue(1).toInt();

    if (m_use24HourClock) {
        return QTime(hourValue, minuteValue);
    }

    const bool postMeridiem = m_popup->columnValue(2).toInt() != 0;
    int hour24 = hourValue % 12;
    if (postMeridiem) {
        hour24 += 12;
    }
    return QTime(hour24, minuteValue);
}

QString FluentTimePicker::periodText(bool postMeridiem) const
{
    return postMeridiem ? m_postMeridiemText : m_anteMeridiemText;
}

QString FluentTimePicker::displayTextForIndex(int fieldIndex) const
{
    if (!m_hasTime) {
        if (m_use24HourClock) {
            return fieldIndex == 0 ? m_hourPlaceholderText : m_minutePlaceholderText;
        }
        if (fieldIndex == 0) {
            return m_hourPlaceholderText;
        }
        if (fieldIndex == 1) {
            return m_minutePlaceholderText;
        }
        return periodText(false);
    }

    const QTime value = QTimeEdit::time();
    if (m_use24HourClock) {
        return fieldIndex == 0
                   ? QStringLiteral("%1").arg(value.hour(), 2, 10, QChar('0'))
                   : QStringLiteral("%1").arg(value.minute(), 2, 10, QChar('0'));
    }

    if (fieldIndex == 0) {
        int hour = value.hour() % 12;
        if (hour == 0) {
            hour = 12;
        }
        return QString::number(hour);
    }
    if (fieldIndex == 1) {
        return QStringLiteral("%1").arg(value.minute(), 2, 10, QChar('0'));
    }
    return periodText(value.hour() >= 12);
}

} // namespace Fluent
