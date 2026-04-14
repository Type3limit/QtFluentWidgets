#include "Fluent/FluentDatePicker.h"

#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"
#include "datePicker/FluentWheelPickerSupport.h"

#include <QEvent>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QLocale>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>
#include <QVariantAnimation>

namespace Fluent {

namespace {

QLocale defaultPickerLocale()
{
    return QLocale(QLocale::Chinese, QLocale::China);
}

QVector<FluentDatePicker::DatePart> orderedDateParts(FluentDatePicker::DateParts parts)
{
    QVector<FluentDatePicker::DatePart> order;
    if (parts.testFlag(FluentDatePicker::MonthPart)) {
        order.push_back(FluentDatePicker::MonthPart);
    }
    if (parts.testFlag(FluentDatePicker::DayPart)) {
        order.push_back(FluentDatePicker::DayPart);
    }
    if (parts.testFlag(FluentDatePicker::YearPart)) {
        order.push_back(FluentDatePicker::YearPart);
    }
    return order;
}

qreal fieldWeight(FluentDatePicker::DatePart part)
{
    switch (part) {
    case FluentDatePicker::MonthPart:
        return 1.45;
    case FluentDatePicker::DayPart:
        return 0.90;
    case FluentDatePicker::YearPart:
        return 1.05;
    }
    return 1.0;
}

} // namespace

FluentDatePicker::FluentDatePicker(QWidget *parent)
    : QWidget(parent)
    , m_minimumDate(1900, 1, 1)
    , m_maximumDate(2100, 12, 31)
    , m_monthFormat(QStringLiteral("MMMM"))
    , m_dayFormat(QStringLiteral("d"))
    , m_yearFormat(QStringLiteral("yyyy"))
    , m_monthPlaceholderText(QStringLiteral("月"))
    , m_dayPlaceholderText(QStringLiteral("日"))
    , m_yearPlaceholderText(QStringLiteral("年"))
{
    setAttribute(Qt::WA_Hover, true);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumHeight(Style::metrics().height);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    setLocale(defaultPickerLocale());

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

    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentDatePicker::applyTheme);
}

FluentDatePicker::~FluentDatePicker() = default;

qreal FluentDatePicker::hoverLevel() const
{
    return m_hoverLevel;
}

void FluentDatePicker::setHoverLevel(qreal value)
{
    m_hoverLevel = qBound(0.0, value, 1.0);
    update();
}

qreal FluentDatePicker::focusLevel() const
{
    return m_focusLevel;
}

void FluentDatePicker::setFocusLevel(qreal value)
{
    m_focusLevel = qBound(0.0, value, 1.0);
    update();
}

void FluentDatePicker::setDate(const QDate &date)
{
    if (!date.isValid()) {
        clearDate();
        return;
    }

    const QDate bounded = boundedDate(date);
    if (m_date == bounded) {
        update();
        return;
    }

    m_date = bounded;
    update();
    emit dateChanged(m_date);
}

QDate FluentDatePicker::date() const
{
    return m_date;
}

void FluentDatePicker::clearDate()
{
    if (!m_date.isValid()) {
        return;
    }

    m_date = QDate();
    update();
    emit dateChanged(QDate());
}

bool FluentDatePicker::hasDate() const
{
    return m_date.isValid();
}

void FluentDatePicker::setMinimumDate(const QDate &date)
{
    if (!date.isValid() || date == m_minimumDate) {
        return;
    }

    m_minimumDate = date;
    if (m_maximumDate < m_minimumDate) {
        m_maximumDate = m_minimumDate;
    }

    if (m_date.isValid()) {
        m_date = boundedDate(m_date);
    }

    if (isPopupVisible()) {
        rebuildPopupColumns(popupDateFromColumns());
    }
    update();
}

QDate FluentDatePicker::minimumDate() const
{
    return m_minimumDate;
}

void FluentDatePicker::setMaximumDate(const QDate &date)
{
    if (!date.isValid() || date == m_maximumDate) {
        return;
    }

    m_maximumDate = date;
    if (m_minimumDate > m_maximumDate) {
        m_minimumDate = m_maximumDate;
    }

    if (m_date.isValid()) {
        m_date = boundedDate(m_date);
    }

    if (isPopupVisible()) {
        rebuildPopupColumns(popupDateFromColumns());
    }
    update();
}

QDate FluentDatePicker::maximumDate() const
{
    return m_maximumDate;
}

void FluentDatePicker::setDateRange(const QDate &minimum, const QDate &maximum)
{
    if (!minimum.isValid() || !maximum.isValid()) {
        return;
    }

    if (minimum <= maximum) {
        m_minimumDate = minimum;
        m_maximumDate = maximum;
    } else {
        m_minimumDate = maximum;
        m_maximumDate = minimum;
    }

    if (m_date.isValid()) {
        m_date = boundedDate(m_date);
    }

    if (isPopupVisible()) {
        rebuildPopupColumns(popupDateFromColumns());
    }
    update();
}

