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

constexpr int kPadding = 10;
constexpr int kHeaderH = 44;
constexpr int kCornerRadius = 10;
constexpr int kNavBtn = 30;
constexpr int kDayNamesH = 24;
constexpr int kOpenSlidePx = 6;
constexpr int kModeSlidePx = 10;

static QDate clampDayToMonth(const QDate &ref, int year, int month)
{
    const QDate first(year, month, 1);
    if (!first.isValid()) {
        return ref;
    }
    const int day = qBound(1, ref.day(), first.daysInMonth());
    return QDate(year, month, day);
}

static QDate gridStartForMonth(int year, int month, Qt::DayOfWeek firstDayOfWeek)
{
    const QDate first(year, month, 1);
    if (!first.isValid()) {
        return QDate();
    }
    const int firstDow = first.dayOfWeek();
    const int targetDow = int(firstDayOfWeek);
    const int shift = (firstDow - targetDow + 7) % 7;
    return first.addDays(-shift);
}

static void applyRoundedWidgetMask(QWidget *w, qreal radius)
{
    if (!w) {
        return;
    }
    const QRect r = w->rect();
    if (r.isEmpty()) {
        return;
    }

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

FluentCalendarPopup::FluentCalendarPopup(QWidget *anchor)
    : QWidget(anchor)
    , m_anchor(anchor)
{
    setWindowFlag(Qt::Popup, true);
    setWindowFlag(Qt::FramelessWindowHint, true);
    setWindowFlag(Qt::NoDropShadowWindowHint, true);

    // Avoid translucent popup windows on Windows (often appears black in light mode).
    setAttribute(Qt::WA_TranslucentBackground, false);
    setAttribute(Qt::WA_StyledBackground, true);
    setAutoFillBackground(false);

    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    m_selected = QDate::currentDate();
    ensurePageFromSelected();

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this]() {
        update();
    });

    m_openAnim = new QVariantAnimation(this);
    m_openAnim->setDuration(140);
    m_openAnim->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_openAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        m_openProgress = v.toReal();

        // Fade + slide to target.
        setWindowOpacity(m_openProgress);
        if (m_targetGeom.isValid()) {
            QRect g = m_targetGeom;
            const int dy = int((1.0 - m_openProgress) * kOpenSlidePx);
            g.translate(0, -dy);
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

    resize(360, 360);
}

void FluentCalendarPopup::setAnchor(QWidget *anchor)
{
    m_anchor = anchor;
}

QWidget *FluentCalendarPopup::anchor() const
{
    return m_anchor;
}

void FluentCalendarPopup::setDate(const QDate &date)
{
    if (date.isValid()) {
        m_selected = date;
        ensurePageFromSelected();
        update();
    }
}

QDate FluentCalendarPopup::date() const
{
    return m_selected;
}

void FluentCalendarPopup::popup()
{
    ensurePageFromSelected();
    positionPopupBelowOrAbove(6);

    // capture target geometry after positioning
    m_targetGeom = geometry();

    // start hidden-ish then animate in
    m_openAnim->stop();
    m_openProgress = 0.0;
    setWindowOpacity(0.0);
    QRect start = m_targetGeom;
    start.translate(0, -kOpenSlidePx);
    setGeometry(start);

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
    if (!isVisible()) {
        return;
    }

    if (m_appFilterInstalled) {
        qApp->removeEventFilter(this);
        m_appFilterInstalled = false;
    }
    m_openAnim->stop();
    setWindowOpacity(1.0);
    hide();
    emit dismissed();
}

bool FluentCalendarPopup::event(QEvent *event)
{
    if (event) {
        if (event->type() == QEvent::WindowDeactivate || event->type() == QEvent::ApplicationDeactivate) {
            // behave like QComboBox popup: close when deactivated
            dismiss();
            return true;
        }
    }
    return QWidget::event(event);
}

