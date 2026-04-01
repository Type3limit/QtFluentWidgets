#include "ColorPickerWidgets.h"

#include "Fluent/FluentTheme.h"

#include <QApplication>
#include <QContextMenuEvent>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QEnterEvent>
#endif
#include <QGradient>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>
#include <cmath>

#include <algorithm>

namespace Fluent::ColorPicker {

static QString toHexArgb(const QColor &c)
{
    if (!c.isValid()) {
        return QString();
    }
    const auto a = QString::number(c.alpha(), 16).rightJustified(2, QLatin1Char('0')).toUpper();
    const auto r = QString::number(c.red(), 16).rightJustified(2, QLatin1Char('0')).toUpper();
    const auto g = QString::number(c.green(), 16).rightJustified(2, QLatin1Char('0')).toUpper();
    const auto b = QString::number(c.blue(), 16).rightJustified(2, QLatin1Char('0')).toUpper();
    return QStringLiteral("#%1%2%3%4").arg(a, r, g, b);
}

static QPointF mouseLocalPosF(QMouseEvent *event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return event->position();
#else
    return event->localPos();
#endif
}

ColorSwatchButton::ColorSwatchButton(const QColor &color, QWidget *parent)
    : QWidget(parent)
    , m_color(color)
{
    setFixedSize(18, 18);
    setCursor(Qt::PointingHandCursor);
    setToolTip(toHexArgb(m_color));
}

void ColorSwatchButton::setColor(const QColor &c)
{
    m_color = c;
    setToolTip(toHexArgb(m_color));
    update();
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void ColorSwatchButton::enterEvent(FluentEnterEvent *event)
{
    Q_UNUSED(event)
    m_hover = true;
    update();
}
#else
void ColorSwatchButton::enterEvent(FluentEnterEvent *event)
{
    Q_UNUSED(event)
    m_hover = true;
    update();
}
#endif

void ColorSwatchButton::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    m_hover = false;
    update();
}

void ColorSwatchButton::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked(m_color);
    }
    QWidget::mousePressEvent(event);
}

void ColorSwatchButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const auto &tc = ThemeManager::instance().colors();
    QColor border = tc.border;
    if (m_hover) {
        border = tc.accent;
    }
    border.setAlpha(220);

    const QRectF r = QRectF(rect()).adjusted(1.0, 1.0, -1.0, -1.0);
    p.setPen(QPen(border, 1.0));

    if (m_color.alpha() < 255) {
        const int s = 4;
        for (int y = 0; y < height(); y += s) {
            for (int x = 0; x < width(); x += s) {
                const bool dark = ((x / s) + (y / s)) % 2 == 0;
                p.fillRect(QRect(x, y, s, s), dark ? QColor(0, 0, 0, 40) : QColor(255, 255, 255, 50));
            }
        }
    }

    p.setBrush(m_color);
    p.drawRoundedRect(r, 5.0, 5.0);
}

SvPanel::SvPanel(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(200);
    setMinimumWidth(120);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setCursor(Qt::CrossCursor);
}

void SvPanel::setHue(int h)
{
    h = qBound(0, h, 359);
    if (m_h == h)
        return;
    m_h = h;
    update();
}

void SvPanel::setSv(int s, int v)
{
    s = qBound(0, s, 255);
    v = qBound(0, v, 255);
    if (m_s == s && m_v == v)
        return;
    m_s = s;
    m_v = v;
    update();
}

void SvPanel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        updateFromPos(mouseLocalPosF(event).toPoint());
    }
    QWidget::mousePressEvent(event);
}

void SvPanel::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        updateFromPos(mouseLocalPosF(event).toPoint());
    }
    QWidget::mouseMoveEvent(event);
}

void SvPanel::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_dragging) {
        updateFromPos(mouseLocalPosF(event).toPoint());
        m_dragging = false;
        emit svChangeFinished(m_s, m_v);
    }
    QWidget::mouseReleaseEvent(event);
}