void FluentDatePicker::setVisibleParts(DateParts parts)
{
    if (parts == 0) {
        parts = MonthPart | DayPart | YearPart;
    }
    if (m_visibleParts == parts) {
        return;
    }

    m_visibleParts = parts;
    if (isPopupVisible()) {
        rebuildPopupColumns(popupDateFromColumns());
    }
    updateGeometry();
    update();
}

FluentDatePicker::DateParts FluentDatePicker::visibleParts() const
{
    return m_visibleParts;
}

void FluentDatePicker::setMonthDisplayFormat(const QString &format)
{
    if (m_monthFormat == format || format.isEmpty()) {
        return;
    }
    m_monthFormat = format;
    if (isPopupVisible()) {
        rebuildPopupColumns(popupDateFromColumns());
    }
    update();
}

QString FluentDatePicker::monthDisplayFormat() const
{
    return m_monthFormat;
}

void FluentDatePicker::setDayDisplayFormat(const QString &format)
{
    if (m_dayFormat == format || format.isEmpty()) {
        return;
    }
    m_dayFormat = format;
    if (isPopupVisible()) {
        rebuildPopupColumns(popupDateFromColumns());
    }
    update();
}

QString FluentDatePicker::dayDisplayFormat() const
{
    return m_dayFormat;
}

void FluentDatePicker::setYearDisplayFormat(const QString &format)
{
    if (m_yearFormat == format || format.isEmpty()) {
        return;
    }
    m_yearFormat = format;
    if (isPopupVisible()) {
        rebuildPopupColumns(popupDateFromColumns());
    }
    update();
}

QString FluentDatePicker::yearDisplayFormat() const
{
    return m_yearFormat;
}

void FluentDatePicker::setMonthPlaceholderText(const QString &text)
{
    if (m_monthPlaceholderText == text) {
        return;
    }

    m_monthPlaceholderText = text;
    update();
}

QString FluentDatePicker::monthPlaceholderText() const
{
    return m_monthPlaceholderText;
}

void FluentDatePicker::setDayPlaceholderText(const QString &text)
{
    if (m_dayPlaceholderText == text) {
        return;
    }

    m_dayPlaceholderText = text;
    update();
}

QString FluentDatePicker::dayPlaceholderText() const
{
    return m_dayPlaceholderText;
}

void FluentDatePicker::setYearPlaceholderText(const QString &text)
{
    if (m_yearPlaceholderText == text) {
        return;
    }

    m_yearPlaceholderText = text;
    update();
}

QString FluentDatePicker::yearPlaceholderText() const
{
    return m_yearPlaceholderText;
}

void FluentDatePicker::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (!event) {
        return;
    }

    if (event->type() == QEvent::LocaleChange && isPopupVisible()) {
        rebuildPopupColumns(popupDateFromColumns());
    }

    if (event->type() == QEvent::EnabledChange || event->type() == QEvent::LocaleChange) {
        applyTheme();
    }
}

void FluentDatePicker::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    const auto &colors = ThemeManager::instance().colors();
    const auto metrics = Style::metrics();

    QPainter painter(this);
    if (!painter.isActive()) {
        return;
    }
    painter.setRenderHint(QPainter::Antialiasing, true);

    Style::paintControlSurface(painter,
                               QRectF(rect()),
                               colors,
                               m_hoverLevel,
                               m_focusLevel,
                               isEnabled(),
                               isPopupVisible());

    const QRect arrowRect(width() - metrics.iconAreaWidth, 0, metrics.iconAreaWidth, height());
    QColor divider = colors.border;
    divider.setAlpha(80);
    painter.setPen(QPen(divider, 1));
    painter.drawLine(QPointF(arrowRect.left() + 0.5, arrowRect.top() + 8),
                     QPointF(arrowRect.left() + 0.5, arrowRect.bottom() - 8));

    const QVector<DatePart> parts = orderedParts();
    const QRect contentRect = rect().adjusted(8, 4, -metrics.iconAreaWidth - 6, -4);

    qreal totalWeight = 0.0;
    for (DatePart part : parts) {
        totalWeight += fieldWeight(part);
    }
    totalWeight = qMax<qreal>(1.0, totalWeight);

    int x = contentRect.left();
    for (int i = 0; i < parts.size(); ++i) {
        const DatePart part = parts.at(i);
        const int remainingWidth = contentRect.right() - x + 1;
        const int widthForField = (i + 1 == parts.size())
                                      ? remainingWidth
                                      : qMax(38, int(contentRect.width() * (fieldWeight(part) / totalWeight)));
        const QRect fieldRect(x, contentRect.top(), qMin(widthForField, remainingWidth), contentRect.height());

        QColor textColor = isEnabled() ? (hasDate() ? colors.text : colors.subText) : colors.disabledText;
        if (!hasDate()) {
            textColor.setAlpha(210);
        }

        painter.setPen(textColor);
        const QString text = displayTextForPart(part, hasDate() ? m_date : popupSeedDate(), !hasDate());
        painter.drawText(fieldRect.adjusted(10, 0, -10, 0), Qt::AlignLeft | Qt::AlignVCenter,
                         painter.fontMetrics().elidedText(text, Qt::ElideRight, qMax(0, fieldRect.width() - 20)));

        x += fieldRect.width();
        if (i + 1 < parts.size()) {
            painter.setPen(QPen(divider, 1));
            painter.drawLine(QPointF(x + 0.5, contentRect.top() + 4), QPointF(x + 0.5, contentRect.bottom() - 4));
        }
    }

    const QColor iconColor = isEnabled() ? colors.subText : colors.disabledText;
    Style::drawChevronDown(painter, arrowRect.center(), iconColor, 8.0, 1.6);
}