bool FluentCalendarPopup::eventFilter(QObject *watched, QEvent *event)
{
    if (!event) {
        return QWidget::eventFilter(watched, event);
    }

    // qApp event filter: it receives events for *all* objects, but `watched` is the real receiver.
    // Do NOT gate on watched == qApp, or outside-click detection will silently fail.
    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::TouchBegin) {
        if (!isVisible()) {
            return QWidget::eventFilter(watched, event);
        }

        // close when clicking outside the popup
        QPoint globalPos;
        if (event->type() == QEvent::MouseButtonPress) {
            auto *me = static_cast<QMouseEvent *>(event);
            if (!me) {
                return QWidget::eventFilter(watched, event);
            }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            globalPos = me->globalPosition().toPoint();
#else
            globalPos = me->globalPos();
#endif
        } else {
            // best effort for touch
            globalPos = QCursor::pos();
        }

        const QPoint localPos = mapFromGlobal(globalPos);
        if (!rect().contains(localPos)) {
            bool clickedAnchor = false;
            if (m_anchor) {
                const QPoint a = m_anchor->mapFromGlobal(globalPos);
                clickedAnchor = m_anchor->rect().contains(a);
            }

            dismiss();

            // QComboBox-like behavior: clicking the anchor closes but shouldn't immediately reopen.
            if (clickedAnchor) {
                return true;
            }
        }
    }

    return QWidget::eventFilter(watched, event);
}

void FluentCalendarPopup::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    applyRoundedMask();
}

void FluentCalendarPopup::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    applyRoundedMask();
}

void FluentCalendarPopup::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    const auto &c = ThemeManager::instance().colors();

    QPainter p(this);
    if (!p.isActive()) {
        return;
    }
    p.setRenderHint(QPainter::Antialiasing, true);

    // Surface
    const QRectF outer = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    p.setPen(QPen(c.border, 1.0));
    p.setBrush(c.surface);
    p.drawRoundedRect(outer, kCornerRadius, kCornerRadius);

    paintHeader(p);

    auto paintMode = [&](ViewMode mode) {
        if (mode == ViewMode::Days) {
            paintDays(p);
        } else if (mode == ViewMode::Months) {
            paintMonths(p);
        } else {
            paintYears(p);
        }
    };

    const bool transitioning = (m_modeAnim && m_modeAnim->state() == QAbstractAnimation::Running && m_modeProgress < 1.0);
    if (!transitioning) {
        paintMode(m_mode);
        return;
    }

    const qreal t = qBound<qreal>(0.0, m_modeProgress, 1.0);

    // prev mode
    p.save();
    p.setOpacity(1.0 - t);
    p.translate(0, -int(t * kModeSlidePx));
    paintMode(m_prevMode);
    p.restore();

    // next mode
    p.save();
    p.setOpacity(t);
    p.translate(0, int((1.0 - t) * kModeSlidePx));
    paintMode(m_mode);
    p.restore();
}

void FluentCalendarPopup::mouseMoveEvent(QMouseEvent *event)
{
    if (!event) {
        return;
    }

    int idx = -1;
    const HitPart part = hitTest(event->pos(), &idx);

    if (part != m_hoverPart || idx != m_hoverIndex) {
        m_hoverPart = part;
        m_hoverIndex = idx;
        update();
    }
}

