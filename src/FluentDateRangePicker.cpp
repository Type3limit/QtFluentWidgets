#include "Fluent/FluentDateRangePicker.h"
#include "Fluent/datePicker/FluentCalendarPopup.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"

#include <QEasingCurve>
#include <QEvent>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>
#include <QVariantAnimation>

namespace Fluent {

FluentDateRangePicker::FluentDateRangePicker(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);
    setMinimumHeight(Style::metrics().height);
    setCursor(Qt::PointingHandCursor);

    m_hoverAnim = new QVariantAnimation(this);
    m_hoverAnim->setDuration(150);
    m_hoverAnim->setEasingCurve(QEasingCurve::OutQuad);
    connect(m_hoverAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        m_hoverLevel = v.toReal();
        update();
    });

    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &FluentDateRangePicker::applyTheme);
}

// ── Date range ────────────────────────────────────────────────────────────

QDate FluentDateRangePicker::startDate() const { return m_startDate; }
QDate FluentDateRangePicker::endDate()   const { return m_endDate;   }

void FluentDateRangePicker::setDateRange(const QDate &start, const QDate &end)
{
    m_startDate = start.isValid() ? start : QDate();
    m_endDate   = end.isValid()   ? end   : QDate();
    if (m_startDate.isValid() && m_endDate.isValid() && m_endDate < m_startDate)
        qSwap(m_startDate, m_endDate);
    update();
}

// ── Display format ────────────────────────────────────────────────────────

void    FluentDateRangePicker::setDisplayFormat(const QString &fmt) { if (!fmt.isEmpty()) { m_format = fmt; update(); } }
QString FluentDateRangePicker::displayFormat() const                { return m_format; }

// ── Label pieces ──────────────────────────────────────────────────────────

void    FluentDateRangePicker::setStartPrefix(const QString &t) { m_startPrefix = t; update(); }
QString FluentDateRangePicker::startPrefix() const              { return m_startPrefix; }

void    FluentDateRangePicker::setStartSuffix(const QString &t) { m_startSuffix = t; update(); }
QString FluentDateRangePicker::startSuffix() const              { return m_startSuffix; }

void    FluentDateRangePicker::setEndPrefix(const QString &t) { m_endPrefix = t; update(); }
QString FluentDateRangePicker::endPrefix() const              { return m_endPrefix; }

void    FluentDateRangePicker::setEndSuffix(const QString &t) { m_endSuffix = t; update(); }
QString FluentDateRangePicker::endSuffix() const              { return m_endSuffix; }

void    FluentDateRangePicker::setSeparator(const QString &t) { m_separator = t; update(); }
QString FluentDateRangePicker::separator() const              { return m_separator; }

void    FluentDateRangePicker::setStartPlaceholder(const QString &t) { m_startPlaceholder = t; update(); }
QString FluentDateRangePicker::startPlaceholder() const              { return m_startPlaceholder; }

void    FluentDateRangePicker::setEndPlaceholder(const QString &t) { m_endPlaceholder = t; update(); }
QString FluentDateRangePicker::endPlaceholder() const              { return m_endPlaceholder; }

// ── Hover animation property ──────────────────────────────────────────────

qreal FluentDateRangePicker::hoverLevel() const     { return m_hoverLevel; }
void  FluentDateRangePicker::setHoverLevel(qreal v) { m_hoverLevel = qBound(0.0, v, 1.0); update(); }

// ── Size hints ────────────────────────────────────────────────────────────

QSize FluentDateRangePicker::sizeHint() const
{
    const QFontMetrics fm(font());
    const auto m = Style::metrics();
    // Typical text: prefix + "yyyy-MM-dd" + suffix  +  sep  +  prefix + "yyyy-MM-dd" + suffix
    const QString sample = m_startPrefix
                         + QStringLiteral("yyyy-MM-dd")
                         + m_startSuffix
                         + m_separator
                         + m_endPrefix
                         + QStringLiteral("yyyy-MM-dd")
                         + m_endSuffix;
    const int textW = fm.horizontalAdvance(sample);
    return QSize(qMax(textW + m.paddingX * 2 + m.iconAreaWidth + 8, 260), m.height);
}
QSize FluentDateRangePicker::minimumSizeHint() const { return sizeHint(); }

// ── Helper ────────────────────────────────────────────────────────────────

QString FluentDateRangePicker::buildSideText(const QDate  &date,
                                              const QString &prefix,
                                              const QString &suffix,
                                              const QString &placeholder) const
{
    if (date.isValid())
        return prefix + date.toString(m_format) + suffix;
    // No date set: show prefix + placeholder + suffix (if any of them non-empty)
    const QString inner = placeholder;
    return prefix + inner + suffix;
}

// ── Theme ─────────────────────────────────────────────────────────────────