void FluentDatePicker::enterEvent(FluentEnterEvent *event)
{
    QWidget::enterEvent(event);
    startHoverAnimation(1.0);
}

void FluentDatePicker::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    startHoverAnimation(0.0);
}

void FluentDatePicker::mousePressEvent(QMouseEvent *event)
{
    QWidget::mousePressEvent(event);
    if (!event || event->button() != Qt::LeftButton) {
        return;
    }

    QTimer::singleShot(0, this, [this]() {
        togglePopup();
    });
    event->accept();
}

void FluentDatePicker::keyPressEvent(QKeyEvent *event)
{
    if (!event) {
        QWidget::keyPressEvent(event);
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

    QWidget::keyPressEvent(event);
}

void FluentDatePicker::focusInEvent(QFocusEvent *event)
{
    QWidget::focusInEvent(event);
    startFocusAnimation(1.0);
}

void FluentDatePicker::focusOutEvent(QFocusEvent *event)
{
    QWidget::focusOutEvent(event);
    if (!isPopupVisible()) {
        startFocusAnimation(0.0);
    }
}

QSize FluentDatePicker::sizeHint() const
{
    const int partCount = orderedParts().size();
    return QSize(partCount >= 3 ? 260 : 200, Style::metrics().height);
}

void FluentDatePicker::applyTheme()
{
    if (m_popup) {
        m_popup->update();
    }
    update();
}

void FluentDatePicker::startHoverAnimation(qreal endValue)
{
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverLevel);
    m_hoverAnim->setEndValue(endValue);
    m_hoverAnim->start();
}

void FluentDatePicker::startFocusAnimation(qreal endValue)
{
    m_focusAnim->stop();
    m_focusAnim->setStartValue(m_focusLevel);
    m_focusAnim->setEndValue(endValue);
    m_focusAnim->start();
}

void FluentDatePicker::togglePopup()
{
    if (isPopupVisible()) {
        hidePopup();
    } else {
        showPopup();
    }
}

void FluentDatePicker::showPopup()
{
    if (!m_popup) {
        m_popup = new Detail::FluentWheelPickerPopup(this);
        m_popup->setAnchor(this);
        m_popup->onAccepted = [this]() {
            setDate(popupDateFromColumns());
            setFocus();
        };
        m_popup->onDismissed = [this](bool) {
            update();
        };
    }

    rebuildPopupColumns(popupSeedDate());
    m_popup->popup();
    update();
}

void FluentDatePicker::hidePopup()
{
    if (m_popup) {
        m_popup->dismiss(false, true);
    }
}

bool FluentDatePicker::isPopupVisible() const
{
    return m_popup && m_popup->isVisible();
}

QVector<FluentDatePicker::DatePart> FluentDatePicker::orderedParts() const
{
    return orderedDateParts(m_visibleParts);
}

int FluentDatePicker::popupColumnIndex(DatePart part) const
{
    const QVector<DatePart> parts = orderedParts();
    for (int i = 0; i < parts.size(); ++i) {
        if (parts.at(i) == part) {
            return i;
        }
    }
    return -1;
}

QDate FluentDatePicker::boundedDate(const QDate &date) const
{
    if (!date.isValid()) {
        return QDate();
    }

    if (date < m_minimumDate) {
        return m_minimumDate;
    }
    if (date > m_maximumDate) {
        return m_maximumDate;
    }
    return date;
}

QDate FluentDatePicker::popupSeedDate() const
{
    if (m_date.isValid()) {
        return boundedDate(m_date);
    }

    const QDate today = QDate::currentDate();
    const QDate boundedToday = boundedDate(today);
    if (boundedToday.isValid()) {
        return boundedToday;
    }
    return m_minimumDate.isValid() ? m_minimumDate : today;
}