void FluentCalendarPopup::mousePressEvent(QMouseEvent *event)
{
    if (!event || event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    int idx = -1;
    const HitPart part = hitTest(event->pos(), &idx);
    m_pressPart = part;
    m_pressIndex = idx;

    auto clearPress = [this]() {
        m_pressPart = HitPart::None;
        m_pressIndex = -1;
    };

    if (part == HitPart::NavPrev) {
        if (m_mode == ViewMode::Days) {
            stepMonth(-1);
        } else if (m_mode == ViewMode::Months) {
            stepYear(-1);
        } else {
            stepYearPage(-1);
        }
        clearPress();
        event->accept();
        return;
    }

    if (part == HitPart::NavNext) {
        if (m_mode == ViewMode::Days) {
            stepMonth(1);
        } else if (m_mode == ViewMode::Months) {
            stepYear(1);
        } else {
            stepYearPage(1);
        }
        clearPress();
        event->accept();
        return;
    }

    if (part == HitPart::HeaderMonth) {
        if (m_mode == ViewMode::Months) {
            setMode(ViewMode::Days);
        } else {
            setMode(ViewMode::Months);
        }
        clearPress();
        event->accept();
        return;
    }

    if (part == HitPart::HeaderYear) {
        if (m_mode == ViewMode::Years) {
            setMode(ViewMode::Days);
        } else {
            setMode(ViewMode::Years);
        }
        clearPress();
        event->accept();
        return;
    }

    if (part == HitPart::HeaderToday) {
        const QDate today = QDate::currentDate();
        if (today.isValid()) {
            m_selected = today;
            m_pageYear = today.year();
            m_pageMonth = today.month();
            if (m_mode != ViewMode::Days) {
                setMode(ViewMode::Days);
            } else {
                update();
            }
        }
        clearPress();
        event->accept();
        return;
    }

    if (part == HitPart::Cell && idx >= 0) {
        if (m_mode == ViewMode::Days) {
            const QDate start = gridStartForMonth(m_pageYear, m_pageMonth, Qt::Monday);
            if (start.isValid()) {
                const QDate d = start.addDays(idx);
                setSelectedDate(d, true);
            }
        } else if (m_mode == ViewMode::Months) {
            const int month = idx + 1;
            m_pageMonth = qBound(1, month, 12);
            m_selected = clampDayToMonth(m_selected, m_pageYear, m_pageMonth);
            setMode(ViewMode::Days);
            update();
        } else {
            const int year = m_yearBase + idx;
            m_pageYear = year;
            m_selected = clampDayToMonth(m_selected, m_pageYear, m_pageMonth);
            setMode(ViewMode::Months);
            update();
        }

        clearPress();
        event->accept();
        return;
    }

    clearPress();
    update();
    event->accept();
}

void FluentCalendarPopup::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    if (m_hoverPart != HitPart::None || m_hoverIndex != -1) {
        m_hoverPart = HitPart::None;
        m_hoverIndex = -1;
        update();
    }
}

void FluentCalendarPopup::wheelEvent(QWheelEvent *event)
{
    if (!event) {
        return;
    }

    const int dy = event->angleDelta().y();
    if (dy == 0) {
        return;
    }

    const int dir = (dy > 0) ? -1 : 1;
    if (m_mode == ViewMode::Days) {
        stepMonth(dir);
    } else if (m_mode == ViewMode::Months) {
        stepYear(dir);
    } else {
        stepYearPage(dir);
    }

    event->accept();
}

void FluentCalendarPopup::keyPressEvent(QKeyEvent *event)
{
    if (!event) {
        return;
    }

    if (event->key() == Qt::Key_Escape) {
        if (m_mode != ViewMode::Days) {
            setMode(ViewMode::Days);
        } else {
            dismiss();
        }
        event->accept();
        return;
    }

    if (m_mode == ViewMode::Days) {
        if (event->key() == Qt::Key_Up || event->key() == Qt::Key_PageUp) {
            stepMonth(-1);
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_Down || event->key() == Qt::Key_PageDown) {
            stepMonth(1);
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_Left) {
            setSelectedDate(m_selected.addDays(-1), false);
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_Right) {
            setSelectedDate(m_selected.addDays(1), false);
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            emit datePicked(m_selected);
            dismiss();
            event->accept();
            return;
        }
    }

    if (m_mode == ViewMode::Months) {
        if (event->key() == Qt::Key_Left) {
            m_pageMonth = qMax(1, m_pageMonth - 1);
            update();
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_Right) {
            m_pageMonth = qMin(12, m_pageMonth + 1);
            update();
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_Up) {
            m_pageMonth = qMax(1, m_pageMonth - 3);
            update();
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_Down) {
            m_pageMonth = qMin(12, m_pageMonth + 3);
            update();
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            m_selected = clampDayToMonth(m_selected, m_pageYear, m_pageMonth);
            setMode(ViewMode::Days);
            update();
            event->accept();
            return;
        }
    }

    if (m_mode == ViewMode::Years) {
        int current = m_pageYear;
        const int minY = m_yearBase;
        const int maxY = m_yearBase + 15;
        current = qBound(minY, current, maxY);

        int idx = current - m_yearBase;
        if (event->key() == Qt::Key_Left) {
            idx = qMax(0, idx - 1);
        } else if (event->key() == Qt::Key_Right) {
            idx = qMin(15, idx + 1);
        } else if (event->key() == Qt::Key_Up) {
            idx = qMax(0, idx - 4);
        } else if (event->key() == Qt::Key_Down) {
            idx = qMin(15, idx + 4);
        } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            m_pageYear = m_yearBase + idx;
            m_selected = clampDayToMonth(m_selected, m_pageYear, m_pageMonth);
            setMode(ViewMode::Months);
            update();
            event->accept();
            return;
        } else {
            QWidget::keyPressEvent(event);
            return;
        }

        m_pageYear = m_yearBase + idx;
        update();
        event->accept();
        return;
    }

    QWidget::keyPressEvent(event);
}