void FluentDateRangePicker::applyTheme() { update(); }

// ── Paint ─────────────────────────────────────────────────────────────────

void FluentDateRangePicker::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    const auto &colors   = ThemeManager::instance().colors();
    const bool  enabled  = isEnabled();
    const bool  expanded = isPopupVisible();

    QPainter painter(this);
    if (!painter.isActive()) return;
    painter.setRenderHint(QPainter::Antialiasing, true);

    Style::paintControlSurface(painter, QRectF(rect()), colors, m_hoverLevel, 0.0, enabled, expanded);

    const auto  m       = Style::metrics();
    const int   h       = rect().height();
    const int   iconW   = m.iconAreaWidth;
    const QRect iconRect(rect().right() - iconW, 0, iconW, h);
    const int   textArea = rect().width() - iconW - m.paddingX;  // available text width

    // Separator line before the chevron icon
    QColor sep = colors.border; sep.setAlpha(80);
    painter.setPen(QPen(sep, 1));
    painter.drawLine(QPointF(iconRect.left() + 0.5, iconRect.top() + 8),
                     QPointF(iconRect.left() + 0.5, iconRect.bottom() - 8));

    // Chevron-down icon
    Style::drawChevronDown(painter, iconRect.center(),
                           enabled ? colors.subText : colors.disabledText, 8.0, 1.7);

    // Build display strings
    const QString startText = buildSideText(m_startDate, m_startPrefix, m_startSuffix, m_startPlaceholder);
    const QString endText   = buildSideText(m_endDate,   m_endPrefix,   m_endSuffix,   m_endPlaceholder);
    const QString sepText   = m_separator;

    const QFontMetrics fm(painter.font());
    const int sepW   = fm.horizontalAdvance(sepText);
    const int sideW  = qMax(1, (textArea - sepW) / 2);

    const QRect startRect(m.paddingX,                    0, sideW, h);
    const QRect sepRect  (m.paddingX + sideW,            0, sepW,  h);
    const QRect endRect  (m.paddingX + sideW + sepW,     0, sideW, h);

    // Start text
    const QColor startColor = (enabled && m_startDate.isValid()) ? colors.text : colors.subText;
    painter.setPen(startColor);
    painter.drawText(startRect, Qt::AlignVCenter | Qt::AlignLeft,
                     fm.elidedText(startText, Qt::ElideRight, sideW));

    // Separator
    painter.setPen(colors.subText);
    painter.drawText(sepRect, Qt::AlignVCenter | Qt::AlignHCenter, sepText);

    // End text
    const QColor endColor = (enabled && m_endDate.isValid()) ? colors.text : colors.subText;
    painter.setPen(endColor);
    painter.drawText(endRect, Qt::AlignVCenter | Qt::AlignLeft,
                     fm.elidedText(endText, Qt::ElideRight, sideW));
}

// ── Mouse events ──────────────────────────────────────────────────────────

void FluentDateRangePicker::mousePressEvent(QMouseEvent *event)
{
    if (!event || event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    QTimer::singleShot(0, this, [this]() {
        if (isPopupVisible()) hidePopup();
        else                  showPopup();
    });
    event->accept();
}

void FluentDateRangePicker::enterEvent(FluentEnterEvent *event)
{
    QWidget::enterEvent(event);
    startHoverAnimation(1.0);
}

void FluentDateRangePicker::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    startHoverAnimation(0.0);
}

void FluentDateRangePicker::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) applyTheme();
}

// ── Popup ─────────────────────────────────────────────────────────────────

void FluentDateRangePicker::showPopup()
{
    if (!m_popup) {
        m_popup = new FluentCalendarPopup(this);
        m_popup->setSelectionMode(FluentCalendarPopup::SelectionMode::Range);

        connect(m_popup, &FluentCalendarPopup::rangePicked,
                this, [this](const QDate &s, const QDate &e) {
            m_startDate = s;
            m_endDate   = e;
            update();
            emit dateRangeChanged(m_startDate, m_endDate);
        });
        connect(m_popup, &FluentCalendarPopup::dismissed,
                this, [this]() { update(); });
    }

    m_popup->setAnchor(this);
    if (m_startDate.isValid() || m_endDate.isValid())
        m_popup->setDateRange(m_startDate, m_endDate);

    m_popup->popup();
}

void FluentDateRangePicker::hidePopup()
{
    if (m_popup) m_popup->dismiss();
}

bool FluentDateRangePicker::isPopupVisible() const
{
    return m_popup && m_popup->isVisible();
}

void FluentDateRangePicker::startHoverAnimation(qreal endValue)
{
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverLevel);
    m_hoverAnim->setEndValue(endValue);
    m_hoverAnim->start();
}

} // namespace Fluent