void SvPanel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRect r = rect().adjusted(1, 1, -1, -1);
    const qreal radius = 8.0;
    const QRectF rf = QRectF(r);

    QPainterPath clip;
    clip.addRoundedRect(rf, radius, radius);
    p.save();
    p.setClipPath(clip);

    const QColor hueColor = QColor::fromHsv(m_h, 255, 255);
    p.fillRect(r, hueColor);

    QLinearGradient wGrad(r.left(), r.top(), r.right(), r.top());
    wGrad.setColorAt(0.0, QColor(255, 255, 255));
    wGrad.setColorAt(1.0, QColor(255, 255, 255, 0));
    p.fillRect(r, wGrad);

    QLinearGradient bGrad(r.left(), r.top(), r.left(), r.bottom());
    bGrad.setColorAt(0.0, QColor(0, 0, 0, 0));
    bGrad.setColorAt(1.0, QColor(0, 0, 0));
    p.fillRect(r, bGrad);

    p.restore();

    const auto &tc = ThemeManager::instance().colors();
    p.setPen(QPen(tc.border, 1.0));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(QRectF(r).adjusted(0.5, 0.5, -0.5, -0.5), radius, radius);

    const qreal x = r.left() + (m_s / 255.0) * r.width();
    const qreal y = r.top() + ((255 - m_v) / 255.0) * r.height();

    p.setPen(QPen(QColor(0, 0, 0, 160), 3.0));
    p.drawEllipse(QPointF(x, y), 7.0, 7.0);
    p.setPen(QPen(QColor(255, 255, 255, 230), 2.0));
    p.drawEllipse(QPointF(x, y), 7.0, 7.0);
}

void SvPanel::updateFromPos(const QPoint &pos)
{
    const QRect r = rect().adjusted(1, 1, -1, -1);
    const int sx = qBound(r.left(), pos.x(), r.right());
    const int sy = qBound(r.top(), pos.y(), r.bottom());
    const int s = qRound(((sx - r.left()) / qMax(1.0, (double)r.width())) * 255.0);
    const int v = 255 - qRound(((sy - r.top()) / qMax(1.0, (double)r.height())) * 255.0);
    setSv(s, v);
    emit svChanged(m_s, m_v);
}

PreviewSwatch::PreviewSwatch(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(28);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void PreviewSwatch::setColor(const QColor &c)
{
    if (m_color == c) {
        return;
    }
    m_color = c;
    update();
}

void PreviewSwatch::setGradientPreview(const QGradientStops &stops)
{
    m_gradStops = stops;
    m_showGradient = (stops.size() >= 2);
    update();
}

void PreviewSwatch::clearGradientPreview()
{
    m_gradStops.clear();
    m_showGradient = false;
    update();
}

void PreviewSwatch::setRadialMode(bool radial)
{
    if (m_isRadial == radial) return;
    m_isRadial = radial;
    if (m_showGradient) update();
}

void PreviewSwatch::setGradientAngle(int angleDegrees)
{
    m_gradAngle = angleDegrees;
    if (m_showGradient && !m_isRadial) update();
}

void PreviewSwatch::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const auto &tc = ThemeManager::instance().colors();
    const QRect r = rect().adjusted(1, 1, -1, -1);
    const qreal radius = 8.0;
    const QRectF rf = QRectF(r);

    QPainterPath clip;
    clip.addRoundedRect(rf, radius, radius);
    p.save();
    p.setClipPath(clip);

    const int s = 6;
    for (int y = r.top(); y <= r.bottom(); y += s) {
        for (int x = r.left(); x <= r.right(); x += s) {
            const bool dark = (((x - r.left()) / s) + ((y - r.top()) / s)) % 2 == 0;
            p.fillRect(QRect(x, y, s, s), dark ? QColor(0, 0, 0, 35) : QColor(255, 255, 255, 45));
        }
    }

    if (m_showGradient && m_gradStops.size() >= 2) {
        if (m_isRadial) {
            // Radial gradient from center outward
            const QPointF center = rf.center();
            const qreal rad = qMin(rf.width(), rf.height()) / 2.0;
            QRadialGradient grad(center, rad, center);
            for (const auto &stop : m_gradStops)
                grad.setColorAt(stop.first, stop.second);
            p.fillRect(r, QBrush(grad));
        } else {
            // Angled linear gradient — angle 0 = left→right, 90 = top→bottom
            const qreal angleRad = qDegreesToRadians((qreal)m_gradAngle);
            const qreal cosA = std::cos(angleRad);
            const qreal sinA = std::sin(angleRad);
            const QPointF c = rf.center();
            const qreal hw = rf.width()  / 2.0;
            const qreal hh = rf.height() / 2.0;
            // Half-length that covers all four corners of the rect
            const qreal halfLen = std::abs(cosA) * hw + std::abs(sinA) * hh;
            const QPointF start = c - QPointF(cosA * halfLen, sinA * halfLen);
            const QPointF end   = c + QPointF(cosA * halfLen, sinA * halfLen);
            QLinearGradient grad(start, end);
            for (const auto &stop : m_gradStops)
                grad.setColorAt(stop.first, stop.second);
            p.fillRect(r, QBrush(grad));
        }
    } else {
        p.fillRect(r, m_color);
    }

    p.restore();

    p.setPen(QPen(tc.border, 1.0));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(QRectF(r).adjusted(0.5, 0.5, -0.5, -0.5), radius, radius);
}