void FluentCalendarPopup::applyRoundedMask()
{
    applyRoundedWidgetMask(this, kCornerRadius);
}

void FluentCalendarPopup::positionPopupBelowOrAbove(int gap)
{
    if (!m_anchor) {
        return;
    }

    const QPoint globalTopLeft = m_anchor->mapToGlobal(QPoint(0, 0));
    QScreen *screen = QApplication::screenAt(globalTopLeft);
    if (!screen) {
        screen = QApplication::primaryScreen();
    }
    const QRect avail = screen ? screen->availableGeometry() : QRect();

    const QSize s = size();
    QPoint pos = m_anchor->mapToGlobal(QPoint(0, m_anchor->height() + gap));

    QRect r(pos, s);

    if (avail.isValid() && r.bottom() > avail.bottom()) {
        // try above
        const QPoint above = m_anchor->mapToGlobal(QPoint(0, -gap - s.height()));
        QRect r2(above, s);
        if (!avail.isValid() || r2.top() >= avail.top()) {
            r = r2;
        }
    }

    if (avail.isValid()) {
        if (r.left() < avail.left()) {
            r.moveLeft(avail.left());
        }
        if (r.right() > avail.right()) {
            r.moveRight(avail.right());
        }
        if (r.top() < avail.top()) {
            r.moveTop(avail.top());
        }
        if (r.bottom() > avail.bottom()) {
            r.moveBottom(avail.bottom());
        }
    }

    setGeometry(r);
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

    QFont f = font();
    f.setPointSizeF(f.pointSizeF() + 0.5);
    QFontMetrics fm(f);
    const int w = qBound(68, fm.horizontalAdvance(QString::number(m_pageMonth) + QStringLiteral("月")) + 22, 120);

    // month after year
    return pillRect(h, y.right() + 10 + 10, w);
}

QRect FluentCalendarPopup::yearPillRect() const
{
    const QRect h = headerRect();
    QFont f = font();
    f.setPointSizeF(f.pointSizeF() + 0.5);
    QFontMetrics fm(f);
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

    QFont f = font();
    f.setPointSizeF(f.pointSizeF() - 0.2);
    QFontMetrics fm(f);
    const QString text = QStringLiteral("今天");
    int w = qBound(44, fm.horizontalAdvance(text) + 20, 80);

    const int maxRight = prev.left() - 8;
    const int x = maxRight - w + 1;
    QRect r = pillRect(h, x, w);
    const int minLeft = monthPillRect().right() + 8;
    if (r.left() < minLeft) {
        return QRect();
    }
    return r;
}

void FluentCalendarPopup::ensurePageFromSelected()
{
    if (!m_selected.isValid()) {
        m_selected = QDate::currentDate();
    }
    m_pageYear = m_selected.year();
    m_pageMonth = m_selected.month();

    const int base = (m_pageYear / 16) * 16;
    m_yearBase = base;
}

