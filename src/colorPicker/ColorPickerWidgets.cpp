#include "ColorPickerWidgets.h"

#include "Fluent/FluentTheme.h"

#include <QApplication>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QEnterEvent>
#endif
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

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
    setFixedSize(22, 22);
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
void ColorSwatchButton::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event)
    m_hover = true;
    update();
}
#else
void ColorSwatchButton::enterEvent(QEvent *event)
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
    setFixedSize(240, 240);
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
    setFixedHeight(34);
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

    p.fillRect(r, m_color);
    p.restore();

    p.setPen(QPen(tc.border, 1.0));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(QRectF(r).adjusted(0.5, 0.5, -0.5, -0.5), radius, radius);
}

HueStrip::HueStrip(QWidget *parent)
    : QWidget(parent)
{
    setFixedWidth(34);
    setCursor(Qt::PointingHandCursor);
    setMouseTracking(true);
}

void HueStrip::setValue(int v)
{
    v = qBound(0, v, 359);
    if (m_value == v) {
        return;
    }
    m_value = v;
    update();
}

void HueStrip::enterEvent(QEvent *event)
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
        setFromY(mouseLocalPosF(event).y(), true);
    }
    QWidget::mousePressEvent(event);
}

void HueStrip::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging) {
        setFromY(mouseLocalPosF(event).y(), true);
    }
    QWidget::mouseMoveEvent(event);
}

void HueStrip::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        setFromY(mouseLocalPosF(event).y(), true);
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
    const int trackW = 14;
    const int padY = 6;
    const QRect track(outer.center().x() - trackW / 2, outer.top() + padY, trackW, qMax(10, outer.height() - padY * 2));

    QLinearGradient grad(track.left(), track.top(), track.left(), track.bottom());
    grad.setColorAt(0.00, QColor("#FF0000"));
    grad.setColorAt(1.0 / 6.0, QColor("#FFFF00"));
    grad.setColorAt(2.0 / 6.0, QColor("#00FF00"));
    grad.setColorAt(3.0 / 6.0, QColor("#00FFFF"));
    grad.setColorAt(4.0 / 6.0, QColor("#0000FF"));
    grad.setColorAt(5.0 / 6.0, QColor("#FF00FF"));
    grad.setColorAt(1.00, QColor("#FF0000"));

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

    p.setPen(QPen(QColor(255, 255, 255, 160), 1.0));
    p.drawLine(QPointF(handleRect.left() + 3.0, handleRect.top() + 4.0), QPointF(handleRect.right() - 3.0, handleRect.top() + 4.0));
}

void HueStrip::setFromY(qreal y, bool emitSignal)
{
    const QRect outer = rect().adjusted(1, 1, -1, -1);
    const int trackW = 14;
    const int padY = 6;
    const QRect track(outer.center().x() - trackW / 2, outer.top() + padY, trackW, qMax(10, outer.height() - padY * 2));
    const qreal yy = qBound((qreal)track.top(), y, (qreal)track.bottom());
    const qreal t = (yy - track.top()) / qMax(1, track.height());
    const int v = qBound(0, qRound(t * 359.0), 359);
    if (v == m_value) {
        return;
    }
    m_value = v;
    update();
    if (emitSignal) {
        emit valueChanged(m_value);
    }
}

AlphaStrip::AlphaStrip(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(26);
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

void AlphaStrip::enterEvent(QEvent *event)
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

} // namespace Fluent::ColorPicker