HueStrip::HueStrip(QWidget *parent)
    : QWidget(parent)
{
    // Default: vertical (same historical geometry)
    setFixedWidth(26);
    setCursor(Qt::PointingHandCursor);
    setMouseTracking(true);
}

void HueStrip::setOrientation(Qt::Orientation o)
{
    if (m_orientation == o)
        return;
    m_orientation = o;
    if (o == Qt::Horizontal) {
        setMinimumWidth(0);
        setMaximumWidth(QWIDGETSIZE_MAX);
        setFixedHeight(22);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    } else {
        setMinimumHeight(0);
        setMaximumHeight(QWIDGETSIZE_MAX);
        setFixedWidth(26);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    }
    update();
}

void HueStrip::setValue(int v)
{
    v = qBound(0, v, 359);
    if (m_value == v)
        return;
    m_value = v;
    update();
}

void HueStrip::enterEvent(FluentEnterEvent *event)
{
    Q_UNUSED(event)
    m_hover = true;
    update();
}

void HueStrip::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    m_hover = false;
    update();
}

void HueStrip::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        const QPointF lp = mouseLocalPosF(event);
        setFromPos(m_orientation == Qt::Horizontal ? lp.x() / qMax(1.0, (double)width())
                                                   : lp.y() / qMax(1.0, (double)height()),
                   true);
    }
    QWidget::mousePressEvent(event);
}

void HueStrip::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging) {
        const QPointF lp = mouseLocalPosF(event);
        setFromPos(m_orientation == Qt::Horizontal ? lp.x() / qMax(1.0, (double)width())
                                                   : lp.y() / qMax(1.0, (double)height()),
                   true);
    }
    QWidget::mouseMoveEvent(event);
}

void HueStrip::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        const QPointF lp = mouseLocalPosF(event);
        setFromPos(m_orientation == Qt::Horizontal ? lp.x() / qMax(1.0, (double)width())
                                                   : lp.y() / qMax(1.0, (double)height()),
                   true);
        m_dragging = false;
        emit valueChangeFinished(m_value);
    }
    QWidget::mouseReleaseEvent(event);
}