void FluentCalendarPopup::setMode(ViewMode mode)
{
    if (m_mode == mode) {
        return;
    }

    startModeTransition(m_mode, mode);

    m_mode = mode;

    if (m_mode == ViewMode::Years) {
        const int base = (m_pageYear / 16) * 16;
        m_yearBase = base;
    }

    m_hoverPart = HitPart::None;
    m_hoverIndex = -1;
    update();
}

void FluentCalendarPopup::startModeTransition(ViewMode from, ViewMode to)
{
    Q_UNUSED(to)
    m_prevMode = from;
    if (!m_modeAnim) {
        m_modeProgress = 1.0;
        return;
    }
    m_modeAnim->stop();
    m_modeProgress = 0.0;
    m_modeAnim->setStartValue(0.0);
    m_modeAnim->setEndValue(1.0);
    m_modeAnim->start();
}

void FluentCalendarPopup::stepMonth(int delta)
{
    const QDate pageFirst(m_pageYear, m_pageMonth, 1);
    const QDate nextFirst = pageFirst.addMonths(delta);
    if (!nextFirst.isValid()) {
        return;
    }

    m_pageYear = nextFirst.year();
    m_pageMonth = nextFirst.month();
    m_selected = clampDayToMonth(m_selected, m_pageYear, m_pageMonth);

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

void FluentCalendarPopup::setSelectedDate(const QDate &date, bool emitPicked)
{
    if (!date.isValid()) {
        return;
    }

    m_selected = date;
    m_pageYear = date.year();
    m_pageMonth = date.month();

    if (emitPicked) {
        emit datePicked(m_selected);
        dismiss();
        return;
    }

    update();
}

int FluentCalendarPopup::cellIndexAt(const QPoint &pos) const
{
    const QRect g = gridRect();
    if (!g.contains(pos)) {
        return -1;
    }

    if (m_mode == ViewMode::Days) {
        const QRect dayNames(g.x(), g.y(), g.width(), kDayNamesH);
        if (dayNames.contains(pos)) {
            return -1;
        }
        const QRect cells = QRect(g.x(), g.y() + kDayNamesH, g.width(), g.height() - kDayNamesH);
        const int cw = cells.width() / 7;
        const int ch = cells.height() / 6;
        if (cw <= 0 || ch <= 0) {
            return -1;
        }
        const int col = (pos.x() - cells.x()) / cw;
        const int row = (pos.y() - cells.y()) / ch;
        if (col < 0 || col >= 7 || row < 0 || row >= 6) {
            return -1;
        }
        return row * 7 + col;
    }

    if (m_mode == ViewMode::Months) {
        const int cw = g.width() / 3;
        const int ch = g.height() / 4;
        const int col = (pos.x() - g.x()) / cw;
        const int row = (pos.y() - g.y()) / ch;
        if (col < 0 || col >= 3 || row < 0 || row >= 4) {
            return -1;
        }
        const int idx = row * 3 + col;
        return (idx >= 0 && idx < 12) ? idx : -1;
    }

    // years: 4x4
    const int cw = g.width() / 4;
    const int ch = g.height() / 4;
    const int col = (pos.x() - g.x()) / cw;
    const int row = (pos.y() - g.y()) / ch;
    if (col < 0 || col >= 4 || row < 0 || row >= 4) {
        return -1;
    }
    const int idx = row * 4 + col;
    return (idx >= 0 && idx < 16) ? idx : -1;
}

FluentCalendarPopup::HitPart FluentCalendarPopup::hitTest(const QPoint &pos, int *outIndex) const
{
    if (outIndex) {
        *outIndex = -1;
    }

    if (monthPillRect().contains(pos)) {
        return HitPart::HeaderMonth;
    }
    if (yearPillRect().contains(pos)) {
        return HitPart::HeaderYear;
    }

    const QRect today = todayButtonRect();
    if (today.isValid() && today.contains(pos)) {
        return HitPart::HeaderToday;
    }

    if (navPrevRect().contains(pos)) {
        return HitPart::NavPrev;
    }
    if (navNextRect().contains(pos)) {
        return HitPart::NavNext;
    }

    const int idx = cellIndexAt(pos);
    if (idx >= 0) {
        if (outIndex) {
            *outIndex = idx;
        }
        return HitPart::Cell;
    }

    return HitPart::None;
}

void FluentCalendarPopup::paintHeader(QPainter &p)
{
    const auto &c = ThemeManager::instance().colors();

    const QRect h = headerRect();
    const QRect y = yearPillRect();
    const QRect m = monthPillRect();
    const QRect t = todayButtonRect();

    // subtle bottom divider under header
    QColor headerLine = c.border;
    headerLine.setAlpha(80);
    p.setPen(QPen(headerLine, 1.0));
    p.drawLine(QPointF(h.left(), h.bottom() + 0.5), QPointF(h.right(), h.bottom() + 0.5));

    // vertical separator between year and month
    QColor sep = c.border;
    sep.setAlpha(90);
    const int sx = y.right() + 10;
    const int sy = h.y() + (h.height() - 18) / 2;
    p.setPen(QPen(sep, 1.0));
    p.drawLine(QPointF(sx + 0.5, sy), QPointF(sx + 0.5, sy + 18));

    auto paintPill = [&](const QRect &r, HitPart part, const QString &text, bool active) {
        const bool hovered = (m_hoverPart == part);
        const bool pressed = (m_pressPart == part);

        QColor bg = Qt::transparent;
        QColor textColor = c.text;

        if (active) {
            bg = c.accent;
            bg.setAlpha(220);
            textColor = QColor("#FFFFFF");

            if (pressed) {
                bg = bg.darker(112);
            } else if (hovered) {
                bg = bg.lighter(106);
            }
        } else {
            if (pressed) {
                bg = c.pressed;
                bg.setAlpha(150);
            } else if (hovered) {
                bg = c.hover;
                bg.setAlpha(120);
            }
        }

        p.setPen(Qt::NoPen);
        p.setBrush(bg);
        p.drawRoundedRect(QRectF(r).adjusted(0.5, 0.5, -0.5, -0.5), 8.0, 8.0);

        p.setPen(textColor);
        p.drawText(r.adjusted(12, 0, -12, 0), Qt::AlignVCenter | Qt::AlignLeft, text);
    };

    paintPill(y, HitPart::HeaderYear, QString::number(m_pageYear) + QStringLiteral("年"), false);
    paintPill(m, HitPart::HeaderMonth, QString::number(m_pageMonth) + QStringLiteral("月"), false);

    if (t.isValid()) {
        const bool hovered = (m_hoverPart == HitPart::HeaderToday);
        const bool pressed = (m_pressPart == HitPart::HeaderToday);

        QColor bg = Qt::transparent;
        if (pressed) {
            bg = c.pressed;
            bg.setAlpha(150);
        } else if (hovered) {
            bg = c.hover;
            bg.setAlpha(120);
        }

        p.setPen(Qt::NoPen);
        p.setBrush(bg);
        p.drawRoundedRect(QRectF(t).adjusted(0.5, 0.5, -0.5, -0.5), 8.0, 8.0);

        p.setPen(c.accent);
        p.drawText(t, Qt::AlignCenter, QStringLiteral("今天"));
    }

    auto paintNav = [&](const QRect &r, HitPart part, bool right) {
        const bool hovered = (m_hoverPart == part);
        const bool pressed = (m_pressPart == part);

        QColor bg = Qt::transparent;
        if (pressed) {
            bg = c.pressed;
            bg.setAlpha(150);
        } else if (hovered) {
            bg = c.hover;
            bg.setAlpha(120);
        }

        p.setPen(Qt::NoPen);
        p.setBrush(bg);
        p.drawRoundedRect(QRectF(r).adjusted(0.5, 0.5, -0.5, -0.5), 8.0, 8.0);

        const QColor icon = c.subText;
        if (right) {
            drawChevronRight(p, r.center(), icon);
        } else {
            drawChevronLeft(p, r.center(), icon);
        }
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
    const QRect cells(g.x(), g.y() + kDayNamesH, g.width(), g.height() - kDayNamesH);

    const int cw = cells.width() / 7;
    const int ch = cells.height() / 6;
    if (cw <= 0 || ch <= 0) {
        return;
    }

    // Weekend column highlight (Sat/Sun) - non-accent background
    // Columns are based on Monday-first layout: 0..6 => Mon..Sun
    {
        QColor weekendBg = c.pressed;
        weekendBg.setAlpha(55);
        p.setPen(Qt::NoPen);
        p.setBrush(weekendBg);

        const int totalH = dayNames.height() + cells.height();
        for (int col : {5, 6}) {
            const QRect rc(g.x() + col * cw, dayNames.y(), cw, totalH);
            p.drawRect(QRectF(rc).adjusted(0.0, 1.0, -0.5, -1.0));
        }
    }

    // Day names
    QFont f = font();
    f.setPointSizeF(f.pointSizeF() - 0.5);
    p.setFont(f);
    p.setPen(c.subText);

    for (int i = 0; i < 7; ++i) {
        const QRect r(dayNames.x() + i * cw, dayNames.y(), cw, dayNames.height());
        const int dow = (int(Qt::Monday) + i);
        const QString name = QLocale().standaloneDayName(((dow - 1) % 7) + 1, QLocale::ShortFormat);

        if (i >= 5) {
            p.setPen(c.text);
        } else {
            p.setPen(c.subText);
        }
        p.drawText(r, Qt::AlignCenter, name);
    }

    const QDate start = gridStartForMonth(m_pageYear, m_pageMonth, Qt::Monday);
    if (!start.isValid()) {
        return;
    }

    const QDate today = QDate::currentDate();

    QFont cellFont = font();
    p.setFont(cellFont);

    for (int row = 0; row < 6; ++row) {
        for (int col = 0; col < 7; ++col) {
            const int idx = row * 7 + col;
            const QDate d = start.addDays(idx);
            const QRect rc(cells.x() + col * cw, cells.y() + row * ch, cw, ch);
            const QRectF rr = QRectF(rc).adjusted(6.0, 4.0, -6.0, -4.0);

            const bool inMonth = (d.year() == m_pageYear && d.month() == m_pageMonth);
            const bool selected = (d == m_selected);
            const bool isToday = (d == today);
            const bool hovered = (m_hoverPart == HitPart::Cell && m_hoverIndex == idx);
            const bool isWeekendCol = (col >= 5);

            if (selected) {
                QColor fill = c.accent;
                fill.setAlpha(210);
                p.setPen(Qt::NoPen);
                p.setBrush(fill);
                p.drawRoundedRect(rr, 8.0, 8.0);
            } else if (hovered) {
                QColor fill = c.hover;
                fill.setAlpha(120);
                p.setPen(Qt::NoPen);
                p.setBrush(fill);
                p.drawRoundedRect(rr, 8.0, 8.0);
            }

            if (isToday && !selected) {
                QColor ring = c.accent;
                ring.setAlpha(170);
                p.setPen(QPen(ring, 1.5));
                p.setBrush(Qt::NoBrush);
                p.drawRoundedRect(rr, 8.0, 8.0);
            }

            QColor text;
            if (!inMonth) {
                text = c.disabledText;
            } else if (selected) {
                text = QColor("#FFFFFF");
            } else if (isWeekendCol) {
                text = c.subText;
            } else {
                text = c.text;
            }

            p.setPen(text);
            p.drawText(rc, Qt::AlignCenter, QString::number(d.day()));
        }
    }
}

void FluentCalendarPopup::paintMonths(QPainter &p)
{
    const auto &c = ThemeManager::instance().colors();
    const QRect g = gridRect();
    const int cw = g.width() / 3;
    const int ch = g.height() / 4;

    const QDate today = QDate::currentDate();

    QFont f = font();
    f.setPointSizeF(f.pointSizeF() + 0.2);
    p.setFont(f);

    for (int i = 0; i < 12; ++i) {
        const int row = i / 3;
        const int col = i % 3;
        const QRect rc(g.x() + col * cw, g.y() + row * ch, cw, ch);
        const QRectF rr = QRectF(rc).adjusted(8.0, 8.0, -8.0, -8.0);

        const bool selected = ((i + 1) == m_pageMonth);
        const bool isCurrent = (today.isValid() && m_pageYear == today.year() && (i + 1) == today.month());
        const bool hovered = (m_hoverPart == HitPart::Cell && m_hoverIndex == i);

        if (selected) {
            QColor fill = c.accent;
            fill.setAlpha(210);
            p.setPen(Qt::NoPen);
            p.setBrush(fill);
            p.drawRoundedRect(rr, 10.0, 10.0);
        } else if (hovered) {
            QColor fill = c.hover;
            fill.setAlpha(120);
            p.setPen(Qt::NoPen);
            p.setBrush(fill);
            p.drawRoundedRect(rr, 10.0, 10.0);
        }

        if (isCurrent && !selected) {
            QColor ring = c.accent;
            ring.setAlpha(170);
            p.setPen(QPen(ring, 1.5));
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(rr, 10.0, 10.0);
        }

        QColor text = selected ? QColor("#FFFFFF") : c.text;
        p.setPen(text);
        p.drawText(rc, Qt::AlignCenter, QLocale().standaloneMonthName(i + 1, QLocale::ShortFormat));
    }
}

void FluentCalendarPopup::paintYears(QPainter &p)
{
    const auto &c = ThemeManager::instance().colors();
    const QRect g = gridRect();
    const int cw = g.width() / 4;
    const int ch = g.height() / 4;

    const QDate today = QDate::currentDate();
    const int todayYear = today.isValid() ? today.year() : -1;

    QFont f = font();
    f.setPointSizeF(f.pointSizeF() + 0.2);
    p.setFont(f);

    for (int i = 0; i < 16; ++i) {
        const int row = i / 4;
        const int col = i % 4;
        const QRect rc(g.x() + col * cw, g.y() + row * ch, cw, ch);
        const QRectF rr = QRectF(rc).adjusted(8.0, 8.0, -8.0, -8.0);

        const int year = m_yearBase + i;
        const bool selected = (year == m_pageYear);
        const bool isCurrent = (todayYear > 0 && year == todayYear);
        const bool hovered = (m_hoverPart == HitPart::Cell && m_hoverIndex == i);

        if (selected) {
            QColor fill = c.accent;
            fill.setAlpha(210);
            p.setPen(Qt::NoPen);
            p.setBrush(fill);
            p.drawRoundedRect(rr, 10.0, 10.0);
        } else if (hovered) {
            QColor fill = c.hover;
            fill.setAlpha(120);
            p.setPen(Qt::NoPen);
            p.setBrush(fill);
            p.drawRoundedRect(rr, 10.0, 10.0);
        }

        if (isCurrent && !selected) {
            QColor ring = c.accent;
            ring.setAlpha(170);
            p.setPen(QPen(ring, 1.5));
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(rr, 10.0, 10.0);
        }

        QColor text = selected ? QColor("#FFFFFF") : c.text;
        p.setPen(text);
        p.drawText(rc, Qt::AlignCenter, QString::number(year));
    }

    // range label subtle
    const QRect h = headerRect();
    const QRect rangeRect(h.x(), h.y(), h.width(), h.height());
    Q_UNUSED(rangeRect)
}

void FluentCalendarPopup::drawChevronLeft(QPainter &p, const QPointF &center, const QColor &color) const
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(QPen(color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

    const qreal s = 6.0;
    QPainterPath path;
    path.moveTo(center.x() + s / 2, center.y() - s);
    path.lineTo(center.x() - s / 2, center.y());
    path.lineTo(center.x() + s / 2, center.y() + s);
    p.drawPath(path);
    p.restore();
}

void FluentCalendarPopup::drawChevronRight(QPainter &p, const QPointF &center, const QColor &color) const
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(QPen(color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

    const qreal s = 6.0;
    QPainterPath path;
    path.moveTo(center.x() - s / 2, center.y() - s);
    path.lineTo(center.x() + s / 2, center.y());
    path.lineTo(center.x() - s / 2, center.y() + s);
    p.drawPath(path);
    p.restore();
}

} // namespace Fluent
