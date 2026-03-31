#include "Fluent/datePicker/FluentCalendarPopup.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"
#include <QApplication>
#include <QEvent>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QLocale>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QVariantAnimation>
#include <QWheelEvent>
namespace Fluent {
namespace {
constexpr int kPadding      = 10;
constexpr int kHeaderH      = 44;
constexpr int kCornerRadius = 10;
constexpr int kNavBtn       = 30;
constexpr int kDayNamesH    = 24;
constexpr int kOpenSlidePx  = 6;
constexpr int kModeSlidePx  = 10;
constexpr int kSingleW     = 360;
constexpr int kSingleH     = 360;
constexpr int kRangePanelW = 370;
constexpr int kRangeH      = 380;
static QDate clampDayToMonth(const QDate &ref, int year, int month)
{
    const QDate first(year, month, 1);
    if (!first.isValid()) return ref;
    return QDate(year, month, qBound(1, ref.day(), first.daysInMonth()));
}
static QDate gridStartForMonth(int year, int month, Qt::DayOfWeek firstDayOfWeek)
{
    const QDate first(year, month, 1);
    if (!first.isValid()) return QDate();
    const int shift = (first.dayOfWeek() - int(firstDayOfWeek) + 7) % 7;
    return first.addDays(-shift);
}
static void applyRoundedWidgetMask(QWidget *w, qreal radius)
{
    if (!w) return;
    const QRect r = w->rect();
    if (r.isEmpty()) return;
    QPainterPath path;
    path.addRoundedRect(QRectF(r), radius, radius);
    w->setMask(QRegion(path.toFillPolygon().toPolygon()));
}
static QRect pillRect(const QRect &header, int x, int w)
{
    const int y = header.y() + (header.height() - 30) / 2;
    return QRect(x, y, w, 30);
}
} // namespace
// ── Constructor ──────────────────────────────────────────────────────────
FluentCalendarPopup::FluentCalendarPopup(QWidget *anchor)
    : QWidget(anchor)
    , m_anchor(anchor)
{
    setWindowFlag(Qt::Popup, true);
    setWindowFlag(Qt::FramelessWindowHint, true);
    setWindowFlag(Qt::NoDropShadowWindowHint, true);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setAttribute(Qt::WA_StyledBackground, true);
    setAutoFillBackground(false);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    m_selected = QDate::currentDate();
    ensurePageFromSelected();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this]() { update(); });
    m_openAnim = new QVariantAnimation(this);
    m_openAnim->setDuration(140);
    m_openAnim->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_openAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        m_openProgress = v.toReal();
        setWindowOpacity(m_openProgress);
        if (m_targetGeom.isValid()) {
            QRect g = m_targetGeom;
            g.translate(0, -int((1.0 - m_openProgress) * kOpenSlidePx));
            setGeometry(g);
        }
        update();
    });
    m_modeAnim = new QVariantAnimation(this);
    m_modeAnim->setDuration(160);
    m_modeAnim->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_modeAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        m_modeProgress = v.toReal();
        update();
    });
    resize(kSingleW, kSingleH);
}
// ── Public API ───────────────────────────────────────────────────────────
void FluentCalendarPopup::setAnchor(QWidget *anchor) { m_anchor = anchor; }
QWidget *FluentCalendarPopup::anchor() const          { return m_anchor; }
void FluentCalendarPopup::setDate(const QDate &date)
{
    if (date.isValid()) {
        m_selected = date;
        ensurePageFromSelected();
        update();
    }
}
QDate FluentCalendarPopup::date() const { return m_selected; }
void FluentCalendarPopup::setSelectionMode(SelectionMode mode)
{
    if (m_selectionMode == mode) return;
    m_selectionMode = mode;
    if (mode == SelectionMode::Range) {
        m_rangeStart   = QDate();
        m_rangeEnd     = QDate();
        m_hoverDate    = QDate();
        m_selectingEnd = false;
        ensureRightPage();
        resize(kRangePanelW * 2, kRangeH);
    } else {
        resize(kSingleW, kSingleH);
    }
}
FluentCalendarPopup::SelectionMode FluentCalendarPopup::selectionMode() const { return m_selectionMode; }
void FluentCalendarPopup::setDateRange(const QDate &start, const QDate &end)
{
    m_rangeStart = start.isValid() ? start : QDate();
    m_rangeEnd   = end.isValid()   ? end   : QDate();
    if (m_rangeStart.isValid() && m_rangeEnd.isValid() && m_rangeEnd < m_rangeStart)
        qSwap(m_rangeStart, m_rangeEnd);
    m_selectingEnd = false;
    m_hoverDate    = QDate();
    if (m_rangeStart.isValid()) {
        m_pageYear  = m_rangeStart.year();
        m_pageMonth = m_rangeStart.month();
    }
    ensureRightPage();
    update();
}
QDate FluentCalendarPopup::rangeStart() const { return m_rangeStart; }
QDate FluentCalendarPopup::rangeEnd()   const { return m_rangeEnd;   }
void FluentCalendarPopup::popup()
{
    if (m_selectionMode == SelectionMode::Range) {
        if (!m_pageYear) ensurePageFromSelected();
        m_selectingEnd = false;
        m_hoverDate    = QDate();
        ensureRightPage();
        resize(kRangePanelW * 2, kRangeH);
    } else {
        ensurePageFromSelected();
        resize(kSingleW, kSingleH);
    }
    positionPopupBelowOrAbove(6);
    m_targetGeom = geometry();
    m_openAnim->stop();
    m_openProgress = 0.0;
    setWindowOpacity(0.0);
    QRect startGeom = m_targetGeom;
    startGeom.translate(0, -kOpenSlidePx);
    setGeometry(startGeom);
    show();
    raise();
    activateWindow();
    setFocus();
    if (!m_appFilterInstalled) {
        qApp->installEventFilter(this);
        m_appFilterInstalled = true;
    }
    m_openAnim->setStartValue(0.0);
    m_openAnim->setEndValue(1.0);
    m_openAnim->start();
}
void FluentCalendarPopup::dismiss()
{
    if (!isVisible()) return;
    if (m_appFilterInstalled) {
        qApp->removeEventFilter(this);
        m_appFilterInstalled = false;
    }
    m_openAnim->stop();
    setWindowOpacity(1.0);
    m_hoverDate = QDate();
    hide();
    emit dismissed();
}
// ── Panel geometry ───────────────────────────────────────────────────────
int FluentCalendarPopup::panelWidth() const
{
    return m_selectionMode == SelectionMode::Range ? kRangePanelW : kSingleW;
}
int FluentCalendarPopup::popupHeight() const
{
    return m_selectionMode == SelectionMode::Range ? kRangeH : kSingleH;
}
QRect FluentCalendarPopup::contentRect() const
{
    return rect().adjusted(kPadding, kPadding, -kPadding, -kPadding);
}
QRect FluentCalendarPopup::headerRect() const
{
    const QRect c = contentRect();
    return QRect(c.x(), c.y(), c.width(), kHeaderH);
}
QRect FluentCalendarPopup::gridRect() const
{
    const QRect c = contentRect();
    return QRect(c.x(), c.y() + kHeaderH, c.width(), c.height() - kHeaderH);
}
QRect FluentCalendarPopup::monthPillRect() const
{
    const QRect h = headerRect();
    const QRect y = yearPillRect();
    QFont f = font(); f.setPointSizeF(f.pointSizeF() + 0.5); QFontMetrics fm(f);
    const int w = qBound(68, fm.horizontalAdvance(QString::number(m_pageMonth) + QStringLiteral("月")) + 22, 120);
    return pillRect(h, y.right() + 20, w);
}
QRect FluentCalendarPopup::yearPillRect() const
{
    const QRect h = headerRect();
    QFont f = font(); f.setPointSizeF(f.pointSizeF() + 0.5); QFontMetrics fm(f);
    const int w = qBound(74, fm.horizontalAdvance(QString::number(m_pageYear) + QStringLiteral("年")) + 22, 130);
    return pillRect(h, h.x(), w);
}
QRect FluentCalendarPopup::navPrevRect() const
{
    const QRect h = headerRect();
    const int y = h.y() + (h.height() - kNavBtn) / 2;
    return QRect(h.right() - (kNavBtn * 2) - 6, y, kNavBtn, kNavBtn);
}
QRect FluentCalendarPopup::navNextRect() const
{
    const QRect h = headerRect();
    const int y = h.y() + (h.height() - kNavBtn) / 2;
    return QRect(h.right() - kNavBtn, y, kNavBtn, kNavBtn);
}
QRect FluentCalendarPopup::todayButtonRect() const
{
    const QRect h = headerRect();
    const QRect prev = navPrevRect();
    QFont f = font(); f.setPointSizeF(f.pointSizeF() - 0.2); QFontMetrics fm(f);
    const QString text = QStringLiteral("今天");
    int w = qBound(44, fm.horizontalAdvance(text) + 20, 80);
    const int maxRight = prev.left() - 8;
    QRect r = pillRect(h, maxRight - w + 1, w);
    if (r.left() < monthPillRect().right() + 8) return QRect();
    return r;
}
void FluentCalendarPopup::ensurePageFromSelected()
{
    if (!m_selected.isValid()) m_selected = QDate::currentDate();
    m_pageYear  = m_selected.year();
    m_pageMonth = m_selected.month();
    m_yearBase  = (m_pageYear / 16) * 16;
}
void FluentCalendarPopup::ensureRightPage()
{
    if (m_selectionMode != SelectionMode::Range) return;
    int leftYear  = (m_pageYear  > 0) ? m_pageYear  : QDate::currentDate().year();
    int leftMonth = (m_pageMonth > 0) ? m_pageMonth : QDate::currentDate().month();
    QDate left(leftYear, leftMonth, 1);
    QDate right = left.addMonths(1);
    if (m_rangeEnd.isValid()) {
        QDate candidate(m_rangeEnd.year(), m_rangeEnd.month(), 1);
        if (candidate > left) right = candidate;
    }
    m_rightPageYear  = right.year();
    m_rightPageMonth = right.month();
}
// ── Navigation ───────────────────────────────────────────────────────────
void FluentCalendarPopup::stepMonth(int delta)
{
    const QDate next = QDate(m_pageYear, m_pageMonth, 1).addMonths(delta);
    if (!next.isValid()) return;
    m_pageYear  = next.year();
    m_pageMonth = next.month();
    m_selected  = clampDayToMonth(m_selected, m_pageYear, m_pageMonth);
    update();
}
void FluentCalendarPopup::stepRightMonth(int delta)
{
    const QDate next = QDate(m_rightPageYear, m_rightPageMonth, 1).addMonths(delta);
    if (!next.isValid()) return;
    m_rightPageYear  = next.year();
    m_rightPageMonth = next.month();
    m_hoverPart = HitPart::None; m_hoverIndex = -1;
    update();
}
void FluentCalendarPopup::stepYear(int delta)
{
    m_pageYear = qBound(1, m_pageYear + delta, 9999);
    m_selected = clampDayToMonth(m_selected, m_pageYear, m_pageMonth);
    update();
}
void FluentCalendarPopup::stepYearPage(int deltaPages)
{
    m_yearBase = qBound(1, m_yearBase + deltaPages * 16, 9999 - 15);
    m_pageYear = qBound(m_yearBase, m_pageYear, m_yearBase + 15);
    update();
}
// ── Range helpers ────────────────────────────────────────────────────────
QDate FluentCalendarPopup::dateFromCellIndex(int idx, int pageYear, int pageMonth) const
{
    const QDate s = gridStartForMonth(pageYear, pageMonth, Qt::Monday);
    return s.isValid() ? s.addDays(idx) : QDate();
}
QDate FluentCalendarPopup::effectiveRangeStart() const
{
    if (m_selectingEnd && m_hoverDate.isValid())
        return qMin(m_rangeStart, m_hoverDate);
    return m_rangeStart;
}
QDate FluentCalendarPopup::effectiveRangeEnd() const
{
    if (m_selectingEnd && m_hoverDate.isValid())
        return qMax(m_rangeStart, m_hoverDate);
    return m_rangeEnd;
}
void FluentCalendarPopup::handleRangeClick(const QDate &date)
{
    if (!date.isValid()) return;
    if (!m_selectingEnd) {
        m_rangeStart = date; m_rangeEnd = QDate();
        m_hoverDate = QDate(); m_selectingEnd = true;
    } else {
        QDate s = m_rangeStart, e = date;
        if (e < s) qSwap(s, e);
        m_rangeStart = s; m_rangeEnd = e;
        m_hoverDate = QDate(); m_selectingEnd = false;
        emit rangePicked(m_rangeStart, m_rangeEnd);
        dismiss();
    }
    update();
}
void FluentCalendarPopup::setSelectedDate(const QDate &date, bool emitPicked)
{
    if (!date.isValid()) return;
    m_selected = date; m_pageYear = date.year(); m_pageMonth = date.month();
    if (emitPicked) { emit datePicked(m_selected); dismiss(); return; }
    update();
}
// ── Events ───────────────────────────────────────────────────────────────
bool FluentCalendarPopup::event(QEvent *event)
{
    if (event) {
        if (event->type() == QEvent::WindowDeactivate ||
            event->type() == QEvent::ApplicationDeactivate) {
            dismiss(); return true;
        }
    }
    return QWidget::event(event);
}
bool FluentCalendarPopup::eventFilter(QObject *watched, QEvent *event)
{
    if (!event) return QWidget::eventFilter(watched, event);
    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::TouchBegin) {
        if (!isVisible()) return QWidget::eventFilter(watched, event);
        QPoint globalPos;
        if (event->type() == QEvent::MouseButtonPress) {
            auto *me = static_cast<QMouseEvent *>(event);
            if (!me) return QWidget::eventFilter(watched, event);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            globalPos = me->globalPosition().toPoint();
#else
            globalPos = me->globalPos();
#endif
        } else { globalPos = QCursor::pos(); }
        const QPoint local = mapFromGlobal(globalPos);
        if (!rect().contains(local)) {
            bool clickedAnchor = false;
            if (m_anchor)
                clickedAnchor = m_anchor->rect().contains(m_anchor->mapFromGlobal(globalPos));
            dismiss();
            if (clickedAnchor) return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}
void FluentCalendarPopup::showEvent(QShowEvent *e)  { QWidget::showEvent(e);  applyRoundedMask(); }
void FluentCalendarPopup::resizeEvent(QResizeEvent *e) { QWidget::resizeEvent(e); applyRoundedMask(); }
// ── Paint ────────────────────────────────────────────────────────────────
void FluentCalendarPopup::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    const auto &c = ThemeManager::instance().colors();
    QPainter p(this);
    if (!p.isActive()) return;
    p.setRenderHint(QPainter::Antialiasing, true);
    const QRectF outer = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    p.setPen(QPen(c.border, 1.0)); p.setBrush(c.surface);
    p.drawRoundedRect(outer, kCornerRadius, kCornerRadius);
    if (m_selectionMode == SelectionMode::Range) {
        paintRangePanelHeader(p, 0,            m_pageYear,      m_pageMonth,      false);
        paintRangePanelDays  (p, 0,            m_pageYear,      m_pageMonth,      false);
        paintRangePanelHeader(p, kRangePanelW, m_rightPageYear, m_rightPageMonth, true);
        paintRangePanelDays  (p, kRangePanelW, m_rightPageYear, m_rightPageMonth, true);
        QColor div = c.border; div.setAlpha(100);
        p.setPen(QPen(div, 1.0));
        p.drawLine(QPointF(kRangePanelW + 0.5, 12), QPointF(kRangePanelW + 0.5, height() - 12));
        return;
    }
    paintHeader(p);
    auto paintMode = [&](ViewMode mode) {
        if (mode == ViewMode::Days)        paintDays(p);
        else if (mode == ViewMode::Months) paintMonths(p);
        else                               paintYears(p);
    };
    const bool trans = (m_modeAnim && m_modeAnim->state() == QAbstractAnimation::Running && m_modeProgress < 1.0);
    if (!trans) { paintMode(m_mode); return; }
    const qreal t = qBound<qreal>(0.0, m_modeProgress, 1.0);
    p.save(); p.setOpacity(1.0 - t); p.translate(0, -int(t * kModeSlidePx)); paintMode(m_prevMode); p.restore();
    p.save(); p.setOpacity(t); p.translate(0, int((1.0 - t) * kModeSlidePx)); paintMode(m_mode); p.restore();
}
// ── Range panel painting ─────────────────────────────────────────────────
void FluentCalendarPopup::paintRangePanelHeader(QPainter &p, int panelX,
    int pageYear, int pageMonth, bool isRight)
{
    const auto &c = ThemeManager::instance().colors();
    const QRect content(panelX + kPadding, kPadding, kRangePanelW - 2*kPadding, height() - 2*kPadding);
    const QRect h(content.x(), content.y(), content.width(), kHeaderH);
    QFont f = font(); f.setPointSizeF(f.pointSizeF() + 0.5); QFontMetrics fm(f);
    const int yearW  = qBound(74,  fm.horizontalAdvance(QString::number(pageYear)  + QStringLiteral("年")) + 22, 130);
    const int monthW = qBound(68,  fm.horizontalAdvance(QString::number(pageMonth) + QStringLiteral("月")) + 22, 120);
    const QRect yearPill  = pillRect(h, h.x(), yearW);
    const QRect monthPill = pillRect(h, yearPill.right() + 20, monthW);
    const int navY = h.y() + (h.height() - kNavBtn) / 2;
    const QRect navPrev(h.right() - kNavBtn*2 - 6, navY, kNavBtn, kNavBtn);
    const QRect navNext(h.right() - kNavBtn,        navY, kNavBtn, kNavBtn);
    // Bottom divider line
    QColor line = c.border; line.setAlpha(80);
    p.setPen(QPen(line, 1.0));
    p.drawLine(QPointF(h.left(), h.bottom() + 0.5), QPointF(h.right(), h.bottom() + 0.5));
    // Separator between year and month
    QColor sep = c.border; sep.setAlpha(80);
    const int sx = yearPill.right() + 10, sy = h.y() + (h.height() - 18) / 2;
    p.setPen(QPen(sep, 1.0));
    p.drawLine(QPointF(sx + 0.5, sy), QPointF(sx + 0.5, sy + 18));
    // Year / month labels
    p.setFont(f);
    p.setPen(c.text);
    p.drawText(yearPill.adjusted(8,0,-8,0),  Qt::AlignVCenter|Qt::AlignLeft, QString::number(pageYear)  + QStringLiteral("年"));
    p.drawText(monthPill.adjusted(8,0,-8,0), Qt::AlignVCenter|Qt::AlignLeft, QString::number(pageMonth) + QStringLiteral("月"));
    p.setFont(font());
    const HitPart prevPart = isRight ? HitPart::RNavPrev : HitPart::NavPrev;
    const HitPart nextPart = isRight ? HitPart::RNavNext : HitPart::NavNext;
    auto paintNav = [&](const QRect &r, HitPart part, bool right) {
        const bool hov = (m_hoverPart == part), prs = (m_pressPart == part);
        QColor bg = Qt::transparent;
        if (prs)      { bg = c.pressed; bg.setAlpha(150); }
        else if (hov) { bg = c.hover;   bg.setAlpha(120); }
        p.setPen(Qt::NoPen); p.setBrush(bg);
        p.drawRoundedRect(QRectF(r).adjusted(0.5,0.5,-0.5,-0.5), 8.0, 8.0);
        if (right) drawChevronRight(p, r.center(), c.subText);
        else       drawChevronLeft (p, r.center(), c.subText);
    };
    paintNav(navPrev, prevPart, false);
    paintNav(navNext, nextPart, true);
}
void FluentCalendarPopup::paintRangePanelDays(QPainter &p, int panelX,
    int pageYear, int pageMonth, bool isRight)
{
    const auto &c = ThemeManager::instance().colors();
    const QRect content(panelX + kPadding, kPadding, kRangePanelW - 2*kPadding, height() - 2*kPadding);
    const QRect g(content.x(), content.y()+kHeaderH, content.width(), content.height()-kHeaderH);
    const QRect dayNames(g.x(), g.y(), g.width(), kDayNamesH);
    const QRect cells(g.x(), g.y()+kDayNamesH, g.width(), g.height()-kDayNamesH);
    const int cw = cells.width() / 7, ch = cells.height() / 6;
    if (cw <= 0 || ch <= 0) return;
    // Weekend columns
    {
        QColor wkBg = c.pressed; wkBg.setAlpha(45);
        p.setPen(Qt::NoPen); p.setBrush(wkBg);
        const int totalH = dayNames.height() + cells.height();
        for (int col : {5, 6})
            p.drawRect(QRectF(g.x()+col*cw, dayNames.y(), cw, totalH).adjusted(0,1.0,-0.5,-1.0));
    }
    // Day name row
    QFont sf = font(); sf.setPointSizeF(sf.pointSizeF() - 0.5); p.setFont(sf);
    for (int i = 0; i < 7; ++i) {
        const QRect r(dayNames.x()+i*cw, dayNames.y(), cw, dayNames.height());
        const int dow = int(Qt::Monday) + i;
        const QString name = QLocale().standaloneDayName(((dow-1)%7)+1, QLocale::ShortFormat);
        p.setPen(i >= 5 ? c.text : c.subText);
        p.drawText(r, Qt::AlignCenter, name);
    }
    p.setFont(font());
    const QDate gridStart = gridStartForMonth(pageYear, pageMonth, Qt::Monday);
    if (!gridStart.isValid()) return;
    const QDate today = QDate::currentDate();
    const QDate effS  = effectiveRangeStart();
    const QDate effE  = effectiveRangeEnd();
    const bool hasRange = effS.isValid() && effE.isValid();
    const HitPart cellPart = isRight ? HitPart::RCell : HitPart::Cell;
    for (int row = 0; row < 6; ++row) {
        for (int col = 0; col < 7; ++col) {
            const int   idx = row*7+col;
            const QDate d   = gridStart.addDays(idx);
            const QRect rc(cells.x()+col*cw, cells.y()+row*ch, cw, ch);
            const QRectF rr = QRectF(rc).adjusted(6.0, 4.0, -6.0, -4.0);
            const bool inMonth  = (d.year() == pageYear && d.month() == pageMonth);
            const bool isStart  = hasRange && (d == effS);
            const bool isEnd    = hasRange && (d == effE);
            const bool inRange  = hasRange && (d > effS && d < effE);
            const bool isToday  = (d == today);
            const bool hovered  = (m_hoverPart == cellPart && m_hoverIndex == idx);
            // 1. Continuous range band (full slot width = no horizontal gaps)
            if (hasRange) {
                const qreal bY = rc.y() + 4.0, bH = rc.height() - 8.0;
                const qreal cx = rc.x() + rc.width() / 2.0;
                QColor band = c.accent; band.setAlphaF(0.15);
                p.setPen(Qt::NoPen); p.setBrush(band);
                if (inRange) {
                    p.drawRect(QRectF(rc.x(), bY, rc.width(), bH));
                } else if (isStart && d != effE) {
                    // Right half: from cell centre to the cell's right edge.
                    // Use QRectF(rc).right() (= rc.x()+rc.width()) instead of
                    // rc.right() (= rc.x()+rc.width()-1) to avoid a 1-px gap.
                    p.drawRect(QRectF(cx, bY, QRectF(rc).right() - cx, bH));
                } else if (isEnd && d != effS) {
                    p.drawRect(QRectF(rc.x(), bY, cx - rc.x(), bH));
                }
            }
            // 2. Cell circle
            if (isStart || isEnd) {
                p.setPen(Qt::NoPen); p.setBrush(c.accent);
                p.drawRoundedRect(rr, 8.0, 8.0);
            } else if (hovered && !inRange) {
                QColor fill = c.hover; fill.setAlpha(120);
                p.setPen(Qt::NoPen); p.setBrush(fill);
                p.drawRoundedRect(rr, 8.0, 8.0);
            }
            // 3. Today ring
            if (isToday && !isStart && !isEnd) {
                QColor ring = c.accent; ring.setAlpha(170);
                p.setPen(QPen(ring, 1.5)); p.setBrush(Qt::NoBrush);
                p.drawRoundedRect(rr, 8.0, 8.0);
            }
            // 4. Text
            QColor textColor;
            if (!inMonth)         textColor = c.disabledText;
            else if (isStart || isEnd) textColor = QColor(Qt::white);
            else if (col >= 5)    textColor = c.subText;
            else                  textColor = c.text;
            p.setPen(textColor);
            p.drawText(rc, Qt::AlignCenter, QString::number(d.day()));
        }
    }
}
// ── Single-mode painting ─────────────────────────────────────────────────
void FluentCalendarPopup::paintHeader(QPainter &p)
{
    const auto &c = ThemeManager::instance().colors();
    const QRect h = headerRect(), y = yearPillRect(), m = monthPillRect(), t = todayButtonRect();
    QColor hl = c.border; hl.setAlpha(80);
    p.setPen(QPen(hl, 1.0));
    p.drawLine(QPointF(h.left(), h.bottom()+0.5), QPointF(h.right(), h.bottom()+0.5));
    QColor sep = c.border; sep.setAlpha(90);
    const int sx = y.right()+10, sy = h.y()+(h.height()-18)/2;
    p.setPen(QPen(sep, 1.0));
    p.drawLine(QPointF(sx+0.5, sy), QPointF(sx+0.5, sy+18));
    auto paintPill = [&](const QRect &r, HitPart part, const QString &text, bool active) {
        const bool hov = (m_hoverPart==part), prs = (m_pressPart==part);
        QColor bg = Qt::transparent, tc = c.text;
        if (active) {
            bg = c.accent; bg.setAlpha(220); tc = QColor(Qt::white);
            if (prs) bg=bg.darker(112); else if (hov) bg=bg.lighter(106);
        } else {
            if (prs)      { bg=c.pressed; bg.setAlpha(150); }
            else if (hov) { bg=c.hover;   bg.setAlpha(120); }
        }
        p.setPen(Qt::NoPen); p.setBrush(bg);
        p.drawRoundedRect(QRectF(r).adjusted(0.5,0.5,-0.5,-0.5), 8.0, 8.0);
        p.setPen(tc);
        p.drawText(r.adjusted(12,0,-12,0), Qt::AlignVCenter|Qt::AlignLeft, text);
    };
    paintPill(y, HitPart::HeaderYear,  QString::number(m_pageYear)  + QStringLiteral("年"), false);
    paintPill(m, HitPart::HeaderMonth, QString::number(m_pageMonth) + QStringLiteral("月"), false);
    if (t.isValid()) {
        const bool hov=(m_hoverPart==HitPart::HeaderToday), prs=(m_pressPart==HitPart::HeaderToday);
        QColor bg=Qt::transparent;
        if (prs)      { bg=c.pressed; bg.setAlpha(150); }
        else if (hov) { bg=c.hover;   bg.setAlpha(120); }
        p.setPen(Qt::NoPen); p.setBrush(bg);
        p.drawRoundedRect(QRectF(t).adjusted(0.5,0.5,-0.5,-0.5), 8.0, 8.0);
        p.setPen(c.accent);
        p.drawText(t, Qt::AlignCenter, QStringLiteral("今天"));
    }
    auto paintNav = [&](const QRect &r, HitPart part, bool right) {
        const bool hov=(m_hoverPart==part), prs=(m_pressPart==part);
        QColor bg=Qt::transparent;
        if (prs)      { bg=c.pressed; bg.setAlpha(150); }
        else if (hov) { bg=c.hover;   bg.setAlpha(120); }
        p.setPen(Qt::NoPen); p.setBrush(bg);
        p.drawRoundedRect(QRectF(r).adjusted(0.5,0.5,-0.5,-0.5), 8.0, 8.0);
        if (right) drawChevronRight(p, r.center(), c.subText);
        else       drawChevronLeft (p, r.center(), c.subText);
    };
    paintNav(navPrevRect(), HitPart::NavPrev, false);
    paintNav(navNextRect(), HitPart::NavNext, true);
    Q_UNUSED(h)
}
void FluentCalendarPopup::paintDays(QPainter &p)
{
    const auto &c = ThemeManager::instance().colors();
    const QRect g = gridRect();
    const QRect dayNames(g.x(), g.y(), g.width(), kDayNamesH);
    const QRect cells(g.x(), g.y()+kDayNamesH, g.width(), g.height()-kDayNamesH);
    const int cw = cells.width()/7, ch = cells.height()/6;
    if (cw<=0||ch<=0) return;
    {
        QColor wkBg=c.pressed; wkBg.setAlpha(55);
        p.setPen(Qt::NoPen); p.setBrush(wkBg);
        const int totalH=dayNames.height()+cells.height();
        for (int col:{5,6})
            p.drawRect(QRectF(g.x()+col*cw, dayNames.y(), cw, totalH).adjusted(0,1.0,-0.5,-1.0));
    }
    QFont sf=font(); sf.setPointSizeF(sf.pointSizeF()-0.5); p.setFont(sf);
    for (int i=0;i<7;++i) {
        const QRect r(dayNames.x()+i*cw, dayNames.y(), cw, dayNames.height());
        const int dow=int(Qt::Monday)+i;
        const QString name=QLocale().standaloneDayName(((dow-1)%7)+1, QLocale::ShortFormat);
        p.setPen(i>=5?c.text:c.subText); p.drawText(r, Qt::AlignCenter, name);
    }
    p.setFont(font());
    const QDate start = gridStartForMonth(m_pageYear, m_pageMonth, Qt::Monday);
    if (!start.isValid()) return;
    const QDate today = QDate::currentDate();
    for (int row=0;row<6;++row) for (int col=0;col<7;++col) {
        const int idx=row*7+col; const QDate d=start.addDays(idx);
        const QRect rc(cells.x()+col*cw, cells.y()+row*ch, cw, ch);
        const QRectF rr=QRectF(rc).adjusted(6.0,4.0,-6.0,-4.0);
        const bool inMonth=(d.year()==m_pageYear&&d.month()==m_pageMonth);
        const bool selected=(d==m_selected), isToday=(d==today);
        const bool hovered=(m_hoverPart==HitPart::Cell&&m_hoverIndex==idx);
        if (selected) { QColor fill=c.accent; fill.setAlpha(210); p.setPen(Qt::NoPen); p.setBrush(fill); p.drawRoundedRect(rr,8.0,8.0); }
        else if (hovered) { QColor fill=c.hover; fill.setAlpha(120); p.setPen(Qt::NoPen); p.setBrush(fill); p.drawRoundedRect(rr,8.0,8.0); }
        if (isToday&&!selected) { QColor ring=c.accent; ring.setAlpha(170); p.setPen(QPen(ring,1.5)); p.setBrush(Qt::NoBrush); p.drawRoundedRect(rr,8.0,8.0); }
        QColor tc;
        if (!inMonth) tc=c.disabledText;
        else if (selected) tc=QColor(Qt::white);
        else if (col>=5) tc=c.subText;
        else tc=c.text;
        p.setPen(tc); p.drawText(rc, Qt::AlignCenter, QString::number(d.day()));
    }
}
void FluentCalendarPopup::paintMonths(QPainter &p)
{
    const auto &c=ThemeManager::instance().colors();
    const QRect g=gridRect(); const int cw=g.width()/3, ch=g.height()/4;
    const QDate today=QDate::currentDate();
    QFont f=font(); f.setPointSizeF(f.pointSizeF()+0.2); p.setFont(f);
    for (int i=0;i<12;++i) {
        const int row=i/3, col=i%3;
        const QRect rc(g.x()+col*cw, g.y()+row*ch, cw, ch);
        const QRectF rr=QRectF(rc).adjusted(8.0,8.0,-8.0,-8.0);
        const bool sel=(i+1)==m_pageMonth, cur=(today.isValid()&&m_pageYear==today.year()&&(i+1)==today.month());
        const bool hov=(m_hoverPart==HitPart::Cell&&m_hoverIndex==i);
        if (sel) { QColor fill=c.accent; fill.setAlpha(210); p.setPen(Qt::NoPen); p.setBrush(fill); p.drawRoundedRect(rr,10.0,10.0); }
        else if (hov) { QColor fill=c.hover; fill.setAlpha(120); p.setPen(Qt::NoPen); p.setBrush(fill); p.drawRoundedRect(rr,10.0,10.0); }
        if (cur&&!sel) { QColor ring=c.accent; ring.setAlpha(170); p.setPen(QPen(ring,1.5)); p.setBrush(Qt::NoBrush); p.drawRoundedRect(rr,10.0,10.0); }
        p.setPen(sel?QColor(Qt::white):c.text);
        p.drawText(rc, Qt::AlignCenter, QLocale().standaloneMonthName(i+1, QLocale::ShortFormat));
    }
    p.setFont(font());
}
void FluentCalendarPopup::paintYears(QPainter &p)
{
    const auto &c=ThemeManager::instance().colors();
    const QRect g=gridRect(); const int cw=g.width()/4, ch=g.height()/4;
    const int ty=QDate::currentDate().year();
    QFont f=font(); f.setPointSizeF(f.pointSizeF()+0.2); p.setFont(f);
    for (int i=0;i<16;++i) {
        const int row=i/4, col=i%4;
        const QRect rc(g.x()+col*cw, g.y()+row*ch, cw, ch);
        const QRectF rr=QRectF(rc).adjusted(8.0,8.0,-8.0,-8.0);
        const int year=m_yearBase+i;
        const bool sel=(year==m_pageYear), cur=(year==ty);
        const bool hov=(m_hoverPart==HitPart::Cell&&m_hoverIndex==i);
        if (sel) { QColor fill=c.accent; fill.setAlpha(210); p.setPen(Qt::NoPen); p.setBrush(fill); p.drawRoundedRect(rr,10.0,10.0); }
        else if (hov) { QColor fill=c.hover; fill.setAlpha(120); p.setPen(Qt::NoPen); p.setBrush(fill); p.drawRoundedRect(rr,10.0,10.0); }
        if (cur&&!sel) { QColor ring=c.accent; ring.setAlpha(170); p.setPen(QPen(ring,1.5)); p.setBrush(Qt::NoBrush); p.drawRoundedRect(rr,10.0,10.0); }
        p.setPen(sel?QColor(Qt::white):c.text);
        p.drawText(rc, Qt::AlignCenter, QString::number(year));
    }
    p.setFont(font());
}
// ── Mouse events ─────────────────────────────────────────────────────────
void FluentCalendarPopup::mouseMoveEvent(QMouseEvent *event)
{
    if (!event) return;
    int idx=-1; const HitPart part=hitTest(event->pos(), &idx);
    if (part!=m_hoverPart||idx!=m_hoverIndex) {
        m_hoverPart=part; m_hoverIndex=idx;
        if (m_selectionMode==SelectionMode::Range && m_selectingEnd) {
            QDate newHover;
            if (part==HitPart::Cell  && idx>=0) newHover=dateFromCellIndex(idx, m_pageYear, m_pageMonth);
            else if (part==HitPart::RCell && idx>=0) newHover=dateFromCellIndex(idx, m_rightPageYear, m_rightPageMonth);
            m_hoverDate=newHover;
        }
        update();
    }
}
void FluentCalendarPopup::mousePressEvent(QMouseEvent *event)
{
    if (!event||event->button()!=Qt::LeftButton) { QWidget::mousePressEvent(event); return; }
    int idx=-1; const HitPart part=hitTest(event->pos(), &idx);
    m_pressPart=part; m_pressIndex=idx;
    auto clr=[this](){ m_pressPart=HitPart::None; m_pressIndex=-1; };
    if (m_selectionMode==SelectionMode::Range) {
        if (part==HitPart::NavPrev)  { stepMonth(-1);      clr(); event->accept(); return; }
        if (part==HitPart::NavNext)  { stepMonth(1);       clr(); event->accept(); return; }
        if (part==HitPart::RNavPrev) { stepRightMonth(-1); clr(); event->accept(); return; }
        if (part==HitPart::RNavNext) { stepRightMonth(1);  clr(); event->accept(); return; }
        if (part==HitPart::Cell  && idx>=0) { handleRangeClick(dateFromCellIndex(idx, m_pageYear, m_pageMonth)); clr(); event->accept(); return; }
        if (part==HitPart::RCell && idx>=0) { handleRangeClick(dateFromCellIndex(idx, m_rightPageYear, m_rightPageMonth)); clr(); event->accept(); return; }
        clr(); update(); event->accept(); return;
    }
    if (part==HitPart::NavPrev) {
        if (m_mode==ViewMode::Days) stepMonth(-1); else if (m_mode==ViewMode::Months) stepYear(-1); else stepYearPage(-1);
        clr(); event->accept(); return;
    }
    if (part==HitPart::NavNext) {
        if (m_mode==ViewMode::Days) stepMonth(1); else if (m_mode==ViewMode::Months) stepYear(1); else stepYearPage(1);
        clr(); event->accept(); return;
    }
    if (part==HitPart::HeaderMonth) { setMode(m_mode==ViewMode::Months?ViewMode::Days:ViewMode::Months); clr(); event->accept(); return; }
    if (part==HitPart::HeaderYear)  { setMode(m_mode==ViewMode::Years ?ViewMode::Days:ViewMode::Years);  clr(); event->accept(); return; }
    if (part==HitPart::HeaderToday) {
        const QDate today=QDate::currentDate();
        if (today.isValid()) { m_selected=today; m_pageYear=today.year(); m_pageMonth=today.month(); if (m_mode!=ViewMode::Days) setMode(ViewMode::Days); else update(); }
        clr(); event->accept(); return;
    }
    if (part==HitPart::Cell && idx>=0) {
        if (m_mode==ViewMode::Days) {
            const QDate s=gridStartForMonth(m_pageYear, m_pageMonth, Qt::Monday);
            if (s.isValid()) setSelectedDate(s.addDays(idx), true);
        } else if (m_mode==ViewMode::Months) {
            m_pageMonth=qBound(1,idx+1,12); m_selected=clampDayToMonth(m_selected,m_pageYear,m_pageMonth); setMode(ViewMode::Days); update();
        } else {
            m_pageYear=m_yearBase+idx; m_selected=clampDayToMonth(m_selected,m_pageYear,m_pageMonth); setMode(ViewMode::Months); update();
        }
        clr(); event->accept(); return;
    }
    clr(); update(); event->accept();
}
void FluentCalendarPopup::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    if (m_hoverPart!=HitPart::None||m_hoverIndex!=-1) {
        m_hoverPart=HitPart::None; m_hoverIndex=-1;
        if (m_selectionMode==SelectionMode::Range) m_hoverDate=QDate();
        update();
    }
}
void FluentCalendarPopup::wheelEvent(QWheelEvent *event)
{
    if (!event) return;
    const int dy=event->angleDelta().y(); if (dy==0) return;
    const int dir=(dy>0)?-1:1;
    if (m_selectionMode==SelectionMode::Range) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        const qreal px=event->position().x();
#else
        const qreal px=event->posF().x();
#endif
        if (px<kRangePanelW) stepMonth(dir); else stepRightMonth(dir);
        event->accept(); return;
    }
    if (m_mode==ViewMode::Days) stepMonth(dir);
    else if (m_mode==ViewMode::Months) stepYear(dir);
    else stepYearPage(dir);
    event->accept();
}
void FluentCalendarPopup::keyPressEvent(QKeyEvent *event)
{
    if (!event) return;
    if (m_selectionMode==SelectionMode::Range) {
        if (event->key()==Qt::Key_Escape) {
            if (m_selectingEnd) { m_selectingEnd=false; m_rangeStart=QDate(); m_hoverDate=QDate(); update(); }
            else dismiss();
            event->accept(); return;
        }
        QWidget::keyPressEvent(event); return;
    }
    if (event->key()==Qt::Key_Escape) { if (m_mode!=ViewMode::Days) setMode(ViewMode::Days); else dismiss(); event->accept(); return; }
    if (m_mode==ViewMode::Days) {
        if (event->key()==Qt::Key_Up||event->key()==Qt::Key_PageUp)   { stepMonth(-1); event->accept(); return; }
        if (event->key()==Qt::Key_Down||event->key()==Qt::Key_PageDown){ stepMonth(1);  event->accept(); return; }
        if (event->key()==Qt::Key_Left)  { setSelectedDate(m_selected.addDays(-1),false); event->accept(); return; }
        if (event->key()==Qt::Key_Right) { setSelectedDate(m_selected.addDays(1), false); event->accept(); return; }
        if (event->key()==Qt::Key_Return||event->key()==Qt::Key_Enter) { emit datePicked(m_selected); dismiss(); event->accept(); return; }
    }
    if (m_mode==ViewMode::Months) {
        if (event->key()==Qt::Key_Left)  { m_pageMonth=qMax(1,m_pageMonth-1); update(); event->accept(); return; }
        if (event->key()==Qt::Key_Right) { m_pageMonth=qMin(12,m_pageMonth+1); update(); event->accept(); return; }
        if (event->key()==Qt::Key_Up)    { m_pageMonth=qMax(1,m_pageMonth-3); update(); event->accept(); return; }
        if (event->key()==Qt::Key_Down)  { m_pageMonth=qMin(12,m_pageMonth+3); update(); event->accept(); return; }
        if (event->key()==Qt::Key_Return||event->key()==Qt::Key_Enter) {
            m_selected=clampDayToMonth(m_selected,m_pageYear,m_pageMonth); setMode(ViewMode::Days); update(); event->accept(); return; }
    }
    if (m_mode==ViewMode::Years) {
        int idx=qBound(0,m_pageYear-m_yearBase,15);
        if (event->key()==Qt::Key_Left) idx=qMax(0,idx-1);
        else if (event->key()==Qt::Key_Right) idx=qMin(15,idx+1);
        else if (event->key()==Qt::Key_Up)    idx=qMax(0,idx-4);
        else if (event->key()==Qt::Key_Down)  idx=qMin(15,idx+4);
        else if (event->key()==Qt::Key_Return||event->key()==Qt::Key_Enter) {
            m_pageYear=m_yearBase+idx; m_selected=clampDayToMonth(m_selected,m_pageYear,m_pageMonth);
            setMode(ViewMode::Months); update(); event->accept(); return;
        } else { QWidget::keyPressEvent(event); return; }
        m_pageYear=m_yearBase+idx; update(); event->accept(); return;
    }
    QWidget::keyPressEvent(event);
}
// ── Hit testing ──────────────────────────────────────────────────────────
int FluentCalendarPopup::cellIndexInGrid(const QPoint &pos, const QRect &cells, int cw, int ch) const
{
    if (!cells.contains(pos)) return -1;
    const int col=(pos.x()-cells.x())/cw, row=(pos.y()-cells.y())/ch;
    if (col<0||col>=7||row<0||row>=6) return -1;
    return row*7+col;
}
int FluentCalendarPopup::cellIndexAt(const QPoint &pos) const
{
    const QRect g=gridRect();
    if (m_mode==ViewMode::Days) {
        const QRect dn(g.x(),g.y(),g.width(),kDayNamesH);
        if (dn.contains(pos)) return -1;
        const QRect cells(g.x(),g.y()+kDayNamesH,g.width(),g.height()-kDayNamesH);
        const int cw=cells.width()/7, ch=cells.height()/6;
        return (cw>0&&ch>0)?cellIndexInGrid(pos,cells,cw,ch):-1;
    }
    if (m_mode==ViewMode::Months) {
        const int cw=g.width()/3, ch=g.height()/4;
        const int col=(pos.x()-g.x())/cw, row=(pos.y()-g.y())/ch;
        if (col<0||col>=3||row<0||row>=4) return -1;
        const int idx=row*3+col; return (idx>=0&&idx<12)?idx:-1;
    }
    const int cw=g.width()/4, ch=g.height()/4;
    const int col=(pos.x()-g.x())/cw, row=(pos.y()-g.y())/ch;
    if (col<0||col>=4||row<0||row>=4) return -1;
    const int idx=row*4+col; return (idx>=0&&idx<16)?idx:-1;
}
FluentCalendarPopup::HitPart FluentCalendarPopup::hitTest(const QPoint &pos, int *outIndex) const
{
    if (outIndex) *outIndex=-1;
    if (m_selectionMode==SelectionMode::Range) {
        const bool rp=(pos.x()>=kRangePanelW);
        const int panelX = rp ? kRangePanelW : 0;
        const QRect content(panelX+kPadding, kPadding, kRangePanelW-2*kPadding, height()-2*kPadding);
        const QRect h(content.x(),content.y(),content.width(),kHeaderH);
        const QRect g(content.x(),content.y()+kHeaderH,content.width(),content.height()-kHeaderH);
        const QRect dayNames(g.x(),g.y(),g.width(),kDayNamesH);
        const QRect cells(g.x(),g.y()+kDayNamesH,g.width(),g.height()-kDayNamesH);
        const int navY=h.y()+(h.height()-kNavBtn)/2;
        const QRect navPrev(h.right()-kNavBtn*2-6, navY, kNavBtn, kNavBtn);
        const QRect navNext(h.right()-kNavBtn,      navY, kNavBtn, kNavBtn);
        if (navPrev.contains(pos)) return rp?HitPart::RNavPrev:HitPart::NavPrev;
        if (navNext.contains(pos)) return rp?HitPart::RNavNext:HitPart::NavNext;
        const int cw=cells.width()/7, ch=cells.height()/6;
        if (cw>0&&ch>0&&!dayNames.contains(pos)) {
            const int idx=cellIndexInGrid(pos,cells,cw,ch);
            if (idx>=0) { if (outIndex)*outIndex=idx; return rp?HitPart::RCell:HitPart::Cell; }
        }
        return HitPart::None;
    }
    if (monthPillRect().contains(pos))  return HitPart::HeaderMonth;
    if (yearPillRect().contains(pos))   return HitPart::HeaderYear;
    const QRect t=todayButtonRect();
    if (t.isValid()&&t.contains(pos))   return HitPart::HeaderToday;
    if (navPrevRect().contains(pos))    return HitPart::NavPrev;
    if (navNextRect().contains(pos))    return HitPart::NavNext;
    const int idx=cellIndexAt(pos);
    if (idx>=0) { if (outIndex)*outIndex=idx; return HitPart::Cell; }
    return HitPart::None;
}
// ── Mode transition ──────────────────────────────────────────────────────
void FluentCalendarPopup::setMode(ViewMode mode)
{
    if (m_mode==mode) return;
    startModeTransition(m_mode,mode);
    m_mode=mode;
    if (m_mode==ViewMode::Years) m_yearBase=(m_pageYear/16)*16;
    m_hoverPart=HitPart::None; m_hoverIndex=-1; update();
}
void FluentCalendarPopup::startModeTransition(ViewMode from, ViewMode to)
{
    Q_UNUSED(to) m_prevMode=from;
    if (!m_modeAnim) { m_modeProgress=1.0; return; }
    m_modeAnim->stop(); m_modeProgress=0.0;
    m_modeAnim->setStartValue(0.0); m_modeAnim->setEndValue(1.0); m_modeAnim->start();
}
// ── Utilities ────────────────────────────────────────────────────────────
void FluentCalendarPopup::applyRoundedMask() { applyRoundedWidgetMask(this, kCornerRadius); }
void FluentCalendarPopup::positionPopupBelowOrAbove(int gap)
{
    if (!m_anchor) return;
    const QPoint gl=m_anchor->mapToGlobal(QPoint(0,0));
    QScreen *screen=QApplication::screenAt(gl);
    if (!screen) screen=QApplication::primaryScreen();
    const QRect avail=screen?screen->availableGeometry():QRect();
    const QSize s=size();
    QRect r(m_anchor->mapToGlobal(QPoint(0, m_anchor->height()+gap)), s);
    if (avail.isValid()&&r.bottom()>avail.bottom()) {
        QRect r2(m_anchor->mapToGlobal(QPoint(0,-gap-s.height())), s);
        if (!avail.isValid()||r2.top()>=avail.top()) r=r2;
    }
    if (avail.isValid()) {
        if (r.left()<avail.left())   r.moveLeft(avail.left());
        if (r.right()>avail.right()) r.moveRight(avail.right());
        if (r.top()<avail.top())     r.moveTop(avail.top());
        if (r.bottom()>avail.bottom()) r.moveBottom(avail.bottom());
    }
    setGeometry(r);
}
void FluentCalendarPopup::drawChevronLeft(QPainter &p, const QPointF &center, const QColor &color) const
{
    p.save(); p.setRenderHint(QPainter::Antialiasing,true);
    p.setPen(QPen(color,1.8,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
    const qreal s=6.0; QPainterPath path;
    path.moveTo(center.x()+s/2, center.y()-s);
    path.lineTo(center.x()-s/2, center.y());
    path.lineTo(center.x()+s/2, center.y()+s);
    p.drawPath(path); p.restore();
}
void FluentCalendarPopup::drawChevronRight(QPainter &p, const QPointF &center, const QColor &color) const
{
    p.save(); p.setRenderHint(QPainter::Antialiasing,true);
    p.setPen(QPen(color,1.8,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
    const qreal s=6.0; QPainterPath path;
    path.moveTo(center.x()-s/2, center.y()-s);
    path.lineTo(center.x()+s/2, center.y());
    path.lineTo(center.x()-s/2, center.y()+s);
    p.drawPath(path); p.restore();
}
} // namespace Fluent