void HueStrip::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const auto &tc = ThemeManager::instance().colors();
    const QRect outer = rect().adjusted(1, 1, -1, -1);

    if (m_orientation == Qt::Horizontal) {
        // ── Horizontal mode ──────────────────────────────────────────────
        const int trackH = 14;
        const int padX = 6;
        const QRect track(outer.left() + padX, outer.center().y() - trackH / 2,
                          qMax(10, outer.width() - padX * 2), trackH);

        QLinearGradient grad(track.left(), track.top(), track.right(), track.top());
        grad.setColorAt(0.00,       QColor("#FF0000"));
        grad.setColorAt(1.0 / 6.0, QColor("#FFFF00"));
        grad.setColorAt(2.0 / 6.0, QColor("#00FF00"));
        grad.setColorAt(3.0 / 6.0, QColor("#00FFFF"));
        grad.setColorAt(4.0 / 6.0, QColor("#0000FF"));
        grad.setColorAt(5.0 / 6.0, QColor("#FF00FF"));
        grad.setColorAt(1.00,       QColor("#FF0000"));

        const qreal radius = 7.0;
        p.setPen(QPen(tc.border, 1.0));
        p.setBrush(grad);
        p.drawRoundedRect(QRectF(track).adjusted(0.5, 0.5, -0.5, -0.5), radius, radius);

        // Vertical handle
        const qreal t = m_value / 359.0;
        const qreal x = track.left() + t * track.width();
        const qreal handleW = 14.0;
        const qreal handleH = qMin((qreal)outer.height() - 6.0, (qreal)trackH + 18.0);
        const qreal xx = qBound((qreal)outer.left() + 3.0, x - handleW / 2.0, (qreal)outer.right() - handleW - 3.0);
        const qreal yy = outer.center().y() - handleH / 2.0;
        const QRectF handleRect(xx, yy, handleW, handleH);

        QColor outline = m_dragging ? tc.accent : tc.border;
        if (m_hover && !m_dragging)
            outline = tc.hover;
        outline.setAlpha(220);

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 40));
        p.drawRoundedRect(handleRect.translated(0.0, 1.0), 7.0, 7.0);

        p.setPen(QPen(outline, 1.0));
        p.setBrush(QColor(255, 255, 255, 238));
        p.drawRoundedRect(handleRect, 7.0, 7.0);

        p.setPen(QPen(QColor(255, 255, 255, 160), 1.0));
        p.drawLine(QPointF(handleRect.left() + 4.0, handleRect.top() + 3.0),
                   QPointF(handleRect.left() + 4.0, handleRect.bottom() - 3.0));
    } else {
        // ── Vertical mode (original) ──────────────────────────────────────
        const int trackW = 14;
        const int padY = 6;
        const QRect track(outer.center().x() - trackW / 2, outer.top() + padY,
                          trackW, qMax(10, outer.height() - padY * 2));

        QLinearGradient grad(track.left(), track.top(), track.left(), track.bottom());
        grad.setColorAt(0.00,       QColor("#FF0000"));
        grad.setColorAt(1.0 / 6.0, QColor("#FFFF00"));
        grad.setColorAt(2.0 / 6.0, QColor("#00FF00"));
        grad.setColorAt(3.0 / 6.0, QColor("#00FFFF"));
        grad.setColorAt(4.0 / 6.0, QColor("#0000FF"));
        grad.setColorAt(5.0 / 6.0, QColor("#FF00FF"));
        grad.setColorAt(1.00,       QColor("#FF0000"));

        const qreal radius = 7.0;
        p.setPen(QPen(tc.border, 1.0));
        p.setBrush(grad);
        p.drawRoundedRect(QRectF(track).adjusted(0.5, 0.5, -0.5, -0.5), radius, radius);

        const qreal t = m_value / 359.0;
        const qreal y = track.top() + t * track.height();
        const qreal handleH = 14.0;
        const qreal handleW = qMin((qreal)outer.width() - 6.0, (qreal)trackW + 18.0);
        const qreal yy = qBound((qreal)outer.top() + 3.0, y - handleH / 2.0, (qreal)outer.bottom() - handleH - 3.0);
        const qreal xx = outer.center().x() - handleW / 2.0;
        const QRectF handleRect(xx, yy, handleW, handleH);

        QColor outline = m_dragging ? tc.accent : tc.border;
        if (m_hover && !m_dragging)
            outline = tc.hover;
        outline.setAlpha(220);

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 40));
        p.drawRoundedRect(handleRect.translated(0.0, 1.0), 7.0, 7.0);

        p.setPen(QPen(outline, 1.0));
        p.setBrush(QColor(255, 255, 255, 238));
        p.drawRoundedRect(handleRect, 7.0, 7.0);

        p.setPen(QPen(QColor(255, 255, 255, 160), 1.0));
        p.drawLine(QPointF(handleRect.left() + 3.0, handleRect.top() + 4.0),
                   QPointF(handleRect.right() - 3.0, handleRect.top() + 4.0));
    }
}