QDate FluentDatePicker::popupDateFromColumns() const
{
    QDate result = popupSeedDate();
    if (!m_popup) {
        return result;
    }

    const int yearIndex = popupColumnIndex(YearPart);
    if (yearIndex >= 0) {
        const QVariant year = m_popup->columnValue(yearIndex);
        if (year.isValid()) {
            result.setDate(year.toInt(), result.month(), qMin(result.day(), QDate(year.toInt(), result.month(), 1).daysInMonth()));
        }
    }

    const int monthIndex = popupColumnIndex(MonthPart);
    if (monthIndex >= 0) {
        const QVariant month = m_popup->columnValue(monthIndex);
        if (month.isValid()) {
            result.setDate(result.year(), month.toInt(), qMin(result.day(), QDate(result.year(), month.toInt(), 1).daysInMonth()));
        }
    }

    const int dayIndex = popupColumnIndex(DayPart);
    if (dayIndex >= 0) {
        const QVariant day = m_popup->columnValue(dayIndex);
        if (day.isValid()) {
            result.setDate(result.year(), result.month(), day.toInt());
        }
    }

    return boundedDate(result);
}

void FluentDatePicker::rebuildPopupColumns(const QDate &selectedDate)
{
    if (!m_popup) {
        return;
    }

    const QDate selected = boundedDate(selectedDate.isValid() ? selectedDate : popupSeedDate());
    const QVector<DatePart> parts = orderedParts();
    const QLocale popupLocale = locale();

    QVector<Detail::PickerColumnConfig> configs;
    configs.reserve(parts.size());

    for (DatePart part : parts) {
        Detail::PickerColumnConfig config;
        config.alignment = (part == MonthPart) ? Qt::AlignLeft : Qt::AlignCenter;

        switch (part) {
        case MonthPart: {
            const int minMonth = (parts.contains(YearPart) && selected.year() == m_minimumDate.year()) ? m_minimumDate.month() : 1;
            const int maxMonth = (parts.contains(YearPart) && selected.year() == m_maximumDate.year()) ? m_maximumDate.month() : 12;
            for (int month = minMonth; month <= maxMonth; ++month) {
                config.options.push_back({ popupLocale.toString(QDate(selected.year(), month, 1), m_monthFormat), month });
            }
            config.currentValue = selected.month();
            config.width = parts.size() >= 3 ? 168 : 182;
            break;
        }
        case DayPart: {
            const int minDay = (selected.year() == m_minimumDate.year() && selected.month() == m_minimumDate.month()) ? m_minimumDate.day() : 1;
            const int maxDay = (selected.year() == m_maximumDate.year() && selected.month() == m_maximumDate.month())
                                   ? m_maximumDate.day()
                                   : QDate(selected.year(), selected.month(), 1).daysInMonth();
            for (int day = minDay; day <= maxDay; ++day) {
                const QDate value(selected.year(), selected.month(), day);
                config.options.push_back({ popupLocale.toString(value, m_dayFormat), day });
            }
            config.currentValue = selected.day();
            config.width = 104;
            break;
        }
        case YearPart:
            for (int year = m_minimumDate.year(); year <= m_maximumDate.year(); ++year) {
                config.options.push_back({ popupLocale.toString(QDate(year, 1, 1), m_yearFormat), year });
            }
            config.currentValue = selected.year();
            config.width = 112;
            break;
        }

        configs.push_back(config);
    }

    m_syncingPopupColumns = true;
    m_popup->setColumnConfigs(configs);

    auto bindColumn = [this](DatePart part) {
        const int index = popupColumnIndex(part);
        if (index < 0) {
            return;
        }

        if (auto *column = m_popup->columnAt(index)) {
            disconnect(column, &QListWidget::currentRowChanged, this, nullptr);
            if (part == DayPart) {
                return;
            }
            connect(column, &QListWidget::currentRowChanged, this, [this](int) {
                if (m_syncingPopupColumns) {
                    return;
                }
                rebuildPopupColumns(popupDateFromColumns());
            });
        }
    };

    bindColumn(MonthPart);
    bindColumn(YearPart);
    m_syncingPopupColumns = false;
}

QString FluentDatePicker::displayTextForPart(DatePart part, const QDate &value, bool placeholder) const
{
    if (placeholder || !value.isValid()) {
        switch (part) {
        case MonthPart:
            return m_monthPlaceholderText;
        case DayPart:
            return m_dayPlaceholderText;
        case YearPart:
            return m_yearPlaceholderText;
        }
    }

    const QLocale displayLocale = locale();
    switch (part) {
    case MonthPart:
        return displayLocale.toString(value, m_monthFormat);
    case DayPart:
        return displayLocale.toString(value, m_dayFormat);
    case YearPart:
        return displayLocale.toString(value, m_yearFormat);
    }
    return QString();
}

} // namespace Fluent