void HueStrip::setFromPos(qreal normalized, bool emitSignal)
{
    normalized = qBound(0.0, normalized, 1.0);
    const int v = qBound(0, qRound(normalized * 359.0), 359);
    if (v == m_value)
        return;
    m_value = v;
    update();
    if (emitSignal)
        emit valueChanged(m_value);
}

AlphaStrip::AlphaStrip(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(22);
    setCursor(Qt::PointingHandCursor);
    setMouseTracking(true);
}

void AlphaStrip::setValue(int v)
{
    v = qBound(0, v, 255);
    if (m_value == v) {
        return;
    }
    m_value = v;
    update();
}

void AlphaStrip::setBaseColor(const QColor &c)
{
    if (m_baseColor == c) {
        return;
    }
    m_baseColor = c;
    update();
}

void AlphaStrip::enterEvent(FluentEnterEvent *event)
{
    Q_UNUSED(event)
    m_hover = true;
    update();
}

void AlphaStrip::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    m_hover = false;
    update();
}

void AlphaStrip::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        setFromX(mouseLocalPosF(event).x(), true);
    }
    QWidget::mousePressEvent(event);
}

void AlphaStrip::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging) {
        setFromX(mouseLocalPosF(event).x(), true);
    }
    QWidget::mouseMoveEvent(event);
}

void AlphaStrip::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        setFromX(mouseLocalPosF(event).x(), true);
        m_dragging = false;
        emit valueChangeFinished(m_value);
    }
    QWidget::mouseReleaseEvent(event);
}

void AlphaStrip::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const auto &tc = ThemeManager::instance().colors();
    const QRect outer = rect().adjusted(1, 1, -1, -1);
    const int trackH = 14;
    const int padX = 6;
    const QRect track(outer.left() + padX, outer.center().y() - trackH / 2, qMax(10, outer.width() - padX * 2), trackH);

    const qreal radius = 7.0;

    p.save();
    QPainterPath clip;
    clip.addRoundedRect(QRectF(track).adjusted(0.5, 0.5, -0.5, -0.5), radius, radius);
    p.setClipPath(clip);

    const int s = 6;
    for (int y = track.top(); y <= track.bottom(); y += s) {
        for (int x = track.left(); x <= track.right(); x += s) {
            const bool dark = (((x - track.left()) / s) + ((y - track.top()) / s)) % 2 == 0;
            p.fillRect(QRect(x, y, s, s), dark ? QColor(0, 0, 0, 35) : QColor(255, 255, 255, 45));
        }
    }

    QColor left = m_baseColor;
    left.setAlpha(0);
    QColor right = m_baseColor;
    right.setAlpha(255);
    QLinearGradient grad(track.left(), track.top(), track.right(), track.top());
    grad.setColorAt(0.0, left);
    grad.setColorAt(1.0, right);
    p.fillRect(track, grad);
    p.restore();

    p.setPen(QPen(tc.border, 1.0));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(QRectF(track).adjusted(0.5, 0.5, -0.5, -0.5), radius, radius);

    const qreal t = m_value / 255.0;
    const qreal x = track.left() + t * track.width();
    const qreal handleW = 14.0;
    const qreal handleH = qMin((qreal)outer.height() - 6.0, (qreal)trackH + 18.0);
    const qreal xx = qBound((qreal)outer.left() + 3.0, x - handleW / 2.0, (qreal)outer.right() - handleW - 3.0);
    const qreal yy = outer.center().y() - handleH / 2.0;
    const QRectF handleRect(xx, yy, handleW, handleH);

    QColor outline = m_dragging ? tc.accent : tc.border;
    if (m_hover && !m_dragging) {
        outline = tc.hover;
    }
    outline.setAlpha(220);

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 40));
    p.drawRoundedRect(handleRect.translated(0.0, 1.0), 7.0, 7.0);

    p.setPen(QPen(outline, 1.0));
    p.setBrush(QColor(255, 255, 255, 238));
    p.drawRoundedRect(handleRect, 7.0, 7.0);

    p.setPen(QPen(QColor(255, 255, 255, 150), 1.0));
    p.drawLine(QPointF(handleRect.left() + 3.0, handleRect.top() + 4.0), QPointF(handleRect.right() - 3.0, handleRect.top() + 4.0));
}

void AlphaStrip::setFromX(qreal x, bool emitSignal)
{
    const QRect outer = rect().adjusted(1, 1, -1, -1);
    const int trackH = 14;
    const int padX = 6;
    const QRect track(outer.left() + padX, outer.center().y() - trackH / 2, qMax(10, outer.width() - padX * 2), trackH);

    const qreal xx = qBound((qreal)track.left(), x, (qreal)track.right());
    const qreal t = (xx - track.left()) / qMax(1, track.width());
    const int v = qBound(0, qRound(t * 255.0), 255);
    if (v == m_value) {
        return;
    }
    m_value = v;
    update();
    if (emitSignal) {
        emit valueChanged(m_value);
    }
}

// ────────────────────────────────────────────────────────────────────────────
// EyeDropperButton
// ────────────────────────────────────────────────────────────────────────────

EyeDropperButton::EyeDropperButton(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(30, 30);
    setCursor(Qt::PointingHandCursor);
    setToolTip(tr("从屏幕取色"));
}

void EyeDropperButton::enterEvent(FluentEnterEvent *event)
{
    Q_UNUSED(event)
    m_hover = true;
    update();
}

void EyeDropperButton::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    m_hover = false;
    update();
}

void EyeDropperButton::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_pressed = true;
        update();
    }
    QWidget::mousePressEvent(event);
}

void EyeDropperButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_pressed) {
        m_pressed = false;
        update();
        if (rect().contains(mouseLocalPosF(event).toPoint()))
            emit clicked();
    }
    QWidget::mouseReleaseEvent(event);
}

void EyeDropperButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const auto &tc = ThemeManager::instance().colors();
    const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);

    // Background
    QColor bg = tc.surface;
    if (m_pressed)
        bg = tc.pressed;
    else if (m_hover)
        bg = tc.hover;

    p.setPen(QPen(tc.border, 1.0));
    p.setBrush(bg);
    p.drawRoundedRect(r, 5.0, 5.0);

    // Draw pipette icon (center 16x16 box)
    const QPointF c = r.center();
    const qreal hw = 7.0;
    const QRectF glyph(c.x() - hw, c.y() - hw, hw * 2.0, hw * 2.0);

    QColor ic = tc.text;
    ic.setAlpha(200);
    p.setPen(QPen(ic, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);

    // Barrel: diagonal from top-right to lower-left
    const QPointF tipPt(glyph.left()  + 1.5, glyph.bottom() - 1.5);
    const QPointF colPt(glyph.right() - 2.5, glyph.top()    + 2.5);
    p.drawLine(tipPt, colPt);

    // Collar square at top-right
    const QRectF collar(glyph.right() - 5.0, glyph.top(), 5.0, 5.0);
    p.drawRoundedRect(collar, 1.2, 1.2);

    // Pickup tip: small cross at bottom-left
    const qreal cs = 2.0;
    p.drawLine(QPointF(tipPt.x() - cs, tipPt.y()), QPointF(tipPt.x() + cs, tipPt.y()));
    p.drawLine(QPointF(tipPt.x(), tipPt.y() - cs), QPointF(tipPt.x(), tipPt.y() + cs));
}

// ────────────────────────────────────────────────────────────────────────────
// GradientStopBar
// ────────────────────────────────────────────────────────────────────────────

GradientStopBar::GradientStopBar(QWidget *parent)
    : QWidget(parent)
{
    const int total = kBarTop + kBarH + kGap + kHandleR * 2 + 4;
    setFixedHeight(total);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);

    // Default: black→white
    m_stops = { {0.0, QColor(0, 0, 0)}, {1.0, QColor(255, 255, 255)} };
}

QRect GradientStopBar::barRect() const
{
    return QRect(kBarPadX, kBarTop, width() - 2 * kBarPadX, kBarH);
}

int GradientStopBar::handleCenterY() const
{
    return kBarTop + kBarH + kGap + kHandleR;
}

int GradientStopBar::posToX(qreal pos) const
{
    const QRect bar = barRect();
    return bar.left() + qRound(pos * bar.width());
}

qreal GradientStopBar::xToPos(int x) const
{
    const QRect bar = barRect();
    return qBound(0.0, (x - bar.left()) / (double)qMax(1, bar.width()), 1.0);
}

int GradientStopBar::hitTestHandle(const QPoint &pos) const
{
    const int cy = handleCenterY();
    const int hitR = kHandleR + 4;
    for (int i = 0; i < m_stops.size(); ++i) {
        const int cx = posToX(m_stops[i].pos);
        if (qAbs(pos.x() - cx) <= hitR && qAbs(pos.y() - cy) <= hitR)
            return i;
    }
    return -1;
}

QColor GradientStopBar::colorAtPos(qreal pos) const
{
    if (m_stops.isEmpty()) return Qt::white;
    if (m_stops.size() == 1) return m_stops[0].color;

    const GradientStop *left = nullptr, *right = nullptr;
    for (const auto &s : m_stops) {
        if (s.pos <= pos && (!left || s.pos > left->pos))  left  = &s;
        if (s.pos >= pos && (!right || s.pos < right->pos)) right = &s;
    }
    if (!left)  return right->color;
    if (!right) return left->color;
    if (qFuzzyCompare(left->pos, right->pos)) return left->color;
    const qreal t = (pos - left->pos) / (right->pos - left->pos);
    return QColor(
        qRound(left->color.red()   + t * (right->color.red()   - left->color.red())),
        qRound(left->color.green() + t * (right->color.green() - left->color.green())),
        qRound(left->color.blue()  + t * (right->color.blue()  - left->color.blue())),
        qRound(left->color.alpha() + t * (right->color.alpha() - left->color.alpha()))
    );
}

void GradientStopBar::sortStops()
{
    std::stable_sort(m_stops.begin(), m_stops.end(),
                     [](const GradientStop &a, const GradientStop &b) { return a.pos < b.pos; });
    m_selected = qBound(0, m_selected, m_stops.size() - 1);
}

void GradientStopBar::setStops(const QVector<GradientStop> &stops)
{
    m_stops = stops;
    sortStops();
    update();
}

void GradientStopBar::selectStop(int idx)
{
    idx = qBound(0, idx, m_stops.size() - 1);
    if (m_selected == idx) return;
    m_selected = idx;
    update();
    emit stopSelected(m_selected);
}

QColor GradientStopBar::selectedColor() const
{
    if (m_selected < 0 || m_selected >= m_stops.size()) return QColor();
    return m_stops[m_selected].color;
}

void GradientStopBar::setSelectedColor(const QColor &c)
{
    if (m_selected < 0 || m_selected >= m_stops.size()) return;
    if (m_stops[m_selected].color == c) return;
    m_stops[m_selected].color = c;
    update();
    emit stopsChanged();
}

void GradientStopBar::addStop(qreal pos, const QColor &c)
{
    GradientStop s;
    s.pos   = qBound(0.0, pos, 1.0);
    s.color = c;
    m_stops.append(s);
    sortStops();
    // Find the index of the just-added stop
    for (int i = 0; i < m_stops.size(); ++i) {
        if (qFuzzyCompare(m_stops[i].pos, s.pos) && m_stops[i].color == s.color) {
            m_selected = i;
            break;
        }
    }
    update();
    emit stopsChanged();
    emit stopSelected(m_selected);
}

void GradientStopBar::removeSelectedStop()
{
    if (m_stops.size() <= 2) return;
    m_stops.removeAt(m_selected);
    m_selected = qBound(0, m_selected, m_stops.size() - 1);
    update();
    emit stopsChanged();
    emit stopSelected(m_selected);
}

void GradientStopBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        const QPoint lp = mouseLocalPosF(event).toPoint();
        const int hit = hitTestHandle(lp);
        if (hit >= 0) {
            m_dragging = hit;
            m_selected = hit;
            emit stopSelected(m_selected);
            update();
        } else if (barRect().contains(lp)) {
            // Add new stop
            const qreal pos = xToPos(lp.x());
            addStop(pos, colorAtPos(pos));
        }
    }
    QWidget::mousePressEvent(event);
}

void GradientStopBar::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging >= 0 && m_dragging < m_stops.size() && (event->buttons() & Qt::LeftButton)) {
        qreal pos = xToPos(mouseLocalPosF(event).toPoint().x());
        // Clamp between neighbours so order is preserved
        const qreal lo = (m_dragging > 0)                    ? m_stops[m_dragging - 1].pos + 0.001 : 0.0;
        const qreal hi = (m_dragging < m_stops.size() - 1)   ? m_stops[m_dragging + 1].pos - 0.001 : 1.0;
        m_stops[m_dragging].pos = qBound(lo, pos, hi);
        update();
        emit stopsChanged();
    }
    QWidget::mouseMoveEvent(event);
}

void GradientStopBar::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_dragging >= 0) {
        m_dragging = -1;
        emit stopsChanged();
    }
    QWidget::mouseReleaseEvent(event);
}

void GradientStopBar::contextMenuEvent(QContextMenuEvent *event)
{
    const int hit = hitTestHandle(event->pos());
    if (hit < 0) { QWidget::contextMenuEvent(event); return; }

    m_selected = hit;
    update();
    emit stopSelected(m_selected);

    QMenu menu(this);
    QAction *del = menu.addAction(tr("删除色标"));
    del->setEnabled(m_stops.size() > 2);
    const QAction *chosen = menu.exec(event->globalPos());
    if (chosen == del)
        removeSelectedStop();
}

void GradientStopBar::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace)
        removeSelectedStop();
    else
        QWidget::keyPressEvent(event);
}

void GradientStopBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const auto &tc = ThemeManager::instance().colors();
    const QRect bar = barRect();

    // ── Checkerboard background ───────────────────────────────────────────
    {
        const int s = 6;
        QPainterPath clip;
        clip.addRoundedRect(QRectF(bar).adjusted(0.5, 0.5, -0.5, -0.5), 6.0, 6.0);
        p.save();
        p.setClipPath(clip);
        for (int y = bar.top(); y <= bar.bottom(); y += s) {
            for (int x = bar.left(); x <= bar.right(); x += s) {
                const bool dark = (((x - bar.left()) / s) + ((y - bar.top()) / s)) % 2 == 0;
                p.fillRect(QRect(x, y, s, s), dark ? QColor(0, 0, 0, 35) : QColor(255, 255, 255, 45));
            }
        }
        // Gradient fill
        if (m_stops.size() >= 2) {
            QLinearGradient grad(bar.left(), 0, bar.right(), 0);
            for (const auto &stop : m_stops)
                grad.setColorAt(stop.pos, stop.color);
            p.fillRect(bar, grad);
        }
        p.restore();
    }
    // Bar border
    p.setPen(QPen(tc.border, 1.0));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(QRectF(bar).adjusted(0.5, 0.5, -0.5, -0.5), 6.0, 6.0);

    // ── Stop handles (circles) ────────────────────────────────────────────
    const int cy = handleCenterY();
    for (int i = 0; i < m_stops.size(); ++i) {
        const int cx = posToX(m_stops[i].pos);
        const QPointF center(cx, cy);

        // Shadow
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 40));
        p.drawEllipse(center + QPointF(0.0, 1.0), (qreal)kHandleR, (qreal)kHandleR);

        // Fill with stop colour
        p.setBrush(m_stops[i].color);
        QColor border = (i == m_selected) ? tc.accent : tc.border;
        border.setAlpha(220);
        p.setPen(QPen(border, i == m_selected ? 2.0 : 1.0));
        p.drawEllipse(center, (qreal)kHandleR, (qreal)kHandleR);

        // Small tick from bar bottom to handle
        p.setPen(QPen(border, 1.0));
        p.drawLine(QPointF(cx, bar.bottom() + 1), QPointF(cx, cy - kHandleR));
    }
}

} // namespace Fluent::ColorPicker
